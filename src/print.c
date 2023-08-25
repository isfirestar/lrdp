#include "print.h"

#include "zmalloc.h"

#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <execinfo.h>
#include <stdlib.h>

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
}

void generical_print(enum genericPrintLevel level, const char *file, int line, const char *func, const char *fmt, ...)
{
  char buffer[1024], *pbuff;
  int pos, total;
  va_list ap,aq;
  const char *slash;
	static const char *genericPrintLevelText[4] = { "Info", "Warning", "Error", "Fatal" };
	
	if (level >= kPrintMaximumFunction || !fmt) {
		return;
	}

  slash = strrchr(file, '/');
  slash = (NULL == slash) ? file : (slash + 1);
  total = 0;
  pbuff = buffer;

  va_start(ap,fmt);
  va_copy(aq, ap);

	total =  snprintf(NULL, 0,"[%s]", genericPrintLevelText[level]);
	if (file) total += snprintf(NULL, 0,"[%s:%d]", slash, line);
	if (func) total += snprintf(NULL, 0, "[%s]", func);
	total += 3; // space \n \0
  total += vsnprintf(NULL, 0, fmt, aq);
	
  do {
    if (total >= sizeof(buffer)) {
      pbuff = (char *)ztrymalloc(total + 1);
      if (!pbuff) {
        break;
      }
    }
    
    pos = snprintf(pbuff, total, "[%s]", genericPrintLevelText[level]);
		if (file) pos += snprintf(pbuff + pos, total - pos, "[%s:%d]", slash, line);
		if (func) pos += snprintf(pbuff + pos, total - pos, "[%s]", func);
		pos += snprintf(pbuff + pos, total - pos, " ");
    pos += vsnprintf(pbuff + pos, total - pos, fmt, ap);
		pos += snprintf(pbuff + pos, total - pos, "\n");
		write(level < kPrintError ? 1 : 2, pbuff, pos);
  } while(0);

  if (pbuff != &buffer[0]) {
    zfree(pbuff);
  }
  
  va_end(aq);
  va_end(ap);
  
  // abort if fatal
  if (level >= kPrintFatal) {
    __trace_on_fatal();
    abort();
  }
}

