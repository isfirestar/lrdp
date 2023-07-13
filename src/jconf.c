#include "jconf.h"
#include "ifos.h"
#include "zmalloc.h"
#include "cjson.h"

static struct jconf_entry jentry = { .preinitproc = { 0 }, .postinitproc = { 0 },
    .head = { .ctxsize = 0, .module = { 0 }, .name = { 0 } } };

static cJSON *__jconf_load_head(cJSON *jcursor, struct jconf_head *cfhead)
{
    cJSON *jnext;

    if (!jcursor || !cfhead) {
        return NULL;
    }

    if (jcursor->type != cJSON_Object) {
        return NULL;
    }

    strncpy(cfhead->name, jcursor->string, sizeof(cfhead->name) - 1);
    jnext = jcursor->child;

    while (jnext) {
        if (0 == strcasecmp(jnext->string, "module") && jnext->type == cJSON_String) {
            strncpy(cfhead->module, jnext->valuestring, sizeof(cfhead->module) - 1);
        } else if (0 == strcasecmp(jnext->string, "freeproc") && jnext->type == cJSON_String) {
            strncpy(cfhead->freeproc, jnext->valuestring, sizeof(cfhead->freeproc) - 1);
        } else if (0 == strcasecmp(jnext->string, "readproc") && jnext->type == cJSON_String) {
            strncpy(cfhead->readproc, jnext->valuestring, sizeof(cfhead->readproc) - 1);
        } else if (0 == strcasecmp(jnext->string, "vreadproc") && jnext->type == cJSON_String) {
            strncpy(cfhead->vreadproc, jnext->valuestring, sizeof(cfhead->vreadproc) - 1);
        } else if (0 == strcasecmp(jnext->string, "writeproc") && jnext->type == cJSON_String) {
            strncpy(cfhead->writeproc, jnext->valuestring, sizeof(cfhead->writeproc) - 1);
        } else if (0 == strcasecmp(jnext->string, "vwriteproc") && jnext->type == cJSON_String) {
            strncpy(cfhead->vwriteproc, jnext->valuestring, sizeof(cfhead->vwriteproc) - 1);
        } else if ( (0 == strcasecmp(jnext->string, "ctxsize") || 0 == strcasecmp(jnext->string, "contextsize") ) && jnext->type == cJSON_Number) {
            cfhead->ctxsize = jnext->valueint;
        } else {
            ;
        }
        jnext = jnext->next;
    }

    return jcursor->child;
}

/* see demo/demo.json
 * "entry" section have 3 validate sub sections, their are:
 * 1.   "module" which indicate the module name
 * 2.   "entryproc" which indicate the entry procedure name
 * 3.   "entrytimer" option, which indicate the entry timer if you need
*/
static void __jconf_entry_load(cJSON *entry)
{
    cJSON *jcursor;

    jcursor = __jconf_load_head(entry, &jentry.head);
    while (jcursor) {
        if (jcursor->type == cJSON_String) {
            if (0 == strcasecmp(jcursor->string, "preinitproc")) {
                strncpy(jentry.preinitproc, jcursor->valuestring, sizeof(jentry.preinitproc) - 1);
            } else if (0 == strcasecmp(jcursor->string, "postinitproc")) {
                strncpy(jentry.postinitproc, jcursor->valuestring, sizeof(jentry.postinitproc) - 1);
            }
        }
        jcursor = jcursor->next;
    }

    strcpy(jentry.head.name, "mainloop");
}

jconf_entry_pt jconf_entry_get(void)
{
    return &jentry;
}

static void __jconf_lwp_load(cJSON *entry);
static void __jconf_net_load(cJSON *entry);
static void __jconf_tty_load(cJSON *entry);
static void __jconf_timer_load(cJSON *entry);
static void __jconf_redis_server_load(cJSON *entry);
static void __jconf_subscriber_load(cJSON *entry);
static void __jconf_rawobj_load(cJSON *entry);
static void __jconf_epollobj_load(cJSON *entry);

