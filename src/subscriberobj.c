#include "subscriberobj.h"

#include "redisobj.h"
#include "hiredis.h"
#include "async.h"
#include "lobj.h"
#include "zmalloc.h"
#include "ifos.h"
#include "print.h"
#include "print.h"

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

static void __on_subscribed(struct redisAsyncContext *ac, void *r, void *priveData)
{
    lobj_pt lop;
    struct subscriberobj *subobj;
    redisReply *reply;

    if (!ac) {
        lrdp_generic_error("Subscriber redis asynchronous callback without legal context.");
        return;
    }

    if (ac->err) {
        if (REDIS_ERR_IO == ac->err) {
            lrdp_generic_error("Subscriber error: %s", strerror(errno));
        } else if (ac->errstr && ac->errstr[0]) {
            lrdp_generic_error("Subscriber error : %s", ac->errstr);
        } else {
            lrdp_generic_error("Subscriber error : %d", ac->err);
        }
        return;
    }

    reply = (redisReply *)r;
    if (!reply) {
        lrdp_generic_error("Subscriber reply not exist");
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
        lrdp_generic_error("Insufficient memory for subscriber object.");
        return;
    }
    vsize = (size_t *)ztrycalloc(sizeof(size_t) * vcount);
    if (!vsize) {
        lrdp_generic_error("Insufficient memory for subscriber object.");
        zfree(vdata);
        return;
    }

    vdata[0] = (const char *)&__on_subscribed;
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

static void __do_unsubscriberobj(lobj_pt lop)
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
        lrdp_generic_error("Insufficient memory for subscriber object.");
        return;
    }
    vsize = (size_t *)ztrycalloc(sizeof(size_t) * vcount);
    if (!vsize) {
        lrdp_generic_error("Insufficient memory for subscriber object.");
        zfree(vdata);
        return;
    }

    vdata[0] = NULL;
    vsize[0] = 0;
    vdata[1] = (void *)lobj_get_seq(lop);
    vsize[1] = sizeof(void *);
    vdata[2] = "PUNSUBSCRIBE";
    vsize[2] = strlen("PUNSUBSCRIBE");
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
    if (!subobj) {
        return;
    }

    // unsubscribe all channels
    __do_unsubscriberobj(lop);

    // release memory
    if (subobj->channel) {
        zfree(subobj->channel);
    }

    // detach the reference of redis object
    if (subobj->redislop) {
        lobj_derefer(subobj->redislop);
    }
}

void subscriberobj_create(const jconf_subscriber_pt jsubcfg)
{
    lobj_pt redislop, sublop;
    struct lobj_fx_sym sym = { NULL };
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
        lrdp_generic_error("Redis-server object %s for subscriber no found.", jsubcfg->redisserver);
        lobj_ldestroy(sublop);
        return;
    }
    subobj->redislop = redislop;

    // load handler procedure
    subobj->execproc = lobj_dlsym(sublop, jsubcfg->execproc);
    if (!subobj->execproc) {
        lrdp_generic_warning("Subscribe proc symbol %s no found, error : %s", jsubcfg->execproc, ifos_dlerror());
    }

    // allocate and save channels/patterns
    subobj->channels = jsubcfg->channels_count;
    subobj->channel = (struct subscribemsg *)ztrycalloc(sizeof(struct subscribemsg) * subobj->channels);
    if (!subobj->channel) {
        lrdp_generic_error("Insufficient memory for subscriber channel.");
        lobj_ldestroy(sublop);
        lobj_derefer(redislop);
        lrdp_generic_error("insufficient memory for subscriber channels.");
        return;
    }
    i = 0;
    list_for_each_safe(pos, n, &jsubcfg->head_of_channels) {
        jsubchannelcfg = list_entry(pos, struct jconf_subscriber_channel, ele_of_channels);
        if (i >= subobj->channels) {
            break;
        }
        subobj->channel[i].pattern[sizeof(subobj->channel[i].pattern) - 1] = '\0'; // ensure the last byte is '\0' for strncpy
        strncpy(subobj->channel[i].pattern, jsubchannelcfg->pattern, sizeof(subobj->channel[i].pattern) - 1);
        ++i;
    }

    // start subscribe
    __do_subscriberobj(sublop);
}
