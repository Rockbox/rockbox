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

#ifdef CPU_ARM
#define sample_output_1ch_1ch_2i_shr    sample_output_mono
#define sample_output_1ch_2ch_2i_shr    sample_output_mono
#define sample_output_2ch_2ch_2i_shr    sample_output_stereo
#endif

#ifdef CPU_COLDFIRE
#define sample_output_1ch_1ch_2i_shr    sample_output_mono_m
#define sample_output_1ch_2ch_2i_shr    sample_output_mono
#define sample_output_2ch_2ch_2i_shr    sample_output_stereo
#endif

#ifdef CONFIG_PCM_MULTISIZE
#define CONFIG_DSP_OUT_MULTISIZE
#endif

#if (CONFIG_PCM_FORMAT_CAPS & PCM_FORMAT_CAP_3_BYTE_CAPS)
#define CONFIG_DSP_OUT_3BYTE_INT
#endif

#if (CONFIG_PCM_FORMAT_CAPS & PCM_FORMAT_CAP_4_BYTE_CAPS)
#define CONFIG_DSP_OUT_4BYTE_INT
#endif

#define NATIVE_DEPTH                    PCM_DMA_T_DEPTH
#define DSP_OUT_DEFAULT_PCM_FORMAT      PCM_DMA_T_FORMAT_CODE
#define DSP_OUT_DEFAULT_PCM_CHANNELS    PCM_DMA_T_CHANNELS
#define DSP_OUT_DEFAULT_PCM_SIZE        PCM_DMA_T_SIZE
#define DSP_OUT_DEFAULT_PCM_BITS        PCM_DMA_T_DEPTH
#define DSP_OUT_MAX_PCM_CHANNELS        2

bool tdspeed_alloc_buffers(int32_t **buffers, const int *buf_s, int nbuf);
void tdspeed_free_buffers(int32_t **buffers, int nbuf);

#endif

