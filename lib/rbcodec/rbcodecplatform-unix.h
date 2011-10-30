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

#endif
