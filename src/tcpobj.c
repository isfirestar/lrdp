#include "tcpobj.h"

#include "netobj.h"
#include "nis.h"
#include "jconf.h"
#include "naos.h"

struct tcpobj
{
    endpoint_t local;
    endpoint_t remote;
    HTCPLINK link;
};

static void __tcpobj_close(lobj_pt lop, void *ctx, size_t ctxsize)
{
    struct tcpobj *obj;

    obj = lobj_body(struct tcpobj *, lop);
    if (INVALID_HTCPLINK != obj->link) {
        tcp_destroy(obj->link);
        obj->link = INVALID_HTCPLINK;
    }
}

static int __tcpobj_write(lobj_pt lop, const void *data, size_t n)
{
    struct tcpobj *obj;

    if (!data || !n) {
        return -1;
    }

    obj = lobj_body(struct tcpobj *, lop);
    if (INVALID_HTCPLINK == obj->link) {
        return -1;
    }

    return tcp_write(obj->link, data, n, NULL);;
}

/* in vwrite parameter sequence:
 *  vdata[0] is pointer to endpoint string or a domain name begin with "ipc:" indicate the target host
 *  vdata[1] is pointer to data which will be send
 */
static int __tcpobj_vwrite(lobj_pt lop, int elements, const void **vdata, const size_t *vsize)
{
    return __tcpobj_write(lop, vdata[1], vsize[1]);
}


static void __tcpobj_on_accepted(lobj_pt lop, HTCPLINK acceptlink)
{
    lobj_pt acceptlop;
    struct tcpobj *acceptobj;

    // create a new object for the accepted link
    acceptlop = lobj_dup(NULL, lop);
    if (!acceptlop) {
        return;
    }
    acceptobj = lobj_body(struct tcpobj *, acceptlop);

    // set the accepted link to the new object
    acceptobj->link = acceptlink;
    nis_cntl(acceptlink, NI_SETCTX, acceptlop);
    // recover the host endpoint information
    tcp_getaddr(acceptlink, LINK_ADDR_LOCAL, &acceptobj->local.inet, &acceptobj->local.port);
    naos_ipv4tos(acceptobj->local.inet, acceptobj->local.ipv4, sizeof(acceptobj->local.ipv4));
    acceptobj->local.eptype = ENDPOINT_TYPE_IPv4;
    tcp_getaddr(acceptlink, LINK_ADDR_REMOTE, &acceptobj->remote.inet, &acceptobj->remote.port);
    naos_ipv4tos(acceptobj->remote.inet, acceptobj->remote.ipv4, sizeof(acceptobj->remote.ipv4));
    acceptobj->remote.eptype = ENDPOINT_TYPE_IPv4;

    // callback to user
    lobj_fx_on_recvdata(lop, acceptlop, sizeof(acceptlop));
}

static void __tcpobj_common_callback(const struct nis_event *event, const void *data)
{
    const tcp_data_pt tcpdata = (const tcp_data_pt)data;
    HTCPLINK link;
    lobj_pt lop;

    if (!event || !data) {
        return;
    }

    link = event->Ln.Tcp.Link;
    if (INVALID_HTCPLINK == link) {
        return;
    }

    nis_cntl(link, NI_GETCTX, (void **)&lop);
    if (!lop) {
        return;
    }
    lobj_raise(lop);  // to increase a reference count
    //obj = lobj_body(struct tcpobj *, lop);

    switch (event->Event) {
        case EVT_RECEIVEDATA:
            lobj_fx_on_recvdata(lop, tcpdata->e.Packet.Data, tcpdata->e.Packet.Size);
            break;
        case EVT_PRE_CLOSE:
            break;
        case EVT_CLOSED:
            break;
        case EVT_TCP_ACCEPTED:
            __tcpobj_on_accepted(lop, tcpdata->e.Accept.AcceptLink);
            break;
        case EVT_TCP_CONNECTED:
        default:
            break;
    }

    lobj_shrink(lop);  // to decrease a reference count
}

lobj_pt tcpobj_create(const struct lobj_fx *fx, const char *name, size_t ctxsize, const endpoint_string_pt *epstr)
{
    return NULL;
}

lobj_pt tcpobj_jcreate(const jconf_tcpobj_pt jtcp)
{
    lobj_pt lop;
    struct tcpobj *obj;
    struct lobj_fx_sym sym = { NULL };
    struct lobj_fx fx = { NULL };
    enum endpoint_type eptype;

    if (!jtcp) {
        return NULL;
    }

    tcp_init2(0);

    fx.freeproc = &__tcpobj_close;
    fx.writeproc = &__tcpobj_write;
    fx.vwriteproc = &__tcpobj_vwrite;
    lop = lobj_create(jtcp->head.name, jtcp->head.module, sizeof(*obj), jtcp->head.ctxsize, &fx);
    if (!lop) {
        return NULL;
    }
    obj = lobj_body(struct tcpobj *, lop);
    obj->link = INVALID_HTCPLINK;

    sym.touchproc_sym = jtcp->head.touchproc;
    sym.freeproc_sym = NULL;
    sym.writeproc_sym = NULL;
    sym.vwriteproc_sym =  NULL;
    sym.readproc_sym =  jtcp->head.readproc;
    sym.vreadproc_sym = jtcp->head.vreadproc;
    sym.recvdataproc_sym = jtcp->head.recvdataproc;
    sym.vrecvdataproc_sym = jtcp->head.vrecvdataproc;
    sym.rawinvokeproc_sym = jtcp->head.rawinvokeproc;
    lobj_fx_cover(lop, &sym);

    eptype = netobj_parse_endpoint((const endpoint_string_pt)jtcp->local, &obj->local);
    if (ENDPOINT_TYPE_UNIX_DOMAIN == eptype) {
        obj->link = tcp_create2(&__tcpobj_common_callback, obj->local.domain, 0, NULL);
    } else if (ENDPOINT_TYPE_IPv4 == eptype) {
        obj->link = tcp_create2(&__tcpobj_common_callback, obj->local.ipv4, obj->local.port, NULL);
    } else {
        ;
    }

    if (INVALID_HTCPLINK == obj->link) {
        lobj_ldestroy(lop);
        return NULL;
    }

    nis_cntl(obj->link, NI_SETCTX, lop);
    return lop;
}
