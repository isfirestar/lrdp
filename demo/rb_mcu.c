#include "rb_mcu.h"

#include "lobj.h"

#include "ifos.h"
#include "naos.h"

#include "redis/hiredis.h"

#include <stdio.h>

int rb_mcu_init(lobj_pt lop, int argc, char **argv)
{
    printf("[%d] rb_mcu_init\n", ifos_gettid());
#if 0
    redisContext *c;
    void *objctx;
    size_t ctxsize;

    printf("[%d] motion_muc_initial\n", ifos_gettid());

    objctx = NULL;
    ctxsize = lobj_get_context(lop, &objctx);
    if (ctxsize == 0 || !objctx) {
        printf("[%d] motion_muc_initial context size is not sizeof(redisContext *)\n", ifos_gettid());
        return -1;
    }

    // initialize redis connection, connect to localhost:6379
    c = redisConnect("127.0.0.1", 6379);
    if (c == NULL || c->err) {
        if (c) {
            printf("Connection error: %s\n", c->errstr);
            redisFree(c);
        } else {
            printf("Connection error: can't allocate redis context\n");
        }
        return -1;
    }

    // save redis connection to mainloop object context
    memcpy(objctx, &c, sizeof(redisContext *));
#endif
    return 0;
}

static unsigned char __rb_calculate_mcu_checksum(const unsigned char *origin, unsigned int size)
{
    unsigned int i;
    unsigned char *p = (unsigned char *)origin;
    unsigned char checksum = 0;

    if (!origin || 0 == size || size > RB_MCU_PACKAGE_SIZE) {
        return 0;
    }

    for (i = 0; i < size; i++) {
        checksum += *p++;
    }

    return checksum;
}

static void __rb_write_uart(lobj_pt lop, float xVelocity)
{
    unsigned char tx_buffer[RB_MCU_PACKAGE_SIZE] = { 0 };
    short vx;
    unsigned char mcu_command;

    vx = 0;
    
    if (is_float_zero(xVelocity)) {
        mcu_command = SET_XY_STOP;
        vx = 0;
    } else {
        mcu_command = SET_KEY_XY_CMD;
        vx = xVelocity * 1000;
    }

    if (vx < 0) {
        vx = TRACK_KEY_XY_SPEED_NEGATIVE_PROC(vx);
    }

    tx_buffer[SERIAL_CMD_HEADER] = RB_MCU_PROTO_HEAD;
    tx_buffer[SERIAL_CMD] = mcu_command;
    tx_buffer[SERIAL_DATA_1] = GET_HIGH_8_BIT_FROM_2_BYTE_DATA(vx);
    tx_buffer[SERIAL_DATA_2] = GET_LOW_8_BIT_FROM_2_BYTE_DATA(vx);
    tx_buffer[SERIAL_DATA_3] = GET_HIGH_8_BIT_FROM_2_BYTE_DATA(0);
    tx_buffer[SERIAL_DATA_4] = GET_LOW_8_BIT_FROM_2_BYTE_DATA(0);
    tx_buffer[SERIAL_CHECK] = __rb_calculate_mcu_checksum(&tx_buffer[1], 9);

    // if MCU object are correct, we send data to serial port, otherwise, we only print data buffer
    if (lop) {
        lobj_write(lop, tx_buffer, sizeof(tx_buffer));
    } else {
        naos_hexdump(tx_buffer, sizeof(tx_buffer), 16, NULL);
    }
}

static void __rb_set_velocity(float xVelocity)
{
    lobj_pt lop;

    printf("[%d] __rb_set_velocity %f\n", ifos_gettid(), xVelocity);

    // get mcu tty object
    lop = lobj_refer("motionmcu");
    if (!lop) {
        printf("[%d] __rb_set_velocity motionmcu object is NULL\n", ifos_gettid());
    }

    __rb_write_uart(lop, xVelocity);
    lobj_derefer(lop);
}


/*
"lwps" : {
    "updatev" : {
        "module" : "librbmcu.so",
        "execproc" : "rb_mcu_on_velocity_update",
        "stacksize" : 1024,
        "priority" : 0,
        "contextsize" : 0,
        "affinity" : 0
    }
},
*/
void rb_mcu_on_velocity_update(lobj_pt lop, const char *channel, const char *pattern, const char *message, size_t len)
{
    if (pattern && message) {
        if (0 == strcmp("motion.v_x", pattern)) {
            __rb_set_velocity(atof(message));
        } else {
            printf("pattern missmatch.\n");
        }
    }
}

#if 0
void *rb_mcu_on_velocity_update(lobj_pt lop)
{
    void *objctx;
    size_t ctxsize;
    redisContext *c;
    lobj_pt mllop;
    redisReply *reply;
    const char *subscribeCommand[3];
    size_t subscribeCommandLen[3];

    // fetch redis connection from mainloop object context
    // mainloop object shall obtain by @lobj_refer
    mllop = lobj_refer("mainloop");
    if (!mllop) {
        printf("[%d] rb_mcu_on_velocity_update mainloop object is NULL\n", ifos_gettid());
        return NULL;
    }
    objctx  =NULL;
    ctxsize = lobj_get_context(mllop, &objctx);
    if (ctxsize == 0 || !objctx) {
        printf("[%d] rb_mcu_on_velocity_update mainloop object context is NULL\n", ifos_gettid());
        lobj_derefer(mllop);
        return NULL;
    }

    // redis connection are saved in mainloop object context
    memcpy(&c, objctx, sizeof(redisContext *));

    // mainloop object are no longer needed
    lobj_derefer(mllop);

    // if redis connection is NULL, return
    if (!c) {
        printf("[%d] rb_mcu_on_velocity_update redis connection is NULL\n", ifos_gettid());
        return NULL;
    }

    // ok, subscribe to channel "motion*" from redis-server and print received message
    subscribeCommand[0] = "PSUBSCRIBE";
    subscribeCommandLen[0] = strlen("PSUBSCRIBE");
    subscribeCommand[1] = "v*";
    subscribeCommandLen[1] = strlen("v*");
    redisAppendCommandArgv(c, 2, subscribeCommand, subscribeCommandLen);
    while (1) {
        if (redisGetReply(c, (void **)&reply) != REDIS_OK) {
            printf("[%d] rb_mcu_on_velocity_update redisGetReply error\n", ifos_gettid());
            break;
        }
        if (!reply) {
            printf("[%d] rb_mcu_on_velocity_update reply is NULL\n", ifos_gettid());
            break;
        }
        if (reply->type == REDIS_REPLY_ARRAY) {
            if (reply->elements > 3) {
                if (reply->element[0]->type == REDIS_REPLY_STRING) {
                    printf("[%d] redis command: %s\n", ifos_gettid(), reply->element[0]->str);
                }
                if (reply->element[1]->type == REDIS_REPLY_STRING) {
                    printf("[%d] channel: %s\n", ifos_gettid(), reply->element[1]->str);
                }
                if (reply->element[2]->type == REDIS_REPLY_STRING) {
                    if (0 == strcmp("vx", reply->element[2]->str)) {
                        __rb_set_velocity(atof(reply->element[3]->str));
                    }
                }
            }
        }
        freeReplyObject(reply);
    }

    return NULL;
}
#endif

void rb_mcu_on_recvdata(lobj_pt lop, const void *data, unsigned int size)
{
    printf("recv incoming mcu data:\n");
    naos_hexdump(data, size, 16, NULL);
}
