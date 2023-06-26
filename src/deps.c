#include "deps.h"
#include "ifos.h"

#include <stdio.h>

static dep_entry_t g_dep_entry = { 0 };

dep_entry_pt dep_initial_entry(const jconf_entry_pt jentry)
{
    g_dep_entry.handle = ifos_dlopen(jentry->module);
    if (!g_dep_entry.handle) {
        printf("ifos_dlopen failed, module = %s\n", ifos_dlerror());
        return NULL;
    }

    // load all entry procedure which defined in json configure file, ignore failed
    g_dep_entry.entryproc = ifos_dlsym(g_dep_entry.handle, jentry->entryproc);
    g_dep_entry.exitproc = ifos_dlsym(g_dep_entry.handle, jentry->exitproc);
    g_dep_entry.timerproc = ifos_dlsym(g_dep_entry.handle, jentry->timerproc);

    // simple copy timer interval and context size
    g_dep_entry.timer_interval_millisecond = jentry->timer_interval_millisecond;
    g_dep_entry.timer_context_size = jentry->timer_context_size;

    return &g_dep_entry;
}

nsp_status_t dep_exec_entry(int argc, char **argv)
{
    if (g_dep_entry.entryproc) {
        return g_dep_entry.entryproc(argc, argv);
    }

    return NSP_STATUS_FATAL;
}

void dep_exec_exit(void)
{
    if (g_dep_entry.exitproc) {
        g_dep_entry.exitproc();
    }

    if (g_dep_entry.handle) {
        ifos_dlclose(g_dep_entry.handle);
        g_dep_entry.handle = NULL;
    }
}

void dep_exec_timer(void *context, unsigned int ctxsize)
{
    if (g_dep_entry.timerproc) {
        g_dep_entry.timerproc(context, ctxsize);
    }
}

unsigned int dep_get_entry_timer_interval(void)
{
    return g_dep_entry.timer_interval_millisecond;
}

unsigned int dep_get_entry_context_size()
{
    return g_dep_entry.timer_context_size;
}
