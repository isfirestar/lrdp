#include "netobj.h"

#include "nis.h"
#include "naos.h"
#include "zmalloc.h"

#include <string.h>

// static nsp_status_t __netobj_tcp_create()
// {

// }

/* this function parse endpoint string to the endpoint structure object
 * the endpoint string format is: ip:port, note that we only support IPv4 now
 *  1. check the input and output parameter legal
 *  2. delimiter ':' to get ip and port
 *  3. verify ip and port string is legal
 *  4. copy ip string to endpoint structure "ip" filed
 *  5. convert port string to unsigned short and copy to endpoint structure "port" filed, use little endian
 *  6. convert ip string to unsigned int and copy to endpoint structure "inet" filed, use little endian
 */
nsp_status_t netobj_parse_endpoint(const char *epstr, struct endpoint *epo)
{
    char *p;
    char *nextToken;
    char *tmpstr;
    size_t srclen;
    unsigned long byteValue;
    int i;

    if ( unlikely(!epstr) || unlikely(!epo) ) {
        return posix__makeerror(EINVAL);
    }

    srclen = strlen(epstr);
    i = 0;

    tmpstr = (char *)ztrymalloc(srclen + 1);
    if ( unlikely(!tmpstr) ) {
        return posix__makeerror(ENOMEM);
    }
    strcpy(tmpstr, epstr);

    nextToken = NULL;
    while (NULL != (p = strtok_r(nextToken ? NULL : tmpstr, ":", &nextToken)) && i < 2) {
        if (i == 0) {
            if (!naos_is_legal_ipv4(p)) {
                zfree(tmpstr);
                return posix__makeerror(EINVAL);
            }
            strcpy(epo->ip, p);
            epo->inet = naos_ipv4tou(p, kByteOrder_LittleEndian);
        } else if (i == 1) {
            byteValue = strtoul(p, NULL, 10);
            epo->port = (unsigned short)byteValue;
        }
        ++i;
    }

    zfree(tmpstr);
    return NSP_STATUS_SUCCESSFUL;
}

static void __netobj_on_tcp_accepted(HTCPLINK slink, HTCPLINK clink)
{
    lobj_pt slop, clop;
    lobj_seq_t sseq;
    struct netobj *snetp, *cnetp;
    char holder[LOBJ_MAX_NAME_LEN], *cname;
    lobj_seq_t seq;

    nis_cntl(slink, NI_GETCTX, (void **)&sseq);
    slop = lobj_qrefer(sseq);
    if (!slop) {
        return;
    }

    snetp = lobj_body(struct netobj *, slop);
    if (!snetp) {
        return;
    }

    cname = lobj_random_name(holder, sizeof(holder) - 1);
    clop = lobj_dup(cname, slop);
    if (clop) {
        cnetp = lobj_body(struct netobj *, clop);
        cnetp->link = clink;
        tcp_getaddr(clink, LINK_ADDR_REMOTE, &cnetp->remote.inet, &cnetp->remote.port);
        tcp_getaddr(clink, LINK_ADDR_LOCAL, &cnetp->local.inet, &cnetp->local.port);
        naos_ipv4tos(cnetp->remote.inet, cnetp->remote.ip, sizeof(cnetp->remote.ip));
        naos_ipv4tos(cnetp->local.inet, cnetp->local.ip, sizeof(cnetp->local.ip));
        cnetp->protocol = snetp->protocol;
        cnetp->connectproc = snetp->connectproc;
        // call accept user callback
        if (snetp->acceptproc) {
            snetp->acceptproc(clop);
        }
        // save seq to link context
        seq = lobj_get_seq(clop);
        nis_cntl(clink, NI_SETCTX, (void *)seq);
    }

    lobj_derefer(slop);
}

static void __netobj_tcp_recvdata(HTCPLINK link, const void *data, unsigned int size)
{
    lobj_pt lop;
    lobj_seq_t seq;

    nis_cntl(link, NI_GETCTX, (void **)&seq);
    if (!seq) {
        return;
    }

    lop = lobj_qrefer(seq);
    if (!lop) {
        return;
    }

    if (lobj_fx_read(lop, (void *)data, size) == -ENOENT) {
        lobj_fx_vread(lop, 1, (void **)&data, (size_t *)&size);
    }
    lobj_derefer(lop);
}

static void __netobj_tcp_connected(HTCPLINK link)
{
    lobj_pt lop;
    lobj_seq_t seq;
    struct netobj *netp;

    nis_cntl(link, NI_GETCTX, (void **)&seq);
    if (!seq) {
        return;
    }

    lop = lobj_qrefer(seq);
    if (!lop) {
        return;
    }

    netp = lobj_body(struct netobj *, lop);
    if (netp->connectproc) {
        netp->connectproc(lop);
    }

    lobj_derefer(lop);
}

static void __netobj_prclose(HTCPLINK link)
{
    lobj_pt lop;
    lobj_seq_t seq;

    nis_cntl(link, NI_GETCTX, (void **)&seq);
    if (!seq) {
        return;
    }

    lop = lobj_qrefer(seq);
    if (!lop) {
        return;
    }

    lobj_fx_free(lop);
    lobj_derefer(lop);
}

