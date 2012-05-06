#ifndef RBCODECPLATFORM_H_INCLUDED
#define RBCODECPLATFORM_H_INCLUDED
#if 0
/* assert */
#include <assert.h>

/* isdigit, islower, isprint, isspace, toupper */
#include <ctype.h>

/* {UCHAR,USHRT,UINT,ULONG,SCHAR,SHRT,INT,LONG}_{MIN,MAX} */
#include <limits.h>

/* memchr, memcmp, memcpy, memmove, memset, strcasecmp, strcat, strchr, strcmp,
 * strcpy, strlen, strncmp, strrchr, strlcpy */
#include <string.h>
#include "string-extra.h"

/* snprintf */
#include <stdio.h>
#endif
/* abs, atoi, labs, rand */
#include <stdlib.h>
#if 0
/* debugf */
#include "debug.h"

/* logf */
#include "logf.h"

/* clip_sample_16 */
#include "dsp-util.h"
#define HAVE_CLIP_SAMPLE_16
#endif

bool tdspeed_alloc_buffers(int32_t **buffers, const int *buf_s, int nbuf);
void tdspeed_free_buffers(int32_t **buffers, int nbuf);

#endif

