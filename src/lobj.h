#pragma once

/* this is one of the most important component of lrdp
 *  the lobj component manage all virtual objects which declare in json config file
 *  any object in lobj have below properties:
 *     1. name
 *     2. host module
 *     3. object reference function (use to search object by name)
 */

#define LOBJ_MAX_NAME_LEN 64

#include <stddef.h>
#include <ctype.h>

struct lobj;
typedef void (*freeproc_pfn)(struct lobj *lop);
typedef void (*refer_pfn)(struct lobj *lop);

enum lobj_status
{
    LOS_NORMAL = 0,
    LOS_CLOSE_WAIT,
    LOS_CLOSED,
};

struct lobj
{
    char name[LOBJ_MAX_NAME_LEN];   // name of object
    char module[256];               // host share library name of object
    void *handle;                   // host share library handle of object
    size_t size;                    // size of object, exclude lobj_t
    int refcount;                   // reference count of object
    enum lobj_status status;        // status of object
    freeproc_pfn freeproc;          // callback proc before object free
    refer_pfn    referproc;         // callback proc when object refer
    unsigned char body[0];
};
typedef struct lobj lobj_t, *lobj_pt;

#define lobj_body(type, lop) ((type)((lop)->body))

extern lobj_pt lobj_create(const char *name, const char *module, size_t size, freeproc_pfn freeproc, refer_pfn referproc);
extern void lobj_destroy(const char *name);
extern void lobj_ldestroy(lobj_pt lop);
extern void *lobj_dlsym(const lobj_pt lop, const char *sym);
extern lobj_pt lobj_refer(const char *name);
extern void lobj_derefer(lobj_pt lop);
