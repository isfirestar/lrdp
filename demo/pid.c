#include "pid.h"

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
typedef struct pid1d pid1d_t;

void pid1d_init(velocity1d_pt pid, float Kp, float Ki, float Kd)
{
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;
    pid->target = 0.0;
    pid->current = 0.0;
    pid->error = 0.0;
    pid->error_last = 0.0;
    pid->integral = 0.0;
    pid->output = 0.0;
}

velocity1d_t pid1d_calculate(velocity1d_pt pid, velocity1d_t target, velocity1d_t current)
{
    pid->target = target;
    pid->current = current;
    pid->error = pid->target - pid->current;
    pid->integral += pid->error;
    pid->output = pid->Kp * pid->error + pid->Ki * pid->integral + pid->Kd * (pid->error - pid->error_last);
    pid->error_last = pid->error;
    return pid->output;
}

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
typedef struct pid2d pid2d_t;

void pid2d_init(pid2d_pt pid, float Kp, float Ki, float Kd)
{
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;
    pid->target.vx = pid->target.vy = 0.0;
    pid->current.vx = pid->current.vy = 0.0;
    pid->error.vx = pid->error.vy = 0.0;
    pid->error_last.vx = pid->error_last.vy = 0.0;
    pid->integral.vx = pid->integral.vy = 0.0;
    pid->output.vx = pid->output.vy = 0.0;
}

velocity2d_t *pid2d_calculate(pid2d_pt pid, velocity2d_t target, velocity2d_t current)
{
    pid->target.vx = target.vx;
    pid->target.vy = target.vy;
    pid->current.vx = current.vx;
    pid->current.vy = current.vy;
    pid->error.vx = pid->target.vx - pid->current.vx;
    pid->error.vy = pid->target.vy - pid->current.vy;
    pid->integral.vx += pid->error.vx;
    pid->integral.vy += pid->error.vy;
    pid->output.vx = pid->Kp * pid->error.vx + pid->Ki * pid->integral.vx + pid->Kd * (pid->error.vx - pid->error_last.vx);
    pid->output.vy = pid->Kp * pid->error.vy + pid->Ki * pid->integral.vy + pid->Kd * (pid->error.vy - pid->error_last.vy);
    pid->error_last.vx = pid->error.vx;
    pid->error_last.vy = pid->error.vy;
    return &pid->output;
}
