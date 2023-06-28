#pragma once

#include "jconf.h"
#include "lobj.h"

extern lobj_pt mloop_create(const jconf_entry_pt jentry);
extern void mloop_preinit(lobj_pt lop, int argc, char **argv);
extern int mloop_run(lobj_pt lop);