nsp_status_t jconf_initial_load(const char *jsonfile)
{
    file_descriptor_t fd;
    nsp_status_t status;
    int64_t filesize;
    char *jsondata;
    int nread;
    cJSON *jroot, *jcursor;

    fd = -1;
    jsondata = NULL;
    jroot = NULL;

    status = ifos_file_open(jsonfile, FF_RDACCESS | FF_OPEN_EXISTING, 0644, &fd);
    if (!NSP_SUCCESS(status)) {
        return status;
    }

    do {
        filesize = ifos_file_fgetsize(fd);
        if (filesize <= 0) {
            status = posix__makeerror(ERANGE);
            break;
        }

        jsondata = ztrymalloc(filesize);
        if (!jsondata) {
            status = posix__makeerror(ENOMEM);
            break;
        }

        nread = ifos_file_read(fd, jsondata, filesize);
        if (nread != filesize) {
            status = posix__makeerror(EIO);
            break;
        }

        // parse the json data
        jroot = cJSON_Parse(jsondata);
        if (!jroot) {
            status = posix__makeerror(EINVAL);
            break;
        }

        // travesal the json section
        jcursor = jroot->child;
        while (jcursor) {
            if (jcursor->type == cJSON_Object && 0 == strcasecmp(jcursor->string, "mainloop")) {
                __jconf_entry_load(jcursor);
            } else if (jcursor->type == cJSON_Object && 0 == strcasecmp(jcursor->string, "lwps")) {
                __jconf_lwp_load(jcursor);
            } else if (jcursor->type == cJSON_Object && 0 == strcasecmp(jcursor->string, "nets")) {
                __jconf_net_load(jcursor);
            } else if (jcursor->type == cJSON_Object && 0 == strcasecmp(jcursor->string, "ttys")) {
                __jconf_tty_load(jcursor);
            } else if (jcursor->type == cJSON_Object && 0 == strcasecmp(jcursor->string, "timers")) {
                __jconf_timer_load(jcursor);
            } else if (jcursor->type == cJSON_Object && 0 == strcasecmp(jcursor->string, "redis-server")) {
                __jconf_redis_server_load(jcursor);
            } else if (jcursor->type == cJSON_Object && 0 == strcasecmp(jcursor->string, "subscriber")) {
                __jconf_subscriber_load(jcursor);
            } else if (jcursor->type == cJSON_Object && 0 == strcasecmp(jcursor->string, "imports")) {
                __jconf_rawobj_load(jcursor);
            } else if (jcursor->type == cJSON_Object && 0 == strcasecmp(jcursor->string, "epoll")) {
                __jconf_epollobj_load(jcursor);
            } else {
                ;
            }
            jcursor = jcursor->next;
        }

    } while (0);

    // release resource no matter success or failed
    if (fd != -1) {
        ifos_file_close(fd);
    }

    if (jsondata) {
        zfree(jsondata);
    }

    if (jroot) {
        cJSON_Delete(jroot);
    }

    return status;
}

struct jconf_iterator
{
    struct list_head *cursor;
};

void jconf_release_iterator(jconf_iterator_pt iterator)
{
    if (iterator) {
        zfree(iterator);
    }
}

/* -----------------------------------------------------------------------------------------------------------------------------
 * ------------------------------------------        LWP IMPLEMENTATIONs        ------------------------------------------------
 * ----------------------------------------------------------------------------------------------------------------------------- */
struct jconf_lwp_inner
{
    struct jconf_lwp body;
    struct list_head ele_of_inner;
};

static struct list_head g_jlwps_head = { &g_jlwps_head, &g_jlwps_head };
static unsigned int jlwps_count = 0;

static void __jconf_lwp_load(cJSON *entry)
{
    cJSON *jcursor, *jnext;
    struct jconf_lwp_inner *lwp;

    jcursor = entry->child;

    while (jcursor) {
        lwp = ztrycalloc(sizeof(*lwp));
        if (!lwp) {
            break;
        }

        jnext = __jconf_load_head(jcursor, &lwp->body.head);
        if (!jnext) {
            zfree(lwp);
            break;
        }

        do {
            if (jnext->type == cJSON_String) {
                if (0 == strcasecmp(jnext->string, "execproc")) {
                    strncpy(lwp->body.execproc, jnext->valuestring, sizeof(lwp->body.execproc) - 1);
                }
            } else if (jnext->type == cJSON_Number) {
                if (0 == strcasecmp(jnext->string, "priority")) {
                    lwp->body.priority = jnext->valueint;
                } else if (0 == strcasecmp(jnext->string, "affinity")) {
                    lwp->body.affinity = jnext->valueint;
                }
            }
            jnext = jnext->next;
        } while (jnext);

        list_add_tail(&lwp->ele_of_inner, &g_jlwps_head);
        jlwps_count++;
        jcursor = jcursor->next;
    }
}

