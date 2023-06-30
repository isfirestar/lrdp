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

typedef void (*freeproc_pfn)(struct lobj *lop);
typedef void (*refer_pfn)(struct lobj *lop);
typedef int (*write_pfn)(struct lobj *lop, const void *data, size_t n);
typedef int (*vwrite_pfn)(struct lobj *lop, int elements, const void **vdata, size_t *vsize);

struct lobj_fx
{
    freeproc_pfn freeproc;
    refer_pfn referproc;
    write_pfn writeproc;
    vwrite_pfn vwriteproc;
};

extern nsp_status_t lobj_init();
extern lobj_pt lobj_create(const char *name, const char *module, size_t bodysize, size_t ctxsize, const struct lobj_fx *fx);
extern lobj_pt lobj_dup(const char *name, const lobj_pt olop);
extern void lobj_destroy(const char *name);
extern void lobj_destroy_byseq(lobj_seq_t seq);
extern void lobj_ldestroy(lobj_pt lop);
extern lobj_pt lobj_refer(const char *name);
extern lobj_pt lobj_refer_byseq(lobj_seq_t seq);
extern void lobj_derefer(lobj_pt lop);

/* write data according to the lite object, this IO request will be dispatch to low-level object entity 
 * for SOCKET : UDP socket object MUST call @lobj_vwrite instead of @lobj_write and at least 2 elements in @vdata
 *              the second element in @vdata is a null-terminated string which represent the destination endpoint format like: "253.253.253:65534"
 *              TCP socket object use @lobj_vwrite are almost the same with @lobj_write, vdata and vsize only use the first element
 * for REDIS :  layer of vdata are :
 *              1. asyn callback function with prototype: void (*redisCallbackFn)(struct redisAsyncContext *ac, void *r, void *priveData)
 *              2. private data(can be NULL)
 *              [3,n] are the redis command arguments
 */
extern int lobj_write(lobj_pt lop, const void *data, size_t n);
extern int lobj_vwrite(lobj_pt lop, int elements, const void **vdata, size_t *vsize);

/* object helper function impls */
extern void *lobj_dlsym(const lobj_pt lop, const char *sym);
extern char *lobj_random_name(char *holder, size_t size);
extern size_t lobj_get_context(lobj_pt lop, void **ctx);
extern void *lobj_get_body(lobj_pt lop); // return lop->body
extern const void *lobj_cget_body(const lobj_pt lop); // return lop->body
extern lobj_seq_t lobj_get_seq(lobj_pt lop);
extern const char *lobj_get_name(lobj_pt lop);

#define lobj_body(type, lop) ((type)lobj_get_body(lop))
