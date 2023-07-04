#include "mainloop.h"

#include "lobj.h"
#include "zmalloc.h"

#include <stdio.h>

struct mainloop
{
    int (*preinitproc)(lobj_pt lop, int argc, char **argv);
    void (*postinitproc)(lobj_pt lop);
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
    struct lobj_fx fx = {
        .freeproc = &__mloop_atexit,
        .vwriteproc = NULL,
        .writeproc = NULL,
        .readproc = NULL,
        .vreadproc = NULL,
    };

    if (!jentry) {
        return NULL;
    }

    lop = lobj_create("mainloop", jentry->head.module, sizeof(mainloop_t), jentry->head.ctxsize, &fx);
    if (!lop) {
        return NULL;
    }
    mloop = lobj_body(mainloop_pt, lop);
    // freeproc cannot be covered
    lobj_cover_fx(lop, NULL, jentry->head.vwriteproc, jentry->head.writeproc, jentry->head.readproc, jentry->head.vreadproc);

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
