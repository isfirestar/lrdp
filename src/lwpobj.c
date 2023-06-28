#include "lwpobj.h"

#include "clist.h"
#include "lobj.h"
#include "threading.h"

struct lwp_item
{
    void *(*execproc)(void *, unsigned int);
    lwp_t thread;
    unsigned int stacksize;
    unsigned int priority;
    unsigned int ctxsize;
    unsigned int affinity;
    unsigned char *context;
    struct list_head ele_local;
};

static struct list_head lwps = LIST_HEAD_INIT(lwps);
static unsigned int lwps_count = 0;

static void __ilwp_on_free(struct lobj *lop)
{
    struct lwp_item *lwp;

    lwp = lobj_body(struct lwp_item *, lop);
    if (!lwp) {
        return;
    }

    if (lwp->context) {
        zfree(lwp->context);
    }

    list_del_init(&lwp->ele_local);
    lwps_count--;
}

static void __ilwp_on_refer(struct lobj *lop)
{

}

static void *__lwp_start_rtn(void *parameter)
{
    lobj_pt lop;
    struct lwp_item *lwp;
    void *retval;

    lop = lobj_refer((const char *)parameter);
    if (!lop) {
        return NULL;
    }
    lwp = lobj_body(struct lwp_item *, lop);

    /* save name of this thread which specify by json config */
    lwp_setname(&lwp->thread, lop->name);

    /* adjust thread CPU affinity if needed */
    lwp_setaffinity(&lwp->thread, lwp->affinity);

    /* invoke module handler function */
    if (lwp->execproc) {
        retval = lwp->execproc(lwp->context, lwp->ctxsize);
    } else {
        retval = NULL;
    }

    lobj_derefer(lop);
    lobj_ldestroy(lop);
    lwp_exit(retval);
    return retval;
}

nsp_status_t lwp_spawn(const jconf_lwp_pt jlwpcfg)
{
    nsp_status_t status;
    struct lwp_item *lwp;
    lobj_pt lop;
    struct lobj_fx fx;

    if (!jlwpcfg) {
        return posix__makeerror(EINVAL);
    }

    fx.freeproc = &__ilwp_on_free;
    fx.referproc = &__ilwp_on_refer;
    fx.writeproc = NULL;
    lop = lobj_create(jlwpcfg->name, jlwpcfg->module, sizeof(struct lwp_item), &fx);
    if (!lop) {
        return posix__makeerror(ENOMEM);
    }
    lwp = lobj_body(struct lwp_item *, lop);

    do {
        lwp->execproc = lobj_dlsym(lop, jlwpcfg->execproc);
        lwp->stacksize = jlwpcfg->stacksize;
        lwp->priority = jlwpcfg->priority;
        lwp->affinity = jlwpcfg->affinity;
        /* determine the thread user data size in bytes */
        lwp->ctxsize = jlwpcfg->contextsize;
        if (lwp->ctxsize > 0) {
            lwp->context = (unsigned char *)ztrycalloc(lwp->ctxsize);  // ignore memory allocation error, we can pass a null pointer to thread entry function
        }
        /* link to local list */
        list_add_tail(&lwp->ele_local, &lwps);
        lwps_count++;
        /* create thread */
        status = lwp_create(&lwp->thread, lwp->priority, &__lwp_start_rtn, lop->name);
    } while (0);

    if (!NSP_SUCCESS(status)) {
        lobj_ldestroy(lop);
    }
    return status;
}
