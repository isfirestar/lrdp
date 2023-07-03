#pragma once

#include "compiler.h"
#include "clist.h"

#define JCONF_NORMAL_NAMELEN                    64
#define JCONF_DECLARE_NORMAL_STRING(text)       char text[JCONF_NORMAL_NAMELEN]
#define JCONF_DECLARE_STRING(text, lenght)      char text[lenght]

struct jconf_head
{
    JCONF_DECLARE_NORMAL_STRING(name);
    JCONF_DECLARE_NORMAL_STRING(module);
    unsigned int ctxsize;
};

typedef struct jconf_iterator *jconf_iterator_pt;
extern void jconf_release_iterator(jconf_iterator_pt iterator);

struct jconf_entry
{
    struct jconf_head head;
    JCONF_DECLARE_NORMAL_STRING(preinitproc);
    JCONF_DECLARE_NORMAL_STRING(postinitproc);
    JCONF_DECLARE_NORMAL_STRING(exitproc);
};
typedef struct jconf_entry jconf_entry_t, *jconf_entry_pt;

extern nsp_status_t jconf_initial_load(const char *jsonfile);
extern jconf_entry_pt jconf_entry_get(void);

/* -----------------------------------------------------------------------------------------------------------------------------
 * ------------------------------------------        LWP IMPLEMENTATIONs        ------------------------------------------------
 * ----------------------------------------------------------------------------------------------------------------------------- */
struct jconf_lwp
{
    struct jconf_head head;
    JCONF_DECLARE_NORMAL_STRING(module);
    JCONF_DECLARE_NORMAL_STRING(execproc);
    unsigned int stacksize;
    unsigned int priority;
    unsigned int affinity;
};
typedef struct jconf_lwp jconf_lwp_t, *jconf_lwp_pt;

extern jconf_iterator_pt jconf_lwp_get_iterator(unsigned int *count);
extern jconf_iterator_pt jconf_lwp_get(jconf_iterator_pt iterator, jconf_lwp_pt *jlwp);
extern void jconf_lwp_free();

/* -----------------------------------------------------------------------------------------------------------------------------
 * ------------------------------------------        NET IMPLEMENTATIONs        ------------------------------------------------
 * ----------------------------------------------------------------------------------------------------------------------------- */
#define JCFG_PROTO_ERR  (0)
#define JCFG_PROTO_TCP  (6)
#define JCFG_PROTO_UDP  (17)
struct jconf_net
{
    struct jconf_head head;
    JCONF_DECLARE_STRING(listen, 24);
    JCONF_DECLARE_STRING(remote, 24);
    JCONF_DECLARE_STRING(local, 24);
    JCONF_DECLARE_NORMAL_STRING(acceptproc);
    JCONF_DECLARE_NORMAL_STRING(recvproc);
    JCONF_DECLARE_NORMAL_STRING(closeproc);
    JCONF_DECLARE_NORMAL_STRING(connectproc);
    unsigned int protocol;
};
typedef struct jconf_net jconf_net_t, *jconf_net_pt;

extern jconf_iterator_pt jconf_net_get_iterator(unsigned int *count);
extern jconf_iterator_pt jconf_net_get(jconf_iterator_pt iterator, jconf_net_pt *jnets);
extern void jconf_net_free();

/* -----------------------------------------------------------------------------------------------------------------------------
 * ------------------------------------------        TTY IMPLEMENTATIONs        ------------------------------------------------
 * ----------------------------------------------------------------------------------------------------------------------------- */
struct jconf_tty
{
    struct jconf_head head;
    JCONF_DECLARE_NORMAL_STRING(name);
    JCONF_DECLARE_NORMAL_STRING(module);
    JCONF_DECLARE_NORMAL_STRING(recvproc);
    JCONF_DECLARE_STRING(device, 128);
    unsigned int baudrate;
    unsigned int databits;
    unsigned int stopbits;
    JCONF_DECLARE_STRING(parity, 8);
    JCONF_DECLARE_STRING(flowcontrol, 16);
};
typedef struct jconf_tty jconf_tty_t, *jconf_tty_pt;

