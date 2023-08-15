#pragma once

#include "jconf.h"
#include "lobj.h"
#include "ae.h"

extern lobj_pt aeobj_create(const char *name, const char *module, int setsize);
extern lobj_pt aeobj_jcreate(const jconf_aeobj_pt jaecfg);
extern aeEventLoop *aeobj_getel(lobj_pt lop);
extern void aeobj_run(lobj_pt lop);
extern void aeobj_onecycle(lobj_pt lop);
