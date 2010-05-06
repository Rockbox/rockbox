#ifndef _STDIO_H_
#define _STDIO_H_

#include <_ansi.h>

#define __need_size_t
#include <stddef.h>

#define __need___va_list
#include <stdarg.h>

#ifndef NULL
#define NULL    0
#endif

#define EOF     (-1)

#ifndef SEEK_SET
#define SEEK_SET        0       /* set file offset to offset */
#endif
#ifndef SEEK_CUR
#define SEEK_CUR        1       /* set file offset to current plus offset */
#endif
#ifndef SEEK_END
#define SEEK_END        2       /* set file offset to EOF plus offset */
#endif

#define TMP_MAX         26

#ifdef __GNUC__
#define __VALIST __gnuc_va_list
#else
#define __VALIST char*
#endif

int vsnprintf (char *buf, size_t size, const char *fmt, __VALIST ap);

int sprintf  (char *buf, const char *fmt, ...) ATTRIBUTE_PRINTF(2, 3);

int snprintf (char *buf, size_t size, const char *fmt, ...)
              ATTRIBUTE_PRINTF(3, 4);

/* callback function is called for every output character (byte) with userp and
 * should return 0 when ch is a char other than '\0' that should stop printing */
int vuprintf(int (*push)(void *userp, unsigned char data),
              void *userp, const char *fmt, __VALIST ap);

int sscanf(const char *s, const char *fmt, ...)
    ATTRIBUTE_SCANF(2, 3);

#ifdef SIMULATOR
typedef void FILE;
int vfprintf(FILE *stream, const char *format, __VALIST ap);
#ifdef WIN32
#define FILENAME_MAX 260 /* ugly hard-coded value of a limit that is set
                            in file.h */
#endif
#endif

#endif /* _STDIO_H_ */
