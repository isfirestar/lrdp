#include "aeobj.h"

#include "ae.h"

struct aeobj_item
{
    aeEventLoop *el;
    int setsize;
};

void __aeo_free(lobj_pt lop, void *ctx, size_t ctxsize)
{
    struct aeobj_item *aeo;

    aeo = lobj_body(struct aeobj_item *, lop);

    if (aeo->el) {
        aeStop(aeo->el);
        aeDeleteEventLoop(aeo->el);
    }
}

lobj_pt aeobj_create(const char *name, const char *module, int setsize)
{
    lobj_pt lop;
    struct lobj_fx fx = { NULL };
    struct aeobj_item *aeo;

    fx.freeproc = &__aeo_free;
    lop = lobj_create(name, module, sizeof(*aeo), 0, &fx);
    if (!lop) {
        return NULL;
    }
    aeo = lobj_body(struct aeobj_item *, lop);

    aeo->el = aeCreateEventLoop(setsize);

    return lop;
}

lobj_pt aeobj_jcreate(const jconf_aeobj_pt jaecfg)
{
    lobj_pt lop;
    struct lobj_fx_sym sym = { NULL };
    struct aeobj_item *aeo;

    lop = aeobj_create(jaecfg->head.name, jaecfg->head.module, jaecfg->setsize);
    if (!lop) {
        return NULL;
    }

    sym.touchproc_sym = jaecfg->head.touchproc;
    sym.freeproc_sym = NULL;
    sym.writeproc_sym = jaecfg->head.writeproc;
    sym.vwriteproc_sym = jaecfg->head.vwriteproc;
    sym.readproc_sym = jaecfg->head.readproc;
    sym.vreadproc_sym = jaecfg->head.vreadproc;
    sym.recvdataproc_sym = jaecfg->head.recvdataproc;
    sym.rawinvokeproc_sym = jaecfg->head.rawinvokeproc;
    lobj_fx_cover(lop, &sym);

    aeo = lobj_body(struct aeobj_item *, lop);
    aeo->setsize = jaecfg->setsize;
    if (jaecfg->setsize <= 0) {
        aeo->setsize = 256;
    }
    aeo->el = aeCreateEventLoop(aeo->setsize);
    return lop;
}

aeEventLoop *aeobj_getel(lobj_pt lop)
{
    struct aeobj_item *aeo;

    aeo = lobj_body(struct aeobj_item *, lop);

    return aeo->el;
}

void aeobj_run(lobj_pt lop)
{
    struct aeobj_item *aeo;

    aeo = lobj_body(struct aeobj_item *, lop);

    aeMain(aeo->el);
}

void aeobj_onecycle(lobj_pt lop)
{
    struct aeobj_item *aeo;

    aeo = lobj_body(struct aeobj_item *, lop);

    aeProcessEvents(aeo->el, AE_ALL_EVENTS);
}
