#pragma once

#include "threading.h"
#include "jconf.h"

/* lwpmgr - Light Weight Process Manager
 * this component is responsible for managing the light weight processes(threads) in the system
 */

struct lwp_item
{
    void *handle;
    char module[64];
    void *(*execproc)(void *);
    lwp_t obj;
    unsigned int stacksize;
    unsigned int priority;
    unsigned int contextsize;
    unsigned int affinity;
    unsigned char *context;
};

extern nsp_status_t lwp_spawn(const jconf_lwp_pt jlwpcfg);   
