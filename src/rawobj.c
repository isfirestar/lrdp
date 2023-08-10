#include "rawobj.h"
#include "ifos.h"

lobj_pt rawobj_create(const jconf_rawobj_pt jrawobj)
{
    lobj_pt lop;
    struct lobj_fx fx = {
        .freeproc = NULL,
        .writeproc = NULL,
        .vwriteproc = NULL,
        .readproc = NULL,
        .vreadproc = NULL,
    };
    void (*rawinitproc)(lobj_pt lop);

    lop = lobj_create(jrawobj->head.name, jrawobj->head.module, 0, jrawobj->head.ctxsize, &fx);
    if (lop) {
        lobj_cover_fx(lop, jrawobj->head.freeproc, jrawobj->head.writeproc, jrawobj->head.vwriteproc, jrawobj->head.readproc, jrawobj->head.vreadproc, NULL);
        if (0 != jrawobj->initproc) {
            rawinitproc = lobj_dlsym(lop, jrawobj->initproc);
            if (rawinitproc) {
                rawinitproc(lop);
            }
        }
    }
    return lop;
}
