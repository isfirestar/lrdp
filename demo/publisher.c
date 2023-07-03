#include "lobj.h"
#include "zmalloc.h"

int nav_publish_velocity(lobj_pt lop, const void *data, size_t n)
{
    lobj_pt redislop;
    const char *vdata[5];
    size_t vsize[5];
    int written;

    redislop = lobj_refer("myrobot");
    if (!redislop) {
        return -1;
    }

    vdata[0] = NULL;
    vsize[0] = 0;
    vdata[1] = NULL;
    vsize[1] = 0;
    vdata[2] = "PUBLISH";
    vsize[2] = strlen("PUBLISH");
    vdata[3] = "motion.v_x";
    vsize[3] = strlen("motion.v_x");
    vdata[4] = data;
    vsize[4] = n;

    written = lobj_vwrite(redislop, 5, (const void **)vdata, vsize);
    lobj_derefer(redislop);
    return written;
}

int nav_publish_any(lobj_pt lop, int elements, const void **pdata, size_t *psize)
{
    lobj_pt redislop;
    const void **vdata;
    size_t *vsize;
    int i;
    int n;

    if (elements < 2) {
        return -1;
    }

    vdata = ztrycalloc(sizeof(const void *) * (elements + 3));
    if (!vdata) {
        return -1;
    }
    vsize = (size_t *)ztrycalloc(sizeof(size_t) * (elements + 3));
    if (!vsize) {
        zfree(vdata);
        return -1;
    }

    redislop = lobj_refer("myrobot");
    if (!redislop) {
        zfree(vdata);
        zfree(vsize);
        return -1;
    }

    vdata[0] = NULL;
    vsize[0] = 0;
    vdata[1] = NULL;
    vsize[1] = 0;
    vdata[2] = "PUBLISH";
    vsize[2] = strlen("PUBLISH");
    for (i = 0; i < elements; i++) {
        vdata[3 + i] = pdata[i];
        vsize[3 + i] = psize[i];
    }

    n = lobj_vwrite(redislop, elements + 3, (const void **)vdata, vsize);
    lobj_derefer(redislop);
    zfree(vdata);
    zfree(vsize);
    return n;
}
