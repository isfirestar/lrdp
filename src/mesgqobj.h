#pragma once

#include "lobj.h"
#include "jconf.h"
#include "ae.h"

extern lobj_pt mesgqobj_create(const jconf_mesgqobj_pt jmesgq, aeEventLoop *el);
