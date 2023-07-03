#include "lobj.h"

#include "ifos.h"
#include "naos.h"
#include "rand.h"

#include <stdio.h>
#include <time.h>

int nav_entry(lobj_pt lop, int argc, char **argv)
{
    printf("[%d] nav_entry\n", ifos_gettid());
    return 0;
}

static const char *velocity[5] = { "0.12", "0.14", "0.16", "0.11", "0.20" };

void nav_update_velocity(lobj_pt lop)
{
    lobj_pt publisher;

    publisher = lobj_refer("publisher");
    if (!publisher) {
        return;
    }

    lobj_write(publisher, velocity[redisLrand48() % 5], 4);
    lobj_derefer(publisher);
}

void nav_on_velocity_feedback(lobj_pt lop, const char *channel, const char *pattern, const char *message, size_t len)
{
    if (pattern && message) {
        if (0 == strcmp("motion.v_x", pattern)) {
            printf("feedback velocity:%s\n", message);
        } else {
            printf("pattern missmatch.\n");
        }
    }
}
