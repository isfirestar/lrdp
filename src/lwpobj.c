#include "lwpobj.h"
#include "lobj.h"
#include "threading.h"

struct lwp_item
{
    void *(*execproc)(lobj_pt lop);
    lwp_t thread;
    unsigned int stacksize;
    unsigned int priority;
    unsigned int ctxsize;
    unsigned int affinity;
};

static void *__lwp_start_rtn(void *parameter)
{
    lobj_pt lop;
    struct lwp_item *lwp;
    void *retval;
    void *objctx;
    size_t objctxsize;

    retval = NULL;
    lop = (lobj_pt)parameter;
    lwp = lobj_body(struct lwp_item *, lop);

    /* save name of this thread which specify by json config */
    lwp_setname(&lwp->thread, lobj_get_name(lop));

    /* adjust thread CPU affinity if needed */
    lwp_setaffinity(&lwp->thread, lwp->affinity);

    /* invoke module handler function */
    if (lwp->execproc) {
        retval = lwp->execproc(lop);
    } else {
        objctxsize = lobj_get_context(lop, &objctx);
        lobj_fx_read(lop, objctx, objctxsize);
    }

    lobj_ldestroy(lop);
    lwp_exit(retval);
    return retval;
}

nsp_status_t lwp_spawn(const jconf_lwp_pt jlwpcfg)
{
    nsp_status_t status;
    struct lwp_item *lwp;
    lobj_pt lop;
    struct lobj_fx fx = {
        .freeproc = NULL,
        .writeproc = NULL,
        .vwriteproc = NULL,
        .readproc = NULL,
        .vreadproc = NULL,
    };

    if (!jlwpcfg) {
        return posix__makeerror(EINVAL);
    }

    lop = lobj_create(jlwpcfg->head.name, jlwpcfg->head.module, sizeof(struct lwp_item), jlwpcfg->head.ctxsize, &fx);
    if (!lop) {
        return posix__makeerror(ENOMEM);
    }
    lwp = lobj_body(struct lwp_item *, lop);
    lobj_cover_fx(lop, jlwpcfg->head.freeproc, jlwpcfg->head.writeproc, jlwpcfg->head.vwriteproc, jlwpcfg->head.readproc, jlwpcfg->head.vreadproc);

    do {
        lwp->execproc = lobj_dlsym(lop, jlwpcfg->execproc);
        lwp->stacksize = jlwpcfg->stacksize;
        lwp->priority = jlwpcfg->priority;
        lwp->affinity = jlwpcfg->affinity;
        /* create thread */
        status = lwp_create(&lwp->thread, lwp->priority, &__lwp_start_rtn, lop);
    } while (0);

    if (!NSP_SUCCESS(status)) {
        lobj_ldestroy(lop);
    }
    return status;
}
