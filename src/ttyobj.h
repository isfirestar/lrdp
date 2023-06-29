#pragma once

#include "jconf.h"
#include "lobj.h"
#include "ae.h"

extern lobj_pt ttyobj_create(const jconf_tty_pt jtty);
extern void ttyobj_add_file(struct aeEventLoop *eventLoop, lobj_pt lop);
