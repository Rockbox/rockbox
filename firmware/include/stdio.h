#ifndef _STDIO_H_
#define	_STDIO_H_

#define __need_size_t
#include <stddef.h>

#define __need___va_list
#include <stdarg.h>

#ifndef NULL
#define	NULL	0
#endif

#define	EOF	(-1)

#ifndef SEEK_SET
#define	SEEK_SET	0	/* set file offset to offset */
#endif
#ifndef SEEK_CUR
#define	SEEK_CUR	1	/* set file offset to current plus offset */
#endif
#ifndef SEEK_END
#define	SEEK_END	2	/* set file offset to EOF plus offset */
#endif

#define	TMP_MAX		26

#ifdef __GNUC__
#define __VALIST __gnuc_va_list
#else
#define __VALIST char*
#endif

int	_EXFUN(fprintf, (FILE *, const char *, ...));
int	_EXFUN(fscanf, (FILE *, const char *, ...));
int	_EXFUN(printf, (const char *, ...));
int	_EXFUN(vfprintf, (FILE *, const char *, __VALIST));
int	_EXFUN(vprintf, (const char *, __VALIST));
int	_EXFUN(vsprintf, (char *, const char *, __VALIST));

#endif /* _STDIO_H_ */