jconf_iterator_pt jconf_lwp_get_iterator(unsigned int *count)
{
    jconf_iterator_pt iterator;

    if (count) {
        *count = jlwps_count;
    }

    if (0 == count) {
        return NULL;
    }

    iterator = ztrymalloc(sizeof(*iterator));
    if (!iterator) {
        return NULL;
    }

    iterator->cursor = g_jlwps_head.next;
    return iterator;
}

jconf_iterator_pt jconf_lwp_get(jconf_iterator_pt iterator, jconf_lwp_pt *jlwp)
{
    struct list_head *cursor;
    struct jconf_lwp_inner *inner;

    if (!iterator || !jlwp) {
        return NULL;
    }

    cursor = iterator->cursor;
    if (cursor == &g_jlwps_head) {
        zfree(iterator);
        return NULL;
    }

    inner = container_of(cursor, struct jconf_lwp_inner, ele_of_inner);
    *jlwp  = &inner->body;
    iterator->cursor = cursor->next;
    return iterator;
}

void jconf_lwp_free()
{
    struct list_head *cursor, *next;
    struct jconf_lwp_inner *inner;

    list_for_each_safe(cursor, next, &g_jlwps_head) {
        inner = container_of(cursor, struct jconf_lwp_inner, ele_of_inner);
        list_del(cursor);
        zfree(inner);
    }
}

/* -----------------------------------------------------------------------------------------------------------------------------
 * ------------------------------------------        NET IMPLEMENTATIONs        ------------------------------------------------
 * ----------------------------------------------------------------------------------------------------------------------------- */
struct jconf_net_inner
{
    struct jconf_net body;
    struct list_head ele_of_inner;
};
static struct list_head g_jnets_head = { &g_jnets_head, &g_jnets_head };
static unsigned int jnets_count = 0;

static void __jconf_net_load(cJSON *entry)
{
    cJSON *jcursor, *jnext;
    struct jconf_net_inner *net;

    jcursor = entry->child;

    while (jcursor) {
        net = ztrycalloc(sizeof(*net));
        if (!net) {
            break;
        }

        jnext = __jconf_load_head(jcursor, &net->body.head);
        if (!jnext) {
            zfree(net);
            break;
        }

        do {
            if (jnext->type == cJSON_String) {
                if (0 == strcasecmp(jnext->string, "acceptproc")) {
                    strncpy(net->body.acceptproc, jnext->valuestring, sizeof(net->body.acceptproc) - 1);
                } else if (0 == strcasecmp(jnext->string, "connectproc")) {
                    strncpy(net->body.connectproc, jnext->valuestring, sizeof(net->body.connectproc) - 1);
                } else if (0 == strcasecmp(jnext->string, "listen")) {
                    strncpy(net->body.listen, jnext->valuestring, sizeof(net->body.listen) - 1);
                } else if (0 == strcasecmp(jnext->string, "remote")) {
                    strncpy(net->body.remote, jnext->valuestring, sizeof(net->body.remote) - 1);
                } else if (0 == strcasecmp(jnext->string, "local")) {
                    strncpy(net->body.local, jnext->valuestring, sizeof(net->body.local) - 1);
                } else if (0 == strcasecmp(jnext->string, "protocol")) {
                    if (0 == strcasecmp(jnext->valuestring, "tcp")) {
                        net->body.protocol = JCFG_PROTO_TCP;
                    } else if (0 == strcasecmp(jnext->valuestring, "udp")) {
                        net->body.protocol = JCFG_PROTO_UDP;
                    } else {
                        net->body.protocol = JCFG_PROTO_ERR;
                    }
                }
            }
            jnext = jnext->next;
        } while (jnext);

        list_add_tail(&net->ele_of_inner, &g_jnets_head);
        jnets_count++;
        jcursor = jcursor->next;
    }
}

