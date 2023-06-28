#include "mainloop.h"

#include "lobj.h"
#include "zmalloc.h"

#include <stdio.h>

void __mloop_on_refer(struct lobj *ptr)
{
    mainloop_pt loopptr;

    if (!ptr) {
        return;
    }

    loopptr = (mainloop_pt)ptr;
    if (loopptr->context) {
        zfree(loopptr->context);
    }
}

void __mloop_on_free(struct lobj *lop)
{
    mainloop_pt loopptr;

    if (!lop) {
        return;
    }

    loopptr = lobj_body(mainloop_pt, lop);
}

mainloop_pt mloop_initial(const jconf_entry_pt jentry)
{
    lobj_pt lop;
    mainloop_pt loopptr;
    struct lobj_fx fx;

    if (!jentry) {
        return NULL;
    }

    fx.freeproc = &__mloop_on_free;
    fx.referproc = &__mloop_on_refer;
    fx.writeproc = NULL;
    lop = lobj_create("mainloop", jentry->module, sizeof(mainloop_t), &fx);
    if (!lop) {
        return NULL;
    }
    loopptr = lobj_body(mainloop_pt, lop);

    // load all entry procedure which defined in json configure file, ignore failed
    loopptr->preinitproc = lobj_dlsym(lop, jentry->preinitproc);
    loopptr->postinitproc = lobj_dlsym(lop, jentry->postinitproc);
    loopptr->exitproc = lobj_dlsym(lop, jentry->exitproc);
    loopptr->timerproc = lobj_dlsym(lop, jentry->timerproc);

    // simple copy timer interval and context size
    loopptr->interval = jentry->interval;
    loopptr->ctxsize = jentry->ctxsize;

    // allocate mainloop context
    if (loopptr->ctxsize > 0) {
        loopptr->context = (unsigned char *)ztrycalloc(loopptr->ctxsize);
    }

    return loopptr;
}

nsp_status_t mloop_exec_preinit(mainloop_pt mloop,int argc, char **argv)
{
    nsp_status_t status;

    if (mloop->preinitproc) {
        status = mloop->preinitproc(argc, argv);
    } else {
        status = NSP_STATUS_SUCCESSFUL;
    }
    return status;
}

void mloop_exec_postinit(mainloop_pt mloop)
{
    if (mloop->postinitproc) {
        mloop->postinitproc(mloop->context, mloop->ctxsize);
    }
}

void mloop_exec_exit(mainloop_pt mloop)
{
    if (mloop->exitproc) {
        mloop->exitproc();
    }
}

int mloop_exec_on_timer(mainloop_pt mloop)
{
    if (mloop->timerproc) {
        mloop->timerproc(mloop->context, mloop->ctxsize);
    }

    return (int)mloop->interval;
}
