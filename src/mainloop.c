#include "mainloop.h"

#include "dep.h"
#include "ifos.h"
#include "dict.h"

#include <stdio.h>

static mainloop_t g_mainloop = { 0 };

mainloop_pt dep_initial_mainloop(const jconf_entry_pt jentry)
{
    strncpy(g_mainloop.module, jentry->module, sizeof(g_mainloop.module) - 1);
    g_mainloop.handle = dep_open_library(g_mainloop.module);
    if (!g_mainloop.handle) {
        printf("ifos_dlopen failed, module = %s\n", ifos_dlerror());
        return NULL;
    }

    // load all entry procedure which defined in json configure file, ignore failed
    g_mainloop.entryproc = ifos_dlsym(g_mainloop.handle, jentry->entryproc);
    g_mainloop.exitproc = ifos_dlsym(g_mainloop.handle, jentry->exitproc);
    g_mainloop.timerproc = ifos_dlsym(g_mainloop.handle, jentry->timerproc);

    // simple copy timer interval and context size
    g_mainloop.timer_interval_millisecond = jentry->timer_interval_millisecond;
    g_mainloop.timer_context_size = jentry->timer_context_size;

    return &g_mainloop;
}

nsp_status_t dep_exec_mainloop_entry(int argc, char **argv)
{
    if (g_mainloop.entryproc) {
        return g_mainloop.entryproc(argc, argv);
    }

    return NSP_STATUS_SUCCESSFUL;
}

void dep_mainloop_atexit(void)
{
    if (g_mainloop.exitproc) {
        g_mainloop.exitproc();
    }
    
    dep_close_library(g_mainloop.module);
}

void dep_mainloop_ontimer(void *context, unsigned int ctxsize)
{
    if (g_mainloop.timerproc) {
        g_mainloop.timerproc(context, ctxsize);
    }
}

unsigned int dep_get_mainloop_timer_interval(void)
{
    return g_mainloop.timer_interval_millisecond;
}

unsigned int dep_get_mainloop_timer_context_size()
{
    return g_mainloop.timer_context_size;
}