jconf_iterator_pt jconf_net_get_iterator(unsigned int *count)
{
    jconf_iterator_pt iterator;

    if (count) {
        *count = jnets_count;
    }

    if (0 == count) {
        return NULL;
    }

    iterator = ztrymalloc(sizeof(*iterator));
    if (!iterator) {
        return NULL;
    }

    iterator->cursor = g_jnets_head.next;
    return iterator;
}

jconf_iterator_pt jconf_net_get(jconf_iterator_pt iterator, jconf_net_pt *jnets)
{
    struct list_head *cursor;
    struct jconf_net_inner *inner;

    if (!iterator || !jnets) {
        return NULL;
    }

    cursor = iterator->cursor;
    if (cursor == &g_jnets_head) {
        zfree(iterator);
        return NULL;
    }

    inner = container_of(cursor, struct jconf_net_inner, ele_of_inner);
    *jnets  = &inner->body;
    iterator->cursor = cursor->next;
    return iterator;
}

void jconf_net_free()
{
    struct list_head *cursor, *next;
    struct jconf_net_inner *inner;

    list_for_each_safe(cursor, next, &g_jnets_head) {
        inner = container_of(cursor, struct jconf_net_inner, ele_of_inner);
        list_del(cursor);
        zfree(inner);
    }
}

/* -----------------------------------------------------------------------------------------------------------------------------
 * ------------------------------------------        TTY IMPLEMENTATIONs        ------------------------------------------------
 * ----------------------------------------------------------------------------------------------------------------------------- */
struct jconf_tty_inner
{
    struct jconf_tty body;
    struct list_head ele_of_inner_tty;
};
static struct list_head g_jtty_head = { &g_jtty_head, &g_jtty_head };
static unsigned int jtty_count = 0;

static void __jconf_tty_load(cJSON *entry)
{
    cJSON *jcursor, *jnext;
    struct jconf_tty_inner *tty;

    jcursor = entry->child;

    while (jcursor) {
        tty = ztrycalloc(sizeof(*tty));
        if (!tty) {
            break;
        }

        jnext = __jconf_load_head(jcursor, &tty->body.head);
        if (!jnext) {
            zfree(tty);
            break;
        }

        do {
            if (jnext->type == cJSON_String) {
                if (0 == strcasecmp(jnext->string, "device")) {
                    strncpy(tty->body.device, jnext->valuestring, sizeof(tty->body.device) - 1);
                } else if (0 == strcasecmp(jnext->string, "parity")) {
                    strncpy(tty->body.parity, jnext->valuestring, sizeof(tty->body.parity) - 1);
                } else if (0 == strcasecmp(jnext->string, "flowcontrol")) {
                    strncpy(tty->body.flowcontrol, jnext->valuestring, sizeof(tty->body.flowcontrol) - 1);
                }
            } else if (jnext->type == cJSON_Number) {
                if (0 == strcasecmp(jnext->string, "baudrate")) {
                    tty->body.baudrate = jnext->valueint;
                } else if (0 == strcasecmp(jnext->string, "databits")) {
                    tty->body.databits = jnext->valueint;
                } else if (0 == strcasecmp(jnext->string, "stopbits")) {
                    tty->body.stopbits = jnext->valueint;
                }
            }

            jnext = jnext->next;
        }while (jnext);

        list_add_tail(&tty->ele_of_inner_tty, &g_jtty_head);
        jtty_count++;
        jcursor = jcursor->next;
    }
}

jconf_iterator_pt jconf_tty_get_iterator(unsigned int *count)
{
    jconf_iterator_pt iterator;

    if (count) {
        *count = jtty_count;
    }

    if (0 == count) {
        return NULL;
    }

    iterator = ztrymalloc(sizeof(*iterator));
    if (!iterator) {
        return NULL;
    }

    iterator->cursor = g_jtty_head.next;
    return iterator;
}

jconf_iterator_pt jconf_tty_get(jconf_iterator_pt iterator, jconf_tty_pt *jttys)
{
    struct list_head *cursor;
    struct jconf_tty_inner *inner;

    if (!iterator || !jttys) {
        return NULL;
    }

    cursor = iterator->cursor;
    if (cursor == &g_jtty_head) {
        zfree(iterator);
        return NULL;
    }

    inner = container_of(cursor, struct jconf_tty_inner, ele_of_inner_tty);
    *jttys  = &inner->body;
    iterator->cursor = cursor->next;
    return iterator;
}

