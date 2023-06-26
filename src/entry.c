#include "jconf.h"
#include "ifos.h"
#include "deps.h"
#include "ae.h"
#include "zmalloc.h"

#include <stdio.h>

static int dep_entry_timer(struct aeEventLoop *eventLoop, long long id, void *clientData)
{
    dep_exec_timer(clientData, dep_get_entry_context_size());
    return (int)dep_get_entry_timer_interval();
}

int main(int argc, char *argv[])
{
    nsp_status_t status;
    jconf_entry_pt jentry;
    dep_entry_pt depentry;
    aeEventLoop *aeloop;
    long long timerid;
    unsigned char *timerctx;

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
    depentry = dep_initial_entry(jentry);
    if (!depentry) {
        printf("dep_initial_entry failed\n");
        return 1;
    }

    /* call entry proc, MUST success */
    status = dep_exec_entry(argc - 2, argv + 2);
    if (!NSP_SUCCESS(status)) {
        printf("dep_exec_entry failed, status = %ld\n", status);
        dep_exec_exit();
        return 1;
    }

    /* all ok now, create a timer for main loop */
    timerctx = NULL;
    aeloop = aeCreateEventLoop(1);
    if (aeloop) {
        if (depentry->timer_context_size > 0) {
            timerctx = (unsigned char *)ztrymalloc(depentry->timer_context_size);
        }
        if (timerctx) {
            memset(timerctx, 0, depentry->timer_context_size);
        }
        timerid = aeCreateTimeEvent(aeloop, (long long)dep_get_entry_timer_interval(), &dep_entry_timer, timerctx, NULL);
        if (timerid != AE_ERR) {
            /* main loop */
            aeMain(aeloop);
        }
    }

    /* call exit proc */
    dep_exec_exit();
    /* release resource */
    if (aeloop) {
        aeDeleteEventLoop(aeloop);
    }
    if (timerctx) {
        zfree(timerctx);
    }

    return 0;
}
