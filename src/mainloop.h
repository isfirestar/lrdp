#pragma once

#include "jconf.h"

struct mainloop
{
    void *handle;
    char module[64];
    nsp_status_t (*entryproc)(int argc, char **argv);
    void (*exitproc)(void);
    void (*timerproc)(void *context, unsigned int ctxsize);
    unsigned int timer_interval_millisecond;
    unsigned int timer_context_size;
};
typedef struct mainloop mainloop_t, *mainloop_pt;

extern mainloop_pt dep_initial_mainloop(const jconf_entry_pt jentry);
extern nsp_status_t dep_exec_mainloop_entry(int argc, char **argv);
extern void dep_mainloop_atexit(void);
extern void dep_mainloop_ontimer(void *context, unsigned int ctxsize);
extern unsigned int dep_get_mainloop_timer_interval(void);
extern unsigned int dep_get_mainloop_timer_context_size();
