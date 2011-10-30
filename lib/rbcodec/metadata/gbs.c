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

static bool parse_gbs_header(int fd, struct mp3entry* id3)
{
    /* Use the trackname part of the id3 structure as a temporary buffer */
    unsigned char* buf = (unsigned char *)id3->path;
    lseek(fd, 0, SEEK_SET);
    if (read(fd, buf, 112) < 112)
        return false;

    /* Calculate track length with number of subtracks */
    id3->length = buf[4] * 1000;

    /* If meta info was found in the m3u skip next step */
    if (id3->title && id3->title[0]) return true;
    
    char *p = id3->id3v2buf;

    /* Some metadata entries have 32 bytes length */
    /* Game */
    memcpy(p, &buf[16], 32); *(p + 33) = '\0';
    id3->title = p;
    p += strlen(p)+1;

    /* Artist */
    memcpy(p, &buf[48], 32); *(p + 33) = '\0';
    id3->artist = p;
    p += strlen(p)+1;

    /* Copyright */
    memcpy(p, &buf[80], 32); *(p + 33) = '\0';
    id3->album = p;

    return true;
}

bool get_gbs_metadata(int fd, struct mp3entry* id3)
{
    char gbs_type[3];
    if ((lseek(fd, 0, SEEK_SET) < 0) ||
         (read(fd, gbs_type, 3) < 3))
        return false;

    id3->vbr = false;
    id3->filesize = filesize(fd);
    /* we only render 16 bits, 44.1KHz, Stereo */
    id3->bitrate = 706;
    id3->frequency = 44100;

    /* Check for GBS magic */
    if (memcmp( gbs_type, "GBS", 3 ) != 0)
        return false;

    return parse_gbs_header(fd, id3);
}
