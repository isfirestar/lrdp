#include "udpobj.h"

#include "netobj.h"
#include "udp.h"

struct udpobj
{
    struct endpoint local;
    char domain[128];
    HUDPLINK link;
};

static void __udpobj_close(lobj_pt lop, void *ctx, size_t ctxsize)
{
    struct udpobj *obj;

    obj = lobj_body(struct udpobj *, lop);
    if (INVALID_HUDPLINK != obj->link) {
        udp_destroy(obj->link);
        obj->link = INVALID_HUDPLINK;
    }
}

static void __udpobj_on_recvdata(HUDPLINK link, const udp_data_pt udpdata)
{
    lobj_pt lop;
    const void *vdata[2];
    size_t vsize[2];
    char epstr[24];

    nis_cntl(link, NI_GETCTX, (void **)&lop);
    if (!lop) {
        return;
    }

    if (0 == udpdata->e.Packet.RemotePort && udpdata->e.Packet.Domain) {
        vdata[0] = udpdata->e.Packet.Domain;
        vsize[0] = strlen(udpdata->e.Packet.Domain);
        vdata[1] = udpdata->e.Packet.Data;
        vsize[1] = udpdata->e.Packet.Size;
        if ( lobj_fx_on_vrecvdata(lop, 2, vdata, vsize) < 0 ) {
            lobj_fx_on_recvdata(lop, udpdata->e.Packet.Data, udpdata->e.Packet.Size);
        }
    } else {
        snprintf(epstr, sizeof(epstr), "%s:%u", udpdata->e.Packet.RemoteAddress, udpdata->e.Packet.RemotePort);
        vdata[0] = epstr;
        vsize[0] = strlen(epstr);
        vdata[1] = udpdata->e.Packet.Data;
        vsize[1] = udpdata->e.Packet.Size;
        if ( lobj_fx_on_vrecvdata(lop, 2, vdata, vsize) < 0 ) {
            lobj_fx_on_recvdata(lop, udpdata->e.Packet.Data, udpdata->e.Packet.Size);
        }
    }
}

static void __udpobj_common_callback(const struct nis_event *event, const void *data)
{
    const udp_data_pt udpdata = (const udp_data_pt)data;

    if (!event || !data) {
        return;
    }

    switch (event->Event) {
        case EVT_RECEIVEDATA:
            __udpobj_on_recvdata(event->Ln.Udp.Link, udpdata);
            break;
        case EVT_PRE_CLOSE:
            break;
        case EVT_CLOSED:
            break;
        default:
            break;
    }
}

static int __udpobj_write(lobj_pt lop, const void *data, size_t n)
{
    return -1;
}

/* in vwrite parameter sequence:
 *  vdata[0] is pointer to endpoint string or a domain name begin with "ipc:" indicate the target host
 *  vdata[1] is pointer to data which will be send
 */
static int __udpobj_vwrite(lobj_pt lop, int elements, const void **vdata, size_t *vsize)
{
    struct udpobj *obj;
    struct endpoint to;
    nsp_status_t status;

    if (!vdata || !vsize || elements < 2) {
        return -1;
    }

    if (!vdata[0] || !vdata[1] || 0 == vsize[0] || 0 == vsize[1]) {
        return -1;
    }

    obj = lobj_body(struct udpobj *, lop);
    if (INVALID_HUDPLINK == obj->link) {
        return -1;
    }

    status = netobj_parse_endpoint((const char *)vdata[0], &to);
    if (!NSP_SUCCESS(status)) {
        if (0 == strncasecmp("IPC:", (const char *)vdata[0], 4)) {
            status = udp_write(obj->link, vdata[1], vsize[1], vdata[0], 0, NULL);
        }
    } else {
        status = udp_write(obj->link, vdata[1], vsize[1], to.ip, to.port, NULL);
    }

    return status;
}

lobj_pt udpobj_create(const jconf_udpobj_pt judp)
{
    lobj_pt lop;
    struct udpobj *obj;
    struct lobj_fx_sym sym = { NULL };
    struct lobj_fx fx = { NULL };
    nsp_status_t status;

    if (!judp) {
        return NULL;
    }

    udp_init2(0);

    fx.freeproc = &__udpobj_close;
    fx.writeproc = &__udpobj_write;
    fx.vwriteproc = &__udpobj_vwrite;
    lop = lobj_create(judp->head.name, judp->head.module, sizeof(*obj), judp->head.ctxsize, &fx);
    if (!lop) {
        return NULL;
    }
    obj = lobj_body(struct udpobj *, lop);
    obj->link = INVALID_HUDPLINK;

    sym.touchproc_sym = judp->head.touchproc;
    sym.freeproc_sym = NULL;
    sym.writeproc_sym = NULL;
    sym.vwriteproc_sym =  NULL;
    sym.readproc_sym =  judp->head.readproc;
    sym.vreadproc_sym = judp->head.vreadproc;
    sym.recvdataproc_sym = judp->head.recvdataproc;
    sym.vrecvdataproc_sym = judp->head.vrecvdataproc;
    sym.rawinvokeproc_sym = judp->head.rawinvokeproc;
    lobj_fx_cover(lop, &sym);

    status = netobj_parse_endpoint(judp->local, &obj->local);
    if (!NSP_SUCCESS(status)) {
        if (0 == strncasecmp("IPC:", judp->local, 4)) {
            strncpy(obj->domain, judp->local, sizeof(obj->domain) - 1);
            obj->link = udp_create(&__udpobj_common_callback, obj->domain, 0, UDP_FLAG_NONE);
        }
    } else {
        obj->link = udp_create(&__udpobj_common_callback, obj->local.ip, obj->local.port, UDP_FLAG_NONE);
    }

    if (INVALID_HUDPLINK == obj->link) {
        lobj_ldestroy(lop);
        return NULL;
    }

    nis_cntl(obj->link, NI_SETCTX, lop);
    return lop;
}
