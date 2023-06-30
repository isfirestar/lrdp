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
    lobj_pt redislop;
    const char *vdata[5];
    size_t vsize[5];

    redislop = lobj_refer("myrobot");
    if (!redislop) {
        return;
    }

    vdata[0] = NULL;
    vsize[0] = 0;
    vdata[1] = NULL;
    vsize[1] = 0;
    vdata[2] = "PUBLISH";
    vsize[2] = strlen("PUBLISH");
    vdata[3] = "motion.v_x";
    vsize[3] = strlen("motion.v_x");
    vdata[4] = velocity[redisLrand48() % 5];
    vsize[4] = 4;

    lobj_vwrite(redislop, 5, (const void **)vdata, vsize);
    lobj_derefer(redislop);
}
