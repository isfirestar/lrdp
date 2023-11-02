#include "netobj.h"
#include "naos.h"
#include "zmalloc.h"

#include <string.h>

/* this function parse endpoint string to the endpoint structure object
 * the endpoint string format is: ip:port, note that we only support IPv4 now
 *  1. check the input and output parameter legal
 *  2. delimiter ':' to get ip and port
 *  3. verify ip and port string is legal
 *  4. copy ip string to endpoint structure "ip" filed
 *  5. convert port string to unsigned short and copy to endpoint structure "port" filed, use little endian
 *  6. convert ip string to unsigned int and copy to endpoint structure "inet" filed, use little endian
 */
enum endpoint_type netobj_parse_endpoint(const char *epstr, endpoint_pt ept)
{
    char *p;
    char *nextToken;
    char *tmpstr;
    size_t srclen;
    unsigned long byteValue;
    int i;

    if (unlikely(!ept) ) {
        return ENDPOINT_TYPE_ILLEGAL;
    }

    if (!epstr) {
        strncpy(ept->ipv4, "0.0.0.0", sizeof(ept->ipv4) - 1);
        ept->inet = 0;
        ept->port = 0;
        ept->eptype = ENDPOINT_TYPE_IPv4;
        return ENDPOINT_TYPE_IPv4;
    }

    if (0 == epstr[0]) {
        strncpy(ept->ipv4, "0.0.0.0", sizeof(ept->ipv4) - 1);
        ept->port = 0;
        ept->inet = 0;
        ept->eptype = ENDPOINT_TYPE_IPv4;
        return ENDPOINT_TYPE_IPv4;
    }

    if ( 0 == strncasecmp(epstr, "ipc:", 4) ) {
        strncpy(ept->domain, epstr, sizeof(ept->domain) - 1);
        ept->eptype = ENDPOINT_TYPE_UNIX_DOMAIN;
        return ENDPOINT_TYPE_UNIX_DOMAIN;
    }

    srclen = strlen(epstr);
    i = 0;

    tmpstr = (char *)ztrymalloc(srclen + 1);
    if ( unlikely(!tmpstr) ) {
        return ENDPOINT_TYPE_SYSERR;
    }
    strcpy(tmpstr, epstr);

    nextToken = NULL;
    while (NULL != (p = strtok_r(nextToken ? NULL : tmpstr, ":", &nextToken)) && i < 2) {
        if (i == 0) {
            if (!naos_is_legal_ipv4(p)) {
                zfree(tmpstr);
                strncpy(ept->domain, epstr, sizeof(ept->domain) - 1);
                ept->eptype = ENDPOINT_TYPE_UNIX_DOMAIN;
                return ENDPOINT_TYPE_UNIX_DOMAIN;
            }
            strcpy(ept->ipv4, p);
            ept->inet = naos_ipv4tou(p, kByteOrder_LittleEndian);
        } else if (i == 1) {
            byteValue = strtoul(p, NULL, 10);
            ept->port = (unsigned short)byteValue;
        }
        ++i;
    }

    zfree(tmpstr);
    ept->eptype = ENDPOINT_TYPE_IPv4;
    return ENDPOINT_TYPE_IPv4;
}
