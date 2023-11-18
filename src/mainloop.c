#include "mainloop.h"

#include "lobj.h"
#include "zmalloc.h"

#include <stdio.h>

struct mainloop
{
    int (*preinitproc)(lobj_pt lop, int argc, char **argv);
    int (*postinitproc)(lobj_pt lop, int argc, char **argv);
};
typedef struct mainloop mainloop_t, *mainloop_pt;

void __mloop_atexit(struct lobj *lop, void *ctx, size_t ctxsize)
{
    lobj_fx_free(lop);
}

lobj_pt mloop_create(const jconf_entry_pt jentry)
{
    lobj_pt lop;
    mainloop_pt mloop;
    struct lobj_fx_sym sym = { NULL };
    struct lobj_fx fx = { NULL };

    if (!jentry) {
        return NULL;
    }

    fx.freeproc = &__mloop_atexit;
    lop = lobj_create("mainloop", jentry->head.module, sizeof(mainloop_t), jentry->head.ctxsize, &fx);
    if (!lop) {
        return NULL;
    }
    mloop = lobj_body(mainloop_pt, lop);

    // freeproc can not covert
    sym.touchproc_sym = jentry->head.touchproc;
    sym.freeproc_sym = jentry->head.freeproc;
    sym.writeproc_sym = jentry->head.writeproc;
    sym.vwriteproc_sym = jentry->head.vwriteproc;
    sym.readproc_sym = jentry->head.readproc;
    sym.vreadproc_sym = jentry->head.vreadproc;
    sym.recvdataproc_sym = jentry->head.recvdataproc;
    sym.rawinvokeproc_sym = jentry->head.rawinvokeproc;
    lobj_fx_cover(lop, &sym);

    // load all entry procedure which defined in json configure file, ignore failed
    mloop->preinitproc = lobj_dlsym(lop, jentry->preinitproc);
    mloop->postinitproc = lobj_dlsym(lop, jentry->postinitproc);
    return lop;
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

int mloop_post_init(lobj_pt lop, int argc, char **argv)
{
    mainloop_pt mloop;
    int exit;

    mloop = lobj_body(mainloop_pt, lop);
    exit = 0;

    if (mloop->postinitproc) {
        exit = mloop->postinitproc(lop, argc, argv);
    }

    return exit;
}
