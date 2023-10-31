#include "print.h"

#include "zmalloc.h"
#include "clock.h"
#include "ifos.h"

#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <execinfo.h>

static int __print_access = LRDP_PRINT_LEVEL | LRDP_PRINT_FILE | LRDP_PRINT_FUNC | LRDP_PRINT_TIMESTAMP | LRDP_PRINT_TID;

static void __trace_on_fatal()
{
    void *buffer[100];
    int nptrs, i;
    char **strings;

    nptrs = backtrace(buffer, 100);
    strings = backtrace_symbols(buffer, nptrs);
    if (!strings) {
        return;
    }

    for (i = 0; i < nptrs; i++) {
        printf("%s\n", strings[i]);
    }

    free(strings);
    abort();
}

void generical_print(enum genericPrintLevel level, const char *file, int line, const char *func, const char *fmt, ...)
{
    char buffer[1024], *pbuff;
    int pos, total;
    va_list ap,aq;
    const char *slash;
    datetime_t currst;
    static const char *genericPrintLevelText[5] = { "Info", "Warning", "Error", "Fatal", "Raw" };

    if (level < kPrintInfo ||  level >= kPrintMaximumFunction || (0 == __print_access && !fmt)) {
        return;
    }

    total = 0;
    pbuff = buffer;

    va_start(ap,fmt);
    va_copy(aq, ap);

    total = 16; // some other character like space, colon, \n, etc.
    if (__print_access & LRDP_PRINT_LEVEL) {
        total  +=  snprintf(NULL, 0,"[%s]", genericPrintLevelText[level]);
    }
    if (__print_access & LRDP_PRINT_TID) {
        total += snprintf(NULL, 0, "[%d]", ifos_gettid());
    }
    if (file && (__print_access & LRDP_PRINT_FILE)) {
        slash = strrchr(file, '/');
        slash = (NULL == slash) ? file : (slash + 1);
        total += snprintf(NULL, 0,"[%s:%d]", slash, line);
    }
    if (func && (__print_access & LRDP_PRINT_FUNC)) {
        total += snprintf(NULL, 0, "[%s]", func);
    }
    if (__print_access & LRDP_PRINT_TIMESTAMP) {
        clock_systime(&currst);
        total += snprintf(NULL, 0, "[%02u:%02u:%02u %04u]", currst.hour, currst.minute, currst.second, (unsigned int)(currst.low / 10000));
    }
    if (fmt) {
        total += vsnprintf(NULL, 0, fmt, ap);
    }

    do {
        if (total > sizeof(buffer)) {
            pbuff = (char *)ztrymalloc(total + 1);
            if (!pbuff) {
                break;
            }
        }

        pos = 0;
        if (__print_access & LRDP_PRINT_LEVEL) {
            pos += snprintf(pbuff, total, "[%s]", genericPrintLevelText[level]);
        }
        if (__print_access & LRDP_PRINT_TID) {
            pos += snprintf(pbuff + pos, total, "[%d]", ifos_gettid());
        }
        if (file && (__print_access & LRDP_PRINT_FILE)) {
            pos += snprintf(pbuff + pos, total - pos, "[%s:%d]", slash, line);
        }
        if (func && (__print_access & LRDP_PRINT_FUNC)) {
            pos += snprintf(pbuff + pos, total - pos, "[%s]", func);
        }
        if (__print_access & LRDP_PRINT_TIMESTAMP) {
            pos += snprintf(pbuff + pos, total - pos, "[%02u:%02u:%02u %04u]", currst.hour, currst.minute, currst.second, (unsigned int)(currst.low / 10000));
        }
        pos += snprintf(pbuff + pos, total - pos, ": ");
        if (fmt) {
            pos += vsnprintf(pbuff + pos, total - pos, fmt, aq);
        }
        pos += snprintf(pbuff + pos, total - pos, "\n");

        write(level < kPrintError ? 1 : 2, pbuff, pos);
    } while(0);

    if (pbuff != &buffer[0] && pbuff) {
        zfree(pbuff);
    }

    va_end(aq);
    va_end(ap);

    // abort if fatal
    if (level >= kPrintFatal) {
        __trace_on_fatal();
    }
}

int lrdp_get_print_access()
{
    return __atomic_load_n(&__print_access, __ATOMIC_SEQ_CST);
}

void lrdp_set_print_access(int access)
{
    __atomic_exchange_n(&__print_access, access, __ATOMIC_SEQ_CST);
}
