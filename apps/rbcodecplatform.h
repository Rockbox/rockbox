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

/* sample abstraction utilities */
#include "dsp-util.h"
#endif

#ifdef CPU_ARM
#define sample_output_1ch_1ch_2i    sample_output_mono
#define sample_output_1ch_2ch_2i    sample_output_mono
#define sample_output_2ch_2ch_2i    sample_output_stereo
#endif

#if 0//def CPU_COLDFIRE
#define sample_output_1ch_1ch_2i    sample_output_mono_m
#define sample_output_1ch_2ch_2i    sample_output_mono
#define sample_output_2ch_2ch_2i    sample_output_stereo
#endif

#ifdef CONFIG_PCM_MULTISIZE
/* Support multiple sample byte sizes, atm: 2, 3, 4 */
#define CONFIG_DSP_OUT_MULTISIZE
#endif

/* Support non-tee'd mono (ie. can output mono as mono) */
#define CONFIG_DSP_OUT_CHNUM

/* Sample size support defined by target PCM config */
#if (CONFIG_PCM_FORMAT_CAPS & PCM_FORMAT_CAP_2_BYTE_INT)
#define CONFIG_DSP_OUT_2BYTE_INT
#endif

#if (CONFIG_PCM_FORMAT_CAPS & PCM_FORMAT_CAP_3_BYTE_INT)
#define CONFIG_DSP_OUT_3BYTE_INT
#endif

#if (CONFIG_PCM_FORMAT_CAPS & PCM_FORMAT_CAP_4_BYTE_INT)
#define CONFIG_DSP_OUT_4BYTE_INT
#endif

/* Tell DSP how to interpret our PCM format specifications */
#define dsp_pcm_format_t                pcm_format_t
#define DSP_PCM_FORMAT_INIT(f)          (f)

#define DSP_OUT_DEFAULT_PCM_FORMAT      PCM_FORMAT_DMA_T

#define DSP_PCM_FORMAT_GET_CHNUM        PCM_FORMAT_GET_CHNUM
#define DSP_PCM_FORMAT_GET_BITS         PCM_FORMAT_GET_BITS
#define DSP_PCM_FORMAT_GET_SIZE         PCM_FORMAT_GET_SIZE 
/* not currently supported; return default support */
#define DSP_PCM_FORMAT_GET_USGN(f)      (false)
#define DSP_PCM_FORMAT_GET_FLT(f)       (false)  
#define DSP_PCM_FORMAT_GET_NI(f)        (false)   
#define DSP_PCM_FORMAT_GET_END(f)       (0)  

bool tdspeed_alloc_buffers(int32_t **buffers, const int *buf_s, int nbuf);
void tdspeed_free_buffers(int32_t **buffers, int nbuf);

#endif

