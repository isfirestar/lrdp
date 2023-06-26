#pragma once

#include "compiler.h"

struct jconf_entry
{
    char module[64];
    char entryproc[64];
    char exitproc[64];
    char timerproc[64];
    unsigned int timer_interval_millisecond;
    unsigned int timer_context_size;
};
typedef struct jconf_entry jconf_entry_t, *jconf_entry_pt;

extern nsp_status_t jconf_initial_load(const char *jsonfile);
extern jconf_entry_pt jconf_entry_get(void);
