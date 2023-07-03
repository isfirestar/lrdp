#include "lobj.h"

#include "ifos.h"
#include "naos.h"
#include "rand.h"

#include <stdio.h>
#include <time.h>
#include <math.h>

int nav_entry(lobj_pt lop, int argc, char **argv)
{
    printf("[%d] nav_entry\n", ifos_gettid());
    return 0;
}

static const char *velocity[5] = { "0.12", "0.14", "0.16", "0.11", "0.20" };

// all unit in millimeter x 1000
struct nav_context
{
    int enable;
    int current_pos;    // indicate the current pos of the robot, 
    int target_pos;     // indicate the target pos of the task
    int predict_xpos;
    int odo_meter;      // indicate the distance the robot has traveled
    int distance_remain;
};

static void __nav_write_velocity(float vx)
{
    lobj_pt publisher;
    char str_vx[5];

    publisher = lobj_refer("publisher");
    if (!publisher) {
        return;
    }

    sprintf(str_vx, "%.02f", vx);
    lobj_write(publisher, str_vx, 4);
    lobj_derefer(publisher);
}

void nav_traject_control(lobj_pt lop)
{
    struct nav_context *nav;
    size_t ctxsize;

    ctxsize = lobj_get_context(lop, (void **)&nav);
    if (ctxsize < sizeof(struct nav_context)) {
        return;
    }

    if (!nav->enable) {
        nav->enable = 1;
    }

    nav->distance_remain = nav->target_pos - nav->current_pos;
    // ignore deviation less than 3mm
    if (fabs(nav->distance_remain) <= 30) {
        return;
    }
}

void nav_on_velocity_feedback(lobj_pt lop, const char *channel, const char *pattern, const char *message, size_t len)
{
    if (pattern && message) {
        if (0 == strcmp("feedback.v_x", pattern)) {
            printf("feedback velocity:%s\n", message);
        } else {
            printf("pattern missmatch.\n");
        }
    }
}
