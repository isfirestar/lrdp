#include "rb_mcu.h"

#include "lobj.h"

#include "ifos.h"
#include "naos.h"

#include <stdio.h>

int rb_mcu_init(lobj_pt lop, int argc, char **argv)
{
    printf("[%d] rb_mcu_init\n", ifos_gettid());
    return 0;
}

void __rb_publish_velocity_feedback(float vx)
{
    lobj_pt publisher;
    char str_vx[5];
    const char *vdata[2];
    size_t vsize[2];

    publisher = lobj_refer("publisher");
    if (!publisher) {
        return;
    }

    vdata[0] = "feedback.v_x";
    vsize[0] = strlen("feedback.v_x");
    sprintf(str_vx, "%.02f", vx);
    vdata[1] = str_vx;
    vsize[1] = strlen(str_vx);
    lobj_vwrite(publisher, 2, (const void **)vdata, vsize);
    lobj_derefer(publisher);
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
    lobj_pt publisher;

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
        // in this case, we using setting velocity as the feedback
        __rb_publish_velocity_feedback(xVelocity);
    }
}

static void __rb_set_velocity(float xVelocity)
{
    lobj_pt lop;

    printf("[%d] __rb_set_velocity %f\n", ifos_gettid(), xVelocity);

    // get mcu tty object
    lop = lobj_refer("rb_motion_tty");
    if (!lop) {
        printf("[%d] __rb_set_velocity rb_motion_tty object is NULL\n", ifos_gettid());
    }

    __rb_write_uart(lop, xVelocity);
    lobj_derefer(lop);
}

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

void rb_mcu_on_recvdata(lobj_pt lop, const void *data, unsigned int size)
{
    printf("recv incoming mcu data:\n");
    naos_hexdump(data, size, 16, NULL);
}
