#include "rawobj.h"
#include "ifos.h"

lobj_pt rawobj_create(const jconf_rawobj_pt jrawobj)
{
    lobj_pt lop;
    void (*rawinitproc)(lobj_pt lop);
    struct lobj_fx_sym sym = { NULL };

    lop = lobj_create(jrawobj->head.name, jrawobj->head.module, 0, jrawobj->head.ctxsize, NULL);
    if (lop) {
        sym.freeproc_sym = jrawobj->head.freeproc;
        sym.writeproc_sym = jrawobj->head.writeproc;
        sym.vwriteproc_sym = jrawobj->head.vwriteproc;
        sym.readproc_sym = jrawobj->head.readproc;
        sym.vreadproc_sym = jrawobj->head.vreadproc;
        sym.recvdataproc_sym = jrawobj->head.recvdataproc;
        sym.rawinvokeproc_sym = jrawobj->head.rawinvokeproc;
        lobj_fx_load(lop, &sym);
        
        if (0 != jrawobj->initproc) {
            rawinitproc = lobj_dlsym(lop, jrawobj->initproc);
            if (rawinitproc) {
                rawinitproc(lop);
            }
        }
    }
    return lop;
}
