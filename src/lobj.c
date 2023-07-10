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

static int64_t g_seq = -1000000;

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

void __lobj_detach_from_dict(lobj_pt lop)
{
    dictDelete(g_dict_hash_byname, (lop)->name);
    dictDelete(g_dict_hash_byseq, &((lop)->seq));
}

static void __lobj_finalize(lobj_pt lop);

// traverse all lobjs and destroy all lobjs which status is LOS_CLOSE_WAIT
// set any lobj's status to LOS_CLOSE_WAIT means that the lobj is waiting for close and couldn't be referenced
void lobj_uninit()
{
    dictIterator *di;
    dictEntry *de;
    lobj_pt lop, *removed;
    size_t real_removed, i;

    __atomic_store_n(&g_seq, -1000000, __ATOMIC_RELEASE);

    if (!g_dict_hash_byname) {
        return;
    }

    removed = (lobj_pt *)ztrycalloc(sizeof(lobj_pt) * dictSize(g_dict_hash_byname));
    if (!removed) {
        return;
    }
    real_removed = 0;

    acquire_spinlock(&g_lobj_container_locker);
    di = dictGetIterator(g_dict_hash_byname);
    while ((de = dictNext(di)) != NULL) {
        lop = (lobj_pt)dictGetVal(de);
         if (lop->refcount > 0) {
            lop->status = LOS_CLOSE_WAIT;
        } else {
            __lobj_detach_from_dict(lop);
            removed[real_removed++] = lop;
        }
    }
    dictReleaseIterator(di);
    release_spinlock(&g_lobj_container_locker);

    // finalize the removed lobjs
    for (i = 0; i < real_removed; i++) {
        __lobj_finalize(removed[i]);
    }
}

