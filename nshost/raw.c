#include "raw.h"

#include "nis.h"
#include "ncb.h"
#include "zmalloc.h"
#include "io.h"
#include "mxx.h"
#include "fifo.h"

#define MAX_RAW_BUFFER_SIZE     (4096)

static nsp_status_t _rawrefr( objhld_t hld, ncb_t **ncb )
{
    if (unlikely( hld < 0 || !ncb)) {
        return posix__makeerror(EINVAL);
    }

    *ncb = objrefr( hld );
    if ( NULL != (*ncb) ) {
        if (likely(-1 == (*ncb)->protocol)) {
            return NSP_STATUS_SUCCESSFUL;
        }

        objdefr( hld );
        *ncb = NULL;
        return posix__makeerror(EPROTOTYPE);
    }

    return posix__makeerror(ENOENT);
}


static nsp_status_t _raw_alloc_rx_buffer(ncb_t *ncb)
{
    if (likely(!ncb->rx_buffer)) {
        ncb->rx_buffer = (unsigned char *)zmalloc(MAX_RAW_BUFFER_SIZE);
        if (unlikely(!ncb->rx_buffer)) {
            return posix__makeerror(ENOMEM);
        }
    }
    ncb->rx_buffer_size = MAX_RAW_BUFFER_SIZE;

    return NSP_STATUS_SUCCESSFUL;
}

static nsp_status_t _raw_rx(ncb_t *ncb)
{
    int rxcb;
    raw_data_t c_data;
    nis_event_t c_event;

    rxcb = read(ncb->sockfd, ncb->rx_buffer, ncb->rx_buffer_size);
    if (rxcb > 0) {
        c_event.Ln.Udp.Link = ncb->hld;
        c_event.Event = EVT_RECEIVEDATA;
        c_data.e.Packet.Data = ncb->rx_buffer;
        c_data.e.Packet.Size = rxcb;
        if (ncb->nis_callback) {
            ncb->nis_callback(&c_event, &c_data);
        }
        return NSP_STATUS_SUCCESSFUL;
    }

    if ( 0 == rxcb ) {
        /* Datagram sockets in various domains (e.g., the UNIX and Internet domains) permit zero-length datagrams.
            When such a datagram is received, the return value is 0. */
        return NSP_STATUS_SUCCESSFUL;
    }

    /* ECONNRESET 104 Connection reset by peer */
    if (rxcb < 0) {
        /* system interrupted */
        if (!NSP_FAILED_AND_ERROR_EQUAL(rxcb, EAGAIN)) {
            mxx_call_ecr("fatal error occurred syscall recvfrom(2), error:%d, link:%lld", errno, ncb->hld );
        }
        return rxcb;
    }

    return NSP_STATUS_SUCCESSFUL;
}

nsp_status_t raw_rx(ncb_t *ncb)
{
    nsp_status_t status;
    int available;

    do {
        if (0 == ioctl(ncb->sockfd, FIONREAD, &available)) {
            if (0 == available) {
                status = posix__makeerror(EAGAIN);
                break;
            }
        }
        status = _raw_rx(ncb);
    } while(NSP_SUCCESS(status));
    return status;
}

HRAWLINK raw_create(raw_io_fp callback, int fd)
{
    ncb_t *ncb;
    struct objcreator creator;
    nsp_status_t status;
    objhld_t hld;

    creator.known = INVALID_OBJHLD;
    creator.size = sizeof(ncb_t);
    creator.initializer = &ncb_allocator;
    creator.unloader = &ncb_deconstruct;
    creator.context = NULL;
    creator.ctxsize = 0;
    hld = objallo3(&creator);
    if (unlikely(hld < 0)) {
        mxx_call_ecr("insufficient resource for allocate inner object");
        return INVALID_HRAWLINK;
    }

    ncb = (ncb_t *) objrefr(hld);
    assert(ncb);

    ncb->hld = hld;
    ncb->protocol = -1;
    ncb->nis_callback = callback;

    do {
        status = _raw_alloc_rx_buffer(ncb);
        if (!NSP_SUCCESS(status)) {
            break;
        }

        /* use the raw socket fd from user specified */
        ncb->sockfd = fd;
        /* set data handler function pointer for Rx/Tx */
        __atomic_store_n(&ncb->ncb_read, &raw_rx, __ATOMIC_RELEASE);
        __atomic_store_n(&ncb->ncb_write, &raw_tx, __ATOMIC_RELEASE);

        /* set the fd to non-blocking mode */
        io_set_nonblock(ncb->sockfd, 1);

        /* attach to epoll */
        if (ncb->nis_callback) {
            status = io_attach(ncb, EPOLLIN);
            if (!NSP_SUCCESS(status)) {
                break;
            }
        }
    } while(0);

    objdefr(hld);
    if (!NSP_SUCCESS(status)) {
        objclos(hld);
        return INVALID_HRAWLINK;
    }

    return ncb->hld;
}

