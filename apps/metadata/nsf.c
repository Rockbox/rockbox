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

struct NESM_HEADER
{
    uint32_t        nHeader;
    uint8_t         nHeaderExtra;
    uint8_t         nVersion;
    uint8_t         nTrackCount;
    uint8_t         nInitialTrack;
    uint16_t        nLoadAddress;
    uint16_t        nInitAddress;
    uint16_t        nPlayAddress;
    uint8_t         szGameTitle[32];
    uint8_t         szArtist[32];
    uint8_t         szCopyright[32];
    uint16_t        nSpeedNTSC;
    uint8_t         nBankSwitch[8];
    uint16_t        nSpeedPAL;
    uint8_t         nNTSC_PAL;
    uint8_t         nExtraChip;
    uint8_t         nExpansion[4];
} __attribute__((packed));

struct NSFE_INFOCHUNK
{
    uint16_t        nLoadAddress;
    uint16_t        nInitAddress;
    uint16_t        nPlayAddress;
    uint8_t         nIsPal;
    uint8_t         nExt;
    uint8_t         nTrackCount;
    uint8_t         nStartingTrack;
} __attribute__((packed));


#define CHAR4_CONST(a, b, c, d) FOURCC(a, b, c, d)

static bool parse_nsfe(int fd, struct mp3entry *id3)
{
    bool info_found = false;
    bool end_found = false;
    bool data_found = false;

    struct NSFE_INFOCHUNK info;
    memset(&info, 0, sizeof(struct NSFE_INFOCHUNK));

     /* default values */
    info.nTrackCount = 1;
    id3->length = 2*1000*60;
    
    /* begin reading chunks */
    while (!end_found)
    {
        int32_t chunk_size, chunk_type;

        if (read_uint32le(fd, &chunk_size) != (int)sizeof(uint32_t))
            return false;

        if (read_uint32be(fd, &chunk_type) != (int)sizeof(uint32_t))
            return false;

        switch (chunk_type)
        {
        case CHAR4_CONST('I', 'N', 'F', 'O'):
        {
            /* only one info chunk permitted */
            if (info_found)
                return false;

            info_found = true;

            /* minimum size */
            if (chunk_size < 8)
                return false;

            ssize_t size = MIN((ssize_t)sizeof(struct NSFE_INFOCHUNK),
                               chunk_size);

            if (read(fd, &info, size) != size)
                return false;

            if (size >= 9)
                id3->length = info.nTrackCount*1000;

            lseek(fd, chunk_size - size, SEEK_CUR);
            break;
            }

        case CHAR4_CONST('a', 'u', 't', 'h'):
        {
            if (!info_found)
                return false;

            /* szGameTitle, szArtist, szCopyright */
            char ** const ar[] = { &id3->title, &id3->artist, &id3->album };

            char *p = id3->id3v2buf;
            long buf_rem = sizeof (id3->id3v2buf);
            unsigned int i;

            for (i = 0; i < ARRAYLEN(ar) && chunk_size && buf_rem; i++)
            {
                long len = read_string(fd, p, buf_rem, '\0', chunk_size);

                if (len < 0)
                    return false;

                *ar[i] = p;
                p += len;
                buf_rem -= len;

                if (chunk_size >= len)
                    chunk_size -= len;
                else
                    chunk_size = 0;
            }

            lseek(fd, chunk_size, SEEK_CUR);
            break;
            }


        case CHAR4_CONST('D', 'A', 'T', 'A'):
            if (chunk_size < 1)
                return false;

            data_found = true;
            /* fall through */
        case CHAR4_CONST('f', 'a', 'd', 'e'):
        case CHAR4_CONST('t', 'i', 'm', 'e'):
        case CHAR4_CONST('B', 'A', 'N', 'K'):
        case CHAR4_CONST('p', 'l', 's', 't'):
        case CHAR4_CONST('t', 'l', 'b', 'l'): /* we unfortunately can't use these anyway */
        {
            if (!info_found)
                return false;

            lseek(fd, chunk_size, SEEK_CUR);
            break;
            }

        case CHAR4_CONST('N', 'E', 'N', 'D'):
        {
            end_found = true;
            break;
            }

        default: /* unknown chunk */
        {
            /* check the first byte */
            chunk_type = (uint8_t)chunk_type;

            /* chunk is vital... don't continue */
            if(chunk_type >= 'A' && chunk_type <= 'Z')
                return false;

            /* otherwise, just skip it */
            lseek(fd, chunk_size, SEEK_CUR);
            break;
            }
        } /* end switch */
    } /* end while */

    /*
     * if we exited the while loop without a 'return', we must have hit an NEND
     *  chunk if this is the case, the file was layed out as it was expected.
     *  now.. make sure we found both an info chunk, AND a data chunk... since
     *  these are minimum requirements for a valid NSFE file
     */
    return info_found && data_found;
}

static bool parse_nesm(int fd, struct mp3entry *id3)
{
    struct NESM_HEADER hdr;
    char *p = id3->id3v2buf;

    lseek(fd, 0, SEEK_SET);
    if (read(fd, &hdr, sizeof(hdr)) != sizeof(hdr))
        return false;

    /* Length */
    id3->length = hdr.nTrackCount*1000;

    /* Title */
    id3->title = p;
    p += strlcpy(p, hdr.szGameTitle, 32) + 1;

    /* Artist */
    id3->artist = p;
    p += strlcpy(p, hdr.szArtist, 32) + 1;

    /* Copyright (per codec) */
    id3->album = p;
    strlcpy(p, hdr.szCopyright, 32);
        
    return true;
}

bool get_nsf_metadata(int fd, struct mp3entry* id3)
{
    uint32_t nsf_type;
      
    if (lseek(fd, 0, SEEK_SET) < 0 ||
        read_uint32be(fd, &nsf_type) != (int)sizeof(nsf_type))
        return false;

    id3->vbr = false;
    id3->filesize = filesize(fd);
    /* we only render 16 bits, 44.1KHz, Mono */
    id3->bitrate = 706;
    id3->frequency = 44100;

    if (nsf_type == CHAR4_CONST('N', 'S', 'F', 'E'))
        return parse_nsfe(fd, id3);
    else if (nsf_type == CHAR4_CONST('N', 'E', 'S', 'M'))
        return parse_nesm(fd, id3);

    /* not a valid format*/
    return false;
}

