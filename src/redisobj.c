#include "redisobj.h"

#include "netobj.h"

#include "hiredis.h"
#include "async.h"
#include "ae.h"

struct redisobj
{
    struct endpoint host;
    //redisContext *c;
    redisAsyncContext *c;
};

static void __redisobj_free(lobj_pt lop, void *context, size_t ctxsize)
{
    struct redisobj *rdsobj;

    rdsobj = lobj_body(struct redisobj *, lop);
    if (rdsobj->c) {
        redisAsyncFree(rdsobj->c);
    }
}

static int __redisobj_vwrite(struct lobj *lop, int elements, const void **vdata, size_t *vsize)
{
    struct redisobj *rdsobj;

    if (!lop || elements < 3 || !vdata || !vsize) {
        return -1;
    }

    if ( (sizeof(void *) != vsize[0]) && (NULL != vdata[0] )) {
        return -1;
    }

    rdsobj = lobj_body(struct redisobj *, lop);
    
    return redisAsyncCommandArgv(rdsobj->c, (redisCallbackFn *)vdata[0], (void *)vdata[1], elements - 2, (const char **)&vdata[2], &vsize[2]);
}

extern int redisAeAttach(aeEventLoop *loop, redisAsyncContext *ac);

void redisobj_create(const jconf_redis_server_pt jredis_server_cfg, aeEventLoop *el)
{
    struct lobj_fx fx = {
        .freeproc = &__redisobj_free,
        .referproc = NULL,
        .writeproc = NULL,
        .vwriteproc = &__redisobj_vwrite,
    };
    lobj_pt lop;
    struct redisobj *rdsobj;
    nsp_status_t status;

    lop = lobj_create(jredis_server_cfg->head.name, NULL, sizeof(struct redisobj), jredis_server_cfg->head.ctxsize, &fx);
    if (!lop) {
        return;
    }
    rdsobj = lobj_body(struct redisobj *, lop);

    do {
        status = netobj_parse_endpoint(jredis_server_cfg->host, &rdsobj->host);
        if (!NSP_SUCCESS(status)) {
            break;
        }

        rdsobj->c = redisAsyncConnect(rdsobj->host.ip, rdsobj->host.port);
        if (!rdsobj->c) {
            printf("Connection error: can't allocate redis context\n");
            break;
        }
        redisAeAttach(el, rdsobj->c);

        if (rdsobj->c->err) {
            printf("Connection error: %s\n", rdsobj->c->errstr);
            break;
        }

        return;
    } while(0);

    lobj_ldestroy(lop);
}
