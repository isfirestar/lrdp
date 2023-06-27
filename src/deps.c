#include "deps.h"
#include "ifos.h"
#include "dict.h"

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

    dep_set_handle(jentry->module, g_dep_entry.handle);
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

/* management implementation */
static dict *g_deps_brief = NULL;

static uint64_t __dep_brief_hash_generator(const void *key) 
{
    return dictGenHashFunction((unsigned char*)key, strlen((char*)key));
}

static int __dep_brief_compare(void *privdata, const void *key1, const void *key2) 
{
    int l1,l2;
    DICT_NOTUSED(privdata);

    l1 = strlen((char*)key1);
    l2 = strlen((char*)key2);
    if (l1 != l2) return 0;
    return memcmp(key1, key2, l1) == 0;
}

static dictType g_dict_type_for_dep = {
    __dep_brief_hash_generator,             /* hash function */
    NULL,                                   /* key dup */
    NULL,                                   /* val dup */
    __dep_brief_compare,                    /* key compare */
    NULL,                                   /* key destructor */
    NULL                                    /* val destructor */
};

void *dep_get_handle(const char *name)
{
    void *handle;
    
    handle = NULL;
    if (g_deps_brief) {
        handle = dictFetchValue(g_deps_brief, name);
    }
    return handle;
}

void dep_set_handle(const char *name, void *handle)
{
    /* create a dictionary to storage deps */
    if (!g_deps_brief) {
        g_deps_brief = dictCreate(&g_dict_type_for_dep, NULL);
    }
    if (g_deps_brief) {
        dictAdd(g_deps_brief, name, handle);
    }
}

void *dep_remove_handle(const char *name)
{
    void *handle;
    
    handle = NULL;
    if (g_deps_brief) {
        handle = dictFetchValue(g_deps_brief, name);
        if (handle) {
            dictDelete(g_deps_brief, name);
        }
    }
    return handle;
}

void dep_clear_all(void)
{
    if (g_deps_brief) {
        dictRelease(g_deps_brief);
        g_deps_brief = NULL;
    }
}
