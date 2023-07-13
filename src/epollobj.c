#include "epollobj.h"

#include <sys/epoll.h>
#include <fcntl.h>

#include "threading.h"
#include "zmalloc.h"
#include "ifos.h"

struct epoll_item
{
    lwp_t *threads;
    unsigned int poolthreads;
    int fd;
    int actived;
    int timeout;
    void (*timeoutproc)(lobj_pt, void *eed);
    void (*errorproc)(lobj_pt, void *eed);
    void (*rdhupproc)(lobj_pt, void *eed);
    lobj_pt lop;
};

void __epoll_free_proc(lobj_pt lop, void *ctx, size_t ctxsize)
{
    struct epoll_item *epollptr;
    unsigned int i;

    if (!lop) {
        return;
    }

    epollptr = lobj_body(struct epoll_item *, lop);

    if (epollptr->fd >= 0) {
        close(epollptr->fd);
    }

    if (epollptr->threads) {
        epollptr->actived = 0;
        for (i = 0; i < epollptr->poolthreads; i++) {
            lwp_join(&epollptr->threads[i], NULL);
        }
        zfree(epollptr->threads);
    }
}

static void __epoll_run_io(const struct epoll_item *epollptr, const struct epoll_event *eventptr)
{
    if ( unlikely(eventptr->events & EPOLLERR) ) {
        if (epollptr->errorproc) {
            epollptr->errorproc(epollptr->lop, eventptr->data.ptr);
        }
        return;
    }

    if (eventptr->events & EPOLLRDHUP) {
        if (epollptr->rdhupproc) {
            epollptr->rdhupproc(epollptr->lop, eventptr->data.ptr);
        }
        return;
    }

    /* system width input cache change from empty to readable */
    if (eventptr->events & EPOLLIN) {
        if (lobj_fx_read(epollptr->lop, eventptr->data.ptr, 0) < 0) {
            return;
        }
    }

    /* system width output cache change from full to writeable */
    if (eventptr->events & EPOLLOUT) {
        if ( 0 == (eventptr->events & EPOLLHUP) ) {
            lobj_fx_write(epollptr->lop, eventptr->data.ptr, 0);
        }
    }
}

static void *__epoll_exec_proc(void *p)
{
    struct epoll_event evts[1024];
    int sigcnt;
    int i;
    struct epoll_item *epollptr;

    epollptr = (struct epoll_item *)p;

    while (epollptr->actived) {
        SYSCALL_WHILE_EINTR(sigcnt, epoll_wait(epollptr->fd, evts, 1024, epollptr->timeout));
        if (sigcnt < 0) {
            break;
        }

        /* at least one signal is awakened,
            otherwise, timeout trigger. */
        for (i = 0; i < sigcnt; i++) {
            __epoll_run_io(epollptr, &evts[i]);
        }
    }

    lwp_exit( (void *)0 );
    return NULL;
}

lobj_pt epollobj_create(const jconf_epollobj_pt jepoll)
{
    lobj_pt lop;
    struct epoll_item *epollptr;
    const struct lobj_fx fx = {
        .freeproc = &__epoll_free_proc,
        .writeproc = NULL,
        .vwriteproc = NULL,
        .readproc = NULL,
        .vreadproc = NULL,
    };
    unsigned int i;
    int nprocs;

    if (!jepoll) {
        return NULL;
    }

    nprocs = ifos_getnprocs();
    if (jepoll->poolthreads <= 0 || jepoll->poolthreads >= (nprocs * 3)) {
        jepoll->poolthreads = nprocs;
    }

    lop = lobj_create(jepoll->head.name, jepoll->head.module, sizeof(*epollptr), jepoll->head.ctxsize, &fx);
    if (!lop) {
        return NULL;
    }
    epollptr = lobj_body(struct epoll_item *, lop);

    do {
        epollptr->poolthreads = jepoll->poolthreads;
        epollptr->timeout = jepoll->timeout;
        epollptr->timeoutproc = lobj_dlsym(lop, jepoll->timeoutproc);
        epollptr->errorproc = lobj_dlsym(lop, jepoll->errorproc);
        epollptr->rdhupproc = lobj_dlsym(lop, jepoll->rdhupproc);

        epollptr->fd = epoll_create1(EPOLL_CLOEXEC);
        if (epollptr->fd < 0) {
            break;
        }

        epollptr->threads = ztrymalloc(sizeof(lwp_t) * epollptr->poolthreads);
        if (!epollptr->threads) {
            break;
        }

        for (i = 0; i < epollptr->poolthreads; ++i) {
            lwp_create(&epollptr->threads[i], 0, &__epoll_exec_proc, epollptr);  // ignore thread spwnan error
        }

        return lop;
    } while (0);

    lobj_ldestroy(lop);
    return NULL;
}
