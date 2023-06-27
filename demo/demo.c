#include <stdio.h>
#include <time.h>
#include <stdint.h>

#include "ifos.h"

int dep_entry(int argc, char **argv)
{
    printf("dep_entry\n");
    return 0;
}

void dep_atexit(void)
{
    printf("dep_atexit\n");
}

void dep_on_timer(void *context, unsigned int ctxsize)
{
    uint64_t previous,now;
    struct timespec tv;

    previous = *(uint64_t *)context;

    clock_gettime(CLOCK_MONOTONIC, (struct timespec *)&tv);
    now = tv.tv_sec * 1000 + tv.tv_nsec / 1000000;

    printf("dep_on_timer ms diff: %llu\n", now - previous);

    *(uint64_t *)context = now;
}

void *dep_bg_exec(void *parameter)
{
    printf("dep_bg_exec, this thread id is:%d, parameter address:%p\n", ifos_gettid(), parameter);
    return NULL;
}