nsp_status_t raw_txn(ncb_t *ncb, void *p)
{
    int wcb;
    struct tx_node *node;

	node = (struct tx_node *)p;
	if (!node) {
		return -EINVAL;
	}

    while (node->offset < node->wcb) {
        wcb = write(ncb->sockfd, node->data + node->offset, node->wcb - node->offset);

        /* fatal-error/connection-terminated  */
        if (0 == wcb) {
            mxx_call_ecr("Fatal error occurred syscall write(2), the return value equal to zero, link:%lld", ncb->hld);
            return NSP_STATUS_FATAL;
        }

        if (wcb < 0) {
            /* the write buffer is full, active EPOLLOUT and waitting for epoll event trigger
             * at this point, we need to deal with the queue header node and restore the unprocessed node back to the queue header.
             * the way 'oneshot' focus on the write operation completion point */
            if (NSP_FAILED_AND_ERROR_EQUAL(wcb, EAGAIN)) {
                mxx_call_ecr("syscall write(2) would block cause by kernel memory overload,link:%lld", ncb->hld);
            } else {
                /* other error, these errors should cause link close */
                mxx_call_ecr("fatal error occurred syscall write(2), error:%d, link:%lld",errno, ncb->hld );
            }
            return posix__makeerror(errno);
        }

        node->offset += wcb;
    }

    return NSP_STATUS_SUCCESSFUL;
}

nsp_status_t raw_tx(ncb_t *ncb)
{
    struct tx_node *node;
    nsp_status_t status;

    /* try to write front package into system kernel send-buffer */
    status = fifo_top(ncb, &node);
    if (NSP_SUCCESS(status)) {
        status = raw_txn(ncb, node);
        if (NSP_SUCCESS(status)) {
            return fifo_pop(ncb, NULL);
        }
    }

    return status;
}

void raw_destroy(HRAWLINK link)
{
    ncb_t *ncb;

    if (!NSP_SUCCESS(_rawrefr(link, &ncb))) {
        return;
    }

    mxx_call_ecr("link:%lld order to destroy", ncb->hld);
    objdefr(link);
}

nsp_status_t raw_write(HRAWLINK link, const void *origin, int size, const nis_serializer_fp serializer)
{
    ncb_t *ncb;
    unsigned char *buffer;
    struct tx_node *node;
    nsp_status_t status;

    if (unlikely( size <= 0) || (link < 0) || (size > MAX_RAW_BUFFER_SIZE) || !origin) {
        return posix__makeerror(EINVAL);
    }

    buffer = NULL;
    node = NULL;

    status = _rawrefr(link, &ncb);
    if (!NSP_SUCCESS(status)) {
        return status;
    }

    do {
        status = NSP_STATUS_FATAL;
        buffer = (unsigned char *)ztrymalloc(size);
        if (unlikely(!buffer)) {
            status = posix__makeerror(ENOMEM);
            break;
        }

        /* serialize data into packet or direct use data pointer by @origin */
        if (serializer) {
            if ((*serializer)(buffer, origin, size) < 0 ) {
                break;
            }
        } else {
            memcpy(buffer, origin, size);
        }

        node = (struct tx_node *)ztrymalloc(sizeof (struct tx_node));
        if (unlikely(!node)) {
            status = posix__makeerror(ENOMEM);
            break;
        }
        memset(node, 0, sizeof(struct tx_node));
        node->data = buffer;
        node->wcb = size;
        node->offset = 0;

        if (!fifo_tx_overflow(ncb)) {

            /* the return value means direct failed when it equal to -1 or success when it greater than zero.
             * in these case, destroy memory resource outside loop, no matter what the actually result it is.
             */
            status = raw_txn(ncb, node);

            /* break if success or failed without EAGAIN
             */
            if (!NSP_FAILED_AND_ERROR_EQUAL(status, EAGAIN)) {
                break;
            }
        }

        /* 1. when the IO state is blocking, any send or write call certain to be fail immediately,
         *
         * 2. the meaning of -EAGAIN return by @udp_txn is send or write operation cannot be complete immediately,
         *      IO state should change to blocking now
         *
         * one way to handle the above two aspects, queue data to the tail of fifo manager, preserve the sequence of output order
         * in this case, memory of @buffer and @node cannot be destroy until asynchronous completed
         *
         * after @fifo_queue success called, IO blocking flag is set, and EPOLLOUT event has been associated with ncb object.
         * wpool thread canbe awaken by any kernel cache writable event trigger  */
        status = fifo_queue(ncb, node);
        if (!NSP_SUCCESS(status)) {
            break;
        }

        objdefr(link);
        return status;
    } while (0);

    if (likely(buffer)) {
        zfree(buffer);
    }

    if (likely(node)) {
        zfree(node);
    }

    objdefr(link);

    /* @fifo_queue may raise a EBUSY error indicate the user-level cache of sender is full.
     * in this case, current send request is going to be ignore but link shall not be close.
     * otherwise, close link nomatter what error code it is
    if ( !NSP_SUCCESS_OR_ERROR_EQUAL(status, EBUSY)) {
        objclos(link);
    }*/

    return status;
}
