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
#include "plugin.h"

bool get_hes_metadata(int fd, struct mp3entry* id3)
{
    /* Use the id3v2 buffer part of the id3 structure as a temporary buffer */
    unsigned char* buf = (unsigned char *)id3->id3v2buf;
    int read_bytes;

    if ((lseek(fd, 0, SEEK_SET) < 0) 
         || ((read_bytes = read(fd, buf, 4)) < 4))
        return false;

    /* Verify this is a HES file */
    if (memcmp(buf,"HESM",4) != 0)
        return false;

    id3->vbr = false;
    id3->filesize = filesize(fd);
    /* we only render 16 bits, 44.1KHz, Stereo */
    id3->bitrate = 706;
    id3->frequency = 44100;

    /* Set default track count (length)*/
    id3->length = 255 * 1000;

    return true;
}

