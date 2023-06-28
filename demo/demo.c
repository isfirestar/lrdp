#include <stdio.h>
#include <time.h>
#include <stdint.h>

#include "ifos.h"
#include "monotonic.h"
#include "lobj.h"
#include "zmalloc.h"

int dep_pre_init(int argc, char **argv)
{
    printf("[%d] dep_pre_init\n", ifos_gettid());
    return 0;
}

void dep_post_init(void *context, unsigned int ctxsize)
{
    if (!context) {
        printf("[%d] dep_post_init context is NULL\n", ifos_gettid());
        return;
    }
    *(uint64_t *)context = getMonotonicMs();
}

void dep_atexit(void)
{
    printf("dep_atexit\n");
}

void dep_on_timer(void *context, unsigned int ctxsize)
{
    uint64_t previous,now;

    if (!context) {
        printf("[%d] dep_on_timer context is NULL\n", ifos_gettid());
        return;
    }

    previous = *(uint64_t *)context;
    now = getMonotonicMs();
    *(uint64_t *)context = now;

    printf("[%d] dep_on_timer ms diff: %lu\n", ifos_gettid(), now - previous);
}

void *dep_bg_exec(void *context, unsigned int ctxsize)
{
    int i;

    for (i = 0; i < 1000; i++) {
        printf("[%d] dep_bg_exec\n", ifos_gettid());
        sleep(1);
    }

    return NULL;
}

void dep_tcp_on_received(lobj_pt lop, const void *data, unsigned int size)
{
    char *p;

    p = ztrymalloc(size + 1);
    if (!p) {
        return;
    }
    memcpy(p, data, size);
    p[size] = '\0';

    printf("[%d] dep_tcp_on_received : %s\n", ifos_gettid(), p);
    zfree(p);

    sleep(1);

    if (0 == strcmp("hello", p)) {
        lobj_write(lop, "world", 5);
    } else {
        if (0 == strcmp("world", p)) {
            lobj_write(lop, "hello", 5);
        }
    }
}

void dep_tcp_on_closed(lobj_pt lop)
{
    printf("[%d] dep_tcp_on_closed\n", ifos_gettid());
}

void dep_tcp_on_accept(lobj_pt lop)
{
    printf("[%d] dep_tcp_on_accept\n", ifos_gettid());
}

void dep_tcp_on_connected(lobj_pt lop)
{
    printf("[%d] dep_tcp_on_connected\n", ifos_gettid());

    // send a simple packet to server
    lobj_write(lop, "hello", 5);
}
