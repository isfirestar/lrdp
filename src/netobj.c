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

    if (0 == epstr[0]) {
        strncpy(epo->ip, "0.0.0.0", sizeof(epo->ip) - 1);
        epo->port = 0;
        epo->inet = 0;
        return NSP_STATUS_SUCCESSFUL;
    }

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
