#pragma once

#include "jconf.h"
#include "lobj.h"
#include "ae.h"

extern lobj_pt mloop_create(const jconf_entry_pt jentry);
extern int mloop_pre_init(lobj_pt lop, int argc, char **argv);
extern int mloop_post_init(lobj_pt lop, int argc, char **argv);
