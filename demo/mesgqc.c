#include "lobj.h"
#include "zmalloc.h"
#include "ifos.h"

void dep_mq_recv_data(lobj_pt lop, void *data, size_t size)
{
    char *p;

    p = ztrymalloc(size + 1);
    if (!p) {
        return;
    }
    memcpy(p, data, size);
    p[size] = '\0';

    printf("[%d] dep_mq_recv_data : %s\n", ifos_gettid(), p);
    zfree(p);
}

void mq_sender_proc(lobj_pt lop)
{
    char buf[1024];
    lobj_pt lopmq;
    static int i = 0;

    lopmq = lobj_refer("myq");
    if (!lopmq) {
        printf("lopmq is NULL\n");
        return;
    }

    snprintf(buf, sizeof(buf), "hello %d", i++);
    lobj_fx_write(lopmq, buf, strlen(buf));

    lobj_derefer(lopmq);
}
