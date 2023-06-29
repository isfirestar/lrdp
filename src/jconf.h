#pragma once

#include "compiler.h"

#define JCONF_NORMAL_NAMELEN                    64
#define JCONF_DECLARE_NORMAL_STRING(text)       char text[JCONF_NORMAL_NAMELEN]
#define JCONF_DECLARE_STRING(text, lenght)      char text[lenght]

typedef struct jconf_iterator *jconf_iterator_pt;
extern void jconf_release_iterator(jconf_iterator_pt iterator);

struct jconf_entry
{
    JCONF_DECLARE_NORMAL_STRING(module);
    JCONF_DECLARE_NORMAL_STRING(preinitproc);
    JCONF_DECLARE_NORMAL_STRING(postinitproc);
    JCONF_DECLARE_NORMAL_STRING(exitproc);
    JCONF_DECLARE_NORMAL_STRING(timerproc);
    unsigned int ctxsize;
    unsigned int interval;
};
typedef struct jconf_entry jconf_entry_t, *jconf_entry_pt;

extern nsp_status_t jconf_initial_load(const char *jsonfile);
extern jconf_entry_pt jconf_entry_get(void);

struct jconf_lwp
{
    JCONF_DECLARE_NORMAL_STRING(name);
    JCONF_DECLARE_NORMAL_STRING(module);
    JCONF_DECLARE_NORMAL_STRING(execproc);
    unsigned int stacksize;
    unsigned int priority;
    unsigned int contextsize;
    unsigned int affinity;
};
typedef struct jconf_lwp jconf_lwp_t, *jconf_lwp_pt;

extern jconf_iterator_pt jconf_lwp_get_iterator(unsigned int *count);
extern jconf_iterator_pt jconf_lwp_get(jconf_iterator_pt iterator, jconf_lwp_pt *jlwp);

#define JCFG_PROTO_ERR  (0)
#define JCFG_PROTO_TCP  (6)
#define JCFG_PROTO_UDP  (17)
struct jconf_net
{
    JCONF_DECLARE_NORMAL_STRING(name);
    JCONF_DECLARE_NORMAL_STRING(module);
    JCONF_DECLARE_STRING(listen, 24);
    JCONF_DECLARE_STRING(remote, 24);
    JCONF_DECLARE_STRING(local, 24);
    JCONF_DECLARE_NORMAL_STRING(acceptproc);
    JCONF_DECLARE_NORMAL_STRING(recvproc);
    JCONF_DECLARE_NORMAL_STRING(closeproc);
    JCONF_DECLARE_NORMAL_STRING(connectproc);
    unsigned int protocol;
    unsigned int contextsize;
};
typedef struct jconf_net jconf_net_t, *jconf_net_pt;

extern jconf_iterator_pt jconf_net_get_iterator(unsigned int *count);
extern jconf_iterator_pt jconf_net_get(jconf_iterator_pt iterator, jconf_net_pt *jnets);

struct jconf_tty
{
    JCONF_DECLARE_NORMAL_STRING(name);
    JCONF_DECLARE_NORMAL_STRING(module);
    JCONF_DECLARE_NORMAL_STRING(recvproc);
    JCONF_DECLARE_STRING(device, 128);
    unsigned int baudrate;
    unsigned int databits;
    unsigned int stopbits;
    JCONF_DECLARE_STRING(parity, 8);
    JCONF_DECLARE_STRING(flowcontrol, 16);
    unsigned int contextsize;
};
typedef struct jconf_tty jconf_tty_t, *jconf_tty_pt;

extern jconf_iterator_pt jconf_tty_get_iterator(unsigned int *count);
extern jconf_iterator_pt jconf_tty_get(jconf_iterator_pt iterator, jconf_tty_pt *jttys);
