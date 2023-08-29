#include <time.h>
#include <stdint.h>

#include "monotonic.h"
#include "lobj.h"
#include "zmalloc.h"
#include "print.h"

int dep_pre_init(lobj_pt lop, int argc, char **argv)
{
    lrdp_generic_info("dep_pre_init");
    return 0;
}

void dep_post_init(lobj_pt lop)
{
    lobj_pt lopr;
    char response[128];
    char *vdata[3];
    size_t vsize[3];

    lopr = lobj_refer("myget");
    if (!lopr) {
        lrdp_generic_error("Object [myget] not found");
        return;
    }

    memset(response, 0, sizeof(response));

    vdata[0] = "get";
    vsize[0] = 3;
    vdata[1] = "hello";
    vsize[1] = 5;
    vdata[2] = response;
    vsize[2] = sizeof(response);

    lobj_fx_vread(lopr, 3, (void **)vdata, vsize);
    lobj_derefer(lopr);
}

void dep_atexit(lobj_pt lop)
{
    lrdp_generic_info("dep_atexit");
}

void dep_on_timer(lobj_pt lop)
{
    uint64_t previous,now;
    void *context;
    size_t ctxsize;

    context = NULL;
    ctxsize = lobj_get_context(lop, &context);
    if (!context) {
        lrdp_generic_error("dep_on_timer context is NULL");
        return;
    }

    previous = *(uint64_t *)context;
    now = getMonotonicMs();
    *(uint64_t *)context = now;

    lrdp_generic_info("dep_on_timer ms diff: %lu", now - previous);
}

void dep_debuger(lobj_pt lop)
{
    uint64_t previous,now;
    void *context;
    size_t ctxsize;

    context = NULL;
    ctxsize = lobj_get_context(lop, &context);
    if (!context) {
        lrdp_generic_error("dep_on_timer context is NULL");
        return;
    }

    previous = *(uint64_t *)context;
    now = getMonotonicMs();
    *(uint64_t *)context = now;

    lrdp_generic_info("dep_debuger ms diff: %lu", now - previous);
}

void *dep_bg_exec(lobj_pt lop)
{
    int i;

    for (i = 0; i < 1000; i++) {
        lrdp_generic_info("dep_bg_exec");
        sleep(1);
    }

    return NULL;
}

void dep_tcp_on_received(lobj_pt lop, void *data, size_t size)
{
    char *p;

    p = ztrymalloc(size + 1);
    if (!p) {
        return;
    }
    memcpy(p, data, size);
    p[size] = '\0';

    lrdp_generic_info("dep_tcp_on_received : %s", p);
    zfree(p);

    sleep(1);

    if (0 == strcmp("hello", p)) {
        lobj_fx_write(lop, "world", 5);
    } else {
        if (0 == strcmp("world", p)) {
            lobj_fx_write(lop, "hello", 5);
        }
    }
}

void dep_tcp_on_closed(lobj_pt lop)
{
    lrdp_generic_info("dep_tcp_on_closed");
}

void dep_tcp_on_accept(lobj_pt lop)
{
    lrdp_generic_info("dep_tcp_on_accept");
}

void dep_tcp_on_connected(lobj_pt lop)
{
    lrdp_generic_info("dep_tcp_on_connected");

    // send a simple packet to server
    lobj_fx_write(lop, "hello", 5);
}

void dep_mq_recv_data(lobj_pt lop, void *data, size_t size)
{
    char *p;

    p = ztrymalloc(size + 1);
    if (!p) {
        return;
    }
    memcpy(p, data, size);
    p[size] = '\0';

    lrdp_generic_info("dep_mq_recv_data : %s", p);
    zfree(p);
}