static lobj_pt __lobj_attach_to_dict(lobj_pt lop)
{
    int dictResult;

    if (!lop) {
        return NULL;
    }

    dictResult = DICT_ERR;

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

void __lobj_free_module(lobj_pt lop, void *ctx, size_t ctxsize)
{
    if (lop->handle) {
        ifos_dlclose(lop->handle);
    }
}

static void *__lobj_create_module(const char *module)
{
    lobj_pt lop;
    struct lobj_fx fx = {
        .freeproc = __lobj_free_module,
    };
    void *handle;
    void (*dl_main)();

    lop = lobj_refer(module);
    if (lop) {
        handle = lop->handle;
        lobj_derefer(lop);
        return handle;
    }

    lop = lobj_create(module, NULL, 0, 0, &fx);
    if (!lop) {
        return NULL;
    }

    lop->handle = ifos_dlopen(module);
    if (!lop->handle) {
        printf("%s\n", ifos_dlerror());
    } else {
        dl_main = ifos_dlsym(lop->handle, "dl_main");
        if (dl_main) {
            dl_main();
        }
    }
    return lop->handle;
}

lobj_pt lobj_create(const char *name, const char *module, size_t size, size_t ctxsize, const struct lobj_fx *fx)
{
    lobj_pt lop;
    const char *oname;
    char holder[64], loader[128];
    int64_t seq;
    void (*on_loaded)(lobj_pt lop, void *ctx, size_t ctxsize);

    // use g_seq to control module init order
    seq =  __atomic_add_fetch(&g_seq, 1, __ATOMIC_SEQ_CST);
    if (seq <= 0) {
        __atomic_sub_fetch(&g_seq, 1, __ATOMIC_SEQ_CST);
        return NULL;
    }

    lop = (lobj_pt)ztrycalloc(sizeof(lobj_t) + size);
    if (!lop) {
        return NULL;
    }

    oname = (NULL == name) ? lobj_random_name(holder, sizeof(holder)) : name;
    lop->seq = seq;
    strncpy(lop->name, oname, sizeof(lop->name) - 1);
    if (module) {
        lop->handle = __lobj_create_module(module);
    }
    lop->size = size;
    lop->refcount = 0;
    lop->status = LOS_NORMAL;
    (NULL != fx) ? memcpy(&lop->fx, fx, sizeof(lop->fx)) : memset(&lop->fx, 0, sizeof(lop->fx));
    lop->ctx = NULL;
    lop->ctxsize = (ctxsize + sizeof(long) - 1) & ~(sizeof(long) - 1); /* ctxsize MUST align to the upper sizeof(long) */
    if (lop->ctxsize > 0) {
        lop->ctx = (unsigned char *)ztrycalloc(lop->ctxsize);
        if (!lop->ctx) {
            lop->ctxsize = 0; /* failed on context allocation didn't affect process continue */
        }
    }

    if (lop->handle && name) {
        snprintf(loader, sizeof(loader), "%s_loaded", name);
        on_loaded = ifos_dlsym(lop->handle, loader);
        if (on_loaded) {
            on_loaded(lop, lop->ctx, lop->ctxsize);
        }
    }

    return __lobj_attach_to_dict(lop);
}

lobj_pt lobj_dup(const char *name, const lobj_pt olop)
{
    lobj_pt lop;
    int64_t seq;
    char holder[64], loader[128];
    void (*on_loaded)(lobj_pt lop, void *ctx, size_t ctxsize);

    if (!olop) {
        return NULL;
    }

    // use g_seq to control module init order
    seq =  __atomic_add_fetch(&g_seq, 1, __ATOMIC_SEQ_CST);
    if (seq <= 0) {
        __atomic_sub_fetch(&g_seq, 1, __ATOMIC_SEQ_CST);
        return NULL;
    }

    lop = (lobj_pt)ztrycalloc(sizeof(lobj_t) + olop->size);
    if (!lop) {
        return NULL;
    }

    lop->seq = seq;
    strncpy(lop->name, (NULL == name ? lobj_random_name(holder, sizeof(holder)) : name), sizeof(lop->name) - 1);
    lop->handle = olop->handle;
    lop->size = olop->size;
    lop->refcount = 0;
    lop->status = LOS_NORMAL;
    memcpy(&lop->fx, &olop->fx, sizeof(lop->fx));
    lop->ctx = NULL;
    lop->ctxsize = olop->ctxsize;
    if (lop->ctxsize > 0) {
        lop->ctx = (unsigned char *)ztrycalloc(lop->ctxsize);
        if (!lop->ctx) {
            lop->ctxsize = 0; // ignore context allocate failure
        }
    }

    if (lop->handle && name) {
        snprintf(loader, sizeof(loader), "%s_loaded", name);
        on_loaded = ifos_dlsym(lop->handle, loader);
        if (on_loaded) {
            on_loaded(lop, lop->ctx, lop->ctxsize);
        }
    }

    return __lobj_attach_to_dict(lop);
}

void lobj_cover_fx(lobj_pt lop, const char *freeproc, const char *writeproc, const char *vwriteproc, const char *readproc, const char *vreadproc)
{
    if (!lop) {
        return;
    }

    if (!lop->handle) {
        return;
    }

    if (freeproc) {
        if (0 != freeproc[0]) {
            lop->fx.freeproc = (freeproc_pfn)ifos_dlsym(lop->handle, freeproc);
        }
    }
    if (writeproc) {
        if (0 != writeproc[0]) {
            lop->fx.writeproc = (write_pfn)ifos_dlsym(lop->handle, writeproc);
        }
    }
    if (vwriteproc) {
        if (0 != vwriteproc[0]) {
            lop->fx.vwriteproc = (vwrite_pfn)ifos_dlsym(lop->handle, vwriteproc);
        }
    }
    if (readproc) {
        if (0 != readproc[0]) {
            lop->fx.readproc = (read_pfn)ifos_dlsym(lop->handle, readproc);
        }
    }
    if (vreadproc) {
        if (0 != vreadproc[0]) {
            lop->fx.vreadproc = (vread_pfn)ifos_dlsym(lop->handle, vreadproc);
        }
    }
}

static void __lobj_finalize(lobj_pt lop)
{
    if (!lop) {
        return;
    }

    if (lop->fx.freeproc) {
        lop->fx.freeproc(lop, lop->ctx, lop->ctxsize);
    }

    if (lop->ctx) {
        zfree(lop->ctx);
    }
    zfree(lop);

    // the dict is on the way of destroying, so we need to check if it is empty
    if (__atomic_load_n(&g_seq, __ATOMIC_ACQUIRE) < 0) {
        acquire_spinlock(&g_lobj_container_locker);
        if (0 == dictSize(g_dict_hash_byname)) {
            dictRelease(g_dict_hash_byname);
            g_dict_hash_byname = NULL;
        }
        release_spinlock(&g_lobj_container_locker);
    }
}

void lobj_destroy(const char *name)
{
    lobj_pt lop;

    if (!name || !g_dict_hash_byname) {
        return;
    }

    // recycle the object if no reference, otherwise, mark it as close wait
    acquire_spinlock(&g_lobj_container_locker);
    lop = (lobj_pt)dictFetchValue(g_dict_hash_byname, name);
    if (lop) {
        lop->status = LOS_CLOSE_WAIT;
        if (lop->refcount > 0) {
            lop = NULL;
        } else {
            __lobj_detach_from_dict(lop);
        }
    }
    release_spinlock(&g_lobj_container_locker);

     __lobj_finalize(lop);
}

void lobj_qdestroy(int64_t seq)
{
    lobj_pt lop;

    if (seq < 0 || !g_dict_hash_byseq) {
        return;
    }

    acquire_spinlock(&g_lobj_container_locker);
    lop = (lobj_pt)dictFetchValue(g_dict_hash_byseq, &seq);
    if (lop) {
        lop->status = LOS_CLOSE_WAIT;
        if (lop->refcount > 0) {
            lop = NULL;
        } else {
            __lobj_detach_from_dict(lop);
        }
    }
    release_spinlock(&g_lobj_container_locker);

    __lobj_finalize(lop);
}

void lobj_ldestroy(lobj_pt lop)
{
    if (!lop) {
        return;
    }

    acquire_spinlock(&g_lobj_container_locker);
    lop->status = LOS_CLOSE_WAIT;
    if (lop->refcount > 0) {
        lop = NULL;
    } else {
        __lobj_detach_from_dict(lop);
    }
    release_spinlock(&g_lobj_container_locker);

    __lobj_finalize(lop);
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

    if (!name || !g_dict_hash_byname || __atomic_load_n(&g_seq, __ATOMIC_ACQUIRE) < 0 ) {
        return NULL;
    }

    acquire_spinlock(&g_lobj_container_locker);
    lop = (lobj_pt)dictFetchValue(g_dict_hash_byname, name);
    if (lop) {
        if (LOS_CLOSE_WAIT == lop->status) {
            lop = NULL;
         } else {
            lop->refcount++;
         }
    }
    release_spinlock(&g_lobj_container_locker);
    return lop;
}

lobj_pt lobj_qrefer(int64_t seq)
{
    lobj_pt lop;

    if (seq < 0 || !g_dict_hash_byseq || __atomic_load_n(&g_seq, __ATOMIC_ACQUIRE) < 0) {
        return NULL;
    }

    acquire_spinlock(&g_lobj_container_locker);
    lop = (lobj_pt)dictFetchValue(g_dict_hash_byseq, &seq);
    if (lop) {
        if (LOS_CLOSE_WAIT == lop->status) {
            lop = NULL;
         } else {
            lop->refcount++;
         }
    }
    release_spinlock(&g_lobj_container_locker);
    return lop;
}

void lobj_derefer(lobj_pt lop)
{
    lobj_pt ref;

    if (!lop) {
        return;
    }

    ref = NULL;
    acquire_spinlock(&g_lobj_container_locker);
    do {
        if (lop->refcount <= 0) {
            break;
        }

        lop->refcount--;
        if (LOS_CLOSE_WAIT != lop->status) {
            break;
        }

        if (lop->refcount > 0) {
            break;
        }

        dictDelete(g_dict_hash_byname, lop->name);
        dictDelete(g_dict_hash_byseq, &lop->seq);
        ref = lop;
    } while (0);
    release_spinlock(&g_lobj_container_locker);

    __lobj_finalize(ref);
}

void lobj_fx_free(lobj_pt lop)
{
    if (!lop) {
        return;
    }

    if (lop->fx.freeproc) {
        lop->fx.freeproc(lop, lop->ctx, lop->ctxsize);
    }
}

int lobj_fx_write(lobj_pt lop, const void *data, size_t n)
{
    if (!lop || !data || 0 == n) {
        return posix__makeerror(EINVAL);
    }

    if (!lop->fx.writeproc) {
        return posix__makeerror(ENOENT);
    }

    return lop->fx.writeproc(lop, data, n);
}

int lobj_fx_vwrite(lobj_pt lop, int elements, const void **vdata, size_t *vsize)
{
    if (!lop || !vdata || !vsize) {
        return posix__makeerror(EINVAL);
    }

    if (!lop->fx.vwriteproc) {
        return posix__makeerror(ENOENT);
    }

    return lop->fx.vwriteproc(lop, elements, vdata, vsize);
}

int lobj_fx_read(lobj_pt lop, void *data, size_t n)
{
    if (!lop || !data || 0 == n) {
        return posix__makeerror(EINVAL);
    }

    if (!lop->fx.readproc) {
        return posix__makeerror(ENOENT);
    }

    return lop->fx.readproc(lop, data, n);
}

int lobj_fx_vread(lobj_pt lop, int elements, void **data, size_t *n)
{
    if (!lop || !data || !n || 0 == elements) {
        return posix__makeerror(EINVAL);
    }

    if (!lop->fx.vreadproc) {
        return posix__makeerror(ENOENT);
    }

    return lop->fx.vreadproc(lop, elements, data, n);
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
    if (!lop) {
        return 0;
    }

    if (ctx) {
        *ctx = lop->ctx;
    }

    return lop->ctxsize;
}

void *lobj_resize_context(lobj_pt lop, size_t newsize)
{
    void *newctx;

    if (!lop) {
        return NULL;
    }

    if (0 == newsize) {
        if (lop->ctx) {
            zfree(lop->ctx);
            lop->ctx = NULL;
        }
        lop->ctxsize = 0;
        return NULL;
    }

    if (!lop->ctx) {
        newctx = zmalloc(newsize);
    } else {
        newctx = ztryrealloc(lop->ctx, newsize);
    }

    if (!newctx) {
        return NULL;
    }

    lop->ctx = newctx;
    lop->ctxsize = newsize;
    return newctx;
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
