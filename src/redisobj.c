#include "redisobj.h"

#include "netobj.h"

#include "hiredis.h"
#include "async.h"
#include "ae.h"

struct redisobj
{
    struct endpoint host;
    redisAsyncContext *c;
    aeEventLoop *el;
};

struct redisobj_na
{
    struct endpoint host;
    redisContext *c;
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

static void __redisobj_free_na(lobj_pt lop, void *context, size_t ctxsize)
{
    struct redisobj_na *redis_server_objna;

    redis_server_objna = lobj_body(struct redisobj_na *, lop);
    if (redis_server_objna->c) {
        redisFree(redis_server_objna->c);
    }
}

static void __redisobj_default_callbackfn(struct redisAsyncContext *ac, void *r, void *privateData)
{
}

static int __redisobj_vwrite(struct lobj *lop, int elements, const void **vdata, size_t *vsize)
{
    struct redisobj *redis_server_obj;
    redisCallbackFn *redisobjDefaultCallback;

    if (!lop || elements < 3 || !vdata || !vsize) {
        return -1;
    }

    if ( (sizeof(void *) != vsize[0]) && (NULL != vdata[0] )) {
        return -1;
    }

    redis_server_obj = lobj_body(struct redisobj *, lop);

    redisobjDefaultCallback = &__redisobj_default_callbackfn;
    if (vdata[0]) {
        redisobjDefaultCallback = (redisCallbackFn *)vdata[0];
    }

    return redisAsyncCommandArgv(redis_server_obj->c, redisobjDefaultCallback, (void *)vdata[1], elements - 2, (const char **)&vdata[2], &vsize[2]);
}

static int __redisobj_vread_na(lobj_pt lop, int elements, void **vdata, size_t *vsize)
{
    struct redisobj_na *redis_server_objna;
    redisReply *reply;
    int retval;

    if (!lop || !vdata || !vsize || elements < 2) {
        return -1;
    }

    redis_server_objna = lobj_body(struct redisobj_na *, lop);
    if (!redis_server_objna) {
        return -1;
    }

    reply = (redisReply *)redisCommandArgv(redis_server_objna->c, elements - 1, (const char **)vdata, vsize);
    if (!reply) {
        return -1;
    }

    retval = -1;
    switch (reply->type) {
        case REDIS_REPLY_ERROR:
            printf("Error: %s\n", reply->str);
            break;
        case REDIS_REPLY_INTEGER:
            if (vsize[elements - 1] >= sizeof(long long)) {
                *((long long *)&vdata[elements - 1]) = reply->integer;
                retval = sizeof(long long);
            }
            break;
        case REDIS_REPLY_STRING:
            retval = min(reply->len, vsize[elements - 1]);
            memcpy(vdata[elements - 1], reply->str, retval);
            break;
        case REDIS_REPLY_NIL:
            retval = 0;
            break;
        default:
            break;
    }

    freeReplyObject(reply);
    return retval;
}

static int __redisobj_vwrite_na(struct lobj *lop, int elements, const void **vdata, size_t *vsize)
{
    struct redisobj_na *redis_server_objna;
    redisReply *reply;

    if (!lop || !vdata || !vsize) {
        return -1;
    }

    redis_server_objna = lobj_body(struct redisobj_na *, lop);
    if (!redis_server_objna) {
        return -1;
    }

    reply = (redisReply *)redisCommandArgv(redis_server_objna->c, elements, (const char **)vdata, vsize);
    if (!reply) {
        return -1;
    }

    if (reply->type == REDIS_REPLY_ERROR) {
        printf("Error: %s\n", reply->str);
        freeReplyObject(reply);
        return -1;
    }

    return 0;
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
        .writeproc = NULL,
        .vwriteproc = &__redisobj_vwrite,
        .readproc = NULL,
        .vreadproc = NULL,
    };
    lobj_pt lop;
    struct redisobj *redis_server_obj;
    nsp_status_t status;

    lop = lobj_create(jredis_server_cfg->head.name, NULL, sizeof(struct redisobj), jredis_server_cfg->head.ctxsize, &fx);
    if (!lop) {
        return;
    }
    redis_server_obj = lobj_body(struct redisobj *, lop);
    // freeproc and vwrite proc can not be covered
    lobj_cover_fx(lop, NULL, jredis_server_cfg->head.writeproc, NULL, jredis_server_cfg->head.readproc, jredis_server_cfg->head.vreadproc);

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

extern void redisobj_create_na(const jconf_redis_server_pt jredis_server_cfg, aeEventLoop *el)
{
    struct lobj_fx fx = {
        .freeproc = &__redisobj_free_na,
        .writeproc = NULL,
        .vwriteproc = &__redisobj_vwrite_na,
        .readproc = NULL,
        .vreadproc = &__redisobj_vread_na,
    };
    lobj_pt lop;
    struct redisobj_na *redis_server_objna;
    nsp_status_t status;

    lop = lobj_create(jredis_server_cfg->head.name, NULL, sizeof(struct redisobj_na), jredis_server_cfg->head.ctxsize, &fx);
    if (!lop) {
        return;
    }
    redis_server_objna = lobj_body(struct redisobj_na *, lop);
    // freeproc and vwrite proc can not be covered
    lobj_cover_fx(lop, NULL, jredis_server_cfg->head.writeproc, NULL, jredis_server_cfg->head.readproc, NULL);

    do {
        status = netobj_parse_endpoint(jredis_server_cfg->host, &redis_server_objna->host);
        if (!NSP_SUCCESS(status)) {
            break;
        }

        redis_server_objna->el = el;
        redis_server_objna->c = redisConnect(redis_server_objna->host.ip, redis_server_objna->host.port);
        if (!redis_server_objna->c) {
            printf("Connection error: can't allocate redis context\n");
            break;
        }

        if (redis_server_objna->c->err) {
            printf("Connection error: %s\n", redis_server_objna->c->errstr);
            break;
        }

        return;
    } while(0);

    lobj_ldestroy(lop);
}
