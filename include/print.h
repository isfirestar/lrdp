#pragma once

#include <stdio.h>

enum genericPrintLevel
{
	kPrintInfo = 0,
	kPrintWarning,
	kPrintError,
	kPrintFatal,
	kPrintRaw,
	kPrintMaximumFunction,
};

extern void generical_print(enum genericPrintLevel level, const char *file, int line, const char *func, const char *fmt, ...);

#define lrdp_generic_info(fmt, ...) generical_print(kPrintInfo, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define lrdp_generic_warning(fmt, ...) generical_print(kPrintWarning, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define lrdp_generic_error(fmt, ...) generical_print(kPrintError, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define lrdp_generic_fatal(fmt, ...) generical_print(kPrintFatal, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define lrdp_generic_raw(fmt,...)	printf(fmt, ##__VA_ARGS__)

#define LRDP_PRINT_LEVEL			(1)
#define LRDP_PRINT_FILE				(2)
#define LRDP_PRINT_FUNC				(4)
#define LRDP_PRINT_TIMESTAMP		(8)

extern int lrdp_get_print_access();
extern void lrdp_set_print_access(int access);


