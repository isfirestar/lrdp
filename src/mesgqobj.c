#include "mesgqobj.h"

#include "mesgq.h"
#include "jconf.h"

struct mesgq_item
{
    mqd_t fd;
    aeEventLoop *el;
    char mqname[64];
    long maxmsg;
    long msgsize;
    unsigned int method;
    int na;
};

static void __mesgq_free(lobj_pt lop, void *context, size_t ctxsize)
{
    struct mesgq_item *mesgq;

    mesgq = lobj_body(struct mesgq_item *, lop);
    if (mesgq->fd) {
        if (!mesgq->na) {
            aeDeleteFileEvent(mesgq->el, mesgq->fd, AE_READABLE);
        }
        mesgq_close(mesgq->fd);
    }
}

static int __mesgq_write(lobj_pt lop, const void *data, size_t n)
{
    struct mesgq_item *mesgq;

    mesgq = lobj_body(struct mesgq_item *, lop);
    if (!mesgq) {
        return -1;
    }

    return mesgq_sendmsg(mesgq->fd, (const char *)data, n, 0, -1);
}

static int __mesgq_vwrite(lobj_pt lop, int elements, const void **vdata, size_t *vsize)
{
    struct mesgq_item *mesgq;
    int i;
    int retsum;

    mesgq = lobj_body(struct mesgq_item *, lop);
    if (!mesgq) {
        return -1;
    }

    retsum = 0;
    for (i = 0; i < elements; i++) {
        retsum += mesgq_sendmsg(mesgq->fd, (const char *)vdata[i], vsize[i], 0, -1);
    }
    return retsum;
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
    lobj_pt lop;
    struct mesgq_item *mesgq;
    nsp_status_t status;
    struct lobj_fx_sym sym = { NULL };
    struct lobj_fx fx = { NULL };

    fx.freeproc = &__mesgq_free;
    fx.writeproc = &__mesgq_write;
    fx.vwriteproc = &__mesgq_vwrite;
    fx.readproc = &__mesgq_read;
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
            printf("failed on open named messageq [%s], error status:%ld\n", jmesgq->mqname, status);
            break;
        }

        if (jmesgq->method == MESGQ_OPEN_ALWAYS || jmesgq->method == MESGQ_OPEN_EXISTING) {
            // get message queue info
            status = mesgq_getattr(mesgq->fd, &mesgq->maxmsg, &mesgq->msgsize, NULL);
            if (!NSP_SUCCESS(status)) {
                lrdp_generic_error("failed on get named messageq [%s] info, error status:%ld", jmesgq->mqname, status);
                break;
            }
        }

        // free/read/write proc can not be covered
        sym.touchproc_sym = jmesgq->head.touchproc;
        sym.freeproc_sym = NULL;
        sym.writeproc_sym = NULL;
        sym.vwriteproc_sym = NULL;
        sym.readproc_sym = NULL;
        sym.vreadproc_sym = jmesgq->head.vreadproc;
        sym.recvdataproc_sym = jmesgq->head.recvdataproc;
        sym.rawinvokeproc_sym = jmesgq->head.rawinvokeproc;
        lobj_fx_cover(lop, &sym);

        // set nonblock
        if (!jmesgq->na && el) {
            status = mesgq_set_nonblocking(mesgq->fd);
            if (!NSP_SUCCESS(status)) {
                printf("failed mark named mq [%s] to non-blocking, error status:%ld\n", jmesgq->mqname, status);
                break;
            }
            // save ae target
            mesgq->el = el;
            // attach to ae
            aeCreateFileEvent(el, mesgq->fd, AE_READABLE, &__mesgq_ae_read, lop);
        }
        return lop;
    } while(0);

    lobj_ldestroy(lop);
    return NULL;
}

