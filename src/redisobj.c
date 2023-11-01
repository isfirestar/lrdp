#include "redisobj.h"
#include "aeobj.h"
#include "hiredis.h"
#include "async.h"
#include "print.h"

#include "netobj.h"

struct redisobj
{
    redisAsyncContext *c;
    struct endpoint host;
    char domain[128];
    lobj_pt lop_aeo;
};

struct redisobj_na
{
    redisContext *c;
    struct endpoint host;
    char domain[128];
    aeEventLoop *el;
};

static void __redisobj_free(lobj_pt lop, void *context, size_t ctxsize)
{
    struct redisobj *robj;

    robj = lobj_body(struct redisobj *, lop);
    if (robj->c) {
        redisAsyncFree(robj->c);
        robj->c = NULL;
    }

    if (robj->lop_aeo) {
        lobj_derefer(robj->lop_aeo);
        robj->lop_aeo = NULL;
    }
}

static void __redisobj_free_na(lobj_pt lop, void *context, size_t ctxsize)
{
    struct redisobj_na *robjna;

    robjna = lobj_body(struct redisobj_na *, lop);
    if (robjna->c) {
        redisFree(robjna->c);
    }
}

static void __redisobj_default_callbackfn(struct redisAsyncContext *ac, void *r, void *privateData)
{
    ;
}

static int __redisobj_write(lobj_pt lop, const void *data, size_t n)
{
    struct redisobj *robj;
    int retval;

    if (!lop || !data || 0 == n) {
        return -1;
    }

    robj = lobj_body(struct redisobj *, lop);
    retval = redisAsyncFormattedCommand(robj->c,  __redisobj_default_callbackfn, NULL, (const char *)data, n);
    if (retval != REDIS_OK) {
        return -1;
    }
    redisAsyncHandleWrite(robj->c);
    return retval;
}

static int __redisobj_vwrite(struct lobj *lop, int elements, const void **vdata, const size_t *vsize)
{
    struct redisobj *robj;
    redisCallbackFn *redisobjDefaultCallback;
    int retval;

    if (!lop || elements < 3 || !vdata || !vsize) {
        return -1;
    }

    if ( (sizeof(void *) != vsize[0]) && (NULL != vdata[0] )) {
        return -1;
    }

    robj = lobj_body(struct redisobj *, lop);

    redisobjDefaultCallback = &__redisobj_default_callbackfn;
    if (vdata[0]) {
        redisobjDefaultCallback = (redisCallbackFn *)vdata[0];
    }

    retval = redisAsyncCommandArgv(robj->c, redisobjDefaultCallback, (void *)vdata[1], elements - 2, (const char **)&vdata[2], &vsize[2]);
    if (retval != REDIS_OK) {
        return -1;
    }
    redisAsyncHandleWrite(robj->c);
    return retval;
}

extern int redisAeAttach(aeEventLoop *loop, redisAsyncContext *ac);

static void __redisobj_on_connected(const redisAsyncContext *c, int status)
{
    if (c->c.connection_type == REDIS_CONN_UNIX) {
        lrdp_generic_info("Connected to unix domain %s", c->c.unix_sock.path);
    } else {
        lrdp_generic_info("Connect to %s:%d", c->c.tcp.host, c->c.tcp.port);
    }
}

static void __redisobj_on_disconnect(const redisAsyncContext *c, int status)
{
    lrdp_generic_error("Disconnected to redis server %s", c->c.connection_type == REDIS_CONN_UNIX ? c->c.unix_sock.path : c->c.tcp.host);
}

