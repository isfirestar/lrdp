#pragma once

enum endpoint_type
{
    ENDPOINT_TYPE_SYSERR = -1,
    ENDPOINT_TYPE_ILLEGAL  = 0,
    ENDPOINT_TYPE_IPv6,
    ENDPOINT_TYPE_IPv4,
    ENDPOINT_TYPE_UNIX_DOMAIN,
};

struct endpoint
{
    enum endpoint_type eptype;
    union {
        struct {
            char ipv4[16];
            unsigned int inet;
            unsigned short port;
        };
        char domain[128];
    };
};
typedef struct endpoint endpoint_t, *endpoint_pt;

extern enum endpoint_type netobj_parse_endpoint(const char *epstr, endpoint_pt ept);
