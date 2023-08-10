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

static void __timerobj_atexit(struct lobj *lop)
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
    struct lobj_fx fx = {
        .freeproc = NULL,
        .writeproc = NULL,
        .vwriteproc = NULL,
        .readproc = NULL,
        .vreadproc = NULL,
    };
    struct timerobj *timer;

    lop = lobj_create(jtimer->head.name, jtimer->head.module, sizeof(struct timerobj), jtimer->head.ctxsize, &fx);
    if (!lop) {
        return;
    }
    timer = lobj_body(struct timerobj *, lop);
    lobj_cover_fx(lop, jtimer->head.freeproc, jtimer->head.writeproc, jtimer->head.vwriteproc, jtimer->head.readproc, jtimer->head.vreadproc, NULL);
    timer->el = el;
    timer->timerproc = lobj_dlsym(lop, jtimer->timerproc);
    timer->interval = jtimer->interval;
    timer->evnid = aeCreateTimeEvent(timer->el, timer->interval, &__timerobj_proc, lop, NULL);
}