void redisobj_create(const jconf_redis_server_pt jredis_server_cfg, aeEventLoop *el)
{
    aeEventLoop *self_el;
    struct lobj_fx fx = {
        .freeproc = &__redisobj_free,
        .writeproc = &__redisobj_write,
        .vwriteproc = &__redisobj_vwrite,
        .readproc = NULL,
        .vreadproc = NULL,
    };
    struct lobj_fx_sym sym = { NULL };
    lobj_pt lop;
    struct redisobj *robj;
    nsp_status_t status;

    lop = lobj_create(jredis_server_cfg->head.name, NULL, sizeof(struct redisobj), jredis_server_cfg->head.ctxsize, &fx);
    if (!lop) {
        return;
    }
    robj = lobj_body(struct redisobj *, lop);

    // freeproc and vwrite proc can not be covered
    sym.touchproc_sym = jredis_server_cfg->head.touchproc;
    sym.freeproc_sym = NULL;
    sym.writeproc_sym = jredis_server_cfg->head.writeproc;
    sym.vwriteproc_sym = NULL;
    sym.readproc_sym = jredis_server_cfg->head.readproc;
    sym.vreadproc_sym = jredis_server_cfg->head.vreadproc;
    sym.recvdataproc_sym = jredis_server_cfg->head.recvdataproc;
    sym.rawinvokeproc_sym = jredis_server_cfg->head.rawinvokeproc;
    lobj_fx_cover(lop, &sym);

    do {
        // use which ae
        robj->lop_aeo = lobj_refer(jredis_server_cfg->aeo);
        if (robj->lop_aeo) {
            self_el = aeobj_getel(robj->lop_aeo);
        } else {
            self_el = el;
        }

        // get target host
        status = netobj_parse_endpoint(jredis_server_cfg->host, &robj->host);
        if (NSP_SUCCESS(status)) {
            robj->c = redisAsyncConnect(robj->host.ip, robj->host.port);
        } else {
            robj->domain[sizeof(robj->domain) - 1] = '\0'; // ensure null-terminated
            strncpy(robj->domain, jredis_server_cfg->host, sizeof(robj->domain) - 1);
            robj->c = redisAsyncConnectUnix(robj->domain);
        }

        if (!robj->c) {
            lrdp_generic_error("Connection error: can't allocate redis context");
            break;
        }

        redisAsyncSetConnectCallback(robj->c, &__redisobj_on_connected);
        redisAsyncSetDisconnectCallback(robj->c, &__redisobj_on_disconnect);
        redisAeAttach(self_el, robj->c);

        if (robj->c->err) {
            if (REDIS_ERR_IO == robj->c->err) {
                lrdp_generic_error("Connection error: %s", strerror(errno));
            } else if (robj->c->errstr && robj->c->errstr[0]) {
                lrdp_generic_error("Connection error: %s", robj->c->errstr);
            } else {
                lrdp_generic_error("Connection error: %d", robj->c->err);
            }
            break;
        }

        return;
    } while(0);

    lobj_ldestroy(lop);
}

static int __redisobj_vread_na(lobj_pt lop, int elements, void **vdata, size_t *vsize)
{
    struct redisobj_na *robjna;
    redisReply *reply;
    int retval;

    if (!lop || !vdata || !vsize || elements < 2) {
        return -EINVAL;
    }

    robjna = lobj_body(struct redisobj_na *, lop);
    if (!robjna) {
        return -ENODATA;
    }

    reply = (redisReply *)redisCommandArgv(robjna->c, elements - 1, (const char **)vdata, vsize);
    if (!reply) {
        return -ENOMEM;
    }

    retval = -1;
    switch (reply->type) {
        case REDIS_REPLY_ERROR:
            lrdp_generic_error("Reply error: %s", reply->str);
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
        case REDIS_REPLY_DOUBLE:
            if (vsize[elements - 1] >= sizeof(double)) {
                *((double *)&vdata[elements - 1]) = reply->dval;
                retval = sizeof(double);
            }
            break;
        case REDIS_REPLY_NIL:
            retval = 0;
            break;
        default:
            retval = -EPROTOTYPE;
            break;
    }

    freeReplyObject(reply);
    return retval;
}