void jconf_tty_free()
{
    struct list_head *cursor, *next;
    struct jconf_tty_inner *inner;

    list_for_each_safe(cursor, next, &g_jtty_head) {
        inner = container_of(cursor, struct jconf_tty_inner, ele_of_inner_tty);
        list_del(cursor);
        zfree(inner);
    }
}

/* -----------------------------------------------------------------------------------------------------------------------------
 * ------------------------------------------        TIMER IMPLEMENTATIONs        ------------------------------------------------
 * ----------------------------------------------------------------------------------------------------------------------------- */
struct jconf_timer_inner
{
    struct jconf_timer body;
    struct list_head ele_of_inner_timer;
};
static struct list_head g_jtimer_head = { &g_jtimer_head, &g_jtimer_head };
static unsigned int jtimer_count = 0;

static void __jconf_timer_load(cJSON *entry)
{
    cJSON *jcursor, *jnext;
    struct jconf_timer_inner *timer;

    jcursor = entry->child;

    while (jcursor) {
        timer = ztrycalloc(sizeof(*timer));
        if (!timer) {
            break;
        }

        jnext = __jconf_load_head(jcursor, &timer->body.head);
        if (!jnext) {
            zfree(timer);
            break;
        }
        do {
            if (jnext->type == cJSON_String) {
                if (0 == strcasecmp(jnext->string, "timerproc")) {
                    strncpy(timer->body.timerproc, jnext->valuestring, sizeof(timer->body.timerproc) - 1);
                }
            } else if (jnext->type == cJSON_Number) {
                if (0 == strcasecmp(jnext->string, "interval")) {
                    timer->body.interval = jnext->valueint;
                }
            }

            jnext = jnext->next;
        } while (jnext);

        list_add_tail(&timer->ele_of_inner_timer, &g_jtimer_head);
        jtimer_count++;
        jcursor = jcursor->next;
    }
}

jconf_iterator_pt jconf_timer_get_iterator(unsigned int *count)
{
    jconf_iterator_pt iterator;

    if (count) {
        *count = jtimer_count;
    }

    if (0 == count) {
        return NULL;
    }

    iterator = ztrymalloc(sizeof(*iterator));
    if (!iterator) {
        return NULL;
    }

    iterator->cursor = g_jtimer_head.next;
    return iterator;
}

jconf_iterator_pt jconf_timer_get(jconf_iterator_pt iterator, jconf_timer_pt *jtimers)
{
    struct list_head *cursor;
    struct jconf_timer_inner *inner;

    if (!iterator || !jtimers) {
        return NULL;
    }

    cursor = iterator->cursor;
    if (cursor == &g_jtimer_head) {
        zfree(iterator);
        return NULL;
    }

    inner = container_of(cursor, struct jconf_timer_inner, ele_of_inner_timer);
    *jtimers  = &inner->body;
    iterator->cursor = cursor->next;
    return iterator;
}

void jconf_timer_free()
{
    struct list_head *cursor, *next;
    struct jconf_timer_inner *inner;

    list_for_each_safe(cursor, next, &g_jtimer_head) {
        inner = container_of(cursor, struct jconf_timer_inner, ele_of_inner_timer);
        list_del(cursor);
        zfree(inner);
    }
}

/* -----------------------------------------------------------------------------------------------------------------------------
 * ------------------------------------------        REDIS-SERVER IMPLEMENTATIONs        ------------------------------------------------
 * ----------------------------------------------------------------------------------------------------------------------------- */
struct jconf_redis_server_inner
{
    struct jconf_redis_server body;
    struct list_head ele_of_inner_redis_server;
};
static struct list_head g_jredis_server_head = { &g_jredis_server_head, &g_jredis_server_head };
static unsigned int jredis_server_count = 0;

static void __jconf_redis_server_load(cJSON *entry)
{
    cJSON *jcursor, *jnext;
    struct jconf_redis_server_inner *redis_server;

    jcursor = entry->child;

    while (jcursor) {
        redis_server = ztrycalloc(sizeof(*redis_server));
        if (!redis_server) {
            break;
        }

        jnext = __jconf_load_head(jcursor, &redis_server->body.head);
        if (!jnext) {
            zfree(redis_server);
            break;
        }

        do {
            if (jnext->type == cJSON_String) {
                if (0 == strcasecmp(jnext->string, "host")) {
                    strncpy(redis_server->body.host, jnext->valuestring, sizeof(redis_server->body.host) - 1);
                }
            }
            jnext = jnext->next;
        } while (jnext);

        list_add_tail(&redis_server->ele_of_inner_redis_server, &g_jredis_server_head);
        jredis_server_count++;
        jcursor = jcursor->next;
    }
}

