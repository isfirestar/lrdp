#pragma once

#include "jconf.h"
#include "lobj.h"
#include "ae.h"

extern lobj_pt ttyobj_create(const jconf_tty_pt jtty, struct aeEventLoop *el);
