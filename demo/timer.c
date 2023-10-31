#include "lobj.h"
#include "print.h"

// gcc ../demo/timer.c -olibdemo_timer.so -I ../include/ -fPIC -shared -g3

void bg_on_timer(lobj_pt lop)
{
    lrdp_generic_info(" sunny");
}

void fg_on_timer(lobj_pt lop)
{
    lrdp_generic_info(" raining");
}
