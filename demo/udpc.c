#include "lobj.h"
#include "print.h"
#include "clock.h"

#define TARGET_HOST "127.0.0.1:7773"
#define TARGET_DOMAIN   "ipc:/mnt/d/git/my/lrdp/udpsrv.domain.sock"
// gcc ../demo/udpc.c -olibudpcli.so -I ../include/ -fPIC -shared

void on_udp_recvdata(lobj_pt lop, int elements, const void **vdata, const size_t *vsize)
{

}

void on_udp_domain_recvdata(lobj_pt lop, int elements, const void **vdata, const size_t *vsize)
{

}

void udp_senddata(lobj_pt lop)
{
    lobj_pt lop_endpoint, lop_domain;
    void *vdata_send[2];
    size_t vsize_send[2];
    char output[128];
    uint64_t now;

    lrdp_generic_info("send data to : %s", TARGET_HOST);

    now = clock_monotonic();
    vdata_send[1] = output;
    vsize_send[1] = sprintf(vdata_send[1], "%llu", now) + 1;

    lop_endpoint = lobj_refer("endpoint");
    if (lop_endpoint) {
        vdata_send[0] = TARGET_HOST;
        vsize_send[0] = strlen(TARGET_HOST);
        lobj_fx_vwrite(lop_endpoint, 2, (const void **)vdata_send, vsize_send);
        lobj_derefer(lop_endpoint);
    }

    lop_domain = lobj_refer("domain");
    if (lop_domain) {
        vdata_send[0] = TARGET_DOMAIN;
        vsize_send[0] = strlen(TARGET_DOMAIN);
        lobj_fx_vwrite(lop_domain, 2, (const void **)vdata_send, vsize_send);
        lobj_derefer(lop_domain);
    }
}
