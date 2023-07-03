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

// all unit in millimeter
struct nav_context
{
    int enable;
    int current_pos;    // indicate the current pos of the robot,
    int target_pos;     // indicate the target pos of the task
    int predict_xpos;
    int current_vx;     // mm/s
    int odo_meter;      // indicate the distance the robot has traveled
    int distance_remain;
    int deceleration_point;
    float deltaT;
};
static struct nav_context *g_nav = NULL;

#define MAX_VX  (300)
#define MIN_VX  (100)
#define MAX_ACC (50)   // 0.2 Metre per second squared (0.2m/s^2)
#define PREMISSIVE_DEVIATION (3)    // 3mm

static void __nav_write_velocity(float vx)
{
    lobj_pt publisher;
    char str_vx[5];

    publisher = lobj_refer("publisher");
    if (!publisher) {
        return;
    }

    if (!is_float_equal(0, vx)) {
        printf("vx: %.02f\n", vx);
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
        nav->deltaT = 0.02;
    }

    if (!g_nav) {
        g_nav = nav;
    }

    nav->distance_remain = nav->target_pos - nav->current_pos;
    // ignore deviation less than 3mm
    if (fabs(nav->distance_remain) <= PREMISSIVE_DEVIATION) {
        __nav_write_velocity(0.0);
        nav->deceleration_point = 0;
        return;
    }

    // determine the maximum velocity in this task
    // traject control timer interval is 20 ms
    // the maximum acceleration is 0.5 m/s^2 ( or -0.5 m/s^2 )

    // 1st. determine the best deceleration point
    // do not consider with current velocity, only consider with the maximum velocity and remain distance
    if (0 == nav->deceleration_point) {
        nav->deceleration_point = nav->target_pos * 0.9;
    }

    // 2nd. infer the predict pos by current velocity
    nav->predict_xpos = nav->current_pos + nav->current_vx * nav->deltaT;

    // 3th. determine the appropriate velocity
    if (nav->distance_remain > 0) {
        // the robot is moving forward
        if (nav->predict_xpos < nav->deceleration_point) {
            // the robot is accelerating
            nav->current_vx += MAX_ACC;
            if (nav->current_vx > MAX_VX) {
                nav->current_vx = MAX_VX;
            }
        } else {
            // the robot is decelerating
            nav->current_vx -= MAX_ACC;
            if (nav->current_vx < MIN_VX) {
                nav->current_vx = MIN_VX;
            }
        }
    } else {
        // the robot is moving backward
        if (nav->predict_xpos > nav->deceleration_point) {
            // the robot is accelerating
            nav->current_vx -= MAX_ACC;
            if (nav->current_vx < -MAX_VX) {
                nav->current_vx = -MAX_VX;
            }
        } else {
            // the robot is decelerating
            nav->current_vx += MAX_ACC;
            if (nav->current_vx > -MIN_VX) {
                nav->current_vx = -MIN_VX;
            }
        }
    }

    __nav_write_velocity(nav->current_vx / 1000.0);
}

// Calculate the current position by integrating the velocity
void __nav_adjust_position(struct nav_context *nav, float vx)
{
    nav->current_pos += vx * 1000 * nav->deltaT;
    nav->odo_meter += vx * 1000 * nav->deltaT;

    if (nav->current_pos != 0 && fabs(nav->current_pos - nav->target_pos) > PREMISSIVE_DEVIATION) {
        printf("current_pos: %d, odo_meter: %d\n", nav->current_pos, nav->odo_meter);
    }
}

void nav_on_velocity_feedback(lobj_pt lop, const char *channel, const char *pattern, const char *message, size_t len)
{
    float vx;

    if (pattern && message) {
        if (0 == strcmp("feedback.v_x", pattern)) {
            vx = atof(message);
            __nav_adjust_position(g_nav, vx);
        } else {
            printf("pattern missmatch.\n");
        }
    }
}

void nav_on_motion_task(lobj_pt lop, const char *channel, const char *pattern, const char *message, size_t len)
{
    float pos_in_meter;

    if (pattern && message) {
        if (0 == strcmp("motion.pos.x", pattern)) {
            pos_in_meter = atof(message);
            g_nav->current_pos = pos_in_meter * 1000;  // convert to millimeter
        } else {
            printf("pattern missmatch.\n");
        }
    }
}
