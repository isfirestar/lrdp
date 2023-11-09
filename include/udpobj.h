#pragma once

#include "lobj.h"
#include "netobj.h"

extern lobj_pt udpobj_create(const char *name, const char *module, const endpoint_string_pt local, struct lobj_fx *ifx, size_t ctxsize);
