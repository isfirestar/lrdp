#include "lobj.h"
#include "math2.h"

#include <stdio.h>

// gcc ../demo/math2_test.c -olibmath2_testli.so -I ../include/ -fPIC -shared
int math2_test_entry(lobj_pt lop, int argc, char **argv)
{
    double y;

    y = integral_h(sin, 0, MATH2_PI);
    printf("integral_h(sin, 0, MATH2_PI) = %lf\n", y);
    return 1;
}
