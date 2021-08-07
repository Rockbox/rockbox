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

typedef unsigned char  uchar;
typedef unsigned short ushort;
typedef unsigned int   uint;
typedef unsigned long  ulong;

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

#define VTX_STRING_MAX 254

static uint Reader_ReadByte(int fd) {
    unsigned char c = 0;
    (void)read(fd, &c, sizeof(c));
    return c;
}

static uint Reader_ReadWord(int fd) {
    unsigned short s = 0;
    (void)read(fd, &s, sizeof(s));
    return letoh16(s);
}

static uint Reader_ReadDWord(int fd) {
    unsigned int i = 0;
    (void)read(fd, &i, sizeof(i));
    return letoh32(i);
}

static char* Reader_ReadString(int fd, char *str) {
    /*Note: still advances file buffer even if no string buffer supplied */
    int i = 0;
    char ch = '\0';
    char *p = str;

    while (i < VTX_STRING_MAX && (ch || i == 0))
    {
        if (read(fd, &ch, sizeof(ch) == sizeof(ch)))
        {
            if (str)
                *str++ = ch;
        }
        else
            break;
        i++;
    }
    if (str)
        *str = '\0';

    return p;
}

/* vtx info */

bool get_vtx_metadata(int fd, struct mp3entry* id3)
{
    vtx_info_t info;
    char *p = id3->id3v2buf;
    char buf[VTX_STRING_MAX+1];

    if (lseek(fd, 0, SEEK_SET) < 0)
        goto exit_bad;

    if (filesize(fd) < 20)
        goto exit_bad;

    uint hdr = Reader_ReadWord(fd);

    if ((hdr != 0x7961) && (hdr != 0x6d79))
        goto exit_bad;

    info.layout = (vtx_layout_t)Reader_ReadByte(fd);
    info.loop = Reader_ReadWord(fd);
    info.chipfreq = Reader_ReadDWord(fd);
    info.playerfreq = Reader_ReadByte(fd);
    info.year = Reader_ReadWord(fd);
    info.regdata_size = Reader_ReadDWord(fd);
    info.frames = info.regdata_size / 14;
    info.title = Reader_ReadString(fd, buf);
    if (buf[0]) {
        /* Title */
        id3->title = p;
        p += strlcpy(p, info.title, VTX_STRING_MAX) + 1;
    }
    info.author = Reader_ReadString(fd, buf);
    if (buf[0]) {
        /* Artist */
        id3->artist = p;
        p += strlcpy(p, info.author, VTX_STRING_MAX) + 1;
    }
    info.from = Reader_ReadString(fd, NULL);
    info.tracker = Reader_ReadString(fd, NULL);
    info.comment = Reader_ReadString(fd, buf);
    if (buf[0]) {
        /* Comment */
        id3->comment = p;
        p += strlcpy(p, info.comment, VTX_STRING_MAX) + 1;
    }

    id3->vbr = false;
    id3->bitrate = 706;
    id3->frequency = 44100; // XXX allow this to be configured?

    id3->filesize = filesize(fd);
    id3->length = info.frames * 1000 / info.playerfreq;

    return true;

exit_bad:
    return false;
}
