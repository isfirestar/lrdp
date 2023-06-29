#include "lobj.h"

#include "dict.h"
#include "zmalloc.h"
#include "monotonic.h"
#include "ifos.h"
#include "rand.h"
#include "spinlock.h"

enum lobj_status
{
    LOS_NORMAL = 0,
    LOS_CLOSE_WAIT,
    LOS_CLOSED,
};

struct lobj
{
    lobj_seq_t seq;                 // seq id of object
    char name[LOBJ_MAX_NAME_LEN];   // name of object
    char module[256];               // host share library name of object
    void *handle;                   // host share library handle of object
    int refcount;                   // reference count of object
    enum lobj_status status;        // status of object
    struct lobj_fx fx;              // function table of object
    size_t ctxsize;                 // size of object context
    unsigned char *ctx;             // context of object
    size_t size;                    // size of object body, exclude lobj_t
    unsigned char body[0];
};
typedef struct lobj lobj_t;

static dict *g_dict_hash_byname = NULL;
static dict *g_dict_hash_byseq = NULL;
static struct spin_lock g_lobj_container_locker = SPIN_LOCK_INITIALIZER;

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

static lobj_pt __lobj_add_dict(lobj_pt lop)
{
    int dictResult;

    if (!lop) {
        return NULL;
    }

    acquire_spinlock(&g_lobj_container_locker);
    dictResult = dictAdd(g_dict_hash_byname, lop->name, lop);
    if (DICT_OK == dictResult) {
        dictResult = dictAdd(g_dict_hash_byseq, &lop->seq, lop);
    }
    if (DICT_OK != dictResult) {
        dictDelete(g_dict_hash_byname, lop->name);
        dictDelete(g_dict_hash_byseq, &lop->seq);
    }
    release_spinlock(&g_lobj_container_locker);

    if (DICT_OK != dictResult) {
        if (lop->ctx) {
            zfree(lop->ctx);
        }
        zfree(lop);
        return NULL;
    }
    return lop;
}

lobj_pt lobj_create(const char *name, const char *module, size_t size, size_t ctxsize, const struct lobj_fx *fx)
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
    if (module) {
        strncpy(lop->module, module, sizeof(lop->module) - 1);
        lop->handle = ifos_dlopen(module);
        if (!lop->handle) {
            printf("%s\n", ifos_dlerror());
        }
    }
    lop->size = size;
    lop->refcount = 0;
    lop->status = LOS_NORMAL;
    (NULL != fx) ? memcpy(&lop->fx, fx, sizeof(lop->fx)) : memset(&lop->fx, 0, sizeof(lop->fx));
    lop->ctx = NULL;
    lop->ctxsize = (ctxsize + sizeof(long) - 1) & ~(sizeof(long) - 1); /* ctxsize MUST align to the upper sizeof(long) */
    if (lop->ctxsize > 0) {
        lop->ctx = (unsigned char *)ztrycalloc(lop->ctxsize);
    }

    return __lobj_add_dict(lop);
}

lobj_pt lobj_dup(const char *name, const lobj_pt olop)
{
    lobj_pt lop;

    if (!name || !olop || __atomic_load_n(&g_seq, __ATOMIC_ACQUIRE) < 0) {
        return NULL;
    }

    lop = (lobj_pt)ztrycalloc(sizeof(lobj_t) + olop->size);
    if (!lop) {
        return NULL;
    }

    lop->seq = __atomic_add_fetch(&g_seq, 1, __ATOMIC_SEQ_CST);
    strncpy(lop->name, name, sizeof(lop->name) - 1);
    strncpy(lop->module, olop->module, sizeof(lop->module) - 1);
    lop->handle = olop->handle;
    lop->size = olop->size;
    lop->refcount = 0;
    lop->status = LOS_NORMAL;
    memcpy(&lop->fx, &olop->fx, sizeof(lop->fx));
    lop->ctx = NULL;
    lop->ctxsize = olop->ctxsize;
    if (lop->ctxsize > 0) {
        lop->ctx = (unsigned char *)ztrycalloc(lop->ctxsize);
    }

    return __lobj_add_dict(lop);
}

