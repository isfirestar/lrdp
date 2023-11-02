#include "mainloop.h"
#include "lwpobj.h"
#include "netobj.h"
#include "ttyobj.h"
#include "timerobj.h"
#include "redisobj.h"
#include "subscriberobj.h"
#include "rawobj.h"
#include "rand.h"
#include "monotonic.h"
#include "aeobj.h"
#include "mesgqobj.h"
#include "aeobj.h"
#include "udpobj.h"
#include "tcpobj.h"
#include "print.h"

static void __lrdp_load_aeo()
{
    jconf_aeobj_pt jaeo = NULL;
    jconf_iterator_pt iterator;
    unsigned int count;

    iterator = jconf_aeobj_get_iterator(&count);
    while (NULL != (iterator = jconf_aeobj_get(iterator, &jaeo))) {
        aeobj_jcreate(jaeo);
    }
    jconf_aeobj_free();
}


static void __lrdp_load_lwp()
{
    jconf_lwp_pt jlwpcfg = NULL;
    jconf_iterator_pt iterator;
    unsigned int count;

    iterator = jconf_lwp_get_iterator(&count);
    while (NULL != (iterator = jconf_lwp_get(iterator, &jlwpcfg))) {
        lwp_spawn(jlwpcfg);
    }
    jconf_lwp_free();
}

static void __lrdp_load_tty(aeEventLoop *el)
{
    jconf_tty_pt jttycfg = NULL;
    jconf_iterator_pt iterator;
    unsigned int count;

    iterator = jconf_tty_get_iterator(&count);
    while (NULL != (iterator = jconf_tty_get(iterator, &jttycfg))) {
        ttyobj_create(jttycfg, el);
    }
    jconf_tty_free();
}

static void __lrdp_load_timer(aeEventLoop *el)
{
    jconf_timer_pt jtimercfg = NULL;
    jconf_iterator_pt iterator;
    unsigned int count;

    iterator = jconf_timer_get_iterator(&count);
    while (NULL != (iterator = jconf_timer_get(iterator, &jtimercfg))) {
        timerobj_create(jtimercfg, el);
    }
    jconf_timer_free();
}

static void __lrdp_load_redisserver(aeEventLoop *el)
{
    jconf_redis_server_pt jredis_server_cfg = NULL;
    jconf_iterator_pt iterator;
    unsigned int count;

    iterator = jconf_redis_server_get_iterator(&count);
    while (NULL != (iterator = jconf_redis_server_get(iterator, &jredis_server_cfg))) {
        (jredis_server_cfg->na) ? redisobj_create_na(jredis_server_cfg) : redisobj_create(jredis_server_cfg, el);
    }
    jconf_redis_server_free();
}

static void __lrdp_load_subscriber()
{
    jconf_subscriber_pt jsubcfg = NULL;
    jconf_iterator_pt iterator;
    unsigned int count;

    iterator = jconf_subscriber_get_iterator(&count);
    while (NULL != (iterator = jconf_subscriber_get(iterator, &jsubcfg))) {
        subscriberobj_create(jsubcfg);
    }
    jconf_subscriber_free();
}

static void __lrdp_load_raw()
{
    jconf_rawobj_pt jrawcfg = NULL;
    jconf_iterator_pt iterator;
    unsigned int count;

    iterator = jconf_rawobj_get_iterator(&count);
    while (NULL != (iterator = jconf_rawobj_get(iterator, &jrawcfg))) {
        rawobj_create(jrawcfg);
    }
    jconf_rawobj_free();
}

static void __lrdp_load_mesgq(aeEventLoop *el)
{
    jconf_mesgqobj_pt jmesgq = NULL;
    jconf_iterator_pt iterator;
    unsigned int count;

    iterator = jconf_mesgqobj_get_iterator(&count);
    while (NULL != (iterator = jconf_mesgqobj_get(iterator, &jmesgq))) {
        mesgqobj_create(jmesgq, el);
    }
    jconf_mesgqobj_free();
}

static void __lrdp_load_udp()
{
    jconf_udpobj_pt judp = NULL;
    jconf_iterator_pt iterator;
    unsigned int count;

    iterator = jconf_udpobj_get_iterator(&count);
    while (NULL != (iterator = jconf_udpobj_get(iterator, &judp))) {
        udpobj_create(judp);
    }
    jconf_udpobj_free();
}

extern lobj_pt tcpobj_create(const jconf_tcpobj_pt jtcp);  // this function can not define in head file "tcpobj.h"
static void __lrdp_load_tcp()
{
    jconf_tcpobj_pt jtcp = NULL;
    jconf_iterator_pt iterator;
    unsigned int count;

    iterator = jconf_tcpobj_get_iterator(&count);
    while (NULL != (iterator = jconf_tcpobj_get(iterator, &jtcp))) {
        tcpobj_create(jtcp);
    }
    jconf_tcpobj_free();
}

int main(int argc, char *argv[])
{
    nsp_status_t status;
    jconf_entry_pt jentry;
    lobj_pt mlop, aelop;
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
        lrdp_generic_error("jconf_initial_load failed, status = %ld", status);
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
    aelop = aeobj_create("ae-mloop", NULL, 100);
    if (!aelop) {
        return 1;
    }
    el = aeobj_getel(aelop);

    /* load entry module */
    mlop = mloop_create(jentry);
    if (!mlop) {
        return 1;
    }

    /* pre-init applicate */
    if (0 != mloop_pre_init(mlop, argc - 2, argv + 2)) {
        lrdp_generic_error("Mainloop pre-initial proc prevent program running.");
        return 1;
    }

    /* load and create aeo component */
    __lrdp_load_aeo();

    /* load and create multi-thread component */
    __lrdp_load_lwp();

    /* load and create tty component */
    __lrdp_load_tty(el);

    /* load and create timer */
    __lrdp_load_timer(el);

    /* load and create redis server */
    __lrdp_load_redisserver(el);

    /* load and create subscriber object */
    __lrdp_load_subscriber();

    /* load and create raw object */
    __lrdp_load_raw();

    /* load mesgq object */
    __lrdp_load_mesgq(el);

    /* load udp object */
    __lrdp_load_udp();

    /* load tcp object */
    __lrdp_load_tcp();

    /* post init completed message to entry module */
    mloop_post_init(mlop, argc - 2, argv + 2);

    /* start main loop */
    aeobj_run(aelop);
    lobj_ldestroy(aelop);
    lobj_ldestroy(mlop);
    return 0;
}

