#include "mainloop.h"
#include "lwpobj.h"
#include "netobj.h"
#include "rand.h"
#include "monotonic.h"

int main(int argc, char *argv[])
{
    nsp_status_t status;
    jconf_entry_pt jentry;
    jconf_iterator_pt lwp_iterator;
    jconf_iterator_pt net_iterator;
    jconf_lwp_pt jlwpcfg = NULL;
    jconf_net_pt jnetcfg = NULL;
    unsigned int lwp_count;
    unsigned int net_count;
    lobj_pt mloop;

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
    
    /* load entry module */
    mloop = mloop_create(jentry);
    if (!mloop) {
        printf("mloop_initial failed\n");
        return 1;
    }

    /* pre-init applicate */
    mloop_preinit(mloop, argc - 2, argv + 2);

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
    /* finally, execute the main loop in entry module */
    return mloop_run(mloop);
}
