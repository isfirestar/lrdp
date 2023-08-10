#include "ttyobj.h"

#include <fcntl.h>
#include <unistd.h>
#include <termios.h>

struct ttyobj
{
    int fd;
    struct termios options;
    char device[128];
    struct aeEventLoop *el;
};

/* this macro use to convert visibility databit value to linux databit definition */
#define __tyyobj_convert_databit_posix(databit)     CS##databit
// databit map table
static const struct {
    unsigned int databit;
    unsigned int posix;
} __ttyobj_databit_map[] = {
    { 5, __tyyobj_convert_databit_posix(5) },
    { 6, __tyyobj_convert_databit_posix(6) },
    { 7, __tyyobj_convert_databit_posix(7) },
    { 8, __tyyobj_convert_databit_posix(8) }
};
static int __ttyobj_set_databit(unsigned int databit, struct termios *options)
{
    unsigned int i;
    for (i = 0; i < sizeof(__ttyobj_databit_map) / sizeof(__ttyobj_databit_map[0]); i++) {
        if (__ttyobj_databit_map[i].databit == databit) {
            options->c_cflag &= ~CSIZE;
            options->c_cflag |= __ttyobj_databit_map[i].posix;
            return 1;
        }
    }

    return 0;
}

/* this macro use to convert visibility baudrate value to linux baudrate definition macro */
#define __ttyobj_convert_baudrate_posix(baudrate)   B##baudrate
// baudrate map table
static const struct {
    unsigned int baudrate;
    unsigned int posix;
} __ttyobj_baudrate_map[] = {
    { 50, __ttyobj_convert_baudrate_posix(50) },
    { 75, __ttyobj_convert_baudrate_posix(75) },
    { 110, __ttyobj_convert_baudrate_posix(110) },
    { 134, __ttyobj_convert_baudrate_posix(134) },
    { 150, __ttyobj_convert_baudrate_posix(150) },
    { 200, __ttyobj_convert_baudrate_posix(200) },
    { 300, __ttyobj_convert_baudrate_posix(300) },
    { 600, __ttyobj_convert_baudrate_posix(600) },
    { 1200, __ttyobj_convert_baudrate_posix(1200) },
    { 1800, __ttyobj_convert_baudrate_posix(1800) },
    { 2400, __ttyobj_convert_baudrate_posix(2400) },
    { 4800, __ttyobj_convert_baudrate_posix(4800) },
    { 9600, __ttyobj_convert_baudrate_posix(9600) },
    { 19200, __ttyobj_convert_baudrate_posix(19200) },
    { 38400, __ttyobj_convert_baudrate_posix(38400) },
    { 57600, __ttyobj_convert_baudrate_posix(57600) },
    { 115200, __ttyobj_convert_baudrate_posix(115200) },
    { 230400, __ttyobj_convert_baudrate_posix(230400) },
    { 460800, __ttyobj_convert_baudrate_posix(460800) },
    { 500000, __ttyobj_convert_baudrate_posix(500000) },
    { 576000, __ttyobj_convert_baudrate_posix(576000) },
    { 921600, __ttyobj_convert_baudrate_posix(921600) },
    { 1000000, __ttyobj_convert_baudrate_posix(1000000) },
    { 1152000, __ttyobj_convert_baudrate_posix(1152000) },
    { 1500000, __ttyobj_convert_baudrate_posix(1500000) },
    { 2000000, __ttyobj_convert_baudrate_posix(2000000) },
    { 2500000, __ttyobj_convert_baudrate_posix(2500000) },
    { 3000000, __ttyobj_convert_baudrate_posix(3000000) },
    { 3500000, __ttyobj_convert_baudrate_posix(3500000) },
    { 4000000, __ttyobj_convert_baudrate_posix(4000000) },
};
static int __ttyobj_set_baudrate(unsigned int baudrate, struct termios *options)
{
    unsigned int i;
    for (i = 0; i < sizeof(__ttyobj_baudrate_map) / sizeof(__ttyobj_baudrate_map[0]); i++) {
        if (__ttyobj_baudrate_map[i].baudrate == baudrate) {
            cfsetispeed(options, __ttyobj_baudrate_map[i].posix);
            cfsetospeed(options, __ttyobj_baudrate_map[i].posix);
            return 1;
        }
    }

    return 0;
}

static void __ttyobj_on_free(struct lobj *lop, void *context, size_t ctxsize)
{
    struct ttyobj *ttyp;

    ttyp = lobj_body(struct ttyobj *, lop);

    if (!ttyp) {
        return;
    }

    if (ttyp->el) {
        aeDeleteFileEvent(ttyp->el, ttyp->fd, AE_READABLE);
    }

    if (ttyp->fd > 0) {
        close(ttyp->fd);
    }
}

static int __ttyobj_write(struct lobj *lop, const void *data, size_t n)
{
    struct ttyobj *ttyp;

    if (!lop || !data || !n) {
        return -1;
    }

    ttyp = lobj_body(struct ttyobj *, lop);

    return (ttyp->fd > 0) ? write(ttyp->fd, data, n) : -1;
}