static void __lobj_release(lobj_pt lop)
{
    if (lop->fx.freeproc) {
        lop->fx.freeproc(lop);
    }

    if (lop->ctx) {
        zfree(lop->ctx);
    }

    acquire_spinlock(&g_lobj_container_locker);
    dictDelete(g_dict_hash_byname, lop->name);
    dictDelete(g_dict_hash_byseq, &lop->seq);
    release_spinlock(&g_lobj_container_locker);

    zfree(lop);
}

void lobj_destroy(const char *name)
{
    lobj_pt lop;

    if (!name || !g_dict_hash_byname) {
        return;
    }

    acquire_spinlock(&g_lobj_container_locker);
    lop = (lobj_pt)dictFetchValue(g_dict_hash_byname, name);
    lobj_ldestroy(lop);
    release_spinlock(&g_lobj_container_locker);
}

void lobj_destroy_byseq(int64_t seq)
{
    lobj_pt lop;

    if (seq < 0 || !g_dict_hash_byseq) {
        return;
    }

    acquire_spinlock(&g_lobj_container_locker);
    lop = (lobj_pt)dictFetchValue(g_dict_hash_byseq, &seq);
    lobj_ldestroy(lop);
    release_spinlock(&g_lobj_container_locker);
}

void lobj_ldestroy(lobj_pt lop)
{
    if (!lop) {
        return;
    }

    if (__atomic_load_n(&lop->refcount, __ATOMIC_ACQUIRE) > 0) {
        __atomic_store_n(&lop->status, LOS_CLOSE_WAIT, __ATOMIC_RELEASE);
    } else {
        __lobj_release(lop);
    }
}

void *lobj_dlsym(const lobj_pt lop, const char *sym)
{
    if (!lop || !sym) {
        return NULL;
    }

    if (!lop->handle) {
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

    acquire_spinlock(&g_lobj_container_locker);
    lop = (lobj_pt)dictFetchValue(g_dict_hash_byname, name);
    release_spinlock(&g_lobj_container_locker);

    if (lop) {
        __atomic_add_fetch(&lop->refcount, 1, __ATOMIC_SEQ_CST);
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

    acquire_spinlock(&g_lobj_container_locker);
    lop = (lobj_pt)dictFetchValue(g_dict_hash_byseq, &seq);
    release_spinlock(&g_lobj_container_locker);

    if (lop) {
        __atomic_add_fetch(&lop->refcount, 1, __ATOMIC_SEQ_CST);
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

    if (__atomic_load_n(&lop->refcount, __ATOMIC_ACQUIRE) <= 0) {
        return;
    }

    if (0 == __atomic_sub_fetch(&lop->refcount, 1, __ATOMIC_SEQ_CST) && 
        LOS_CLOSE_WAIT == __atomic_load_n(&lop->status, __ATOMIC_ACQUIRE)) 
    {
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

/* helper function impls */
char *lobj_random_name(char *holder, size_t size)
{
    monotime now;
    
    now = getMonotonicUs();
    snprintf(holder, size, "%u|%u|%u", (unsigned int)(now >> 32), (unsigned int)(now & 0xffffffff), redisLrand48());
    return holder;
}

size_t lobj_get_context(lobj_pt lop, void **ctx)
{
    if (!lop || !ctx) {
        return 0;
    }

    *ctx = lop->ctx;
    return lop->ctxsize;
}

void *lobj_get_body(lobj_pt lop)
{
    if (!lop) {
        return NULL;
    }

    return lop->body;
}

const void *lobj_cget_body(const lobj_pt lop)
{
    if (!lop) {
        return NULL;
    }

    return (const void *)lop->body;
}

lobj_seq_t lobj_get_seq(lobj_pt lop)
{
    if (!lop) {
        return -1;
    }

    return lop->seq;
}

const char *lobj_get_name(lobj_pt lop)
{
    if (!lop) {
        return " ";
    }

    return &lop->name[0];
}
