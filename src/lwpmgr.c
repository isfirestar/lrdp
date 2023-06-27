#include "lwpmgr.h"

#include "clist.h"
#include "dep.h"
#include "ifos.h"
#include "atom.h"

struct lwp_item_inner
{   
    struct lwp_item body;
    struct list_head ele_of_inner_lwp;
};
static struct list_head lwps = LIST_HEAD_INIT(lwps);
static unsigned int lwps_count = 0;

static void *__lwp_start_rtn(void *parameter)
{
    struct lwp_item_inner *inner = (struct lwp_item_inner *)parameter;
    void *retval;
    unsigned char *context;

    /* save name of this thread which specify by json config */
    lwp_setname(&inner->body.obj, inner->body.module);

    /* adjust thread CPU affinity if needed */
    lwp_setaffinity(&inner->body.obj, inner->body.affinity);

    /* insert into manager queue */
    list_add_tail(&inner->ele_of_inner_lwp, &lwps);
    lwps_count++;

    /* invoke module handler function */
    if (inner->body.execproc) {
        retval = inner->body.execproc(inner->body.context);
    } else {
        retval = NULL;
    }

    /* after thread proc terminated, release resource */
    list_del(&inner->ele_of_inner_lwp);
    lwps_count--;

    /* release body context resource */
    context = __atomic_exchange_n(&inner->body.context, NULL, __ATOMIC_ACQ_REL);
    if (context) {
        zfree(context);
    }

    lwp_exit(retval);
    return retval;
}

nsp_status_t lwp_spawn(const jconf_lwp_pt jlwpcfg)
{
    struct lwp_item_inner *inner;
    struct lwp_item *lwp;
    nsp_status_t status;
    unsigned char *context;

    if (!jlwpcfg) {
        return posix__makeerror(EINVAL);
    }

    inner = ztrymalloc(sizeof(*inner));
    if (!inner) {
        return posix__makeerror(ENOMEM);
    }

    lwp = &inner->body;
    
    strncpy(lwp->module, jlwpcfg->module, sizeof(lwp->module) - 1);
    lwp->handle = dep_open_library(lwp->module);
    if (!lwp->handle) {
        zfree(inner);
        return posix__makeerror(EINVAL);
    }

    do {
        lwp->execproc = ifos_dlsym(lwp->handle, jlwpcfg->execproc);
        if (!lwp->execproc) {
            status = posix__makeerror(EINVAL);
            break;
        }

        /* determine the thread user data size in bytes */
        lwp->contextsize = jlwpcfg->contextsize;
        lwp->context = (unsigned char *)ztrymalloc(lwp->contextsize);
        if (!lwp->context) {
            status = posix__makeerror(ENOMEM);
            break;
        }

        lwp->stacksize = jlwpcfg->stacksize;
        lwp->priority = jlwpcfg->priority;
        lwp->affinity = jlwpcfg->affinity;

        /* create thread */
        status = lwp_create(&lwp->obj, jlwpcfg->priority, &__lwp_start_rtn, inner);
    } while (0);
    
    if (!NSP_SUCCESS(status)) {
        context = __atomic_exchange_n(&inner->body.context, NULL, __ATOMIC_ACQ_REL);
        if (context) {
            zfree(context);
        }

        // do NOT close module on failed
        // if (lwp->module) {
        //     dep_close_module(lwp->module);
        // }
        zfree(inner);
    }
    return status;
}
