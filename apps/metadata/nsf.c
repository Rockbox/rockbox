#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>

#include "system.h"
#include "metadata.h"
#include "metadata_common.h"
#include "metadata_parsers.h"
#include "rbunicode.h"

bool get_nsf_metadata(int fd, struct mp3entry* id3)
{
    /* Use the trackname part of the id3 structure as a temporary buffer */
    unsigned char* buf = (unsigned char *)id3->path;
    char *p;
      
    if ((lseek(fd, 0, SEEK_SET) < 0) 
         || (read(fd, buf, 110) < 110))
    {
        return false;
    }

    id3->length = 120*1000;
    id3->vbr = false;
    id3->filesize = filesize(fd);

    if (memcmp(buf,"NSFE",4) == 0) /* only NESM contain metadata */
    {
        return true;
    }
    else
    { 
        if (memcmp(buf, "NESM",4) != 0)  /* not a valid format*/
        {
            return false;
        }
    }

    p = id3->id3v2buf;

    /* Length */
    id3->length = buf[6]*1000;

    /* Title */
    memcpy(p, &buf[14], 32);
    id3->title = p;
    p += strlen(p)+1;

    /* Artist */
    memcpy(p, &buf[46], 32);
    id3->artist = p;
    p += strlen(p)+1;

    /* Copyright (per codec) */
    memcpy(p, &buf[78], 32);
    id3->album = p;
        
    return true;
}

