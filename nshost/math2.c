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
    return (f(x + dx) - f(x)) / dx;
}

float math2_derivative_f(const math2_unaryfunction_fpt f, float x, float dx)
{
    return (f(x + dx) - f(x)) / dx;
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

/* statistical mathematics */
double variance_lf(const long *origin, unsigned int count)
{
    unsigned int i;
    double aveg;
    long sum;

    if (!origin || 0 == count) {
        return 0;
    }

    sum = 0;
    for (i = 0; i < count; i++) {
        sum += origin[i];
    }
    aveg = (double)sum / count;

    sum = 0;
    for (i = 0; i < count; i++) {
        sum += (origin[i] - aveg) * (origin[i] - aveg);
    }

    return (double)sum / count;
}

float variance_f(const float *origin, unsigned int count)
{
    unsigned int i;
    float aveg, sum;

    if (!origin || 0 == count) {
        return 0.0f;
    }

    sum = 0.0f;
    for (i = 0; i < count; i++) {
        sum += origin[i];
    }
    aveg = sum / count;

    sum = 0.0f;
    for (i = 0; i < count; i++) {
        sum += (origin[i] - aveg) * (origin[i] - aveg);
    }

    return sum / count;
}

// calc standard deviation of a array
double stdev_lf(const long *origin, unsigned int count)
{
    return sqrt(variance_lf(origin, count));
}
float stdev_f(const float *origin, unsigned int count)
{
    return sqrtf(variance_f(origin, count));
}

// calc covariance of two arrays
double covariance_lf(const long *origin1, const long *origin2, unsigned int count)
{
    unsigned int i;
    double aveg1, aveg2;
    long sum;

    if (!origin1 || !origin2 || 0 == count) {
        return 0;
    }

    sum = 0;
    for (i = 0; i < count; i++) {
        sum += origin1[i];
    }
    aveg1 = (double)sum / count;

    sum = 0;
    for (i = 0; i < count; i++) {
        sum += origin2[i];
    }
    aveg2 = (double)sum / count;

    sum = 0;
    for (i = 0; i < count; i++) {
        sum += (origin1[i] - aveg1) * (origin2[i] - aveg2);
    }

    return (double)sum / count;
}

float covariance_f(const float *origin1, const float *origin2, unsigned int count)
{
    unsigned int i;
    float aveg1, aveg2, sum;

    if (!origin1 || !origin2 || 0 == count) {
        return 0.0f;
    }

    sum = 0.0f;
    for (i = 0; i < count; i++) {
        sum += origin1[i];
    }
    aveg1 = sum / count;

    sum = 0.0f;
    for (i = 0; i < count; i++) {
        sum += origin2[i];
    }
    aveg2 = sum / count;

    sum = 0.0f;
    for (i = 0; i < count; i++) {
        sum += (origin1[i] - aveg1) * (origin2[i] - aveg2);
    }

    return sum / count;
}

// mean square error
// MSE = ∑(y - y')^2 / n,n
// n is the number of samples
double mse_lf(const double *origin1, const double *origin2, unsigned int count)
{
    unsigned int i;
    double sum;

    if (!origin1 || !origin2 || 0 == count) {
        return 0.0;
    }

    sum = 0;
    for (i = 0; i < count; i++) {
        sum += (origin1[i] - origin2[i]) * (origin1[i] - origin2[i]);
    }

    return sum / count;
}

float mse_f(const float *origin1, const float *origin2, unsigned int count)
{
    unsigned int i;
    float sum;

    if (!origin1 || !origin2 || 0 == count) {
        return 0.0;
    }

    sum = 0;
    for (i = 0; i < count; i++) {
        sum += (origin1[i] - origin2[i]) * (origin1[i] - origin2[i]);
    }

    return sum / count;
}

double mse_l(const long *origin1, const long *origin2, unsigned int count)
{
    unsigned int i;
    float sum;

    if (!origin1 || !origin2 || 0 == count) {
        return 0.0;
    }

    sum = 0;
    for (i = 0; i < count; i++) {
        sum += (origin1[i] - origin2[i]) * (origin1[i] - origin2[i]);
    }

    return sum / count;
}
