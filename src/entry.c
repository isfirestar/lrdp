#include "mainloop.h"
#include "lwpobj.h"
#include "netobj.h"
#include "ttyobj.h"
#include "rand.h"
#include "monotonic.h"
#include "ae.h"

int main(int argc, char *argv[])
{
    nsp_status_t status;
    jconf_entry_pt jentry;
    jconf_lwp_pt jlwpcfg = NULL;
    jconf_net_pt jnetcfg = NULL;
    jconf_tty_pt jttycfg = NULL;
    jconf_iterator_pt lwp_iterator, net_iterator, tty_iterator;
    unsigned int lwp_count, net_count, tty_count;
    lobj_pt mlop, ttylop;
    aeEventLoop *el;
    
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

    /* initial random seed */
    redisSrand48((int32_t)time(NULL));

    /* initial object management */
    lobj_init();

    /* initial event loop */
    el = aeCreateEventLoop(1);
    if (!el) {
        printf("aeCreateEventLoop failed\n");
        return 1;
    }

    /* load entry module */
    mlop = mloop_create(jentry);
    if (!mlop) {
        printf("mloop_initial failed\n");
        return 1;
    }

    /* pre-init applicate */
    if (0 != mloop_pre_init(mlop, argc - 2, argv + 2)) {
        printf("mainloop pre-initial failed\n");
        return 1;
    }

    /* load and create multi-thread component */
    lwp_iterator = jconf_lwp_get_iterator(&lwp_count);
    while (NULL != (lwp_iterator = jconf_lwp_get(lwp_iterator, &jlwpcfg))) {
        lwp_spawn(jlwpcfg);
    }

    /* load and create networking component */
    net_iterator = jconf_net_get_iterator(&net_count);
    while (NULL != (net_iterator = jconf_net_get(net_iterator, &jnetcfg))) {
        netobj_create(jnetcfg);
    }

    /* load and create tty component */
    tty_iterator = jconf_tty_get_iterator(&tty_count);
    while (NULL != (tty_iterator = jconf_tty_get(tty_iterator, &jttycfg))) {
        if (NULL != (ttylop = ttyobj_create(jttycfg))) {
            ttyobj_add_file(el, ttylop);
        }
    }
    
    mloop_add_timer(el, mlop);
    /* post init completed message to entry module */
    mloop_post_init(mlop);
    
    /* start main loop */
    aeMain(el);
    aeDeleteEventLoop(el);
    lobj_ldestroy(mlop);
    return 0;
}
