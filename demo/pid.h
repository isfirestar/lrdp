#pragma once

/* this file declare the PID(Proportional, Integral, Derivative) control algorithm context structure and related functions */

typedef float velocity1d_t;
typedef struct pid1d *pid1d_pt;

void pid1d_init(pid1d_pt pid, float Kp, float Ki, float Kd);
velocity1d_t pid1d_calculate(pid1d_pt pid, velocity1d_t target, velocity1d_t current);

typedef struct {
    float vx;
    float vy;
} velocity2d_t;
typedef struct pid2d *pid2d_pt;

void pid2d_init(pid2d_pt pid, float Kp, float Ki, float Kd);
velocity2d_t *pid2d_calculate(pid2d_pt pid, velocity2d_t target, velocity2d_t current);
