#include "lwpmgr.h"

#include "clist.h"
#include "deps.h"
#include "ifos.h"

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

    lwp_setname(&inner->body.obj, inner->body.name);
    lwp_setaffinity(&inner->body.obj, inner->body.affinity);

    /* insert into manager queue */
    list_add_tail(&inner->ele_of_inner_lwp, &lwps);
    lwps_count++;

    /* insert into dep manager */
    dep_set_handle(inner->body.name, inner->body.module);

    if (inner->body.execproc) {
        retval = inner->body.execproc(inner->body.context);
    } else {
        retval = NULL;
    }

    /* after thread proc terminated, release resource */
    list_del(&inner->ele_of_inner_lwp);
    lwps_count--;
    dep_set_handle(inner->body.name, NULL);
    if (inner->body.context) {
        zfree(inner->body.context);
    }

    lwp_exit(retval);
    return retval;
}

nsp_status_t lwp_spawn(const jconf_lwp_pt jlwpcfg)
{
    struct lwp_item_inner *inner;
    struct lwp_item *lwp;
    nsp_status_t status;

    if (!jlwpcfg) {
        return posix__makeerror(EINVAL);
    }

    inner = ztrymalloc(sizeof(*inner));
    if (!inner) {
        return posix__makeerror(ENOMEM);
    }

    lwp = &inner->body;
    
    lwp->module = dep_get_handle(jlwpcfg->module);
    if (!lwp->module) {
        lwp->module = ifos_dlopen(jlwpcfg->module);
        if (!lwp->module) {
            zfree(inner);
            return posix__makeerror(EINVAL);
        }
    }

    do {
        lwp->execproc = ifos_dlsym(lwp->module, jlwpcfg->execproc);
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

        strncpy(lwp->name, jlwpcfg->name, sizeof(lwp->name) - 1);
        lwp->stacksize = jlwpcfg->stacksize;
        lwp->priority = jlwpcfg->priority;
        lwp->affinity = jlwpcfg->affinity;

        /* create thread */
        status = lwp_create(&lwp->obj, jlwpcfg->priority, __lwp_start_rtn, inner);
    } while (0);
    
    if (!NSP_SUCCESS(status)) {
        if (lwp->context) {
            zfree(lwp->context);
        }
        if (lwp->module) {
            ifos_dlclose(lwp->module);
        }
        zfree(inner);
    }
    return status;
}
