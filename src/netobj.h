#pragma once

#include "jconf.h"
#include "lobj.h"
#include "avltree.h"
#include "clist.h"
#include "nis.h"

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

void netobj_create(const jconf_net_pt jnetcfg);
