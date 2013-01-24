/*
**	ficllocal.h
**
** Put all local settings here.  This file will always ship empty.
**
*/
#include "plugin.h"
#include "lib/stdio_compat.h"
#include <tlsf.h>

#define malloc tlsf_malloc
#define calloc tlsf_calloc
#define free tlsf_free
#define strcat rb->strcat
#define rename rb->rename
#define strlen rb->strlen
#define strcmp rb->strcmp
#define snprintf rb->snprintf
#define vsnprintf rb->vsnprintf
#define strcpy rb->strcpy
#define rand rb->rand
#define srand rb->srand
#define unlink rb->remove

#define __likely   LIKELY
#define __unlikely UNLIKELY

unsigned long strtoul(const char *str, char **endptr, int base);
