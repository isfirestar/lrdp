#pragma once

#include "jconf.h"
#include "lobj.h"
#include "nisdef.h"

struct endpoint
{
    char ip[16];
    unsigned int inet;
    unsigned short port;
};

extern nsp_status_t netobj_parse_endpoint(const char *epstr, struct endpoint *epo);