jconf_iterator_pt jconf_redis_server_get_iterator(unsigned int *count)
{
    jconf_iterator_pt iterator;

    if (count) {
        *count = jredis_server_count;
    }

    if (0 == count) {
        return NULL;
    }

    iterator = ztrymalloc(sizeof(*iterator));
    if (!iterator) {
        return NULL;
    }

    iterator->cursor = g_jredis_server_head.next;
    return iterator;
}

jconf_iterator_pt jconf_redis_server_get(jconf_iterator_pt iterator, jconf_redis_server_pt *jredis_servers)
{
    struct list_head *cursor;
    struct jconf_redis_server_inner *inner;

    if (!iterator || !jredis_servers) {
        return NULL;
    }

    cursor = iterator->cursor;
    if (cursor == &g_jredis_server_head) {
        zfree(iterator);
        return NULL;
    }

    inner = container_of(cursor, struct jconf_redis_server_inner, ele_of_inner_redis_server);
    *jredis_servers  = &inner->body;
    iterator->cursor = cursor->next;
    return iterator;
}

void jconf_redis_server_free()
{
    struct list_head *cursor, *next;
    struct jconf_redis_server_inner *inner;

    list_for_each_safe(cursor, next, &g_jredis_server_head) {
        inner = container_of(cursor, struct jconf_redis_server_inner, ele_of_inner_redis_server);
        list_del_init(cursor);
        zfree(inner);
    }
}

/* -----------------------------------------------------------------------------------------------------------------------------
 * ------------------------------------------        SUBSCRIBER IMPLEMENTATIONs        ------------------------------------------------
 * ----------------------------------------------------------------------------------------------------------------------------- */
struct jconf_subscriber_inner
{
    struct jconf_subscriber body;
    struct list_head ele_of_inner_subscriber;
};
static struct list_head g_jsubscriber_head = { &g_jsubscriber_head, &g_jsubscriber_head };
static unsigned int jsubscriber_count = 0;

static void __jconf_subscriber_load(cJSON *entry)
{
    cJSON *jcursor, *jsubscriber, *jchannel;
    struct jconf_subscriber_inner *subscriber;
    struct jconf_subscriber_channel *channel;

    jcursor = entry->child;

    while (jcursor) {
        subscriber = ztrycalloc(sizeof(*subscriber));
        if (!subscriber) {
            break;
        }
        INIT_LIST_HEAD(&subscriber->body.head_of_channels);

        jsubscriber = __jconf_load_head(jcursor, &subscriber->body.head);
        if (!jsubscriber) {
            zfree(subscriber);
            break;
        }

        do {
            if (jsubscriber->type == cJSON_String) {
                if (0 == strcasecmp(jsubscriber->string, "redis-server")) {
                    strncpy(subscriber->body.redisserver, jsubscriber->valuestring, sizeof(subscriber->body.redisserver) - 1);
                } else if (0 == strcasecmp(jsubscriber->string, "execproc")) {
                    strncpy(subscriber->body.execproc, jsubscriber->valuestring, sizeof(subscriber->body.execproc) - 1);
                }
            }  else if (jsubscriber->type == cJSON_Array) {
                if (0 == strcasecmp(jsubscriber->string, "channels")) {
                    jchannel = jsubscriber->child;
                    while (jchannel) {
                        if (jchannel->type == cJSON_String) {
                            channel = ztrycalloc(sizeof(*channel));
                            if (!channel) {
                                break;
                            }
                            strncpy(channel->pattern, jchannel->valuestring, sizeof(channel->pattern) - 1);
                            list_add_tail(&channel->ele_of_channels, &subscriber->body.head_of_channels);
                            subscriber->body.channels_count++;
                        }
                        jchannel = jchannel->next;
                    }
                }
            }
            jsubscriber = jsubscriber->next;
        } while (jsubscriber);

        list_add_tail(&subscriber->ele_of_inner_subscriber, &g_jsubscriber_head);
        jsubscriber_count++;
        jcursor = jcursor->next;
    }
}

