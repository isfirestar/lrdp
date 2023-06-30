#pragma once

#include "jconf.h"
#include "lobj.h"

extern void redisobj_create(const jconf_redis_server_pt jredis_server_cfg);
extern void *redisobj_get_connection(lobj_pt lop);
