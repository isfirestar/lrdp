#pragma once

#include "jconf.h"

struct mainloop
{
    nsp_status_t (*preinitproc)(int argc, char **argv);
    void (*postinitproc)(void *context, unsigned int ctxsize);
    void (*exitproc)(void);
    void (*timerproc)(void *context, unsigned int ctxsize);
    unsigned int interval;
    unsigned int ctxsize;
    unsigned char *context;
};
typedef struct mainloop mainloop_t, *mainloop_pt;

extern mainloop_pt mloop_initial(const jconf_entry_pt jentry);
extern nsp_status_t mloop_exec_preinit(mainloop_pt mloop, int argc, char **argv);
extern void mloop_exec_postinit(mainloop_pt mloop);
extern void mloop_exec_exit(mainloop_pt mloop);
extern int mloop_exec_on_timer(mainloop_pt mloop);
