#pragma once

#include <math.h>
#include <stdint.h>

typedef enum {
    MATH2_FALSE = 0,
    MATH2_TRUE = 1
} MATH2_BOOL;

/* common definition of unary functions in math2 */
typedef double (*math2_unaryfunction_lfpt)(double x);
typedef float (*math2_unaryfunction_fpt)(float x);
typedef int (*math2_unaryfunction_pt)(int x);
typedef long (*math2_unaryfunction_lpt)(long x);

/* zero float accuracy */
#if defined EPSINON
    #undef EPSINON
#endif

#define EPSINON  0.000001
#define is_float_zero(x)    (((x) < EPSINON) && ((x) > -EPSINON))
#define is_float_eq(n, m)   ((fabsf((n)-(m))) <= EPSINON )
#define is_float_ne(n, m)   ((fabsf((n)-(m))) > EPSINON)
#define is_float_gt(n, m)   (((n) - (m)) >   EPSINON)
#define is_float_ge(n, m)   (((n) - (m)) >= -EPSINON)
#define is_float_lt(n, m)   (((n) - (m)) <  -EPSINON)
#define is_float_le(n, m)   (((n) - (m)) <=  EPSINON)

#define is_integer_even(x)  (((x) & 0x1) == 0)
#define is_integer_odd(x)   (((x) & 0x1) == 1)
#define is_float_even(x)    is_float_eq((x) % 2, 0)
#define is_float_odd(x)     is_float_ne((x) % 2, 0)

/* maximum and minimum verification */
#if !defined MATH2_MAX
    #define MATH2_MAX(a, b)  (((a) > (b)) ? (a) : (b))
#endif

#if !defined MATH2_MIN
    #define MATH2_MIN(a, b)  (((a) < (b)) ? (a) : (b))
#endif

#if !defined MATH2_PI
    #define MATH2_PI         3.14159265358979323846f
#endif

/* 角度弧度转换(trigonometric function extension) */
#if !defined math2_deg_to_rad
    #define math2_deg_to_rad(deg)  ((deg) * MATH2_PI / 180.0f)
#endif

#if !defined math2_rad_to_deg
    #define math2_rad_to_deg(rad)  ((rad) * 180.0f / MATH2_PI)
#endif

/* trigonometric function extension which didn't define in math.h */
extern double cot(double x);
extern double sec(double x);
extern double csc(double x);
extern double acot(double x);
extern double asec(double x);
extern double acsc(double x);

extern float cotf(float x);
extern float secf(float x);
extern float cscf(float x);
extern float acotf(float x);
extern float asecf(float x);
extern float acscf(float x);

/* 角度归一到[-Pi, Pi] */
extern double normalize_angle_lf(double sita);
extern float normalize_angle_f(float sita);

/* 常规函数一阶导(derivation) */
extern double derivative_lf(const math2_unaryfunction_lfpt f, double x, double dx);
extern float derivative_f(const math2_unaryfunction_fpt f, float x, float dx);
#define derivative_h(f,x)  derivative_lf((f), (x), EPSINON)
#define derivative_hf(f,x) derivative_f((f), (x), EPSINONf)
/* 连续点可导性 */
extern int differentiable_lf(const double *samples, const uint64_t *us, unsigned int n, double limit, double *gap);
extern int differentiable_f(const float *samples, const uint64_t *us, unsigned int n, float limit, float *gap);
/* 常规函数定积分 */
extern double integral_lf(const math2_unaryfunction_lfpt f, double a, double b, double dx);
extern float integral_f(const math2_unaryfunction_fpt f, float a, float b, float dx);
#define integral_h(f,a,b)  integral_lf((f), (a), (b), EPSINON)
#define integral_hf(f,a,b) integral_f((f), (a), (b), EPSINONf)

/* 阶乘和二阶阶乘(factorial and double factorial) */
extern int factorial(int n);
extern long factorial_l(long n);
extern int doublefactorial(int n);
extern long doublefactorial_l(long n);

