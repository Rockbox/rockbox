#include <stdarg.h>
#include <stdio.h>

int rockbox_snprintf (char *buf, size_t size, const char *fmt, ...);
int rockbox_vsnprintf (char *buf, int size, const char *fmt, va_list ap);
int rockbox_fprintf (int fd, const char *fmt, ...);

#ifndef NO_REDEFINES_PLEASE
#define snprintf rockbox_snprintf
#define vsnprintf rockbox_vsnprintf
#define fprintf rockbox_fprintf
#endif
