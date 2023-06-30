#include "subscriberobj.h"

#include "redisobj.h"
#include "threading.h"
#include "hiredis.h"
#include "lobj.h"
#include "zmalloc.h"
#include "ifos.h"

struct subscribemsg
{
    char pattern[64];
};

struct subscriberobj
{
    lobj_pt redislop;
    void (*execproc)(lobj_pt lop, const char *channel, const char *pattern, const char *message, size_t len);
    lwp_t subscriberbg;
    struct subscribemsg *channel;
    size_t channels;
    int bg_join;
};

static void *__subscriberobj_bg(void *parameter)
{
    lobj_pt sublop;
    redisReply *reply;
    const char **subscribeCmd;
    size_t *subscribeCmdLen;
    struct subscriberobj *subobj;
    unsigned int i;
    redisContext *c;

    sublop = (lobj_pt)parameter;
    subobj = lobj_body(struct subscriberobj *, sublop);

    subscribeCmdLen = (size_t *)ztrycalloc(sizeof(size_t) * (subobj->channels + 1));
    if (!subscribeCmdLen) {
        printf("[%d] __subscriberobj_bg ztrycalloc subscribeCmdLen error\n", ifos_gettid());
        return NULL;
    }
    subscribeCmd = (const char **)ztrycalloc(sizeof(char *) * (subobj->channels + 1));
    if (!subscribeCmd) {
        printf("[%d] __subscriberobj_bg ztrycalloc subscribeCmd error\n", ifos_gettid());
        zfree(subscribeCmdLen);
        return NULL;
    }

    subscribeCmd[0] = "PSUBSCRIBE";
    subscribeCmdLen[0] = strlen("PSUBSCRIBE");
    for (i = 1; i < subobj->channels; i++) {
        subscribeCmd[i] = subobj->channel[i].pattern;
        subscribeCmdLen[i] = strlen(subobj->channel[i].pattern);
    }

    c = redisobj_get_connection(subobj->redislop);
    redisAppendCommandArgv(c, subobj->channels + 1, subscribeCmd, subscribeCmdLen);
    while (1) {
        if (redisGetReply(c, (void **)&reply) != REDIS_OK) {
            break;
        }
        if (!reply) {
            break;
        }
        if (reply->type == REDIS_REPLY_ARRAY) {
            if (reply->elements > 3 && subobj->execproc) {
                // if (reply->element[0]->type == REDIS_REPLY_STRING) {
                //     printf("[%d] redis command: %s\n", ifos_gettid(), reply->element[0]->str);
                // }
                // if (reply->element[1]->type == REDIS_REPLY_STRING) {
                //     printf("[%d] channel: %s\n", ifos_gettid(), reply->element[1]->str);
                // }
                // if (reply->element[2]->type == REDIS_REPLY_STRING) {
                //     if (0 == strcmp("vx", reply->element[2]->str)) {
                //         __rb_set_velocity(atof(reply->element[3]->str));
                //     }
                // }
                subobj->execproc(sublop, reply->element[1]->str, reply->element[2]->str, reply->element[3]->str, reply->element[3]->len);
            }
        }
        freeReplyObject(reply);
    }

    lwp_exit(NULL);
    return NULL;
}

static void __subscriberobj_free(struct lobj *lop)
{
    struct subscriberobj *subobj;

    if (!lop) {
        return;
    }

    // if (subobj->subscriberbg) {
    //     lwp_join(subobj->subscriberbg);
    // }

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
    static const struct lobj_fx fx = {
        .freeproc = &__subscriberobj_free,
        .referproc = NULL,
        .writeproc = NULL,
    };
    struct subscriberobj *subobj;
    unsigned int i;
    struct list_head *pos, *n;
    struct jconf_subscriber_channel *jsubchannelcfg;

    if (!jsubcfg) {
        return;
    }

    sublop = lobj_create(jsubcfg->head.name, jsubcfg->head.module, sizeof(struct subscriberobj), jsubcfg->head.ctxsize, &fx);
    if (!sublop) {
        return;
    }
    subobj = lobj_body(struct subscriberobj *, sublop);

    // obtain redis server object by given name
    // if redis server not exist, subscriber object will not be created
    if (NULL == (redislop = lobj_refer(jsubcfg->redisserver))) {
        lobj_ldestroy(sublop);
        return;
    }
    subobj->redislop = redislop;
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

    // create a thread for subscriber background working
    if (0 != lwp_create(&subobj->subscriberbg, 0, &__subscriberobj_bg, sublop)) {
        lobj_ldestroy(sublop);
        lobj_derefer(redislop);
        zfree(subobj->channel);
    }
}
