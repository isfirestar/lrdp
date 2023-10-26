#include "lobj.h"
#include "print.h"

// gcc ../demo/udps.c -olibudpsrv.so -I ../include/ -fPIC -shared -g3

void on_udp_recvdata(lobj_pt lop, int elements, const void **vdata, const size_t *vsize)
{
    lrdp_generic_info("recv data from : %s, [%s]", vdata[0], vdata[1]);
}

void on_udp_domain_recvdata(lobj_pt lop, int elements, const void **vdata, const size_t *vsize)
{
    lrdp_generic_info("recv data from : %s, [%s]", vdata[0], vdata[1]);
}
