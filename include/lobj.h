#pragma once

/* this is one of the most important component of lrdp
 *  the lobj component manage all virtual objects which declare in json config file
 *  any object in lobj have below properties:
 *     1. name
 *     2. host module
 *     3. object reference function (use to search object by name)
 */

#define LOBJ_MAX_NAME_LEN 64

#include "compiler.h"

typedef int64_t lobj_seq_t;

struct lobj;
typedef struct lobj *lobj_pt;

struct lobj_event
{
    void (*on_read)(lobj_pt lop, const void *data, size_t n);
    void (*on_write)(lobj_pt lop, const void *data, size_t n);
    void (*on_error)(lobj_pt lop, int errcode);
};
typedef struct lobj_event lobj_event_t, *lobj_event_pt;

typedef void (*freeproc_pfn)(lobj_pt lop, void *ctx, size_t ctxsize);
typedef int (*write_pfn)(lobj_pt lop, const void *data, size_t n);
typedef void (*recvdata_pfn)(lobj_pt lop, const void *data, size_t n);
typedef int (*vwrite_pfn)(lobj_pt lop, int elements, const void **vdata, size_t *vsize);
typedef int (*read_pfn)(lobj_pt lop, void *data, size_t n);
typedef int (*vread_pfn)(lobj_pt lop, int elements, void **data, size_t *n);
typedef int (*rawinvoke_pfn)(lobj_pt lop, const void *datain, size_t nin, void *dataout, size_t *nout);

struct lobj_fx
{
    freeproc_pfn freeproc;
    write_pfn writeproc;
    vwrite_pfn vwriteproc;
    read_pfn readproc;
    vread_pfn vreadproc;
    recvdata_pfn recvdataproc;
    rawinvoke_pfn rawinvokeproc;
};

struct lobj_fx_sym
{
    char *freeproc_sym;
    char *writeproc_sym;
    char *vwriteproc_sym;
    char *readproc_sym;
    char *vreadproc_sym;
    char *recvdataproc_sym;
    char *rawinvokeproc_sym;
};

extern nsp_status_t lobj_init();
extern void lobj_uninit();

/* proce for create/destroy/destroy_all */
extern lobj_pt lobj_create(const char *name, const char *module, size_t bodysize, size_t ctxsize, const struct lobj_fx *fx);
extern lobj_pt lobj_dup(const char *name, const lobj_pt olop);
extern void lobj_destroy(const char *name);
extern void lobj_qdestroy(lobj_seq_t seq);
extern void lobj_ldestroy(lobj_pt lop);

/* proce for refer/derefer */
extern lobj_pt lobj_refer(const char *name);
extern lobj_pt lobj_qrefer(lobj_seq_t seq);
extern void lobj_derefer(lobj_pt lop);

/* write data according to the lite object, this IO request will be dispatch to low-level object entity
 * for SOCKET : UDP socket object MUST call @lobj_fx_vwrite instead of @lobj_fx_write and at least 2 elements in @vdata
 *              the second element in @vdata is a null-terminated string which represent the destination endpoint format like: "253.253.253:65534"
 *              TCP socket object use @lobj_fx_vwrite are almost the same with @lobj_fx_write, vdata and vsize only use the first element
 * for REDIS :  layer of vdata are :
 *              1. asyn callback function with prototype: void (*redisCallbackFn)(struct redisAsyncContext *ac, void *r, void *priveData)
 *              2. private data(can be NULL)
 *              [3,n] are the redis command arguments
 */
extern void lobj_fx_load(lobj_pt lop, const struct lobj_fx_sym *sym);
extern void lobj_fx_free(lobj_pt lop);
extern int lobj_fx_write(lobj_pt lop, const void *data, size_t n);
extern int lobj_fx_vwrite(lobj_pt lop, int elements, const void **vdata, size_t *vsize);
extern int lobj_fx_read(lobj_pt lop, void *data, size_t n);
extern int lobj_fx_vread(lobj_pt lop, int elements, void **data, size_t *n);
extern void lobj_fx_on_recvdata(lobj_pt lop, const void *data, size_t n);
extern int lobj_fx_rawinvoke(lobj_pt lop, const void *datain, size_t nin, void *dataout, size_t *nout);

/* object helper function impls */
extern void *lobj_dlsym(const lobj_pt lop, const char *sym);
extern char *lobj_random_name(char *holder, size_t size);
/* query the context size in bytes of specify object, if @ctx not null, the context pointer will storage on it
 *  ise @lob_resize_context to reset the context buffer size, you can specify zero newsize to delete the existing context buffer */
extern size_t lobj_get_context(lobj_pt lop, void **ctx);
extern void *lobj_resize_context(lobj_pt lop, size_t newsize);
/* obtain the body pointer or constant pointer */
extern void *lobj_get_body(lobj_pt lop);
extern const void *lobj_cget_body(const lobj_pt lop);
/* obtain the index sequence or name of object */
extern lobj_seq_t lobj_get_seq(lobj_pt lop);
extern const char *lobj_get_name(lobj_pt lop);

#define lobj_body(type, lop) ((type)lobj_get_body(lop))
#define lobj_cbody(type, lop) ((const type)lobj_cget_body(lop))