/* 高阶开方 */
#define radication_lf(x, t) pow((x), 1.0 / (t))
#define radication_f(x, t) powf((x), 1.0f / (t))

/* 最大公约数 (GCD, greatest common divisor) */
extern int gcd(int a, int b);
extern long gcd_l(long a, long b);

/* 平均数 */
extern double average_l(const long *origin, unsigned int count);
extern float average_f(const float *origin, unsigned int count);
extern double average_lf(const double *origin, unsigned int count);
/* 方差 */
extern double variance_l(const long *origin, unsigned int count);
extern float variance_f(const float *origin, unsigned int count);
extern double variance_lf(const double *origin, unsigned int count);
/* 标准差 */
extern double stdev_l(const long *origin, unsigned int count);
extern float stdev_f(const float *origin, unsigned int count);
extern double stdev_lf(const double *origin, unsigned int count);
/* 协方差 */
extern double covariance_l(const long *origin1, const long *origin2, unsigned int count);
extern float covariance_f(const float *origin1, const float *origin2, unsigned int count);
extern double covariance_lf(const double *origin1, const double *origin2, unsigned int count);
/* 均方误差(MSE, mean square error) */
extern double mse_l(const long *origin1, const long *origin2, unsigned int count);
extern float mse_f(const float *origin1, const float *origin2, unsigned int count);
extern double mse_lf(const double *origin1, const double *origin2, unsigned int count);
/* 产生（a,b）区间上均匀分布的随机数 */
extern double uniform_random_lf(double a, double b, long *seed);
extern float uniform_random_f(float a, float b, long *seed);

/* 产生正态分布随机数 : @mean 给定均值, @sigma 给定方差
 *
 * 这里遵循正太分布密度函数
 *              1                  -(x - μ)²
 * f(x) = —————————————— * e ^ ( —————————————— )
 *         √(2π) * σ                  2σ²
 *
 * 通常使用N(0,1)表示标准正态分布
 * 设r1,r2,...rn 为（0，1）上n个相互独立的均匀分布的随机数，由于
 *
 * E(r1) = E(r2) = ... = E(rn) = 1/2 D(r1) = D(r2) = ... = D(rn) = 1/12
 * 根据中心极限定理推导, 当 n->∞ 时
 *
 *           12
 *  x = √ (————— ) ∑ (ri - n/2)
 *           n
 * 的分布近似于正态分布N(0,1)，通常取n=12,
 * 最后通过变换y=μ+σx, 便可以得到均值为μ, 方差为σ2 的正太分布随机数。
 *
*/
extern double gaussian_normal_lf(double mean, double sigma, long *seed);
extern float gaussian_normal_f(float mean, float sigma, long *seed);

/*  下列方法，用于求三维空间内的俯仰角，偏航角和滚转角
 * 三个轴的方向分别为: x轴向前, y轴向右, z轴向上
 * 俯仰角: 以x轴为基准, 顺时针旋转为正, 逆时针旋转为负
 * 航向角: 以y轴为基准, 顺时针旋转为正, 逆时针旋转为负
 * 横滚角: 以z轴为基准, 顺时针旋转为正, 逆时针旋转为负
 */
extern double pitch_lf(double ax, double ay, double az);
extern float pitch_f(float ax, float ay, float az);
extern long pitch_l(long ax, long ay, long az);
extern double yaw_lf(double ax, double ay, double az);
extern float yaw_f(float ax, float ay, float az);
extern long yaw_l(long ax, long ay, long az);
extern double roll_lf(double ax, double ay, double az);
extern float roll_f(float ax, float ay, float az);
extern long roll_l(long ax, long ay, long az);
/* 以下是空间中仰角和方位角的计算 */
extern double elevation_lf(double x, double y, double z);
extern float elevation_f(float x, float y, float z);
extern long elevation_l(long x, long y, long z);
extern double azimuth_lf(double x, double y, double z);
extern float azimuth_f(float x, float y, float z);
extern long azimuth_l(long x, long y, long z);
