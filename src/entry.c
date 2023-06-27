#include "jconf.h"
#include "ifos.h"
#include "mainloop.h"
#include "ae.h"
#include "zmalloc.h"
#include "lwpmgr.h"

#include <stdio.h>

static int __lrdp_entry_timer(struct aeEventLoop *eventLoop, long long id, void *clientData)
{
    return mloop_exec_on_timer((mainloop_pt)clientData);
}

static int __lrdp_entry_loop(const mainloop_pt mloop)
{
    aeEventLoop *aeloop;
    long long timerid;

    /* ok, all other initialize have been finish, invoke post init proc if exist */
    mloop_exec_postinit(mloop);

    /*create a timer for main loop */
    aeloop = aeCreateEventLoop(1);
    if (aeloop) {
        timerid = aeCreateTimeEvent(aeloop, (long long)mloop->interval, &__lrdp_entry_timer, mloop, NULL);
        if (timerid != AE_ERR) {
            /* main loop */
            aeMain(aeloop);
        }
    }

    /* call exit proc and release resource */
    mloop_exec_exit(mloop);

    /* release resource */
    if (aeloop) {
        aeDeleteEventLoop(aeloop);
    }

    return 0;
}

int main(int argc, char *argv[])
{
    nsp_status_t status;
    jconf_entry_pt jentry;
    jconf_lwp_iterator_pt lwp_iterator;
    jconf_lwp_pt jlwpcfg = NULL;
    unsigned int lwp_count;
    mainloop_pt mloop;

    /* check program startup parameters, the 2st argument MUST be the path of configure json file
     * if count of argument less than or equal to 1, terminate the program */
    if (argc <= 1)  {
        printf("useage lrdp [config-file]\n");
        return 1;
    }

    /* read json config and load parameters */
    status = jconf_initial_load(argv[1]);
    if (!NSP_SUCCESS(status)) {
        printf("jconf_initial_load failed, status = %ld\n", status);
        return 1;
    }
    jentry = jconf_entry_get();

    /* initial critical component */
    monotonicInit();

    /* load entry module */
    mloop = mloop_initial(jentry);
    if (!mloop) {
        printf("mloop_initial failed\n");
        return 1;
    }

    /* call entry proc, ignore if not registed,
        but we shall terminated program if user return fatal when entry function called */
    status = mloop_exec_preinit(mloop, argc - 2, argv + 2);
    if (!NSP_SUCCESS(status)) {
        printf("mloop_exec_preinit failed, status = %ld\n", status);
        mloop_exec_exit(mloop);
        return 1;
    }

    /* load and create multi-thread component */
    lwp_iterator = jconf_lwp_get_iterator(&lwp_count);
    while (NULL != (lwp_iterator = jconf_lwp_get(lwp_iterator, &jlwpcfg))) {
        lwp_spawn(jlwpcfg);
    }

    /* finally, execute the main loop in entry module */
    return __lrdp_entry_loop(mloop);
}
