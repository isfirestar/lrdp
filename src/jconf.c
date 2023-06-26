#include "jconf.h"
#include "ifos.h"
#include "zmalloc.h"
#include "cjson.h"

static struct jconf_entry jentry = { 0 };

/* see demo/demo.json
 * "entry" section have 3 validate sub sections, their are:
 * 1.   "module" which indicate the module name
 * 2.   "entryproc" which indicate the entry procedure name
 * 3.   "entrytimer" option, which indicate the entry timer if you need
*/
static void __jconf_init_entry(cJSON *entry)
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
                __jconf_init_entry(cursor);
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
