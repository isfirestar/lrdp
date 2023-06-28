#include "jconf.h"
#include "ifos.h"
#include "zmalloc.h"
#include "cjson.h"
#include "clist.h"

static struct jconf_entry jentry = { .module = { 0 }, .preinitproc = { 0 }, .postinitproc = { 0 },
    .exitproc = { 0 }, .timerproc = { 0 }, .interval = 1000, .ctxsize = 0 };

/* see demo/demo.json
 * "entry" section have 3 validate sub sections, their are:
 * 1.   "module" which indicate the module name
 * 2.   "entryproc" which indicate the entry procedure name
 * 3.   "entrytimer" option, which indicate the entry timer if you need
*/
static void __jconf_entry_load(cJSON *entry)
{
    cJSON *jcursor, *jtimer;

    jcursor = entry->child;
    while (jcursor) {
        if (jcursor->type == cJSON_String) {
            if (0 == strcasecmp(jcursor->string, "module")) {
                strncpy(jentry.module, jcursor->valuestring, sizeof(jentry.module) - 1);
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
                jentry.ctxsize = jcursor->valueint;
            }
        }

        if (jcursor->type == cJSON_Object && 0 == strcasecmp(jcursor->string, "entrytimer")) {
            jtimer = jcursor->child;
            while (jtimer) {
                if (jtimer->type == cJSON_String && 0 == strcasecmp(jtimer->string, "timerproc")) {
                    strncpy(jentry.timerproc, jtimer->valuestring, sizeof(jentry.timerproc) - 1);
                } else if (jtimer->type == cJSON_Number && 0 == strcasecmp(jtimer->string, "interval")) {
                    jentry.interval = jtimer->valueint;
                } else {
                    ;
                }
                jtimer = jtimer->next;
            }
        }

        jcursor = jcursor->next;
    }
}

jconf_entry_pt jconf_entry_get(void)
{
    return &jentry;
}

static void __jconf_lwp_load(cJSON *entry);
static void __jconf_net_load(cJSON *entry);

nsp_status_t jconf_initial_load(const char *jsonfile)
{
    file_descriptor_t fd;
    nsp_status_t status;
    int64_t filesize;
    char *jsondata;
    int nread;
    cJSON *root, *cursor;

    fd = -1;
    jsondata = NULL;
    root = NULL;

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
        root = cJSON_Parse(jsondata);
        if (!root) {
            status = posix__makeerror(EINVAL);
            break;
        }

        // travesal the json section
        cursor = root->child;
        while (cursor) {
            if (cursor->type == cJSON_Object && 0 == strcasecmp(cursor->string, "mainloop")) {
                __jconf_entry_load(cursor);
            } else if (cursor->type == cJSON_Object && 0 == strcasecmp(cursor->string, "lwps")) {
                __jconf_lwp_load(cursor);
            } else if (cursor->type == cJSON_Object && 0 == strcasecmp(cursor->string, "nets")) {
                __jconf_net_load(cursor);
            } else {
                ;
            }
            cursor = cursor->next;
        }

    } while (0);

    // release resource no matter success or failed
    if (fd != -1) {
        ifos_file_close(fd);
    }

    if (jsondata) {
        zfree(jsondata);
    }

    if (root) {
        cJSON_Delete(root);
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

static struct list_head jlwps = { &jlwps, &jlwps };
static unsigned int jlwps_count = 0;

/* load json section "lwps", it contant many of subsection.
 * name of every section indicate the name of user thread and every object have below fields:
 *  1. "module", string, require, specify the module of handler function
 *  2. "execproc", string, require, specify the handler function name
 *  3. "stacksize", integer, option, specify the thread stack size in byts
 *  4. "priority", integer, option, specify the nice value of the thread
 *  5. "contextsize", integer, option, specify the thread context size in bytes
 *  6. "affinity", integer, option, specify the affinity of this thread, using bit or for every CPU ids
 * we load all sub sections in parent section "lwps", create many object for "struct jconf_lwp_inner", and link them to the list "jlwps"
 * meanwhile, we shall increase the count of lwp objects "jlwps_count"
 */
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
            strncpy(lwp->body.name, jcursor->string, sizeof(lwp->body.name) - 1);

            jconf_lwp = jcursor->child;
            while (jconf_lwp) {
                if (jconf_lwp->type == cJSON_String) {
                    if (0 == strcasecmp(jconf_lwp->string, "module")) {
                        strncpy(lwp->body.module, jconf_lwp->valuestring, sizeof(lwp->body.module) - 1);
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
                        lwp->body.contextsize = jconf_lwp->valueint;
                    } else if (0 == strcasecmp(jconf_lwp->string, "affinity")) {
                        lwp->body.affinity = jconf_lwp->valueint;
                    } else {
                        ;
                    }
                }
                jconf_lwp = jconf_lwp->next;
            }

            list_add_tail(&lwp->ele_of_inner_lwp, &jlwps);
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

    iterator->cursor = jlwps.next;
    return iterator;
}