jconf_iterator_pt jconf_subscriber_get_iterator(unsigned int *count)
{
    jconf_iterator_pt iterator;

    if (count) {
        *count = jsubscriber_count;
    }

    if (0 == count) {
        return NULL;
    }

    iterator = ztrymalloc(sizeof(*iterator));
    if (!iterator) {
        return NULL;
    }

    iterator->cursor = g_jsubscriber_head.next;
    return iterator;
}

jconf_iterator_pt jconf_subscriber_get(jconf_iterator_pt iterator, jconf_subscriber_pt *jsubscribers)
{
    struct list_head *cursor;
    struct jconf_subscriber_inner *inner;

    if (!iterator || !jsubscribers) {
        return NULL;
    }

    cursor = iterator->cursor;
    if (cursor == &g_jsubscriber_head) {
        zfree(iterator);
        return NULL;
    }

    inner = container_of(cursor, struct jconf_subscriber_inner, ele_of_inner_subscriber);
    *jsubscribers  = &inner->body;
    iterator->cursor = cursor->next;
    return iterator;
}

void jconf_subscriber_free()
{
    struct list_head *pos, *n, *pos1, *n1;
    struct jconf_subscriber_channel *channel;
    struct jconf_subscriber_inner *i_subscriber;

    list_for_each_safe(pos, n, &g_jsubscriber_head) {
        i_subscriber = container_of(pos, struct jconf_subscriber_inner, ele_of_inner_subscriber);
        list_for_each_safe(pos1, n1, &i_subscriber->body.head_of_channels) {
            channel = container_of(pos1, struct jconf_subscriber_channel, ele_of_channels);
            list_del_init(pos1);
            zfree(channel);
        }
        list_del_init(pos);
        zfree(i_subscriber);
    }
}

/* -----------------------------------------------------------------------------------------------------------------------------
 * ------------------------------------------        RAWOBJ IMPLEMENTATIONs        ------------------------------------------------
 * ----------------------------------------------------------------------------------------------------------------------------- */
struct jconf_rawobj_inner
{
    struct jconf_rawobj body;
    struct list_head ele_of_inner_rawobj;
};

static struct list_head g_jrawobj_head = { &g_jrawobj_head, &g_jrawobj_head };
static unsigned int jrawobj_count = 0;

static void __jconf_rawobj_load(cJSON *entry)
{
    cJSON *jcursor, *jnext;
    struct jconf_rawobj_inner *rawobj;

    jcursor = entry->child;

    while (jcursor) {
            rawobj = ztrycalloc(sizeof(*rawobj));
            if (!rawobj) {
                break;
            }

            jnext = __jconf_load_head(jcursor, &rawobj->body.head);
            while (jnext) {
                if (jnext->type == cJSON_String) {
                    if (0 == strcasecmp(jnext->string, "initproc")) {
                        strncpy(rawobj->body.initproc, jnext->valuestring, sizeof(rawobj->body.initproc) - 1);
                    }
                }
                jnext = jnext->next;
            }

            list_add_tail(&rawobj->ele_of_inner_rawobj, &g_jrawobj_head);
            jrawobj_count++;
        jcursor = jcursor->next;
    }
}

jconf_iterator_pt jconf_rawobj_get_iterator(unsigned int *count)
{
    jconf_iterator_pt iterator;

    if (count) {
        *count = jrawobj_count;
    }

    if (0 == count) {
        return NULL;
    }

    iterator = ztrymalloc(sizeof(*iterator));
    if (!iterator) {
        return NULL;
    }

    iterator->cursor = g_jrawobj_head.next;
    return iterator;
}

jconf_iterator_pt jconf_rawobj_get(jconf_iterator_pt iterator, jconf_rawobj_pt *jrawobjs)
{
    struct list_head *cursor;
    struct jconf_rawobj_inner *inner;

    if (!iterator || !jrawobjs) {
        return NULL;
    }

    cursor = iterator->cursor;
    if (cursor == &g_jrawobj_head) {
        zfree(iterator);
        return NULL;
    }

    inner = container_of(cursor, struct jconf_rawobj_inner, ele_of_inner_rawobj);
    *jrawobjs  = &inner->body;
    iterator->cursor = cursor->next;
    return iterator;
}

