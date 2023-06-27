#pragma once

#include "compiler.h"

#define JCONF_NORMAL_NAMELEN                    64
#define JCONF_DECLARE_NORMAL_STRING(text)       char text[JCONF_NORMAL_NAMELEN]

struct jconf_entry
{
    JCONF_DECLARE_NORMAL_STRING(module);
    JCONF_DECLARE_NORMAL_STRING(entryproc);
    JCONF_DECLARE_NORMAL_STRING(exitproc);
    JCONF_DECLARE_NORMAL_STRING(timerproc);
    unsigned int timer_interval_millisecond;
    unsigned int timer_context_size;
};
typedef struct jconf_entry jconf_entry_t, *jconf_entry_pt;

struct jconf_lwp
{
    JCONF_DECLARE_NORMAL_STRING(module);
    JCONF_DECLARE_NORMAL_STRING(name);
    JCONF_DECLARE_NORMAL_STRING(execproc);
    unsigned int stacksize;
    unsigned int priority;
    unsigned int contextsize;
    unsigned int affinity;
};
typedef struct jconf_lwp jconf_lwp_t, *jconf_lwp_pt;
typedef struct jconf_lwp_iterator *jconf_lwp_iterator_pt;

extern nsp_status_t jconf_initial_load(const char *jsonfile);
extern jconf_entry_pt jconf_entry_get(void);

extern jconf_lwp_iterator_pt jconf_lwp_get_iterator(unsigned int *count);
extern void jconf_release_iterator(jconf_lwp_iterator_pt iterator);
extern jconf_lwp_iterator_pt jconf_lwp_get(jconf_lwp_iterator_pt iterator, jconf_lwp_pt *lwp);
