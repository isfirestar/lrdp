#include "jconf.h"
#include "ifos.h"
#include "mainloop.h"
#include "ae.h"
#include "zmalloc.h"
#include "lwpmgr.h"

#include <stdio.h>

static int __lrdp_entry_timer(struct aeEventLoop *eventLoop, long long id, void *clientData)
{
    dep_mainloop_ontimer(clientData, dep_get_mainloop_timer_context_size());
    return (int)dep_get_mainloop_timer_interval();
}

static int __lrdp_entry_loop(const mainloop_pt p_mainloop)
{
    aeEventLoop *aeloop;
    long long timerid;
    unsigned char *timerctx;

    /* all ok now, create a timer for main loop */
    timerctx = NULL;
    aeloop = aeCreateEventLoop(1);
    if (aeloop) {
        if (p_mainloop->timer_context_size > 0) {
            timerctx = (unsigned char *)ztrycalloc(p_mainloop->timer_context_size);
        }
        if (timerctx) {
            memset(timerctx, 0, p_mainloop->timer_context_size);
        }
        timerid = aeCreateTimeEvent(aeloop, (long long)p_mainloop->timer_interval_millisecond, &__lrdp_entry_timer, timerctx, NULL);
        if (timerid != AE_ERR) {
            /* main loop */
            aeMain(aeloop);
        }
    }

    /* call exit proc */
    dep_mainloop_atexit();
    /* release resource */
    if (aeloop) {
        aeDeleteEventLoop(aeloop);
    }
    if (timerctx) {
        zfree(timerctx);
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
    mainloop_pt p_mainloop;

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

    /* load entry module */
    p_mainloop = dep_initial_mainloop(jentry);
    if (!p_mainloop) {
        printf("dep_initial_mainloop failed\n");
        return 1;
    }

    /* call entry proc, ignore if not registed, 
        but we shall terminated program if user return fatal when entry function called */
    status = dep_exec_mainloop_entry(argc - 2, argv + 2);
    if (!NSP_SUCCESS(status)) {
        printf("dep_exec_mainloop_entry failed, status = %ld\n", status);
        dep_mainloop_atexit();
        return 1;
    }

    /* load and create multi-thread component */
    lwp_iterator = jconf_lwp_get_iterator(&lwp_count);
    while (NULL != (lwp_iterator = jconf_lwp_get(lwp_iterator, &jlwpcfg))) {
        lwp_spawn(jlwpcfg);
    }

    /* finally, execute the main loop in entry module */
    return __lrdp_entry_loop(p_mainloop);
}
