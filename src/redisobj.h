#pragma once

#include "jconf.h"
#include "lobj.h"
#include "ae.h"

extern void redisobj_create(const jconf_redis_server_pt jredis_server_cfg, aeEventLoop *el);
extern void redisobj_create_na(const jconf_redis_server_pt jredis_server_cfg, aeEventLoop *el);
