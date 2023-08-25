#pragma once

enum genericPrintLevel
{
	kPrintInfo = 0,
	kPrintWarning,
	kPrintError,
	kPrintFatal,
	kPrintMaximumFunction,
};

extern void generical_print(enum genericPrintLevel level, const char *file, int line, const char *func, const char *fmt, ...);

#define lrdp_generic_info(fmt, ...) generical_print(kPrintInfo, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define lrdp_generic_warning(fmt, ...) generical_print(kPrintWarning, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define lrdp_generic_error(fmt, ...) generical_print(kPrintError, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)
#define lrdp_generic_fatal(fmt, ...) generical_print(kPrintFatal, __FILE__, __LINE__, __FUNCTION__, fmt, ##__VA_ARGS__)

