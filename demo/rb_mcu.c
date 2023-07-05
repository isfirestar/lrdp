#include "rb_mcu.h"

#include "lobj.h"

#include "ifos.h"
#include "naos.h"
#include "nav.h"

#include <stdio.h>

int rb_mcu_init(lobj_pt lop, int argc, char **argv)
{
    printf("[%d] rb_mcu_init\n", ifos_gettid());
    return 0;
}

void __rb_publish_velocity_feedback(int vx)
{
    lobj_pt publisher;
    const void *vdata[2];
    size_t vsize[2];
    struct nav_trace trace;

    publisher = lobj_refer("publisher");
    if (!publisher) {
        return;
    }

    trace.vx = vx;
    trace.vx_tx_timestamp = getMonotonicUs();

    vdata[0] = "feedback.v_x";
    vsize[0] = strlen("feedback.v_x");
    vdata[1] = &trace;
    vsize[1] = sizeof(trace);
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

static void __rb_write_uart(lobj_pt lop, int vx)
{
    unsigned char tx_buffer[RB_MCU_PACKAGE_SIZE] = { 0 };
    unsigned char mcu_command;
    lobj_pt publisher;

    if (is_float_zero(vx)) {
        mcu_command = SET_XY_STOP;
        vx = 0;
    } else {
        mcu_command = SET_KEY_XY_CMD;
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
        __rb_publish_velocity_feedback(vx);
    }
}

static void __rb_set_velocity(const struct nav_trace *trace)
{
    lobj_pt lop;

    printf("[%d] __rb_set_velocity %f\n", ifos_gettid(), (float)trace->vx / 1000);

    // get mcu tty object
    lop = lobj_refer("rb_motion_tty");
    __rb_write_uart(lop, trace->vx);
    lobj_derefer(lop);
}

void rb_mcu_on_velocity_update(lobj_pt lop, const char *channel, const char *pattern, const char *message, size_t len)
{
    if (pattern && message) {
        if (0 == strcmp("motion.v_x", pattern) && len == sizeof(struct nav_trace)) {
            __rb_set_velocity((const struct nav_trace *)message);
        } else {
            printf("pattern missmatch.\n");
        }
    }
}

void rb_mcu_on_recvdata(lobj_pt lop, void *data, unsigned int size)
{
    naos_hexdump(data, size, 16, NULL);
}
