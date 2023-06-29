#pragma once

#include "jconf.h"
#include "lobj.h"
#include "ae.h"

extern lobj_pt mloop_create(const jconf_entry_pt jentry);
extern int mloop_pre_init(lobj_pt lop, int argc, char **argv);
extern void mloop_post_init(lobj_pt lop);
extern void mloop_add_timer(aeEventLoop *el, lobj_pt lop);
