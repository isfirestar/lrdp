#include "redisobj.h"

#include "netobj.h"

#include "hiredis.h"

struct redisobj
{
    struct endpoint host;
    redisContext *c;
};

static void __redisobj_free(lobj_pt lop)
{
    struct redisobj *rdsobj;

    rdsobj = lobj_body(struct redisobj *, lop);
    if (rdsobj->c) {
        redisFree(rdsobj->c);
    }
}

void redisobj_create(const jconf_redis_server_pt jredis_server_cfg)
{
    struct lobj_fx fx = {
        .freeproc = __redisobj_free,
        .referproc = NULL,
        .writeproc = NULL,
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

        rdsobj->c = redisConnect(rdsobj->host.ip, rdsobj->host.port);
        if (!rdsobj->c) {
            printf("Connection error: can't allocate redis context\n");
            break;
        }

        if (rdsobj->c->err) {
            printf("Connection error: %s\n", rdsobj->c->errstr);
            break;
        }

        return;
    } while(0);

    lobj_ldestroy(lop);
}
