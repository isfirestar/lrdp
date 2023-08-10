#include "mesgqobj.h"

#include "mesgq.h"
#include "jconf.h"

struct mesgq_item
{
    mqd_t fd;
    char mqname[64];
    unsigned int maxmsg;
    unsigned int msgsize;
    unsigned int method;
    int na;
};

static void __mesgq_free(lobj_pt lop, void *context, size_t ctxsize)
{
    struct mesgq_item *mesgq;

    mesgq = lobj_body(struct mesgq_item *, lop);
    if (mesgq->fd) {
        mesgq_close(mesgq->fd);
    }
}

static int __mesgq_write(lobj_pt lop, const void *data, size_t n)
{
    struct mesgq_item *mesgq;

    mesgq = lobj_body(struct mesgq_item *, lop);

    return mesgq_sendmsg(mesgq->fd, (const char *)data, n, 0, -1);
}

static void __mesgq_ae_read(struct aeEventLoop *el, int fd, void *clientData, int mask)
{
    lobj_pt lop;
    char buf[1024];
    int n;

    lop = (lobj_pt)clientData;

    n = mesgq_recvmsg(fd, buf, sizeof(buf), NULL, -1);
    if (n > 0) {
        lobj_fx_on_recvdata(lop, buf, n);
    } else if (n < 0) {
        if (errno != EAGAIN) {
            aeDeleteFileEvent(el, fd, AE_READABLE);
        }
    } else {
        ;
    }
}

int __mesgq_read(lobj_pt lop, void *data, size_t n)
{
    struct mesgq_item *mesgq;
    mqd_t fd;
    char buf[1024];

    mesgq = lobj_body(struct mesgq_item *, lop);
    fd = mesgq->fd;

    if (fd < 0) {
        return -1;
    }

    if (n > sizeof(buf)) {
        n = sizeof(buf);
    }

    return mesgq_recvmsg(fd, buf, n, NULL, -1);
}

lobj_pt mesgqobj_create(const jconf_mesgqobj_pt jmesgq, aeEventLoop *el)
{
    struct lobj_fx fx = {
        .freeproc = &__mesgq_free,
        .writeproc = &__mesgq_write,
        .vwriteproc = NULL,
        .readproc = &__mesgq_read,
        .vreadproc = NULL,
        .recvdataproc = NULL,
    };
    lobj_pt lop;
    struct mesgq_item *mesgq;
    nsp_status_t status;
    recvdata_pfn recvdataproc;

    lop = lobj_create(jmesgq->head.name, jmesgq->head.module, sizeof(struct mesgq_item), jmesgq->head.ctxsize, &fx);
    if (!lop) {
        return NULL;
    }
    mesgq = lobj_body(struct mesgq_item *, lop);

    do {
        // save info
        strncpy(mesgq->mqname, jmesgq->mqname, sizeof(mesgq->mqname) - 1);
        mesgq->maxmsg = jmesgq->maxmsg;
        mesgq->msgsize = jmesgq->msgsize;
        mesgq->method = jmesgq->method;
        mesgq->na = jmesgq->na;
        // open message queue
        status = mesgq_open(jmesgq->mqname, jmesgq->method, jmesgq->maxmsg, jmesgq->msgsize, &mesgq->fd);
        if (!NSP_SUCCESS(status)) {
            break;
        }

        // freeproc and write proc can not be covered
        recvdataproc = lobj_dlsym(lop, jmesgq->head.recvdataproc);
        lobj_cover_fx(lop, NULL, NULL, jmesgq->head.vwriteproc, NULL, jmesgq->head.vreadproc, jmesgq->head.recvdataproc);

        // set nonblock
        if (!jmesgq->na && recvdataproc) {
            status = mesgq_set_nonblocking(mesgq->fd);
            if (!NSP_SUCCESS(status)) {
                break;
            }
            // attach to ae
            aeCreateFileEvent(el, mesgq->fd, AE_READABLE, &__mesgq_ae_read, lop);
        }
        return lop;
    } while(0);

    lobj_ldestroy(lop);
    return NULL;
}
