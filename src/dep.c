#include "dep.h"
#include "dict.h"
#include "ifos.h"

#include <stdint.h>
#include <string.h>

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

/* if module are already opened then load it from dictionary
 *  otherwise, open the library and save the module handle to dictionary */
void *dep_open_library(const char *name)
{
    void *handle;

    if (!g_deps_brief) {
        g_deps_brief = dictCreate(&g_dict_type_for_dep, NULL);
        if (!g_deps_brief) {
            return NULL;
        }
    }

    handle = dictFetchValue(g_deps_brief, name);
    if (!handle) {
        handle = ifos_dlopen(name);
        if (handle) {
            dictAdd(g_deps_brief, name, handle);
        }
    }
    return handle;
}

void dep_close_library(const char *name)
{
    void *handle;
    
    if (g_deps_brief) {
        handle = dictFetchValue(g_deps_brief, name);
        if (handle) {
            dictDelete(g_deps_brief, name);
            ifos_dlclose(handle);
        }
    }
}
