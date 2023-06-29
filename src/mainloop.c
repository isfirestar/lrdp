#include "mainloop.h"

#include "lobj.h"
#include "zmalloc.h"

#include <stdio.h>

struct mainloop
{
    int (*preinitproc)(lobj_pt lop, int argc, char **argv);
    void (*postinitproc)(lobj_pt lop);
    void (*exitproc)(lobj_pt lop);
    void (*timerproc)(lobj_pt lop);
    unsigned int interval;
};
typedef struct mainloop mainloop_t, *mainloop_pt;

void __mloop_atexit(struct lobj *lop)
{
    mainloop_pt mloop;

    mloop = lobj_body(mainloop_pt, lop);
    if (mloop->exitproc) {
        mloop->exitproc(lop);
    }
}

lobj_pt mloop_create(const jconf_entry_pt jentry)
{
    lobj_pt lop;
    mainloop_pt mloop;
    struct lobj_fx fx = {
        .freeproc = &__mloop_atexit,
        .referproc = NULL,
        .writeproc = NULL
    };

    if (!jentry) {
        return NULL;
    }

    lop = lobj_create("mainloop", jentry->module, sizeof(mainloop_t), jentry->ctxsize, &fx);
    if (!lop) {
        return NULL;
    }
    mloop = lobj_body(mainloop_pt, lop);

    // load all entry procedure which defined in json configure file, ignore failed
    mloop->preinitproc = lobj_dlsym(lop, jentry->preinitproc);
    mloop->postinitproc = lobj_dlsym(lop, jentry->postinitproc);
    mloop->exitproc = lobj_dlsym(lop, jentry->exitproc);
    mloop->timerproc = lobj_dlsym(lop, jentry->timerproc);

    // simple copy timer interval and context size
    mloop->interval = jentry->interval;

    return lop;
}

static int __mloop_on_timer(struct aeEventLoop *eventLoop, long long id, void *clientData)
{
    mainloop_pt mloop;
    lobj_pt lop;

    lop = (lobj_pt)clientData;
    mloop = lobj_body(mainloop_pt, lop);

    if (mloop->timerproc) {
        mloop->timerproc(lop);
    }

    return mloop->interval;
}

void mloop_add_timer(aeEventLoop *el, lobj_pt lop)
{
    mainloop_pt mloop;

    if (!el || !lop) {
        return;
    }
    mloop = lobj_body(mainloop_pt, lop);
    
    if (mloop->interval <= 0) {
        mloop->interval = 1000;
    }
    aeCreateTimeEvent(el, mloop->interval , &__mloop_on_timer, lop, NULL);
}

int mloop_pre_init(lobj_pt lop,int argc, char **argv)
{
    mainloop_pt mloop;

    if (!lop) {
        return -1;
    }
    mloop = lobj_body(mainloop_pt, lop);

    if (mloop->preinitproc) {
        return mloop->preinitproc(lop, argc, argv);
    }

    return 0;
}

void mloop_post_init(lobj_pt lop)
{
    mainloop_pt mloop;

    if (!lop) {
        return;
    }
    mloop = lobj_body(mainloop_pt, lop);

    if (mloop->postinitproc) {
        mloop->postinitproc(lop);
    } 
}
