#ifndef RBCODECPLATFORM_H_INCLUDED
#define RBCODECPLATFORM_H_INCLUDED

/* assert */
#include <assert.h>

/* O_RDONLY, O_WRONLY, O_CREAT, O_APPEND */
#include <fcntl.h>

/* isdigit, islower, isprint, isspace, toupper */
#include <ctype.h>

/* memchr, memcmp, memcpy, memmove, memset, strcat, strchr, strcmp, strcpy,
 * strlen, strncmp, strrchr */
#include <string.h>

/* strcasecmp */
#include <strings.h>

/* abs, atoi, labs, rand */
#include <stdlib.h>

/* swap16, swap32 */
#include <byteswap.h>
#define swap16(x) bswap_16(x)
#define swap32(x) bswap_32(x)

/* hto{be,le}{16,32}, {be,le}toh{16,32}, ROCKBOX_{BIG,LITTLE}_ENDIAN */
#include <endian.h>
#define betoh16 be16toh
#define betoh32 be32toh
#define letoh16 le16toh
#define letoh32 le32toh
#if BYTE_ORDER == LITTLE_ENDIAN
#define ROCKBOX_LITTLE_ENDIAN
#else
#define ROCKBOX_BIG_ENDIAN
#endif

/* rbcodec_alloc, rbcodec_free */
static inline void *rbcodec_alloc(size_t size,
                                  void (*move)(void *from, void *to)) {
    return malloc(size);
}
static inline void rbcodec_free(void *ptr) {
    free(ptr);
}

/* filesize */
#include <sys/stat.h>
static inline off_t filesize(int fd) {
    struct stat st;
    fstat(fd, &st);
    return st.st_size;
}

/* snprintf */
#include <stdio.h>

/* debugf, logf */
#ifdef DEBUG
#define debugf(...) fprintf(stderr, __VA_ARGS__)
#define logf(...) do { fprintf(stderr, __VA_ARGS__); \
                       putc('\n', stderr);           \
                  } while (0)
#endif

#include "codecs.h"
struct rbcodec_codec_header {
    enum codec_status(*entry_point)(enum codec_entry_call_reason reason);
    enum codec_status(*run_proc)(void);
    struct codec_api **api;
};

#define CODEC_HEADER \
        const struct rbcodec_codec_header __header \
        __attribute__((visibility("default"))) = { \
        codec_start, codec_run, &ci };

/* Character encoding conversion */
typedef enum {
    ENCODING_NONE,
    ENCODING_BIG_5,
    ENCODING_DEFAULT,
    ENCODING_EUC_CN,
    ENCODING_ISO_8859_1,
    ENCODING_SJIS,
    ENCODING_UTF_16BE,
    ENCODING_UTF_16LE,
    ENCODING_UTF_8,
} encoding_t;

#include <iconv.h>
static inline char *decode_text(encoding_t encoding, const char *in, char *out, size_t in_len)
{
    const char *encoding_name;
    size_t inbytesleft = in_len, outbytesleft = 4 * in_len;

    switch (encoding) {
    case ENCODING_BIG_5:      encoding_name = "BIG5";       break;
    case ENCODING_EUC_CN:     encoding_name = "EUC-CN";     break;
    case ENCODING_ISO_8859_1: encoding_name = "ISO-8859-1"; break;
    case ENCODING_SJIS:       encoding_name = "SJIS";       break;
    case ENCODING_UTF_16BE:   encoding_name = "UTF-16BE";   break;
    case ENCODING_UTF_16LE:   encoding_name = "UTF-16LE";   break;
    default: /* fallthrough */
    case ENCODING_UTF_8:      encoding_name = "UTF-8";      break;
    }

    iconv_t cd = iconv_open("UTF-8", encoding_name);
    iconv(cd, (char**)&in, &inbytesleft, &out, &outbytesleft);
    iconv_close(cd);
    return out + 4 * in_len - outbytesleft;
}

#endif