static void STDCALL __netobj_tcp_io(const struct nis_event *event, const void *data)
{

    tcp_data_t *tcpdata;

    tcpdata = (tcp_data_t *)data;

    switch (event->Event) {
        case EVT_PRE_CLOSE:
            __netobj_prclose(event->Ln.Tcp.Link);
            break;
        case EVT_CLOSED:
            break;
        case EVT_RECEIVEDATA:
            __netobj_tcp_recvdata(event->Ln.Tcp.Link, tcpdata->e.Packet.Data, tcpdata->e.Packet.Size);
            break;
        case EVT_TCP_ACCEPTED:
            __netobj_on_tcp_accepted(event->Ln.Tcp.Link, tcpdata->e.Accept.AcceptLink);
            break;
        case EVT_TCP_CONNECTED:
            __netobj_tcp_connected(event->Ln.Tcp.Link);
            break;
    }
}

void __netobj_free(lobj_pt lop, void *context, size_t ctxsize)
{
    struct netobj *netp;

    if (!lop) {
        return;
    }

    netp = lobj_body(struct netobj *, lop);
    if (netp->link != INVALID_HTCPLINK) {
        tcp_destroy(netp->link);
    }
}

int __netobj_write(lobj_pt lop, const void *data, size_t n)
{
    struct netobj *netp;

    netp = lobj_body(struct netobj *, lop);
    if (netp->link == INVALID_HTCPLINK) {
        return -1;
    }

    if (netp->protocol == JCFG_PROTO_TCP) {
        return tcp_write(netp->link, data, n, NULL);
    }

    return -1;
}

int __netobj_vwrite(lobj_pt lop, int elements, const void **vdata, size_t *vsize )
{
    struct netobj *netp;
    struct endpoint ep;
    nsp_status_t status;

    if (!lop || !vdata || !vsize || elements <= 0) {
        return -1;
    }

    netp = lobj_body(struct netobj *, lop);
    if (netp->link == INVALID_HTCPLINK) {
        return -1;
    }

    if (netp->protocol == JCFG_PROTO_UDP) {
        if (elements < 2) {
            return -1;
        }
        status = netobj_parse_endpoint((const char *)vdata[1], &ep);
        if (!NSP_SUCCESS(status)) {
            return -1;
        }
        return udp_write(netp->link, vdata[0], vsize[0], ep.ip, ep.port, NULL);
    }

    if (netp->protocol == JCFG_PROTO_TCP) {
        return tcp_write(netp->link, vdata[0], vsize[0], NULL);
    }
    return -1;
}

void netobj_create(const jconf_net_pt jnetcfg)
{
    lobj_pt lop;
    struct lobj_fx fx = {
        .freeproc = &__netobj_free,
        .writeproc = &__netobj_write,
        .vwriteproc = &__netobj_vwrite,
        .readproc = NULL,
        .vreadproc = NULL,
    };
    struct netobj *netp;
    nsp_status_t status;
    lobj_seq_t seq;

    if (!jnetcfg) {
        return;
    }

    // create object first
    lop = lobj_create(jnetcfg->head.name, jnetcfg->head.module, sizeof(struct netobj), jnetcfg->head.ctxsize, &fx);
    if (!lop) {
        return;
    }
    netp = lobj_body(struct netobj *, lop);
    lobj_cover_fx(lop, jnetcfg->head.freeproc, jnetcfg->head.writeproc, jnetcfg->head.vwriteproc, jnetcfg->head.readproc, jnetcfg->head.vreadproc, NULL);

    do {
        /* determine protocol type and init framework */
        if (jnetcfg->protocol == JCFG_PROTO_TCP) {
            tcp_init2(0);
        } else if (jnetcfg->protocol == JCFG_PROTO_UDP) {
            udp_init2(0);
        } else {
            break;
        }

        // convert endpoint string to endpoint structure
        netobj_parse_endpoint(jnetcfg->listen, &netp->listen);
        netobj_parse_endpoint(jnetcfg->remote, &netp->remote);
        netobj_parse_endpoint(jnetcfg->local, &netp->local);

        // pass callback function from config to object
        netp->acceptproc = lobj_dlsym(lop, jnetcfg->acceptproc);
        netp->connectproc = lobj_dlsym(lop, jnetcfg->connectproc);

        // save protocol
        netp->protocol = jnetcfg->protocol;

        // ok, init tcp/udp socket now
        if (jnetcfg->protocol == JCFG_PROTO_TCP) {
            if (0 != netp->listen.ip[0]) {
                netp->link = tcp_create2(__netobj_tcp_io, netp->listen.ip, netp->listen.port, NULL);  // this is a server
            } else {
                netp->link = tcp_create2(__netobj_tcp_io, netp->local.ip, netp->local.port, NULL);  // this is a client
            }

            if (netp->link == INVALID_HTCPLINK) {
                break;
            }

            seq = lobj_get_seq(lop);
            nis_cntl(netp->link, NI_SETCTX, (void *)seq);
            if (0 != netp->remote.ip[0]) {
                status = tcp_connect(netp->link, netp->remote.ip, netp->remote.port);
            } else {
                status = tcp_listen(netp->link, 100);
            }

            if (!NSP_SUCCESS(status)) {
                break;
            }
        } else if (jnetcfg->protocol == JCFG_PROTO_UDP) {
            ;
        } else {
            break;
        }

        return;
    }while(0);

    // error occured, destroy the object
    lobj_ldestroy(lop);
}