lobj_pt ttyobj_create(const jconf_tty_pt jtty)
{
    lobj_pt lop;
    struct ttyobj *ttyp;
    const struct lobj_fx fx = {
        .freeproc = &__ttyobj_on_free,
        .writeproc = &__ttyobj_write,
        .vwriteproc = NULL,
        .readproc = NULL,
        .vreadproc = NULL,
    };

    if (!jtty) {
        return NULL;
    }

    lop = lobj_create(jtty->head.name, jtty->head.module, sizeof(*ttyp), jtty->head.ctxsize, &fx);
    if (!lop) {
        return NULL;
    }
    ttyp = lobj_body(struct ttyobj *, lop);
    // free and write proc can not be covered
    lobj_cover_fx(lop, NULL, NULL, jtty->head.vwriteproc, jtty->head.readproc, jtty->head.vreadproc, NULL);

    do {
        // open tty file
        strncpy(ttyp->device, jtty->device, sizeof(ttyp->device) - 1);
        ttyp->fd = open(ttyp->device, O_RDWR | O_NOCTTY | O_NDELAY);
        if (ttyp->fd < 0) {
            printf("open %s failed with code:%d\n", ttyp->device, errno);
            break;
        }

        if (!isatty(ttyp->fd)) {
            printf("framework cannot confirm the open file is a tty,code:%d\n", errno);
        }

        // Get the current options for the port
        tcgetattr(ttyp->fd, &ttyp->options);

        // I/O speed(bps)
        if (!__ttyobj_set_baudrate(jtty->baudrate, &ttyp->options)) {
            __ttyobj_set_baudrate(9600, &ttyp->options);
        }

        // Enable the receiver and set local mode
        ttyp->options.c_cflag |= (CLOCAL | CREAD);
        ttyp->options.c_cflag &= ~CSIZE;

        /* Parity None */
        if (0 == strcasecmp(jtty->parity, "None")) {
            ttyp->options.c_cflag &= ~PARENB;
            ttyp->options.c_iflag &= ~INPCK;
        } else if (0 == strcasecmp(jtty->parity, "Odd")) {
            ttyp->options.c_cflag |= PARENB;
            ttyp->options.c_cflag |= PARODD;
        } else if (0 == strcasecmp(jtty->parity, "Even")) {
            ttyp->options.c_cflag |= PARENB;
            ttyp->options.c_cflag &= ~PARODD;
        } else {
            printf("invalid parity %s\n", jtty->parity);
            break;
        }

        /* stop bits 1 */
        if (1 == jtty->stopbits) {
            ttyp->options.c_cflag &= ~CSTOPB;
        } else if (2 == jtty->stopbits) {
            ttyp->options.c_cflag |= CSTOPB;
        } else {
            printf("invalid stopbits %d\n", jtty->stopbits);
            break;
        }

        /* set flow control */
        if (0 == strcasecmp(jtty->flowcontrol, "None")) {
            ttyp->options.c_cflag &= ~CRTSCTS;
            ttyp->options.c_iflag &= ~(IXON | IXOFF | IXANY);
        } else if (0 == strcasecmp(jtty->flowcontrol, "Rts/Cts")) {
            ttyp->options.c_cflag |= CRTSCTS;
            ttyp->options.c_iflag &= ~(IXON | IXOFF | IXANY);
        } else if (0 == strcasecmp(jtty->flowcontrol, "Xon/Xoff")) {
            ttyp->options.c_cflag &= ~CRTSCTS;
            ttyp->options.c_iflag |= (IXON | IXOFF | IXANY);
        } else if (0 == strcasecmp(jtty->flowcontrol, "Dsr/Dtr")) {
            ttyp->options.c_cflag &= ~CRTSCTS;
            ttyp->options.c_iflag &= ~(IXON | IXOFF | IXANY);
        }else {
            printf("invalid flowcontrol %s\n", jtty->flowcontrol);
            break;
        }

        /* data bits */
        if (!__ttyobj_set_databit(jtty->databits, &ttyp->options)) {
            __ttyobj_set_databit(8, &ttyp->options);
        }

        /* set parity */
        ttyp->options.c_cc[VTIME] = 50;
        ttyp->options.c_cc[VMIN] = 0;
        ttyp->options.c_lflag &= ~(ECHO|ECHOE|ISIG|ICANON);
        ttyp->options.c_oflag &= ~OPOST;
        ttyp->options.c_iflag &= ~(IXON|IXOFF|INLCR|IGNCR|ICRNL);

        tcflush(ttyp->fd, TCIFLUSH); // Discards old data in the rx buffer

        // Set the new options for the port
        if (0 != tcsetattr(ttyp->fd, TCSANOW, &ttyp->options)) {
            printf("ttyobj: %s set options failed\n", ttyp->device);
            break;
        }

        return lop;
    } while(0);

    lobj_ldestroy(lop);
    return NULL;
}

static void __ttyobj_read(struct aeEventLoop *el, int fd, void *clientData, int mask)
{
    lobj_pt lop;
    struct ttyobj *ttyp;
    char buf[1024];
    ssize_t n;

    lop = (lobj_pt)clientData;

    n = read(fd, buf, sizeof(buf));
    if (n > 0) {
        lobj_fx_read(lop, buf, n);
    } else if (n < 0) {
        if (errno != EAGAIN) {
            aeDeleteFileEvent(el, fd, AE_READABLE);
            ttyp = lobj_body(struct ttyobj *, lop);
            printf("ttyobj: %s read error: %s\n", ttyp->device, strerror(errno));
        }
    } else {
        ;
    }
}

void ttyobj_add_file(struct aeEventLoop *el, lobj_pt lop)
{
    struct ttyobj *ttyp;

    ttyp = lobj_body(struct ttyobj *, lop);
    ttyp->el = el;
    if (ttyp->fd > 0) {
        aeCreateFileEvent(el, ttyp->fd, AE_READABLE, &__ttyobj_read, lop);
    }
}


