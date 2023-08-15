#include "timerobj.h"

#include "lobj.h"

struct timerobj
{
    void (*timerproc)(lobj_pt lop);
    unsigned int interval;
    long long evnid;
    aeEventLoop *el;
};

static int __timerobj_proc(struct aeEventLoop *eventLoop, long long id, void *clientData)
{
    lobj_pt lop;
    struct timerobj *timer;

    lop = (lobj_pt)clientData;
    timer = lobj_body(struct timerobj *, lop);

    if (timer->timerproc) {
        timer->timerproc(lop);
    }

    return timer->interval;
}

static void __timerobj_atexit(lobj_pt lop, void *ctx, size_t ctxsize)
{
    struct timerobj *timer;

    timer = lobj_body(struct timerobj *, lop);
    if (timer->evnid) {
        aeDeleteTimeEvent(timer->el, timer->evnid);
    }
}

void timerobj_create(aeEventLoop *el, const jconf_timer_pt jtimer)
{
    lobj_pt lop;
    struct lobj_fx fx = { NULL };
    struct lobj_fx_sym sym = { NULL };
    struct timerobj *timer;

    fx.freeproc = &__timerobj_atexit;
    lop = lobj_create(jtimer->head.name, jtimer->head.module, sizeof(struct timerobj), jtimer->head.ctxsize, &fx);
    if (!lop) {
        return;
    }
    timer = lobj_body(struct timerobj *, lop);

    sym.touchproc_sym = jtimer->head.touchproc;
    sym.freeproc_sym = NULL;
    sym.writeproc_sym = jtimer->head.writeproc;
    sym.vwriteproc_sym = jtimer->head.vwriteproc;
    sym.readproc_sym = jtimer->head.readproc;
    sym.vreadproc_sym = jtimer->head.vreadproc;
    sym.recvdataproc_sym = jtimer->head.recvdataproc;
    sym.rawinvokeproc_sym = jtimer->head.rawinvokeproc;
    lobj_fx_cover(lop, &sym);

    timer->el = el;
    timer->timerproc = lobj_dlsym(lop, jtimer->timerproc);
    timer->interval = jtimer->interval;
    timer->evnid = aeCreateTimeEvent(timer->el, timer->interval, &__timerobj_proc, lop, NULL);
}