void jconf_rawobj_free()
{
    struct list_head *cursor, *next;
    struct jconf_rawobj_inner *inner;

    list_for_each_safe(cursor, next, &g_jrawobj_head) {
        inner = container_of(cursor, struct jconf_rawobj_inner, ele_of_inner_rawobj);
        list_del_init(cursor);
        zfree(inner);
    }
}

/* -----------------------------------------------------------------------------------------------------------------------------
 * ------------------------------------------        EPOLLOBJ IMPLEMENTATIONs        ------------------------------------------------
 * ----------------------------------------------------------------------------------------------------------------------------- */
struct jconf_epollobj_inner
{
    struct jconf_epollobj body;
    struct list_head ele_of_inner_epollobj;
};

static struct list_head g_jepollobj_head = { &g_jepollobj_head, &g_jepollobj_head };
static unsigned int jepollobj_count = 0;

static void __jconf_epollobj_load(cJSON *entry)
{
    cJSON *jcursor, *jnext;
    struct jconf_epollobj_inner *epollobj;

    jcursor = entry->child;

    while (jcursor) {
            epollobj = ztrycalloc(sizeof(*epollobj));
            if (!epollobj) {
                break;
            }

            jnext = __jconf_load_head(jcursor, &epollobj->body.head);
            while (jnext) {
                if (jnext->type == cJSON_String) {
                    if (0 == strcasecmp(jnext->string, "poolthreads") && jnext->type == cJSON_Number) {
                        epollobj->body.poolthreads = jnext->valueint;
                    } else if (0 == strcasecmp(jnext->string, "timeout") && jnext->type == cJSON_Number) {
                        epollobj->body.timeout = jnext->valueint;
                    } else if (0 == strcasecmp(jnext->string, "timeoutproc") && jnext->type == cJSON_String) {
                        strncpy(epollobj->body.timeoutproc, jnext->valuestring, sizeof(epollobj->body.timeoutproc) - 1);
                    } else if (0 == strcasecmp(jnext->string, "rdhupproc") && jnext->type == cJSON_String) {
                        strncpy(epollobj->body.rdhupproc, jnext->valuestring, sizeof(epollobj->body.rdhupproc) - 1);
                    } else if (0 == strcasecmp(jnext->string, "errorproc") && jnext->type == cJSON_String) {
                        strncpy(epollobj->body.errorproc, jnext->valuestring, sizeof(epollobj->body.errorproc) - 1);
                    }
                }
                jnext = jnext->next;
            }

            list_add_tail(&epollobj->ele_of_inner_epollobj, &g_jepollobj_head);
            jepollobj_count++;
        jcursor = jcursor->next;
    }
}

jconf_iterator_pt jconf_epollobj_get_iterator(unsigned int *count)
{
    jconf_iterator_pt iterator;

    if (count) {
        *count = jepollobj_count;
    }

    if (0 == count) {
        return NULL;
    }

    iterator = ztrymalloc(sizeof(*iterator));
    if (!iterator) {
        return NULL;
    }

    iterator->cursor = g_jepollobj_head.next;
    return iterator;
}

jconf_iterator_pt jconf_epollobj_get(jconf_iterator_pt iterator, jconf_epollobj_pt *jepollobjs)
{
    struct list_head *cursor;
    struct jconf_epollobj_inner *inner;

    if (!iterator || !jepollobjs) {
        return NULL;
    }

    cursor = iterator->cursor;
    if (cursor == &g_jepollobj_head) {
        zfree(iterator);
        return NULL;
    }

    inner = container_of(cursor, struct jconf_epollobj_inner, ele_of_inner_epollobj);
    *jepollobjs  = &inner->body;
    iterator->cursor = cursor->next;
    return iterator;
}

void jconf_epollobj_free()
{
    struct list_head *cursor, *next;
    struct jconf_epollobj_inner *inner;

    list_for_each_safe(cursor, next, &g_jepollobj_head) {
        inner = container_of(cursor, struct jconf_epollobj_inner, ele_of_inner_epollobj);
        list_del_init(cursor);
        zfree(inner);
    }
}