static int __redisobj_vwrite_na(struct lobj *lop, int elements, const void **vdata, const size_t *vsize)
{
    struct redisobj_na *robjna;
    redisReply *reply;
    int replyType;

    if (!lop || !vdata || !vsize) {
        return -1;
    }

    robjna = lobj_body(struct redisobj_na *, lop);
    if (!robjna) {
        return -1;
    }

    reply = (redisReply *)redisCommandArgv(robjna->c, elements, (const char **)vdata, vsize);
    if (!reply) {
        return -1;
    }

    replyType = reply->type;
    freeReplyObject(reply);
    return (replyType == REDIS_REPLY_ERROR) ? -1 : 0;
}

static int __redisobj_write_na(struct lobj *lop, const void *data, size_t n)
{
    struct redisobj_na *robjna;
    redisReply *reply;
    int replyType;

    if (!lop || !data || 0 == n) {
        return -1;
    }

    robjna = lobj_body(struct redisobj_na *, lop);
    if (!robjna) {
        return -1;
    }

    if (REDIS_OK != redisAppendFormattedCommand(robjna->c, (const char *)data, n)) {
        return -1;
    }

    if (REDIS_OK != redisGetReply(robjna->c, (void **)&reply)) {
        return -1;
    }

    replyType = reply->type;
    freeReplyObject(reply);
    return (replyType == REDIS_REPLY_ERROR) ? -1 : 0;
}

extern void redisobj_create_na(const jconf_redis_server_pt jredis_server_cfg)
{
    struct lobj_fx fx = {
        .freeproc = &__redisobj_free_na,
        .writeproc = &__redisobj_write_na,
        .vwriteproc = &__redisobj_vwrite_na,
        .readproc = NULL,
        .vreadproc = &__redisobj_vread_na,
    };
    struct lobj_fx_sym sym = { NULL };
    lobj_pt lop;
    struct redisobj_na *robjna;
    nsp_status_t status;

    lop = lobj_create(jredis_server_cfg->head.name, NULL, sizeof(struct redisobj_na), jredis_server_cfg->head.ctxsize, &fx);
    if (!lop) {
        return;
    }
    robjna = lobj_body(struct redisobj_na *, lop);

    // freeproc/vwrite/vread proc can not be covered
    sym.touchproc_sym = jredis_server_cfg->head.touchproc;
    sym.freeproc_sym = NULL;
    sym.writeproc_sym = jredis_server_cfg->head.writeproc;
    sym.vwriteproc_sym = NULL;
    sym.readproc_sym = jredis_server_cfg->head.readproc;
    sym.vreadproc_sym = NULL;
    sym.recvdataproc_sym = jredis_server_cfg->head.recvdataproc;
    sym.rawinvokeproc_sym = jredis_server_cfg->head.rawinvokeproc;
    lobj_fx_cover(lop, &sym);

    do {
        status = netobj_parse_endpoint(jredis_server_cfg->host, &robjna->host);
        if (NSP_SUCCESS(status)) {
            robjna->c = redisConnect(robjna->host.ip, robjna->host.port);
        } else {
            robjna->domain[sizeof(robjna->domain) - 1] = '\0'; // ensure null-terminated
            strncpy(robjna->domain, jredis_server_cfg->host, sizeof(robjna->domain) - 1);
            robjna->c = redisConnectUnix(robjna->domain);
        }
        if (!robjna->c) {
            lrdp_generic_error("Connection error: can't allocate redis context");
            break;
        }

        if (robjna->c->err) {
            if (REDIS_ERR_IO == robjna->c->err) {
                lrdp_generic_error("Connection error: %s", strerror(errno));
            } else if (robjna->c->errstr && robjna->c->errstr[0]) {
                lrdp_generic_error("Connection error: %s", robjna->c->errstr);
            } else {
                lrdp_generic_error("Connection error: %d", robjna->c->err);
            }
            break;
        }

        return;
    } while(0);

    lobj_ldestroy(lop);
}
