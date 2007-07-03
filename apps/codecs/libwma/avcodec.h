#ifndef AVCODEC_H
#define AVCODEC_H

/**
 * @file avcodec.h
 * external api header.
 */

#include "common.h"
#include <sys/types.h> /* size_t */

/**
 * Required number of additionally allocated bytes at the end of the input bitstream for decoding.
 * this is mainly needed because some optimized bitstream readers read 
 * 32 or 64 bit at once and could read over the end<br>
 * Note, if the first 23 bits of the additional bytes are not 0 then damaged
 * MPEG bitstreams could cause overread and segfault
 */
#define FF_INPUT_BUFFER_PADDING_SIZE 8

/* memory */
void *av_malloc(unsigned int size);
void *av_mallocz(unsigned int size);
void *av_realloc(void *ptr, unsigned int size);
void av_free(void *ptr);
char *av_strdup(const char *s);
void __av_freep(void **ptr);
#define av_freep(p) __av_freep((void **)(p))
void *av_fast_realloc(void *ptr, unsigned int *size, unsigned int min_size);
/* for static data only */
/* call av_free_static to release all staticaly allocated tables */
void av_free_static(void);
void *__av_mallocz_static(void** location, unsigned int size);
#define av_mallocz_static(p, s) __av_mallocz_static((void **)(p), s)

/* av_log API */

#include <stdarg.h>

#define AV_LOG_ERROR 0
#define AV_LOG_INFO 1
#define AV_LOG_DEBUG 2

extern void av_log(int level, const char *fmt, ...);
extern void av_vlog(int level, const char *fmt, va_list);
extern int av_log_get_level(void);
extern void av_log_set_level(int);
extern void av_log_set_callback(void (*)(int, const char*, va_list));

#undef  AV_LOG_TRAP_PRINTF
#ifdef AV_LOG_TRAP_PRINTF
#define printf DO NOT USE
#define fprintf DO NOT USE
#undef stderr
#define stderr DO NOT USE
#endif

#endif /* AVCODEC_H */
