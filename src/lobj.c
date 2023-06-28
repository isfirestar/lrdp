#include "lobj.h"

#include "dict.h"
#include "zmalloc.h"

#include "ifos.h"

static dict *g_dict_hash_byname = NULL;
static dict *g_dict_hash_byseq = NULL;

static uint64_t __lobj_hash_name(const void *key)
{
    return dictGenHashFunction((unsigned char*)key, strlen((char*)key));
}

static int __lobj_compare_name(void *privdata, const void *key1, const void *key2)
{
    int l1,l2;
    DICT_NOTUSED(privdata);

    l1 = strlen((char*)key1);
    l2 = strlen((char*)key2);
    if (l1 != l2) return 0;
    return memcmp(key1, key2, l1) == 0;
}

static dictType dictSortByName = {
    __lobj_hash_name,                           /* hash function */
    NULL,                                       /* key dup */
    NULL,                                       /* val dup */
    __lobj_compare_name,                        /* key compare */
    NULL,                                       /* key destructor */
    NULL                                        /* val destructor */
};

static uint64_t __lobj_hash_seq(const void *key)
{
    return *(uint64_t *)key;
}

static int __lobj_compare_seq(void *privdata, const void *key1, const void *key2)
{
    int64_t l1,l2;
    DICT_NOTUSED(privdata);

    l1 = *(int64_t *)key1;
    l2 = *(int64_t *)key2;
    return l1 == l2;
}

static dictType dictSortBySeq = {
    __lobj_hash_seq,                            /* hash function */
    NULL,                                       /* key dup */
    NULL,                                       /* val dup */
    __lobj_compare_seq,                         /* key compare */
    NULL,                                       /* key destructor */
    NULL                                        /* val destructor */
};

static int64_t g_seq = -1;

nsp_status_t lobj_init()
{
    g_dict_hash_byname = dictCreate(&dictSortByName, NULL);
    if (!g_dict_hash_byname) {
        return NSP_STATUS_FATAL;
    }

    g_dict_hash_byseq = dictCreate(&dictSortBySeq, NULL);
    if (!g_dict_hash_byseq) {
        dictRelease(g_dict_hash_byname);
        return NSP_STATUS_FATAL;
    }

    __atomic_store_n(&g_seq, 0, __ATOMIC_RELEASE);
    return NSP_STATUS_SUCCESSFUL;
}

lobj_pt lobj_create(const char *name, const char *module, size_t size, const struct lobj_fx *fx)
{
    lobj_pt lop;

    if (!name || __atomic_load_n(&g_seq, __ATOMIC_ACQUIRE) < 0) {
        return NULL;
    }

    lop = (lobj_pt)ztrycalloc(sizeof(lobj_t) + size);
    if (!lop) {
        return NULL;
    }

    lop->seq = __atomic_add_fetch(&g_seq, 1, __ATOMIC_SEQ_CST);
    strncpy(lop->name, name, sizeof(lop->name) - 1);
    strncpy(lop->module, module, sizeof(lop->module) - 1);
    if (module) {
        lop->handle = ifos_dlopen(module);
    }
    lop->size = size;
    lop->refcount = 0;
    lop->status = LOS_NORMAL;
    (NULL != fx) ? memcpy(&lop->fx, fx, sizeof(lop->fx)) : memset(&lop->fx, 0, sizeof(lop->fx));

    if (DICT_OK != dictAdd(g_dict_hash_byname, lop->name, lop)) {
        zfree(lop);
        lop = NULL;
    }

    if (DICT_OK != dictAdd(g_dict_hash_byseq, &lop->seq, lop)) {
        dictDelete(g_dict_hash_byname, lop->name);
        zfree(lop);
        lop = NULL;
    }

    return lop;
}

static void __lobj_release(lobj_pt lop)
{
    if (lop->fx.freeproc) {
        lop->fx.freeproc(lop);
    }

    dictDelete(g_dict_hash_byname, lop->name);
    dictDelete(g_dict_hash_byseq, &lop->seq);
    zfree(lop);
}

void lobj_destroy(const char *name)
{
    lobj_pt lop;

    if (!name || !g_dict_hash_byname) {
        return;
    }

    lop = (lobj_pt)dictFetchValue(g_dict_hash_byname, name);
    lobj_ldestroy(lop);
}

void lobj_destroy_byseq(int64_t seq)
{
    lobj_pt lop;

    if (seq < 0 || !g_dict_hash_byseq) {
        return;
    }

    lop = (lobj_pt)dictFetchValue(g_dict_hash_byseq, &seq);
    lobj_ldestroy(lop);
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

    if (!name || !g_dict_hash_byname) {
        return NULL;
    }

    lop = (lobj_pt)dictFetchValue(g_dict_hash_byname, name);
    if (lop) {
        lop->refcount++;
        if (lop->fx.referproc) {
            lop->fx.referproc(lop);
        }
    }
    return lop;
}

lobj_pt lobj_refer_byseq(int64_t seq)
{
    lobj_pt lop;

    if (seq < 0 || !g_dict_hash_byseq) {
        return NULL;
    }

    lop = (lobj_pt)dictFetchValue(g_dict_hash_byseq, &seq);
    if (lop) {
        lop->refcount++;
        if (lop->fx.referproc) {
            lop->fx.referproc(lop);
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

int lobj_write(lobj_pt lop, const void *data, size_t n)
{
    if (!lop || !data || 0 == n) {
        return posix__makeerror(EINVAL);
    }

    if (!lop->fx.writeproc) {
        return posix__makeerror(EINVAL);
    }

    return lop->fx.writeproc(lop, data, n);
}
