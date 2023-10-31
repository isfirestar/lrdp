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
        } else if (0 == strcasecmp(jnext->string, "touchproc") && jnext->type == cJSON_String) {
            strncpy(cfhead->touchproc, jnext->valuestring, sizeof(cfhead->touchproc) - 1);
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
        } else if (0 == strcasecmp(jnext->string, "recvdataproc") && jnext->type == cJSON_String) {
            strncpy(cfhead->recvdataproc, jnext->valuestring, sizeof(cfhead->recvdataproc) - 1);
        } else if (0 == strcasecmp(jnext->string, "vrecvdataproc") && jnext->type == cJSON_String) {
            strncpy(cfhead->vrecvdataproc, jnext->valuestring, sizeof(cfhead->vrecvdataproc) - 1);
        } else if (0 == strcasecmp(jnext->string, "rawinvokeproc") && jnext->type == cJSON_String) {
            strncpy(cfhead->rawinvokeproc, jnext->valuestring, sizeof(cfhead->rawinvokeproc) - 1);
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

static void __jconf_aeobj_load(cJSON *entry);
static void __jconf_lwp_load(cJSON *entry);
static void __jconf_tty_load(cJSON *entry);
static void __jconf_timer_load(cJSON *entry);
static void __jconf_redis_server_load(cJSON *entry);
static void __jconf_subscriber_load(cJSON *entry);
static void __jconf_rawobj_load(cJSON *entry);
static void __jconf_mesgqobj_load(cJSON *entry);
static void __jconf_udpobj_load(cJSON *entry);

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
            } else if (jcursor->type == cJSON_Object && (0 == strcasecmp(jcursor->string, "lwps") || 0 == strcasecmp(jcursor->string, "threads"))) {
                __jconf_lwp_load(jcursor);
            } else if (jcursor->type == cJSON_Object && (0 == strcasecmp(jcursor->string, "aeo") || 0 == strcasecmp(jcursor->string, "aeobj"))) {
                __jconf_aeobj_load(jcursor);
            } else if (jcursor->type == cJSON_Object && 0 == strcasecmp(jcursor->string, "ttys")) {
                __jconf_tty_load(jcursor);
            } else if (jcursor->type == cJSON_Object && (0 == strcasecmp(jcursor->string, "timers") || 0 == strcasecmp(jcursor->string, "timer"))) {
                __jconf_timer_load(jcursor);
            } else if (jcursor->type == cJSON_Object && 0 == strcasecmp(jcursor->string, "redis-server")) {
                __jconf_redis_server_load(jcursor);
            } else if (jcursor->type == cJSON_Object && 0 == strcasecmp(jcursor->string, "subscriber")) {
                __jconf_subscriber_load(jcursor);
            } else if (jcursor->type == cJSON_Object && 0 == strcasecmp(jcursor->string, "imports")) {
                __jconf_rawobj_load(jcursor);
            } else if (jcursor->type == cJSON_Object && 0 == strcasecmp(jcursor->string, "mesgq")) {
                __jconf_mesgqobj_load(jcursor);
            } else if (jcursor->type == cJSON_Object && 0 == strcasecmp(jcursor->string, "udp")) {
                __jconf_udpobj_load(jcursor);
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
                } else if (0 == strcasecmp(jnext->string, "aeobj") || 0 == strcasecmp(jnext->string, "aeo")) {
                    strncpy(lwp->body.aeo, jnext->valuestring, sizeof(lwp->body.aeo) - 1);
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
                } else if (0 == strcasecmp(jnext->string, "aeo")) {
                    strncpy(timer->body.aeo, jnext->valuestring, sizeof(timer->body.aeo) - 1);
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
                } else if (0 == strcasecmp(jnext->string, "aeo")) {
                    strncpy(redis_server->body.aeo, jnext->valuestring, sizeof(redis_server->body.aeo) - 1);
                }
            }

            if (jnext->type == cJSON_Number) {
                if (0 == strcasecmp(jnext->string, "na")) {
                    redis_server->body.na = jnext->valueint;
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
 * ------------------------------------------        MESGQOBJ IMPLEMENTATIONs        ------------------------------------------------
 * ----------------------------------------------------------------------------------------------------------------------------- */
struct jconf_mesgqobj_inner
{
    struct jconf_mesgqobj body;
    struct list_head ele_of_inner_mesgqobj;
};

static struct list_head g_jmesgqobj_head = { &g_jmesgqobj_head, &g_jmesgqobj_head };
static unsigned int jmesgqobj_count = 0;

static void __jconf_mesgqobj_load(cJSON *entry)
{
    cJSON *jcursor, *jnext;
    struct jconf_mesgqobj_inner *mesgqobj;

    jcursor = entry->child;

    while (jcursor) {
            mesgqobj = ztrycalloc(sizeof(*mesgqobj));
            if (!mesgqobj) {
                break;
            }

            jnext = __jconf_load_head(jcursor, &mesgqobj->body.head);
            while (jnext) {
                if (0 == strcasecmp(jnext->string, "maxmsg") && jnext->type == cJSON_Number) {
                    mesgqobj->body.maxmsg = jnext->valueint;
                } else if (0 == strcasecmp(jnext->string, "msgsize") && jnext->type == cJSON_Number) {
                    mesgqobj->body.msgsize = jnext->valueint;
                } else if (0 == strcasecmp(jnext->string, "method") && jnext->type == cJSON_Number) {
                    mesgqobj->body.method = jnext->valueint;
                } else if (0 == strcasecmp(jnext->string, "na") && jnext->type == cJSON_Number) {
                    mesgqobj->body.na = jnext->valueint;
                } else if (0 == strcasecmp(jnext->string, "mqname") && jnext->type == cJSON_String) {
                    strncpy(mesgqobj->body.mqname, jnext->valuestring, sizeof(mesgqobj->body.mqname) - 1);
                } else {
                    ;
                }
                jnext = jnext->next;
            }

            list_add_tail(&mesgqobj->ele_of_inner_mesgqobj, &g_jmesgqobj_head);
            jmesgqobj_count++;
        jcursor = jcursor->next;
    }
}

jconf_iterator_pt jconf_mesgqobj_get_iterator(unsigned int *count)
{
    jconf_iterator_pt iterator;

    if (count) {
        *count = jmesgqobj_count;
    }

    if (0 == count) {
        return NULL;
    }

    iterator = ztrymalloc(sizeof(*iterator));
    if (!iterator) {
        return NULL;
    }

    iterator->cursor = g_jmesgqobj_head.next;
    return iterator;
}

jconf_iterator_pt jconf_mesgqobj_get(jconf_iterator_pt iterator, jconf_mesgqobj_pt *jmesgqobjs)
{
    struct list_head *cursor;
    struct jconf_mesgqobj_inner *inner;

    if (!iterator || !jmesgqobjs) {
        return NULL;
    }

    cursor = iterator->cursor;
    if (cursor == &g_jmesgqobj_head) {
        zfree(iterator);
        return NULL;
    }

    inner = container_of(cursor, struct jconf_mesgqobj_inner, ele_of_inner_mesgqobj);
    *jmesgqobjs  = &inner->body;
    iterator->cursor = cursor->next;
    return iterator;
}

void jconf_mesgqobj_free()
{
    struct list_head *cursor, *next;
    struct jconf_mesgqobj_inner *inner;

    list_for_each_safe(cursor, next, &g_jmesgqobj_head) {
        inner = container_of(cursor, struct jconf_mesgqobj_inner, ele_of_inner_mesgqobj);
        list_del_init(cursor);
        zfree(inner);
    }
}

/* -----------------------------------------------------------------------------------------------------------------------------
 * ------------------------------------------        AEOBJ IMPLEMENTATIONs        ------------------------------------------------
 * ----------------------------------------------------------------------------------------------------------------------------- */
struct jconf_aeobj_inner
{
    struct jconf_aeobj body;
    struct list_head ele_of_inner_aeobj;
};

static struct list_head g_jaeobj_head = { &g_jaeobj_head, &g_jaeobj_head };
static unsigned int jaeobj_count = 0;

static void __jconf_aeobj_load(cJSON *entry)
{
    cJSON *jcursor, *jnext;
    struct jconf_aeobj_inner *aeobj;

    jcursor = entry->child;

    while (jcursor) {
            aeobj = ztrycalloc(sizeof(*aeobj));
            if (!aeobj) {
                break;
            }

            jnext = __jconf_load_head(jcursor, &aeobj->body.head);
            while (jnext) {
                if (0 == strcasecmp(jnext->string, "setsize") && jnext->type == cJSON_Number) {
                    aeobj->body.setsize = jnext->valueint;
                } else {
                    ;
                }
                jnext = jnext->next;
            }

            list_add_tail(&aeobj->ele_of_inner_aeobj, &g_jaeobj_head);
            jaeobj_count++;
        jcursor = jcursor->next;
    }
}

jconf_iterator_pt jconf_aeobj_get_iterator(unsigned int *count)
{
    jconf_iterator_pt iterator;

    if (count) {
        *count = jaeobj_count;
    }

    if (0 == count) {
        return NULL;
    }

    iterator = ztrymalloc(sizeof(*iterator));
    if (!iterator) {
        return NULL;
    }

    iterator->cursor = g_jaeobj_head.next;
    return iterator;
}

jconf_iterator_pt jconf_aeobj_get(jconf_iterator_pt iterator, jconf_aeobj_pt *jaeobjs)
{
    struct list_head *cursor;
    struct jconf_aeobj_inner *inner;

    if (!iterator || !jaeobjs) {
        return NULL;
    }

    cursor = iterator->cursor;
    if (cursor == &g_jaeobj_head) {
        zfree(iterator);
        return NULL;
    }

    inner = container_of(cursor, struct jconf_aeobj_inner, ele_of_inner_aeobj);
    *jaeobjs  = &inner->body;
    iterator->cursor = cursor->next;
    return iterator;
}

void jconf_aeobj_free()
{
    struct list_head *cursor, *next;
    struct jconf_aeobj_inner *inner;

    list_for_each_safe(cursor, next, &g_jaeobj_head) {
        inner = container_of(cursor, struct jconf_aeobj_inner, ele_of_inner_aeobj);
        list_del_init(cursor);
        zfree(inner);
    }
}

/* -----------------------------------------------------------------------------------------------------------------------------
 * ------------------------------------------        UDPBJ IMPLEMENTATIONs        ------------------------------------------------
 * ----------------------------------------------------------------------------------------------------------------------------- */
struct jconf_udpobj_inner
{
    struct jconf_udpobj body;
    struct list_head ele_of_inner_udpobj;
};

static struct list_head g_judpobj_head = { &g_judpobj_head, &g_judpobj_head };
static unsigned int judpobj_count = 0;

static void __jconf_udpobj_load(cJSON *entry)
{
    cJSON *jcursor, *jnext;
    struct jconf_udpobj_inner *udpobj;

    jcursor = entry->child;

    while (jcursor) {
            udpobj = ztrycalloc(sizeof(*udpobj));
            if (!udpobj) {
                break;
            }

            jnext = __jconf_load_head(jcursor, &udpobj->body.head);
            while (jnext) {
                if (0 == strcasecmp(jnext->string, "setsize") && jnext->type == cJSON_Number) {
                    udpobj->body.setsize = jnext->valueint;
                } if ( (0 == strcasecmp(jnext->string, "bindon") || 0 == strcasecmp(jnext->string, "local") ) && jnext->type == cJSON_String) {
                    strncpy(udpobj->body.local, jnext->valuestring, sizeof(udpobj->body.local) - 1);
                } else {
                    ;
                }
                jnext = jnext->next;
            }

            list_add_tail(&udpobj->ele_of_inner_udpobj, &g_judpobj_head);
            judpobj_count++;
        jcursor = jcursor->next;
    }
}

jconf_iterator_pt jconf_udpobj_get_iterator(unsigned int *count)
{
    jconf_iterator_pt iterator;

    if (count) {
        *count = judpobj_count;
    }

    if (0 == count) {
        return NULL;
    }

    iterator = ztrymalloc(sizeof(*iterator));
    if (!iterator) {
        return NULL;
    }

    iterator->cursor = g_judpobj_head.next;
    return iterator;
}

jconf_iterator_pt jconf_udpobj_get(jconf_iterator_pt iterator, jconf_udpobj_pt *judpobjs)
{
    struct list_head *cursor;
    struct jconf_udpobj_inner *inner;

    if (!iterator || !judpobjs) {
        return NULL;
    }

    cursor = iterator->cursor;
    if (cursor == &g_judpobj_head) {
        zfree(iterator);
        return NULL;
    }

    inner = container_of(cursor, struct jconf_udpobj_inner, ele_of_inner_udpobj);
    *judpobjs  = &inner->body;
    iterator->cursor = cursor->next;
    return iterator;
}

void jconf_udpobj_free()
{
    struct list_head *cursor, *next;
    struct jconf_udpobj_inner *inner;

    list_for_each_safe(cursor, next, &g_judpobj_head) {
        inner = container_of(cursor, struct jconf_udpobj_inner, ele_of_inner_udpobj);
        list_del_init(cursor);
        zfree(inner);
    }
}
