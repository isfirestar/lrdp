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

typedef void (*touch_pfn)(lobj_pt lop, void *userptr, size_t userptrlen);
typedef void (*free_pfn)(lobj_pt lop, void *ctx, size_t ctxsize);
typedef void (*recvdata_pfn)(lobj_pt lop, const void *data, size_t n);
typedef void (*vrecvdata_pfn)(lobj_pt lop, int elements, const void **vdata, const size_t *vsize);
typedef int (*write_pfn)(lobj_pt lop, const void *data, size_t n);
typedef int (*vwrite_pfn)(lobj_pt lop, int elements, const void **vdata, const size_t *vsize);
typedef int (*read_pfn)(lobj_pt lop, void *data, size_t n);
typedef int (*vread_pfn)(lobj_pt lop, int elements, void **data, size_t *n);
typedef int (*rawinvoke_pfn)(lobj_pt lop, const void *datain, size_t nin, void *dataout, size_t *nout);

struct lobj_fx
{
    touch_pfn touchproc;
    free_pfn freeproc;
    write_pfn writeproc;
    vwrite_pfn vwriteproc;
    read_pfn readproc;
    vread_pfn vreadproc;
    recvdata_pfn recvdataproc;
    vrecvdata_pfn vrecvdataproc;
    rawinvoke_pfn rawinvokeproc;
};

struct lobj_fx_sym
{
    char *touchproc_sym;
    char *freeproc_sym;
    char *writeproc_sym;
    char *vwriteproc_sym;
    char *readproc_sym;
    char *vreadproc_sym;
    char *recvdataproc_sym;
    char *vrecvdataproc_sym;
    char *rawinvokeproc_sym;
};

extern nsp_status_t lobj_init();
extern void lobj_uninit();

/* proce for create/destroy/destroy_all */
extern lobj_pt lobj_create(const char *name, const char *module, size_t bodysize, size_t ctxsize, const struct lobj_fx *fx);
extern lobj_pt lobj_dup(const char *name, const lobj_pt olop);
extern void lobj_fx_cover(lobj_pt lop, const struct lobj_fx_sym *sym);
extern void lobj_destroy(const char *name);
extern void lobj_qdestroy(lobj_seq_t seq);
extern void lobj_ldestroy(lobj_pt lop);

/* proce for refer/derefer */
extern void lobj_raise(lobj_pt lop);
extern void lobj_shrink(lobj_pt lop);
extern lobj_pt lobj_refer(const char *name);
extern lobj_pt lobj_qrefer(lobj_seq_t seq);
extern void lobj_derefer(lobj_pt lop);

/* virtual functions declare in lobj_fx */
extern void lobj_fx_touch(lobj_pt lop, void *userptr, size_t userptrlen);
extern void lobj_fx_free(lobj_pt lop);
extern int lobj_fx_write(lobj_pt lop, const void *data, size_t n);
extern int lobj_fx_vwrite(lobj_pt lop, int elements, const void **vdata, const size_t *vsize);
extern int lobj_fx_read(lobj_pt lop, void *data, size_t n);
extern int lobj_fx_vread(lobj_pt lop, int elements, void **data, size_t *n);
extern int lobj_fx_on_recvdata(lobj_pt lop, const void *data, size_t n);
extern int lobj_fx_on_vrecvdata(lobj_pt lop, int elements, const void **vdata, const size_t *vsize);
extern int lobj_fx_rawinvoke(lobj_pt lop, const void *datain, size_t nin, void *dataout, size_t *nout);

/* object helper function impls
 *  follow by the dlsym implementations, we have ability to call other functions which shared by any other modules.
 *  1st, we declare a function potinter to by the target function and then load address from lop object.
 *  2nd, directly invoke function
 * demo:
 *  void myfunc() {
 *     lobj_pt lop = lobj_refer("myobj");
 *     if (lop) {   // check if object exist
 *         lobj_declare_pcall(lop, myfunc, void, int, char *);   // declare a function pointer
 *         lobj_pcall(myfunc, 1, "hello");   // invoke function
 *         lobj_derefer(lop);
 *     }
 *  } */
extern void *lobj_dlsym(const lobj_pt lop, const char *sym);
#define lobj_text2str(x) #x
#define lobj_declare_pcall(lop, name, returntype, ...)   \
    returntype (*lobj_pcall_##name)(__VA_ARGS__) = (returntype (*)(__VA_ARGS__))lobj_dlsym(lop, lobj_text2str(name))

#define lobj_pcall(name, ...)   ((NULL != lobj_pcall_##name) ? lobj_pcall_##name(__VA_ARGS__) : 0)

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
