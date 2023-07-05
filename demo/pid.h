#pragma once

/* this file declare the PID(Proportional, Integral, Derivative) control algorithm context structure and related functions */

typedef float velocity1d_t;

struct pid1d
{
    float Kp, Ki, Kd; // PID parameters
    velocity1d_t target; // target velocity
    velocity1d_t current; // current velocity
    velocity1d_t error; // error between target and current velocity
    velocity1d_t error_last; // error of last time
    velocity1d_t integral; // integral of error
    velocity1d_t output; // output of PID
};
typedef struct pid1d pid1d_t, *pid1d_pt;

void pid1d_init(pid1d_pt pid, float Kp, float Ki, float Kd);
velocity1d_t pid1d_calculate(pid1d_pt pid, velocity1d_t target, velocity1d_t current);

typedef struct {
    float vx;
    float vy;
} velocity2d_t;

struct pid2d
{
    float Kp, Ki, Kd; // PID parameters
    velocity2d_t target; // target velocity
    velocity2d_t current; // current velocity
    velocity2d_t error; // error between target and current velocity
    velocity2d_t error_last; // error of last time
    velocity2d_t integral; // integral of error
    velocity2d_t output; // output of PID
};
typedef struct pid2d pid2d_t, *pid2d_pt;

void pid2d_init(pid2d_pt pid, float Kp, float Ki, float Kd);
velocity2d_t *pid2d_calculate(pid2d_pt pid, velocity2d_t target, velocity2d_t current);
