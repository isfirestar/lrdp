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

double normalize_angle_lf(double sita)
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

float normalize_angle_f(float sita)
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
double derivative_lf(const math2_unaryfunction_lfpt f, double x, double dx)
{
    return (f(x + dx) - f(x)) / dx;
}
float derivative_f(const math2_unaryfunction_fpt f, float x, float dx)
{
    return (f(x + dx) - f(x)) / dx;
}

int differentiable_lf(const double *samples, const uint64_t *timestamps, unsigned int n, double limit, double *gap)
{
    double left_derivation, right_derivation, dt, diff;
    unsigned int i;

    if (n < 3) {
        return 0;
    }

    for (i = 1; i < n - 1; i++) {
        dt = (double)((double)timestamps[i] - timestamps[i - 1]) / 1000000.0;
        left_derivation = (samples[i] - samples[i - 1]) / dt;
        right_derivation = (samples[i + 1] - samples[i]) / dt;

        // check differentiable
        diff = fabs(left_derivation - right_derivation);
        if (is_float_gt(diff, limit) ) {
            if (gap) {
                *gap = diff;
            }
            return i;
        }
    }
    return 0;
}

MATH2_BOOL is_sequence_differentiable_f(const float samples[3], const uint64_t *timestamps, unsigned int n, float limit, float *gap)
{
    float left_derivation, right_derivation, diff;
    double dt;
    unsigned int i;

    if (n < 3) {
        return 0;
    }

    for (i = 1; i < n - 1; i++) {
        dt = (double)((double)timestamps[i] - timestamps[i - 1]) / 1000000.0;
        left_derivation = (samples[i] - samples[i - 1]) / dt;
        right_derivation = (samples[i + 1] - samples[i]) / dt;

        // check differentiable
        diff = fabsf(left_derivation - right_derivation);
        if (is_float_gt(diff, limit) ) {
            if (gap) {
                *gap = diff;
            }
            return i;
        }
    }
    return 0;
}

double integral_lf(const math2_unaryfunction_lfpt f, double a, double b, double dx)
{
    double sum, x;

    sum = 0.0;
    for (x = a; x < b; x += dx) {
        sum += f(x);
    }
    return dx * sum;
}
float integral_f(const math2_unaryfunction_fpt f, float a, float b, float dx)
{
    float sum, x;

    sum = 0.0f;
    for (x = a; x < b; x += dx) {
        sum += f(x);
    }
    return dx * sum;
}

/* factorial */
int factorial(int n)
{
    return n > 1 ? n * factorial(n - 1) : 1;
}
long factorial_l(long n)
{
    return n > 1 ? n * factorial_l(n - 1) : 1;
}
int doublefactorial(int n)
{
    return n > 1 ? n * doublefactorial(n - 2) : 1;
}
long doublefactorial_l(long n)
{
    return n > 1 ? n * doublefactorial_l(n - 2) : 1;
}

/* greatest common divisor(gcd) */
int gcd(int a, int b)
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

long gcd_l(long a, long b)
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
double average_lf(const double *origin, unsigned int count)
{
    unsigned int i;
    double sum;

    if (!origin || 0 == count) {
        return 0.0;
    }

    sum = 0.0;
    for (i = 0; i < count; i++) {
        sum += origin[i];
    }

    return sum / count;
}

float average_f(const float *origin, unsigned int count)
{
    unsigned int i;
    float sum;

    if (!origin || 0 == count) {
        return 0.0f;
    }

    sum = 0.0f;
    for (i = 0; i < count; i++) {
        sum += origin[i];
    }

    return sum / count;
}

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
    long sum;

    if (!origin1 || !origin2 || 0 == count) {
        return 0.0;
    }

    sum = 0;
    for (i = 0; i < count; i++) {
        sum += (origin1[i] - origin2[i]) * (origin1[i] - origin2[i]);
    }

    return (double)sum / count;
}

double uniform_random_lf(double a, double b, long *seed)
{
	double t;

	*seed = 2045.0 * (*seed) + 1;
	*seed = *seed - (*seed / 1048576) * 1048576;
	t = (*seed) / 1048576.0;
	t = a + (b - a) * t;
	return t;
}

float uniform_random_f(float a, float b, long *seed)
{
	float t;

	*seed = 2045.0 * (*seed) + 1;
	*seed = *seed - (*seed / 1048576) * 1048576;
	t = (*seed) / 1048576.0;
	t = a + (b - a) * t;
	return t;
}

double gaussian_normal_lf(double mean, double sigma, long *seed)
{
	int i;
	double x, y;

	for (x=0, i = 0; i < 12; i++) {
		x += uniform_random_lf(0.0, 1.0, seed);
	}
	x = x - 6;
	y = mean + x * sigma;
	return y;
}

float gaussian_normal_f(float mean, float sigma, long *seed)
{
	int i;
	float x, y;

	for (x=0, i = 0; i < 12; i++) {
		x += uniform_random_f(0.0, 1.0, seed);
	}
	x = x - 6;
	y = mean + x * sigma;
	return y;
}

double pitch_lf(double ax, double ay, double az)
{
    return atan2(ax, sqrt(ay * ay + az * az));
}
float pitch_f(float ax, float ay, float az)
{
    return atan2f(ax, sqrtf(ay * ay + az * az));
}
long pitch_l(long ax, long ay, long az)
{
    return atan2l(ax, sqrtl(ay * ay + az * az));
}

double yaw_lf(double ax, double ay, double az)
{
    return atan2(ay, sqrt(ax * ax + az * az));
}
float yaw_f(float ax, float ay, float az)
{
    return atan2f(ay, sqrtf(ax * ax + az * az));
}
long yaw_l(long ax, long ay, long az)
{
    return atan2l(ay, sqrtl(ax * ax + az * az));
}

double roll_lf(double ax, double ay, double az)
{
    return atan2(az, sqrt(ax * ax + ay * ay));
}
float roll_lf(float ax, float ay, float az)
{
    return atan2f(az, sqrtf(ax * ax + ay * ay));
}
long roll_lf(long ax, long ay, long az)
{
    return atan2l(az, sqrtl(ax * ax + ay * ay));
}
