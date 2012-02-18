#ifndef RBCODECPLATFORM_H_INCLUDED
#define RBCODECPLATFORM_H_INCLUDED

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

/* abs, atoi, labs, rand */
#include <stdlib.h>

/* debugf */
#include "debug.h"

/* logf */
#include "logf.h"

/* clip_sample_16 */
#include "dsp-util.h"
#define HAVE_CLIP_SAMPLE_16

void *rbcodec_alloc(size_t size, void (*move)(void *from, void *to));
void rbcodec_free(void *ptr);

/* Character encoding conversion */
#include "rbunicode.h"
typedef enum codepages encoding_t;
#define ENCODING_DEFAULT    -1
#define ENCODING_ISO_8859_1 ISO_8859_1
#define ENCODING_SJIS       SJIS
#define ENCODING_EUC_CN     GB_2312
#define ENCODING_BIG_5      BIG_5
#define ENCODING_UTF_8      UTF_8
#define ENCODING_NONE       NUM_CODEPAGES
#define ENCODING_UTF_16BE   NUM_CODEPAGES+1
#define ENCODING_UTF_16LE   NUM_CODEPAGES+2
static inline char *decode_text(encoding_t encoding, const char *in, char *out,
                                size_t in_bytes)
{
    if (encoding == ENCODING_UTF_16BE)
        return utf16BEdecode(in, out, in_bytes/2);
    else if (encoding == ENCODING_UTF_16LE)
        return utf16LEdecode(in, out, in_bytes/2);
    else
        return iso_decode(in, out, encoding, in_bytes);
}

/* struct lc_header */
#include "load_code.h"

/* codec header */
#include "codecs.h"
struct codec_header {
    struct lc_header lc_hdr; /* must be first */
    enum codec_status(*entry_point)(enum codec_entry_call_reason reason);
    enum codec_status(*run_proc)(void);
    struct codec_api **api;
};

#ifdef CODEC
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
/* plugin_* is correct, codecs use the plugin linker script */
extern unsigned char plugin_start_addr[];
extern unsigned char plugin_end_addr[];
/* decoders */
#define CODEC_HEADER \
        const struct codec_header __header \
        __attribute__ ((section (".header")))= { \
        { CODEC_MAGIC, TARGET_ID, CODEC_API_VERSION, \
        plugin_start_addr, plugin_end_addr }, codec_start, \
        codec_run, &ci };
/* encoders */
#define CODEC_ENC_HEADER \
        const struct codec_header __header \
        __attribute__ ((section (".header")))= { \
        { CODEC_ENC_MAGIC, TARGET_ID, CODEC_API_VERSION, \
        plugin_start_addr, plugin_end_addr }, codec_start, \
        codec_run, &ci };

#else /* def SIMULATOR */
/* decoders */
#define CODEC_HEADER \
        const struct codec_header __header \
        __attribute__((visibility("default"))) = { \
        { CODEC_MAGIC, TARGET_ID, CODEC_API_VERSION, NULL, NULL }, \
        codec_start, codec_run, &ci };
/* encoders */
#define CODEC_ENC_HEADER \
        const struct codec_header __header = { \
        { CODEC_ENC_MAGIC, TARGET_ID, CODEC_API_VERSION, NULL, NULL }, \
        codec_start, codec_run, &ci };
#endif /* SIMULATOR */
#endif /* CODEC */

#endif
