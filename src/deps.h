#pragma once

#include "jconf.h"

struct dep_entry
{
    void *handle;
    nsp_status_t (*entryproc)(int argc, char **argv);
    void (*exitproc)(void);
    void (*timerproc)(void *context, unsigned int ctxsize);
    unsigned int timer_interval_millisecond;
    unsigned int timer_context_size;
};
typedef struct dep_entry dep_entry_t, *dep_entry_pt;

extern dep_entry_pt dep_initial_entry(const jconf_entry_pt jentry);
extern nsp_status_t dep_exec_entry(int argc, char **argv);
extern void dep_exec_exit(void);
extern void dep_exec_timer(void *context, unsigned int ctxsize);
extern unsigned int dep_get_entry_timer_interval(void);
extern unsigned int dep_get_entry_context_size();
