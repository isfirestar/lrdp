#include "jconf.h"
#include "ifos.h"
#include "zmalloc.h"
#include "cjson.h"

static struct jconf_entry jentry = { .preinitproc = { 0 }, .postinitproc = { 0 }, .exitproc = { 0 },
    .head = { .ctxsize = 0, .module = { 0 }, .name = { 0 } } };

/* see demo/demo.json
 * "entry" section have 3 validate sub sections, their are:
 * 1.   "module" which indicate the module name
 * 2.   "entryproc" which indicate the entry procedure name
 * 3.   "entrytimer" option, which indicate the entry timer if you need
*/
static void __jconf_entry_load(cJSON *entry)
{
    cJSON *jcursor;

    jcursor = entry->child;
    while (jcursor) {
        if (jcursor->type == cJSON_String) {
            if (0 == strcasecmp(jcursor->string, "module")) {
                strncpy(jentry.head.module, jcursor->valuestring, sizeof(jentry.head.module) - 1);
            } else if (0 == strcasecmp(jcursor->string, "preinitproc")) {
                strncpy(jentry.preinitproc, jcursor->valuestring, sizeof(jentry.preinitproc) - 1);
            } else if (0 == strcasecmp(jcursor->string, "postinitproc")) {
                strncpy(jentry.postinitproc, jcursor->valuestring, sizeof(jentry.postinitproc) - 1);
            } else if (0 == strcasecmp(jcursor->string, "exitproc")) {
                strncpy(jentry.exitproc, jcursor->valuestring, sizeof(jentry.exitproc) - 1);
            } else {
                ;
            }
        }

        if (jcursor->type == cJSON_Number) {
             if (0 == strcasecmp(jcursor->string, "contextsize") || 0 == strcasecmp(jcursor->string, "ctxsize")) {
                jentry.head.ctxsize = jcursor->valueint;
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
    struct list_head ele_of_inner_lwp;
};

static struct list_head g_jlwps_head = { &g_jlwps_head, &g_jlwps_head };
static unsigned int jlwps_count = 0;

static void __jconf_lwp_load(cJSON *entry)
{
    cJSON *jcursor, *jconf_lwp;
    struct jconf_lwp_inner *lwp;

    jcursor = entry->child;

    while (jcursor) {
        if (jcursor->type == cJSON_Object) {
            lwp = ztrymalloc(sizeof(*lwp));
            if (!lwp) {
                break;
            }

            memset(lwp, 0, sizeof(*lwp));
            strncpy(lwp->body.head.name, jcursor->string, sizeof(lwp->body.head.name) - 1);

            jconf_lwp = jcursor->child;
            while (jconf_lwp) {
                if (jconf_lwp->type == cJSON_String) {
                    if (0 == strcasecmp(jconf_lwp->string, "module")) {
                        strncpy(lwp->body.head.module, jconf_lwp->valuestring, sizeof(lwp->body.module) - 1);
                    } else if (0 == strcasecmp(jconf_lwp->string, "execproc")) {
                        strncpy(lwp->body.execproc, jconf_lwp->valuestring, sizeof(lwp->body.execproc) - 1);
                    } else {
                        ;
                    }
                } else if (jconf_lwp->type == cJSON_Number) {
                    if (0 == strcasecmp(jconf_lwp->string, "stacksize")) {
                        lwp->body.stacksize = jconf_lwp->valueint;
                    } else if (0 == strcasecmp(jconf_lwp->string, "priority")) {
                        lwp->body.priority = jconf_lwp->valueint;
                    } else if (0 == strcasecmp(jconf_lwp->string, "contextsize") || 0 == strcasecmp(jconf_lwp->string, "ctxsize")) {
                        lwp->body.head.ctxsize = jconf_lwp->valueint;
                    } else if (0 == strcasecmp(jconf_lwp->string, "affinity")) {
                        lwp->body.affinity = jconf_lwp->valueint;
                    } else {
                        ;
                    }
                }
                jconf_lwp = jconf_lwp->next;
            }

            list_add_tail(&lwp->ele_of_inner_lwp, &g_jlwps_head);
            jlwps_count++;
        }
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

    inner = container_of(cursor, struct jconf_lwp_inner, ele_of_inner_lwp);
    *jlwp  = &inner->body;
    iterator->cursor = cursor->next;
    return iterator;
}

void jconf_lwp_free()
{
    struct list_head *cursor, *next;
    struct jconf_lwp_inner *inner;

    list_for_each_safe(cursor, next, &g_jlwps_head) {
        inner = container_of(cursor, struct jconf_lwp_inner, ele_of_inner_lwp);
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
    struct list_head ele_of_inner_net;
};
static struct list_head g_jnets_head = { &g_jnets_head, &g_jnets_head };
static unsigned int jnets_count = 0;

static void __jconf_net_load(cJSON *entry)
{
    cJSON *jcursor, *jnet;
    struct jconf_net_inner *net;

    jcursor = entry->child;

    while (jcursor) {
        if (jcursor->type == cJSON_Object) {
            net = ztrycalloc(sizeof(*net));
            if (!net) {
                break;
            }
            strncpy(net->body.head.name, jcursor->string, sizeof(net->body.head.name) - 1);

            jnet = jcursor->child;
            while (jnet) {
                if (jnet->type == cJSON_String) {
                    if (0 == strcasecmp(jnet->string, "module")) {
                        strncpy(net->body.head.module, jnet->valuestring, sizeof(net->body.head.module) - 1);
                    } else if (0 == strcasecmp(jnet->string, "acceptproc")) {
                        strncpy(net->body.acceptproc, jnet->valuestring, sizeof(net->body.acceptproc) - 1);
                    } else if (0 == strcasecmp(jnet->string, "recvproc")) {
                        strncpy(net->body.recvproc, jnet->valuestring, sizeof(net->body.recvproc) - 1);
                    } else if (0 == strcasecmp(jnet->string, "closeproc")) {
                        strncpy(net->body.closeproc, jnet->valuestring, sizeof(net->body.closeproc) - 1);
                    } else if (0 == strcasecmp(jnet->string, "connectproc")) {
                        strncpy(net->body.connectproc, jnet->valuestring, sizeof(net->body.connectproc) - 1);
                    } else if (0 == strcasecmp(jnet->string, "listen")) {
                        strncpy(net->body.listen, jnet->valuestring, sizeof(net->body.listen) - 1);
                    } else if (0 == strcasecmp(jnet->string, "remote")) {
                        strncpy(net->body.remote, jnet->valuestring, sizeof(net->body.remote) - 1);
                    } else if (0 == strcasecmp(jnet->string, "local")) {
                        strncpy(net->body.local, jnet->valuestring, sizeof(net->body.local) - 1);
                    } else if (0 == strcasecmp(jnet->string, "protocol")) {
                        if (0 == strcasecmp(jnet->valuestring, "tcp")) {
                            net->body.protocol = JCFG_PROTO_TCP;
                        } else if (0 == strcasecmp(jnet->valuestring, "udp")) {
                            net->body.protocol = JCFG_PROTO_UDP;
                        } else {
                            net->body.protocol = JCFG_PROTO_ERR;
                        }
                    } else {
                        ;
                    }
                } else if (jnet->type == cJSON_Number) {
                    if (0 == strcasecmp(jcursor->string, "contextsize") || 0 == strcasecmp(jcursor->string, "ctxsize")) {
                        net->body.head.ctxsize = jcursor->valueint;
                    } else {
                        ;
                    }
                } else {
                    ;
                }
                jnet = jnet->next;
            }

            list_add_tail(&net->ele_of_inner_net, &g_jnets_head);
            jnets_count++;
        }
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

    inner = container_of(cursor, struct jconf_net_inner, ele_of_inner_net);
    *jnets  = &inner->body;
    iterator->cursor = cursor->next;
    return iterator;
}

void jconf_net_free()
{
    struct list_head *cursor, *next;
    struct jconf_net_inner *inner;

    list_for_each_safe(cursor, next, &g_jnets_head) {
        inner = container_of(cursor, struct jconf_net_inner, ele_of_inner_net);
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
    cJSON *jcursor, *jtty;
    struct jconf_tty_inner *tty;

    jcursor = entry->child;

    while (jcursor) {
        if (jcursor->type == cJSON_Object) {
            tty = ztrycalloc(sizeof(*tty));
            if (!tty) {
                break;
            }
            strncpy(tty->body.head.name, jcursor->string, sizeof(tty->body.name) - 1);

            jtty = jcursor->child;
            while (jtty) {
                if (jtty->type == cJSON_String) {
                    if (0 == strcasecmp(jtty->string, "module")) {
                        strncpy(tty->body.head.module, jtty->valuestring, sizeof(tty->body.module) - 1);
                    } else if (0 == strcasecmp(jtty->string, "device")) {
                        strncpy(tty->body.device, jtty->valuestring, sizeof(tty->body.device) - 1);
                    } else if (0 == strcasecmp(jtty->string, "recvproc")) {
                        strncpy(tty->body.recvproc, jtty->valuestring, sizeof(tty->body.recvproc) - 1);
                    } else if (0 == strcasecmp(jtty->string, "parity")) {
                        strncpy(tty->body.parity, jtty->valuestring, sizeof(tty->body.parity) - 1);
                    } else if (0 == strcasecmp(jtty->string, "flowcontrol")) {
                        strncpy(tty->body.flowcontrol, jtty->valuestring, sizeof(tty->body.flowcontrol) - 1);
                    } else {
                        ;
                    }
                } else if (jtty->type == cJSON_Number) {
                    if (0 == strcasecmp(jtty->string, "baudrate")) {
                        tty->body.baudrate = jtty->valueint;
                    } else if (0 == strcasecmp(jtty->string, "databits")) {
                        tty->body.databits = jtty->valueint;
                    } else if (0 == strcasecmp(jtty->string, "stopbits")) {
                        tty->body.stopbits = jtty->valueint;
                    } else if (0 == strcasecmp(jtty->string, "contextsize") || 0 == strcasecmp(jtty->string, "ctxsize")) {
                        tty->body.head.ctxsize = jtty->valueint;
                    } else {
                        ;
                    }
                } else {
                    ;
                }

                jtty = jtty->next;
            }

            list_add_tail(&tty->ele_of_inner_tty, &g_jtty_head);
            jtty_count++;
        }

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
    cJSON *jcursor, *jtimer;
    struct jconf_timer_inner *timer;

    jcursor = entry->child;

    while (jcursor) {
        if (jcursor->type == cJSON_Object) {
            timer = ztrycalloc(sizeof(*timer));
            if (!timer) {
                break;
            }
            strncpy(timer->body.head.name, jcursor->string, sizeof(timer->body.head.name) - 1);

            jtimer = jcursor->child;
            while (jtimer) {
                if (jtimer->type == cJSON_String) {
                    if (0 == strcasecmp(jtimer->string, "module")) {
                        strncpy(timer->body.head.module, jtimer->valuestring, sizeof(timer->body.head.module) - 1);
                    } else if (0 == strcasecmp(jtimer->string, "timerproc")) {
                        strncpy(timer->body.timerproc, jtimer->valuestring, sizeof(timer->body.timerproc) - 1);
                    } else {
                        ;
                    }
                } else if (jtimer->type == cJSON_Number) {
                    if (0 == strcasecmp(jtimer->string, "interval")) {
                        timer->body.interval = jtimer->valueint;
                    } else if (0 == strcasecmp(jtimer->string, "contextsize") || 0 == strcasecmp(jtimer->string, "ctxsize")) {
                        timer->body.head.ctxsize = jtimer->valueint;
                    } else {
                        ;
                    }
                } else {
                    ;
                }

                jtimer = jtimer->next;
            }

            list_add_tail(&timer->ele_of_inner_timer, &g_jtimer_head);
            jtimer_count++;
        }

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
    cJSON *jcursor, *jredis_server;
    struct jconf_redis_server_inner *redis_server;

    jcursor = entry->child;

    while (jcursor) {
        if (jcursor->type == cJSON_Object) {
            redis_server = ztrycalloc(sizeof(*redis_server));
            if (!redis_server) {
                break;
            }
            strncpy(redis_server->body.head.name, jcursor->string, sizeof(redis_server->body.head.name) - 1);

            jredis_server = jcursor->child;
            while (jredis_server) {
                if (jredis_server->type == cJSON_String) {
                    if (0 == strcasecmp(jredis_server->string, "host")) {
                        strncpy(redis_server->body.host, jredis_server->valuestring, sizeof(redis_server->body.host) - 1);
                    }
                } else if (jredis_server->type == cJSON_Number) {
                     if (0 == strcasecmp(jredis_server->string, "contextsize") || 0 == strcasecmp(jredis_server->string, "ctxsize")) {
                        redis_server->body.head.ctxsize = jredis_server->valueint;
                    }
                } else {
                    ;
                }

                jredis_server = jredis_server->next;
            }

            list_add_tail(&redis_server->ele_of_inner_redis_server, &g_jredis_server_head);
            jredis_server_count++;
        }

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
        if (jcursor->type == cJSON_Object) {
            subscriber = ztrycalloc(sizeof(*subscriber));
            if (!subscriber) {
                break;
            }
            INIT_LIST_HEAD(&subscriber->body.head_of_channels);
            strncpy(subscriber->body.head.name, jcursor->string, sizeof(subscriber->body.head.name) - 1);

            jsubscriber = jcursor->child;
            while (jsubscriber) {
                if (jsubscriber->type == cJSON_String) {
                    if (0 == strcasecmp(jsubscriber->string, "redis-server")) {
                        strncpy(subscriber->body.redisserver, jsubscriber->valuestring, sizeof(subscriber->body.redisserver) - 1);
                    } else if (0 == strcasecmp(jsubscriber->string, "execproc")) {
                        strncpy(subscriber->body.execproc, jsubscriber->valuestring, sizeof(subscriber->body.execproc) - 1);
                    } else if (0 == strcasecmp(jsubscriber->string, "module")) {
                        strncpy(subscriber->body.head.module, jsubscriber->valuestring, sizeof(subscriber->body.head.module) - 1);
                    } else {
                        ;
                    }
                } else if (jsubscriber->type == cJSON_Number) {
                     if (0 == strcasecmp(jsubscriber->string, "contextsize") || 0 == strcasecmp(jsubscriber->string, "ctxsize")) {
                        subscriber->body.head.ctxsize = jsubscriber->valueint;
                    }
                } else if (jsubscriber->type == cJSON_Array) {
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
                            } else {
                                ;
                            }

                            jchannel = jchannel->next;
                        }
                    }
                } else {
                    ;
                }

                jsubscriber = jsubscriber->next;
            }

            list_add_tail(&subscriber->ele_of_inner_subscriber, &g_jsubscriber_head);
            jsubscriber_count++;
        }

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