jconf_iterator_pt jconf_lwp_get(jconf_iterator_pt iterator, jconf_lwp_pt *lwp)
{
    struct list_head *cursor;
    struct jconf_lwp_inner *inner;

    if (!iterator || !lwp) {
        return NULL;
    }

    cursor = iterator->cursor;
    if (cursor == &jlwps) {
        zfree(iterator);
        return NULL;
    }

    inner = container_of(cursor, struct jconf_lwp_inner, ele_of_inner_lwp);
    *lwp  = &inner->body;
    iterator->cursor = cursor->next;
    return iterator;
}

/* ----------------------------------------------------------------------------------------------------------------------------- 
 * ------------------------------------------        NET IMPLEMENTATIONs        ------------------------------------------------ 
 * ----------------------------------------------------------------------------------------------------------------------------- */
struct jconf_net_inner
{
    struct jconf_net body;
    struct list_head ele_of_inner_net;
};
static struct list_head jnets = { &jnets, &jnets };
static unsigned int jnets_count = 0;

/* load json section "nets", it contant many of subsection.
 * name of every section indicate the name of user thread and every object have below fields:
 *  1. "module", string, require, specify the module of handler function
 *  2. "acceptproc", string, option, specify the handler function name
 *  3. "recvproc", string, require, specify the handler function name
 *  4. "protocol", string, option, specify the protocol of this socket type
 *  5. "listen", string, option, specify the listen endpoint of this socket
 *  6. "remote", string, option, repel to "listen", specify the remote endpoint which connect or send to
 *  7. "local", string, option, repel to "listen", specify the local endpoint which bind on
 * we load all sub sections in parent section "nets", create many object for "struct jconf_net_inner", and link them to the list "jnets"
 * meanwhile, we shall increase the count of net objects "jnets_count"
 */
static void __jconf_net_load(cJSON *entry)
{
    cJSON *cursor, *jnet;
    struct jconf_net_inner *net;

    cursor = entry->child;

    while (cursor) {
        if (cursor->type == cJSON_Object) {
            net = ztrycalloc(sizeof(*net));
            if (!net) {
                break;
            }
            strncpy(net->body.name, cursor->string, sizeof(net->body.name) - 1);

            jnet = cursor->child;
            while (jnet) {
                if (jnet->type == cJSON_String) {
                    if (0 == strcasecmp(jnet->string, "module")) {
                        strncpy(net->body.module, jnet->valuestring, sizeof(net->body.module) - 1);
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
                }
                jnet = jnet->next;
            }

            list_add_tail(&net->ele_of_inner_net, &jnets);
            jnets_count++;
        }
        cursor = cursor->next;
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

    iterator->cursor = jnets.next;
    return iterator;
}

jconf_iterator_pt jconf_net_get(jconf_iterator_pt iterator, jconf_net_pt *net)
{
    struct list_head *cursor;
    struct jconf_net_inner *inner;

    if (!iterator || !net) {
        return NULL;
    }

    cursor = iterator->cursor;
    if (cursor == &jnets) {
        zfree(iterator);
        return NULL;
    }

    inner = container_of(cursor, struct jconf_net_inner, ele_of_inner_net);
    *net  = &inner->body;
    iterator->cursor = cursor->next;
    return iterator;
}
