#pragma once

#include <string.h>

enum endpoint_type
{
    ENDPOINT_TYPE_SYSERR = -1,
    ENDPOINT_TYPE_ILLEGAL  = 0,
    ENDPOINT_TYPE_IPv6,
    ENDPOINT_TYPE_IPv4,
    ENDPOINT_TYPE_UNIX_DOMAIN,
};

struct endpoint_string
{
    char inetstr[128];
};
typedef struct endpoint_string endpoint_string_t, *endpoint_string_pt;

extern endpoint_string_pt netobj_create_endpoint_string(const char *ipv4, unsigned short port, endpoint_string_pt epstr);

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

extern enum endpoint_type netobj_parse_endpoint(const endpoint_string_pt epstr, endpoint_pt ept);
