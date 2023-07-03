#include "rawobj.h"
#include "ifos.h"

lobj_pt rawobj_create(const jconf_rawobj_pt jrawobj)
{
    lobj_pt lop;
    struct lobj_fx fx;
    void *dlhandle;

    dlhandle = ifos_dlopen(jrawobj->head.module);
    if (!dlhandle) {
        return NULL;
    }
    fx.freeproc = ifos_dlsym(dlhandle, jrawobj->freeproc);
    fx.referproc = ifos_dlsym(dlhandle, jrawobj->referproc);
    fx.writeproc = ifos_dlsym(dlhandle, jrawobj->writeproc);
    fx.vwriteproc = ifos_dlsym(dlhandle, jrawobj->vwriteproc);

    lop = lobj_create(jrawobj->head.name, jrawobj->head.module, 0, jrawobj->head.ctxsize, &fx);
    if (lop) {
        if (jrawobj->init) {
            ;
        }
    }
    return lop;
}
