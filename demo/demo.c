#include <stdio.h>
#include <time.h>
#include <stdint.h>

#include "ifos.h"
#include "monotonic.h"

int dep_pre_init(int argc, char **argv)
{
    printf("dep_pre_init\n");
    return 0;
}

void dep_post_init(void *context, unsigned int ctxsize)
{
    *(uint64_t *)context = getMonotonicUs() / 1000;
}

void dep_atexit(void)
{
    printf("dep_atexit\n");
}

void dep_on_timer(void *context, unsigned int ctxsize)
{
    uint64_t previous,now;

    previous = *(uint64_t *)context;
    now = getMonotonicUs() / 1000;
    *(uint64_t *)context = now;

    printf("dep_on_timer ms diff: %llu\n", now - previous);
}

void *dep_bg_exec(void *context, unsigned int ctxsize)
{
    int i;

    for (i = 0; i < 1000; i++) {
        printf("[%d]dep_bg_exec\n", ifos_gettid());
        sleep(1);
    }

    return NULL;
}
