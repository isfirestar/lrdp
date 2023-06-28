#include "mainloop.h"

#include "lobj.h"
#include "zmalloc.h"
#include "ae.h"

#include <stdio.h>

struct mainloop
{
    void (*preinitproc)(int argc, char **argv);
    void (*postinitproc)(void *context, unsigned int ctxsize);
    void (*exitproc)(void);
    void (*timerproc)(void *context, unsigned int ctxsize);
    unsigned int interval;
};
typedef struct mainloop mainloop_t, *mainloop_pt;;

lobj_pt mloop_create(const jconf_entry_pt jentry)
{
    lobj_pt lop;
    mainloop_pt mloop;
    struct lobj_fx fx = {
        .freeproc = NULL,
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
        mloop->timerproc(lop->ctx, lop->ctxsize);
    }

    return mloop->interval;
}

int mloop_run(lobj_pt lop)
{
    aeEventLoop *aeloop;
    long long timerid;
    mainloop_pt mloop;

    if (!lop) {
        return 1;
    }
    mloop = lobj_body(mainloop_pt, lop);

    /* ok, all other initialize have been finish, invoke post init proc if exist */
    if (mloop->postinitproc) {
        mloop->postinitproc(lop->ctx, lop->ctxsize);
    }

    /*create a timer for main loop */
    aeloop = aeCreateEventLoop(1);
    if (aeloop) {
        timerid = aeCreateTimeEvent(aeloop, (long long)mloop->interval, &__mloop_on_timer, lop, NULL);
        if (timerid != AE_ERR) {
            /* main loop */
            aeMain(aeloop);
        }
    }

    /* release resource */
    if (aeloop) {
        aeDeleteEventLoop(aeloop);
    }

    lobj_ldestroy(lop);
    return 0;
}

void mloop_preinit(lobj_pt lop,int argc, char **argv)
{
    mainloop_pt mloop;

    if (!lop) {
        return;
    }
    mloop = lobj_body(mainloop_pt, lop);

    if (mloop->preinitproc) {
        mloop->preinitproc(argc, argv);
    } 
}
