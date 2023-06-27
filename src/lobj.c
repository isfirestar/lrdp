#include "lobj.h"

#include "dict.h"
#include "zmalloc.h"

#include "ifos.h"

static dict *g_lobj_container = NULL;

static uint64_t __lobj_brief_hash_generator(const void *key)
{
    return dictGenHashFunction((unsigned char*)key, strlen((char*)key));
}

static int __lobj_brief_compare(void *privdata, const void *key1, const void *key2)
{
    int l1,l2;
    DICT_NOTUSED(privdata);

    l1 = strlen((char*)key1);
    l2 = strlen((char*)key2);
    if (l1 != l2) return 0;
    return memcmp(key1, key2, l1) == 0;
}

static dictType g_dict_type_for_dep = {
    __lobj_brief_hash_generator,                /* hash function */
    NULL,                                       /* key dup */
    NULL,                                       /* val dup */
    __lobj_brief_compare,                        /* key compare */
    NULL,                                       /* key destructor */
    NULL                                        /* val destructor */
};

lobj_pt lobj_create(const char *name, const char *module, size_t size, freeproc_pfn freeproc, refer_pfn referproc)
{
    lobj_pt lop;

    if (!name) {
        return NULL;
    }

    if (!g_lobj_container) {
        g_lobj_container = dictCreate(&g_dict_type_for_dep, NULL);
    }
    if (!g_lobj_container){
        return NULL;
    }

    lop = (lobj_pt)ztrycalloc(sizeof(lobj_t) + size);
    if (!lop) {
        return NULL;
    }

    strncpy(lop->name, name, sizeof(lop->name) - 1);
    strncpy(lop->module, module, sizeof(lop->module) - 1);
    if (module) {
        lop->handle = ifos_dlopen(module);
    }
    lop->size = size;
    lop->refcount = 0;
    lop->status = LOS_NORMAL;
    lop->freeproc = freeproc;
    lop->referproc = referproc;

    if (DICT_OK != dictAdd(g_lobj_container, lop->name, lop)) {
        zfree(lop);
        lop = NULL;
    }

    return lop;
}

static void __lobj_release(lobj_pt lop)
{
    if (lop->freeproc) {
        lop->freeproc(lop);
    }

    dictDelete(g_lobj_container, lop->name);
    zfree(lop);
}

void lobj_destroy(const char *name)
{
    lobj_pt lop;

    if (!name || !g_lobj_container) {
        return;
    }

    lop = (lobj_pt)dictFetchValue(g_lobj_container, name);
    if (lop) {
        if (lop->refcount > 0) {
            lop->status = LOS_CLOSE_WAIT;
        } else {
            __lobj_release(lop);
        }
    }
}

void lobj_ldestroy(lobj_pt lop)
{
    if (!lop) {
        return;
    }

    if (lop->refcount > 0) {
        lop->status = LOS_CLOSE_WAIT;
    } else {
        __lobj_release(lop);
    }
}

void *lobj_dlsym(const lobj_pt lop, const char *sym)
{
    if (!lop || !sym) {
        return NULL;
    }

    return ifos_dlsym(lop->handle, sym);
}

lobj_pt lobj_refer(const char *name)
{
    lobj_pt lop;

    if (!name || !g_lobj_container) {
        return NULL;
    }

    lop = (lobj_pt)dictFetchValue(g_lobj_container, name);
    if (lop) {
        lop->refcount++;
        if (lop->referproc) {
            lop->referproc(lop);
        }
    }
    return lop;
}

void lobj_derefer(lobj_pt lop)
{
    if (!lop) {
        return;
    }

    if (lop->refcount > 0) {
        lop->refcount--;
    }

    if (0 == lop->refcount && LOS_CLOSE_WAIT == lop->status) {
        __lobj_release(lop);
    }
}
