#include "jconf.h"
#include "ifos.h"
#include "zmalloc.h"
#include "cjson.h"
#include "clist.h"

static struct jconf_entry jentry = { .module = { 0 }, .entryproc = { 0 },
    .exitproc = { 0 }, .timerproc = { 0 }, .timer_interval_millisecond = 1000, .timer_context_size = 0 };

struct jconf_lwp_inner
{
    struct jconf_lwp body;
    struct list_head ele_of_inner_lwp;
};

struct jconf_lwp_iterator
{
    struct list_head *cursor;
};
static struct list_head jlwps = { &jlwps, &jlwps };
static unsigned int jlwps_count = 0;

/* see demo/demo.json
 * "entry" section have 3 validate sub sections, their are:
 * 1.   "module" which indicate the module name
 * 2.   "entryproc" which indicate the entry procedure name
 * 3.   "entrytimer" option, which indicate the entry timer if you need
*/
static void __jconf_entry_load(cJSON *entry)
{
    cJSON *cursor, *timer;

    cursor = entry->child;
    while (cursor) {
        if (cursor->type == cJSON_String) {
            if (0 == strcasecmp(cursor->string, "module")) {
                strncpy(jentry.module, cursor->valuestring, sizeof(jentry.module) - 1);
            } else if (0 == strcasecmp(cursor->string, "entryproc")) {
                strncpy(jentry.entryproc, cursor->valuestring, sizeof(jentry.entryproc) - 1);
            } else if (0 == strcasecmp(cursor->string, "exitproc")) {
                strncpy(jentry.exitproc, cursor->valuestring, sizeof(jentry.exitproc) - 1);
            } else {
                ;
            }
        }

        if (cursor->type == cJSON_Object) {
            if (0 == strcasecmp(cursor->string, "entrytimer")) {
                timer = cursor->child;
                while (timer) {
                    if (timer->type == cJSON_String) {
                        if (0 == strcasecmp(timer->string, "timerproc")) {
                            strncpy(jentry.timerproc, timer->valuestring, sizeof(jentry.timerproc) - 1);
                        }
                    } else if (timer->type == cJSON_Number) {
                        if (0 == strcasecmp(timer->string, "interval")) {
                            jentry.timer_interval_millisecond = timer->valueint;
                        }
                        else if (0 == strcasecmp(timer->string, "contextsize")) {
                            jentry.timer_context_size = timer->valueint;
                        } else {
                            ;
                        }
                    }
                    timer = timer->next;
                }
            }
        }

        cursor = cursor->next;
    }
}

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
    cJSON *cursor, *jconf_lwp;
    struct jconf_lwp_inner *lwp;

    cursor = entry->child;

    while (cursor) {
        if (cursor->type == cJSON_Object) {
            lwp = ztrymalloc(sizeof(*lwp));
            if (!lwp) {
                break;
            }

            memset(lwp, 0, sizeof(*lwp));
            strncpy(lwp->body.name, cursor->string, sizeof(lwp->body.name) - 1);

            jconf_lwp = cursor->child;
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
                    } else if (0 == strcasecmp(jconf_lwp->string, "contextsize")) {
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
        cursor = cursor->next;
    }
}

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
            if (cursor->type == cJSON_Object && 0 == strcasecmp(cursor->string, "entry")) {
                __jconf_entry_load(cursor);
            } else if (cursor->type == cJSON_Object && 0 == strcasecmp(cursor->string, "lwps")) {
                __jconf_lwp_load(cursor);
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

jconf_entry_pt jconf_entry_get(void)
{
    return &jentry;
}

/* the LWP query function implementation */
jconf_lwp_iterator_pt jconf_lwp_get_iterator(unsigned int *count)
{
    jconf_lwp_iterator_pt iterator;

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

void jconf_release_iterator(jconf_lwp_iterator_pt iterator)
{
    if (iterator) {
        zfree(iterator);
    }
}

jconf_lwp_iterator_pt jconf_lwp_get(jconf_lwp_iterator_pt iterator, jconf_lwp_pt *lwp)
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
