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

struct lobj_fx
{
    freeproc_pfn freeproc;
    refer_pfn referproc;
    write_pfn writeproc;
};

extern nsp_status_t lobj_init();
extern lobj_pt lobj_create(const char *name, const char *module, size_t size, size_t ctxsize, const struct lobj_fx *fx);
extern lobj_pt lobj_dup(const char *name, const lobj_pt olop);
extern void lobj_destroy(const char *name);
extern void lobj_destroy_byseq(lobj_seq_t seq);
extern void lobj_ldestroy(lobj_pt lop);
extern lobj_pt lobj_refer(const char *name);
extern lobj_pt lobj_refer_byseq(lobj_seq_t seq);
extern void lobj_derefer(lobj_pt lop);

/* write data according to the lite object, this IO request will be dispatch to low-level object entity */
extern int lobj_write(lobj_pt lop, const void *data, size_t n);

/* object helper function impls */
extern void *lobj_dlsym(const lobj_pt lop, const char *sym);
extern char *lobj_random_name(char *holder, size_t size);
extern size_t lobj_get_context(lobj_pt lop, void **ctx);
extern void *lobj_get_body(lobj_pt lop); // return lop->body
extern const void *lobj_cget_body(const lobj_pt lop); // return lop->body
extern lobj_seq_t lobj_get_seq(lobj_pt lop);
extern const char *lobj_get_name(lobj_pt lop);

#define lobj_body(type, lop) ((type)lobj_get_body(lop))
