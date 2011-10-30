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

static bool parse_kss_header(int fd, struct mp3entry* id3)
{
    /* Use the trackname part of the id3 structure as a temporary buffer */
    unsigned char* buf = (unsigned char *)id3->path;

     lseek(fd, 0, SEEK_SET);
     if (read(fd, buf, 0x20) < 0x20)
        return false;

    /* calculate track length with number of tracks */
    id3->length = 0;
    if (buf[14] == 0x10) {
        id3->length = (get_short_le((void *)(buf + 26)) + 1) * 1000;
    }
    
    if (id3->length <= 0)
        id3->length = 255 * 1000; /* 255 tracks */

    return true;
}


bool get_kss_metadata(int fd, struct mp3entry* id3)
{
   uint32_t kss_type;
    if ((lseek(fd, 0, SEEK_SET) < 0) ||
         read_uint32be(fd, &kss_type) != (int)sizeof(kss_type))
        return false;

    id3->vbr = false;
    id3->filesize = filesize(fd);
    /* we only render 16 bits, 44.1KHz, Stereo */
    id3->bitrate = 706;
    id3->frequency = 44100;
  
    /* Make sure this is an SGC file */
    if (kss_type != FOURCC('K','S','C','C') && kss_type != FOURCC('K','S','S','X'))
        return false;

    return parse_kss_header(fd, id3);
}
