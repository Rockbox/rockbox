#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>
#include "platform.h"

#include "metadata.h"
#include "metadata_common.h"
#include "metadata_parsers.h"
#include "rbunicode.h"
#include "string-extra.h"

typedef enum {
    VTX_CHIP_AY = 0,		/* emulate AY */
    VTX_CHIP_YM			/* emulate YM */
} vtx_chiptype_t;

typedef enum {
    VTX_LAYOUT_MONO = 0,
    VTX_LAYOUT_ABC,
    VTX_LAYOUT_ACB,
    VTX_LAYOUT_BAC,
    VTX_LAYOUT_BCA,
    VTX_LAYOUT_CAB,
    VTX_LAYOUT_CBA,
    VTX_LAYOUT_CUSTOM
} vtx_layout_t;

typedef struct {
    vtx_chiptype_t chiptype;	/* Type of sound chip */
    vtx_layout_t layout;	/* stereo layout */
    uint loop;			/* song loop */
    uint chipfreq;		/* AY chip freq (1773400 for ZX) */
    uint playerfreq;		/* 50 Hz for ZX, 60 Hz for yamaha */
    uint year;			/* year song composed */
    char *title;		/* song title */
    char *author;		/* song author */
    char *from;			/* song from */
    char *tracker;		/* tracker */
    char *comment;		/* comment */
    uint regdata_size;		/* size of unpacked data */
    uint frames;		/* number of AY data frames */
} vtx_info_t;

/* reader */

typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;
typedef unsigned long  ulong;

#define VTX_STRING_MAX 254

typedef struct {
    uchar *ptr;
    uint size;
} reader_t;

reader_t reader;

void Reader_Init(void *pBlock) {
    reader.ptr = (uchar *) pBlock;
    reader.size = 0;
}

uint Reader_ReadByte(void) {
    uint res;
    res = *reader.ptr++;
    reader.size += 1;
    return res;
}

uint Reader_ReadWord(void) {
    uint res;
    res = *reader.ptr++;
    res += *reader.ptr++ << 8;
    reader.size += 2;
    return res;
}

uint Reader_ReadDWord(void) {
    uint res;
    res = *reader.ptr++;
    res += *reader.ptr++ << 8;
    res += *reader.ptr++ << 16;
    res += *reader.ptr++ << 24;
    reader.size += 4;
    return res;
}

char *Reader_ReadString(void) {
    char *res;
    if (reader.ptr == NULL)
      return NULL;
    int len = strlen((const char *)reader.ptr);
    if (len > VTX_STRING_MAX)
      return NULL;
    res = (char *)malloc(len + 1);
    strcpy(res, (const char *)reader.ptr);
    res[len] = '\0';
    reader.ptr += len + 1;
    reader.size += len + 1;
    return res;
}

uchar *Reader_GetPtr(void) {
    return reader.ptr;
}

uint Reader_GetSize(void) {
    return reader.size;
}

/* vtx info */

bool get_vtx_metadata(int fd, struct mp3entry* id3)
{
    void *vtx_buf;
    vtx_info_t info;
    char *p = id3->id3v2buf;
    
    vtx_buf = (void *)malloc(filesize(fd));
    if (vtx_buf == NULL)
        return false;
    
    if ((lseek(fd, 0, SEEK_SET) < 0) ||
         read(fd, vtx_buf, filesize(fd)) != filesize(fd))
        goto exit_bad;

    if (filesize(fd) < 20)
        goto exit_bad;

    Reader_Init(vtx_buf);

    uint hdr = Reader_ReadWord();

    if ((hdr != 0x7961) && (hdr != 0x6d79))
        goto exit_bad;

    info.layout = (vtx_layout_t)Reader_ReadByte();
    info.loop = Reader_ReadWord();
    info.chipfreq = Reader_ReadDWord();
    info.playerfreq = Reader_ReadByte();
    info.year = Reader_ReadWord();
    info.regdata_size = Reader_ReadDWord();
    info.frames = info.regdata_size / 14;
    info.title = Reader_ReadString();
    info.author = Reader_ReadString();
    info.from = Reader_ReadString();
    info.tracker = Reader_ReadString();
    info.comment = Reader_ReadString();

    /* Title */
    id3->title = p;
    p += strlcpy(p, info.title, VTX_STRING_MAX) + 1;

    /* Artist */
    id3->artist = p;
    p += strlcpy(p, info.author, VTX_STRING_MAX) + 1;

    id3->vbr = false;
    id3->bitrate = 706;
    id3->frequency = 44100;

    id3->filesize = filesize(fd);
    id3->length = info.frames * 1000 / info.playerfreq;

    free (vtx_buf);
    return true;

exit_bad:
    free (vtx_buf);
    return false;
}
