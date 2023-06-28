#include "netobj.h"

#include "nis.h"
#include "naos.h"
#include "monotonic.h"
#include "rand.h"
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
static nsp_status_t __netobj_parse_endpoint(const char *epstr, struct endpoint *epo)
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
    int64_t *sseq;
    struct netobj *snetp, *cnetp;
    char cname[LOBJ_MAX_NAME_LEN];
    monotime now;

    nis_cntl(slink, NI_GETCTX, &sseq);
    if (!sseq) {
        return;
    }

    slop = lobj_refer_byseq(*sseq);
    if (!slop) {
        return;
    }

    snetp = lobj_body(struct netobj *, slop);
    if (!snetp) {
        return;
    }

    now = getMonotonicUs();
    snprintf(cname, sizeof(cname) - 1, "%u|%u|%u", (unsigned int)(now >> 32), (unsigned int)(now & 0xffffffff), redisLrand48());
    clop = lobj_create(cname, slop->module, slop->size, slop->ctxsize, &slop->fx);
    if (clop) {
        cnetp = lobj_body(struct netobj *, clop);
        cnetp->link = clink;
        tcp_getaddr(clink, LINK_ADDR_REMOTE, &cnetp->remote.inet, &cnetp->remote.port);
        tcp_getaddr(clink, LINK_ADDR_LOCAL, &cnetp->local.inet, &cnetp->local.port);
        naos_ipv4tos(cnetp->remote.inet, cnetp->remote.ip, sizeof(cnetp->remote.ip));
        naos_ipv4tos(cnetp->local.inet, cnetp->local.ip, sizeof(cnetp->local.ip));
        cnetp->protocol = snetp->protocol;
        cnetp->recvproc = snetp->recvproc;
        cnetp->closeproc = snetp->closeproc;
        cnetp->connectproc = snetp->connectproc;
        // call accept user callback
        if (snetp->acceptproc) {
            snetp->acceptproc(clop);
        }
        // save seq to link context
        nis_cntl(clink, NI_SETCTX, &clop->seq);
    }

    lobj_derefer(slop);
}

static void __netobj_tcp_recvdata(HTCPLINK link, const void *data, unsigned int size)
{
    lobj_pt lop;
    int64_t *seq = NULL;
    struct netobj *netp;
    
    nis_cntl(link, NI_GETCTX, &seq);
    if (!seq) {
        return;
    }

    lop = lobj_refer_byseq(*seq);
    if (!lop) {
        return;
    }

    netp = lobj_body(struct netobj *, lop);
    if (netp->recvproc) {
        netp->recvproc(lop, data, size);
    }

    lobj_derefer(lop);
}

static void __netobj_tcp_connected(HTCPLINK link)
{
    lobj_pt lop;
    int64_t *seq = NULL;
    struct netobj *netp;
    
    nis_cntl(link, NI_GETCTX, &seq);
    if (!seq) {
        return;
    }

    lop = lobj_refer_byseq(*seq);
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
    int64_t *seq = NULL;
    struct netobj *netp;
    
    nis_cntl(link, NI_GETCTX, &seq);
    if (!seq) {
        return;
    }

    lop = lobj_refer_byseq(*seq);
    if (!lop) {
        return;
    }

    netp = lobj_body(struct netobj *, lop);
    if (netp->closeproc) {
        netp->closeproc(lop);
    }

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

void __netobj_free(struct lobj *lop)
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

int __netobj_write(struct lobj *lop, const void *data, size_t n)
{
    struct netobj *netp;

    netp = lobj_body(struct netobj *, lop);
    if (netp->link == INVALID_HTCPLINK) {
        return -1;
    }

    if (netp->protocol == JCFG_PROTO_TCP) {
        return tcp_write(netp->link, data, n, NULL);
    } else if (netp->protocol == JCFG_PROTO_UDP) {
        ;
    } else {
        ;
    }

    return 0;
}

void netobj_create(const jconf_net_pt jnetcfg)
{
    lobj_pt lop;
    struct lobj_fx fx = {
        .freeproc = __netobj_free,
        .referproc = NULL,
        .writeproc = __netobj_write,
    };
    struct netobj *netp;
    nsp_status_t status;

    if (!jnetcfg) {
        return;
    }

    // create object first
    lop = lobj_create(jnetcfg->name, jnetcfg->module, sizeof(struct netobj), jnetcfg->contextsize, &fx);
    if (!lop) {
        return;
    }
    netp = lobj_body(struct netobj *, lop);

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
        __netobj_parse_endpoint(jnetcfg->listen, &netp->listen);
        __netobj_parse_endpoint(jnetcfg->remote, &netp->remote);
        __netobj_parse_endpoint(jnetcfg->local, &netp->local);

        // pass callback function from config to object
        netp->recvproc = lobj_dlsym(lop, jnetcfg->recvproc);
        netp->acceptproc = lobj_dlsym(lop, jnetcfg->acceptproc);
        netp->closeproc = lobj_dlsym(lop, jnetcfg->closeproc);
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

            nis_cntl(netp->link, NI_SETCTX, &lop->seq);
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
    lobj_destroy(lop->name);
}
