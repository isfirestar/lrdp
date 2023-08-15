#include "subscriberobj.h"

#include "redisobj.h"
#include "hiredis.h"
#include "async.h"
#include "lobj.h"
#include "zmalloc.h"
#include "ifos.h"

#include <unistd.h>

struct subscribemsg
{
    char pattern[64];
};

struct subscriberobj
{
    lobj_pt redislop;
    void (*execproc)(lobj_pt lop, const char *channel, const char *pattern, const char *message, size_t len);
    struct subscribemsg *channel;
    size_t channels;
};

static void __redisCallbackFn(struct redisAsyncContext *ac, void *r, void *priveData)
{
    lobj_pt lop;
    struct subscriberobj *subobj;
    redisReply *reply;

    reply = (redisReply *)r;
    if (!reply) {
        return;
    }

    lop = lobj_qrefer((lobj_seq_t)priveData);
    if (!lop) {
        return;
    }
    subobj = lobj_body(struct subscriberobj *, lop);

    if (reply->type == REDIS_REPLY_ARRAY &&
        reply->elements > 3 &&
        subobj->execproc &&
        reply->element[0]->type == REDIS_REPLY_STRING &&
        reply->element[1]->type == REDIS_REPLY_STRING &&
        reply->element[2]->type == REDIS_REPLY_STRING &&
        reply->element[3]->type == REDIS_REPLY_STRING)
    {
        if (0 == strcasecmp(reply->element[0]->str, "pmessage")) {
            subobj->execproc(lop, reply->element[1]->str, reply->element[2]->str, reply->element[3]->str, reply->element[3]->len);
        }
    }

    lobj_derefer(lop);
}

static void __do_subscriberobj(lobj_pt lop)
{
    const char **vdata;
    size_t *vsize;
    struct subscriberobj *subobj;
    unsigned int i;
    unsigned int vcount;

    subobj = lobj_body(struct subscriberobj *, lop);
    vcount = 2 + 1 + subobj->channels;
    // (callback + privateData) + PUBLISH + channels
    vdata = (const char **)ztrycalloc(sizeof(char *) * vcount);
    if (!vdata) {
        printf("[%d] __subscriberobj_bg ztrycalloc subscribeCmd error\n", ifos_gettid());
        return;
    }
    vsize = (size_t *)ztrycalloc(sizeof(size_t) * vcount);
    if (!vsize) {
        printf("[%d] __subscriberobj_bg ztrycalloc subscribeCmdLen error\n", ifos_gettid());
        zfree(vdata);
        return;
    }

    vdata[0] = (const char *)&__redisCallbackFn;
    vsize[0] = sizeof(void *);
    vdata[1] = (void *)lobj_get_seq(lop);
    vsize[1] = sizeof(void *);
    vdata[2] = "PSUBSCRIBE";
    vsize[2] = strlen("PSUBSCRIBE");
    for (i = 0; i < subobj->channels; i++) {
        vdata[i + 3] = subobj->channel[i].pattern;
        vsize[i + 3] = strlen(subobj->channel[i].pattern);
    }

    lobj_fx_vwrite(subobj->redislop,  vcount, (const void **)vdata, vsize);
    zfree(vdata);
    zfree(vsize);
}

static void __subscriberobj_free(struct lobj *lop, void *context, size_t ctxsize)
{
    struct subscriberobj *subobj;

    if (!lop) {
        return;
    }

    subobj = lobj_body(struct subscriberobj *, lop);
    if (subobj->channel) {
        zfree(subobj->channel);
    }

    if (subobj->redislop) {
        lobj_derefer(subobj->redislop);
    }
    zfree(subobj);
}

void subscriberobj_create(const jconf_subscriber_pt jsubcfg)
{
    lobj_pt redislop, sublop;
    struct lobj_fx_sym sym;
    struct lobj_fx fx = { NULL };
    struct subscriberobj *subobj;
    unsigned int i;
    struct list_head *pos, *n;
    struct jconf_subscriber_channel *jsubchannelcfg;

    if (!jsubcfg) {
        return;
    }

    fx.freeproc = &__subscriberobj_free;
    sublop = lobj_create(jsubcfg->head.name, jsubcfg->head.module, sizeof(struct subscriberobj), jsubcfg->head.ctxsize, &fx);
    if (!sublop) {
        return;
    }
    subobj = lobj_body(struct subscriberobj *, sublop);

    sym.touchproc_sym = jsubcfg->head.touchproc;
    sym.freeproc_sym = NULL;
    sym.writeproc_sym = jsubcfg->head.writeproc;
    sym.vwriteproc_sym = jsubcfg->head.vwriteproc;
    sym.readproc_sym = jsubcfg->head.readproc;
    sym.vreadproc_sym = jsubcfg->head.vreadproc;
    sym.recvdataproc_sym = jsubcfg->head.recvdataproc;
    sym.rawinvokeproc_sym = jsubcfg->head.rawinvokeproc;
    lobj_fx_cover(sublop, &sym);

    // obtain redis server object by given name
    // if redis server not exist, subscriber object will not be created
    if (NULL == (redislop = lobj_refer(jsubcfg->redisserver))) {
        lobj_ldestroy(sublop);
        return;
    }
    subobj->redislop = redislop;

    // load handler procedure
    subobj->execproc = lobj_dlsym(sublop, jsubcfg->execproc);
    if (!subobj->execproc) {
        printf("failed open symbol %s, error : %s\n", jsubcfg->execproc, ifos_dlerror());
    }

    // allocate and save channels/patterns
    subobj->channels = jsubcfg->channels_count;
    subobj->channel = (struct subscribemsg *)ztrycalloc(sizeof(struct subscribemsg) * subobj->channels);
    if (!subobj->channel) {
        lobj_ldestroy(sublop);
        lobj_derefer(redislop);
        return;
    }
    i = 0;
    list_for_each_safe(pos, n, &jsubcfg->head_of_channels) {
        jsubchannelcfg = list_entry(pos, struct jconf_subscriber_channel, ele_of_channels);
        if (i >= subobj->channels) {
            break;
        }
        strncpy(subobj->channel[i].pattern, jsubchannelcfg->pattern, sizeof(subobj->channel[i].pattern));
        ++i;
    }

    // start subscribe
    __do_subscriberobj(sublop);
}
