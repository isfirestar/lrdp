#include "math2.h"


/* trigonometric function extension */
double cot(double x)
{
    return 1.0 / tan(x);
}

double sec(double x)
{
    return 1.0 / cos(x);
}

double csc(double x)
{
    return 1.0 / sin(x);
}

double acot(double x)
{
    return atan(1.0 / x);
}

double asec(double x)
{
    return acos(1.0 / x);
}

double acsc(double x)
{
    return asin(1.0 / x);
}

float cotf(float x)
{
    return 1.0f / tanf(x);
}

float secf(float x)
{
    return 1.0f / cosf(x);
}

float cscf(float x)
{
    return 1.0f / sinf(x);
}

float acotf(float x)
{
    return atanf(1.0f / x);
}

float asecf(float x)
{
    return acosf(1.0f / x);
}

float acscf(float x)
{
    return asinf(1.0f / x);
}

double math2_normalize_angle_lf(double sita)
{
    int tp;
	double angle;

    angle = sita;
	if (angle > MATH2_PI) {
		tp = (int)(angle / MATH2_PI);
		if (tp % 2 == 0) {
			angle -= tp * MATH2_PI;
		} else if (tp % 2 == 1) {
			angle -= (tp + 1) * MATH2_PI;
		}
	}
	if (angle < -MATH2_PI) {
		tp = (int)(angle / -MATH2_PI);
		if (tp % 2 == 0) {
			angle += tp * MATH2_PI;
		} else if (tp % 2 == 1) {
			angle += (tp + 1) * MATH2_PI;
		}
	}
	return angle;
}

float math2_normalize_angle_f(float sita)
{
    int tp;
    float angle;

    angle = sita;
    if (angle > MATH2_PI) {
        tp = (int)(angle / MATH2_PI);
        if (tp % 2 == 0) {
            angle -= tp * MATH2_PI;
        } else if (tp % 2 == 1) {
            angle -= (tp + 1) * MATH2_PI;
        }
    }
    if (angle < -MATH2_PI) {
        tp = (int)(angle / -MATH2_PI);
        if (tp % 2 == 0) {
            angle += tp * MATH2_PI;
        } else if (tp % 2 == 1) {
            angle += (tp + 1) * MATH2_PI;
        }
    }
    return angle;
}

/* derivation */
double math2_derivative_lf(const math2_unaryfunction_lfpt f, double x, double dx)
{
    return (f(x + h) - f(x)) / dx;
}

float math2_derivative_f(const math2_unaryfunction_fpt f, float x, float dx)
{
    return (f(x + h) - f(x)) / dx;
}

/* factorial */
int math2_factorial(int n)
{
    return n > 1 ? n * math2_factorial(n - 1) : 1;
}
long math2_factorial_l(long n)
{
    return n > 1 ? n * math2_factorial_l(n - 1) : 1;
}
int math2_doublefactorial(int n)
{
    return n > 1 ? n * math2_doublefactorial(n - 2) : 1;
}
long math2_doublefactorial_l(long n)
{
    return n > 1 ? n * math2_doublefactorial_l(n - 2) : 1;
}

/* greatest common divisor(gcd) */
int math2_gcd(int a, int b)
{
    int r;

    if (a < b) {
        r = a;
        a = b;
        b = r;
    }
    while (b != 0) {
        r = a % b;
        a = b;
        b = r;
    }
    return a;
}

long math2_gcd_l(long a, long b)
{
    long r;

    if (a < b) {
        r = a;
        a = b;
        b = r;
    }
    while (b != 0) {
        r = a % b;
        a = b;
        b = r;
    }
    return a;
}
