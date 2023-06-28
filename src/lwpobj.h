#pragma once

#include "jconf.h"

/* lwpmgr - Light Weight Process Manager
 * this component is responsible for managing the light weight processes(threads) in the system
 */
extern nsp_status_t lwp_spawn(const jconf_lwp_pt jlwpcfg);
