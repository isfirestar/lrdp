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
    aeEventLoop *el;
};

static void __redisobj_free(lobj_pt lop, void *context, size_t ctxsize)
{
    struct redisobj *redis_server_obj;

    redis_server_obj = lobj_body(struct redisobj *, lop);
    if (redis_server_obj->c) {
        redisAsyncFree(redis_server_obj->c);
    }
}

static int __redisobj_vwrite(struct lobj *lop, int elements, const void **vdata, size_t *vsize)
{
    struct redisobj *redis_server_obj;

    if (!lop || elements < 3 || !vdata || !vsize) {
        return -1;
    }

    if ( (sizeof(void *) != vsize[0]) && (NULL != vdata[0] )) {
        return -1;
    }

    redis_server_obj = lobj_body(struct redisobj *, lop);
    
    return redisAsyncCommandArgv(redis_server_obj->c, (redisCallbackFn *)vdata[0], (void *)vdata[1], elements - 2, (const char **)&vdata[2], &vsize[2]);
}

extern int redisAeAttach(aeEventLoop *loop, redisAsyncContext *ac);

static void __redisobj_on_connected(const redisAsyncContext *c, int status) 
{
    printf("Connected...\n");
}

static void __redisobj_on_disconnect(const redisAsyncContext *c, int status) 
{
    printf("Disconnected...\n");
}

void redisobj_create(const jconf_redis_server_pt jredis_server_cfg, aeEventLoop *el)
{
    struct lobj_fx fx = {
        .freeproc = &__redisobj_free,
        .referproc = NULL,
        .writeproc = NULL,
        .vwriteproc = &__redisobj_vwrite,
    };
    lobj_pt lop;
    struct redisobj *redis_server_obj;
    nsp_status_t status;

    lop = lobj_create(jredis_server_cfg->head.name, NULL, sizeof(struct redisobj), jredis_server_cfg->head.ctxsize, &fx);
    if (!lop) {
        return;
    }
    redis_server_obj = lobj_body(struct redisobj *, lop);

    do {
        status = netobj_parse_endpoint(jredis_server_cfg->host, &redis_server_obj->host);
        if (!NSP_SUCCESS(status)) {
            break;
        }

        redis_server_obj->el = el;
        redis_server_obj->c = redisAsyncConnect(redis_server_obj->host.ip, redis_server_obj->host.port);
        if (!redis_server_obj->c) {
            printf("Connection error: can't allocate redis context\n");
            break;
        }
        redisAeAttach(el, redis_server_obj->c);
        redisAsyncSetConnectCallback(redis_server_obj->c, &__redisobj_on_connected);
        redisAsyncSetDisconnectCallback(redis_server_obj->c, &__redisobj_on_disconnect);

        if (redis_server_obj->c->err) {
            printf("Connection error: %s\n", redis_server_obj->c->errstr);
            break;
        }

        return;
    } while(0);

    lobj_ldestroy(lop);
}