extern jconf_iterator_pt jconf_tty_get_iterator(unsigned int *count);
extern jconf_iterator_pt jconf_tty_get(jconf_iterator_pt iterator, jconf_tty_pt *jttys);
extern void jconf_tty_free();

/* -----------------------------------------------------------------------------------------------------------------------------
 * ------------------------------------------        TIMER IMPLEMENTATIONs        ----------------------------------------------
 * ----------------------------------------------------------------------------------------------------------------------------- */
struct jconf_timer
{
    struct jconf_head head;
    JCONF_DECLARE_NORMAL_STRING(timerproc);
    unsigned int interval;
};
typedef struct jconf_timer jconf_timer_t, *jconf_timer_pt;

extern jconf_iterator_pt jconf_timer_get_iterator(unsigned int *count);
extern jconf_iterator_pt jconf_timer_get(jconf_iterator_pt iterator, jconf_timer_pt *jtimers);
extern void jconf_timer_free();

/* -----------------------------------------------------------------------------------------------------------------------------
 * ------------------------------------------        REDIS-SERVER IMPLEMENTATIONs        ---------------------------------------
 * ----------------------------------------------------------------------------------------------------------------------------- */
struct jconf_redis_server
{
    struct jconf_head head;
    JCONF_DECLARE_NORMAL_STRING(host);
};
typedef struct jconf_redis_server jconf_redis_server_t, *jconf_redis_server_pt;

extern jconf_iterator_pt jconf_redis_server_get_iterator(unsigned int *count);
extern jconf_iterator_pt jconf_redis_server_get(jconf_iterator_pt iterator, jconf_redis_server_pt *jredis_server);
extern void jconf_redis_server_free();

/* -----------------------------------------------------------------------------------------------------------------------------
 * ------------------------------------------        SUBSCRIBER IMPLEMENTATIONs        -----------------------------------------
 * ----------------------------------------------------------------------------------------------------------------------------- */
struct jconf_subscriber_channel
{
    struct list_head ele_of_channels;
    JCONF_DECLARE_NORMAL_STRING(pattern);
};

struct jconf_subscriber
{
    struct jconf_head head;
    JCONF_DECLARE_NORMAL_STRING(redisserver);
    JCONF_DECLARE_NORMAL_STRING(execproc);
    struct list_head head_of_channels;
    unsigned int channels_count;
};
typedef struct jconf_subscriber jconf_subscriber_t, *jconf_subscriber_pt;

extern jconf_iterator_pt jconf_subscriber_get_iterator(unsigned int *count);
extern jconf_iterator_pt jconf_subscriber_get(jconf_iterator_pt iterator, jconf_subscriber_pt *jsubscriber);
extern void jconf_subscriber_free();

/* -----------------------------------------------------------------------------------------------------------------------------
 * ------------------------------------------        RAWOBJ IMPLEMENTATIONs        -----------------------------------------
 * ----------------------------------------------------------------------------------------------------------------------------- */
struct jconf_rawobj
{
    struct jconf_head head;
    JCONF_DECLARE_NORMAL_STRING(init);
    JCONF_DECLARE_NORMAL_STRING(writeproc);
    JCONF_DECLARE_NORMAL_STRING(vwriteproc);
    JCONF_DECLARE_NORMAL_STRING(freeproc);
    JCONF_DECLARE_NORMAL_STRING(referproc);
};
typedef struct jconf_rawobj jconf_rawobj_t, *jconf_rawobj_pt;

extern jconf_iterator_pt jconf_rawobj_get_iterator(unsigned int *count);
extern jconf_iterator_pt jconf_rawobj_get(jconf_iterator_pt iterator, jconf_rawobj_pt *jrawobj);
extern void jconf_rawobj_free();
