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

struct lobj;
typedef void (*freeproc_pfn)(struct lobj *lop);
typedef void (*refer_pfn)(struct lobj *lop);
typedef int (*write_pfn)(struct lobj *lop, const void *data, size_t n);

struct lobj_fx
{
    freeproc_pfn freeproc;
    refer_pfn referproc;
    write_pfn writeproc;
};

enum lobj_status
{
    LOS_NORMAL = 0,
    LOS_CLOSE_WAIT,
    LOS_CLOSED,
};

struct lobj
{
    int64_t seq;               // seq id of object
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
typedef struct lobj lobj_t, *lobj_pt;

#define lobj_body(type, lop) ((type)((lop)->body))
#define lobj_head(body) ((lobj_pt)(((unsigned char *)(body)) - offsetof(lobj_t, body)))

extern nsp_status_t lobj_init();
extern lobj_pt lobj_create(const char *name, const char *module, size_t size, size_t ctxsize, const struct lobj_fx *fx);
extern void lobj_destroy(const char *name);
extern void lobj_destroy_byseq(int64_t seq);
extern void lobj_ldestroy(lobj_pt lop);
extern void *lobj_dlsym(const lobj_pt lop, const char *sym);
extern lobj_pt lobj_refer(const char *name);
extern lobj_pt lobj_refer_byseq(int64_t seq);
extern void lobj_derefer(lobj_pt lop);
extern int lobj_write(lobj_pt lop, const void *data, size_t n);

/* object helper function impls */
extern char *lobj_random_name(char *holder, size_t size);
