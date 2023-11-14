#pragma once

#include <math.h>

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

/* trigonometric function extension */
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

extern double math2_normalize_angle_lf(double sita);
extern float math2_normalize_angle_f(float sita);

/* common definition of unary functions in math2 */
typedef double (*math2_unaryfunction_lfpt)(double x);
typedef float (*math2_unaryfunction_fpt)(float x);
typedef int (*math2_unaryfunction_pt)(int x);
typedef long (*math2_unaryfunction_lpt)(long x);

/* derivation */
extern double math2_derivative_lf(const math2_unaryfunction_lfpt f, double x, double dx);
extern float math2_derivative_f(const math2_unaryfunction_fpt f, float x, float dx);
#define math2_derivative_h(f,x)  math2_derivative_lf((f), (x), 0.000001)
#define math2_derivative_hf(f,x) math2_derivative_f((f), (x), 0.000001f)

/* factorial and double factorial */
extern int math2_factorial(int n);
extern long math2_factorial_l(long n);
extern int math2_doublefactorial(int n);
extern long math2_doublefactorial_l(long n);

/* high step radication */
#define math2_radication_lf(x, t) pow((x), 1.0 / (t))
#define math2_radication_f(x, t) powf((x), 1.0f / (t))

/* greatest common divisor(gcd) */
extern int math2_gcd(int a, int b);
extern long math2_gcd_l(long a, long b);
