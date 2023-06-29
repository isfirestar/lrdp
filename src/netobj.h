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

struct netobj
{
    void (*recvproc)(lobj_pt lop, const void *data, unsigned int size);
    void (*closeproc)(lobj_pt lop);
    void (*acceptproc)(lobj_pt lop);
    void (*connectproc)(lobj_pt lop);
    int protocol;
    HLNK link;
    struct endpoint listen;
    struct endpoint remote;
    struct endpoint local;
};

extern void netobj_create(const jconf_net_pt jnetcfg);
extern nsp_status_t netobj_parse_endpoint(const char *epstr, struct endpoint *epo);
