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

static bool parse_sgc_header(int fd, struct mp3entry* id3)
{
    /* Use the trackname part of the id3 structure as a temporary buffer */
    unsigned char* buf = (unsigned char *)id3->path;

     lseek(fd, 0, SEEK_SET);
     if (read(fd, buf, 0xA0) < 0xA0)
        return false;

    /* calculate track length with number of tracks */
    id3->length = buf[37] * 1000;

    /* If meta info was found in the m3u skip next step */
    if (id3->title && id3->title[0]) return true;

    char *p = id3->id3v2buf;

    /* Some metadata entries have 32 bytes length */
    /* Game */
    memcpy(p, &buf[64], 32); *(p + 33) = '\0';
    id3->title = p;
    p += strlen(p)+1;

    /* Artist */
    memcpy(p, &buf[96], 32); *(p + 33) = '\0';
    id3->artist = p;
    p += strlen(p)+1;

    /* Copyright */
    memcpy(p, &buf[128], 32); *(p + 33) = '\0';
    id3->album = p;
    p += strlen(p)+1;
    return true;
}


bool get_sgc_metadata(int fd, struct mp3entry* id3)
{
   uint32_t sgc_type;
    if ((lseek(fd, 0, SEEK_SET) < 0) ||
         read_uint32be(fd, &sgc_type) != (int)sizeof(sgc_type))
        return false;

    id3->vbr = false;
    id3->filesize = filesize(fd);
    /* we only render 16 bits, 44.1KHz, Stereo */
    id3->bitrate = 706;
    id3->frequency = 44100;
  
    /* Make sure this is an SGC file */
    if (sgc_type != FOURCC('S','G','C',0x1A))
        return false;

    return parse_sgc_header(fd, id3);
}
