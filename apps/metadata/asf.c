/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * $Id$
 *
 * Copyright (C) 2007 Dave Chapman
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <inttypes.h>

#include "id3.h"
#include "debug.h"
#include "rbunicode.h"
#include "metadata_common.h"
#include <codecs/libwma/asf.h>

static asf_waveformatex_t wfx;

/* TODO: Just read the GUIDs into a 16-byte array, and use memcmp to compare */
struct guid_s {
    uint32_t v1;
    uint16_t v2;
    uint16_t v3;
    uint8_t  v4[8];
};
typedef struct guid_s guid_t;

struct asf_object_s {
    guid_t       guid;
    uint64_t     size;
    uint64_t     datalen;
};
typedef struct asf_object_s asf_object_t;

enum asf_error_e {
    ASF_ERROR_INTERNAL       = -1,  /* incorrect input to API calls */
    ASF_ERROR_OUTOFMEM       = -2,  /* some malloc inside program failed */
    ASF_ERROR_EOF            = -3,  /* unexpected end of file */
    ASF_ERROR_IO             = -4,  /* error reading or writing to file */
    ASF_ERROR_INVALID_LENGTH = -5,  /* length value conflict in input data */
    ASF_ERROR_INVALID_VALUE  = -6,  /* other value conflict in input data */
    ASF_ERROR_INVALID_OBJECT = -7,  /* ASF object missing or in wrong place */
    ASF_ERROR_OBJECT_SIZE    = -8,  /* invalid ASF object size (too small) */
    ASF_ERROR_SEEKABLE       = -9,  /* file not seekable */
    ASF_ERROR_SEEK           = -10  /* file is seekable but seeking failed */
};

static const guid_t asf_guid_null =
{0x00000000, 0x0000, 0x0000, {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

/* top level object guids */

static const guid_t asf_guid_header =
{0x75B22630, 0x668E, 0x11CF, {0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C}};

static const guid_t asf_guid_data =
{0x75B22636, 0x668E, 0x11CF, {0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C}};

static const guid_t asf_guid_index =
{0x33000890, 0xE5B1, 0x11CF, {0x89, 0xF4, 0x00, 0xA0, 0xC9, 0x03, 0x49, 0xCB}};

/* header level object guids */

static const guid_t asf_guid_file_properties =
{0x8cabdca1, 0xa947, 0x11cf, {0x8E, 0xe4, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65}};

static const guid_t asf_guid_stream_properties =
{0xB7DC0791, 0xA9B7, 0x11CF, {0x8E, 0xE6, 0x00, 0xC0, 0x0C, 0x20, 0x53, 0x65}};

static const guid_t asf_guid_content_description =
{0x75B22633, 0x668E, 0x11CF, {0xA6, 0xD9, 0x00, 0xAA, 0x00, 0x62, 0xCE, 0x6C}};

static const guid_t asf_guid_extended_content_description =
{0xD2D0A440, 0xE307, 0x11D2, {0x97, 0xF0, 0x00, 0xA0, 0xC9, 0x5E, 0xA8, 0x50}};

/* stream type guids */

static const guid_t asf_guid_stream_type_audio =
{0xF8699E40, 0x5B4D, 0x11CF, {0xA8, 0xFD, 0x00, 0x80, 0x5F, 0x5C, 0x44, 0x2B}};

static int asf_guid_match(const guid_t *guid1, const guid_t *guid2)
{
    if((guid1->v1 != guid2->v1) ||
       (guid1->v2 != guid2->v2) ||
       (guid1->v3 != guid2->v3) ||
       (memcmp(guid1->v4, guid2->v4, 8))) {
        return 0;
    }

    return 1;
}

/* Read the 16 byte GUID from a file */
static void asf_readGUID(int fd, guid_t* guid)
{
    read_uint32le(fd, &guid->v1);
    read_uint16le(fd, &guid->v2);
    read_uint16le(fd, &guid->v3);
    read(fd, guid->v4, 8);
}

static void asf_read_object_header(asf_object_t *obj, int fd)
{
    asf_readGUID(fd, &obj->guid);
    read_uint64le(fd, &obj->size);
    obj->datalen = 0;
}

static int asf_parse_header(int fd, struct mp3entry* id3)
{
    asf_object_t current;
    asf_object_t header;
    uint64_t datalen;
    int i;
    int fileprop = 0;
    uint64_t play_duration;
    uint64_t tmp64;
    uint32_t tmp32;
    uint16_t tmp16;
    uint8_t tmp8;
    uint16_t flags;
    uint32_t subobjects;
    uint8_t utf16buf[512];
    uint8_t utf8buf[512];

    asf_read_object_header((asf_object_t *) &header, fd);

    DEBUGF("header.size=%d\n",(int)header.size);
    if (header.size < 30) {
        /* invalid size for header object */
        return ASF_ERROR_OBJECT_SIZE;
    }

    read_uint32le(fd, &subobjects);

    /* Two reserved bytes - do we need to read them? */
    lseek(fd, 2, SEEK_CUR);

    DEBUGF("Read header - size=%d, subobjects=%lu\n",(int)header.size, subobjects);

    if (subobjects > 0) {
        header.datalen = header.size - 30;

        /* TODO: Check that we have datalen bytes left in the file */
        datalen = header.datalen;

        for (i=0; i<(int)subobjects; i++) {
            DEBUGF("Parsing header object %d - datalen=%d\n",i,(int)datalen);
            if (datalen < 24) {
                DEBUGF("not enough data for reading object\n");
                break;
            }

            asf_read_object_header(&current, fd);

            if (current.size > datalen || current.size < 24) {
                DEBUGF("invalid object size - current.size=%d, datalen=%d\n",(int)current.size,(int)datalen);
                break;
            }

            if (asf_guid_match(&current.guid, &asf_guid_file_properties)) {
                    if (current.size < 104)
                        return ASF_ERROR_OBJECT_SIZE;

                    if (fileprop) {
                        /* multiple file properties objects not allowed */
                        return ASF_ERROR_INVALID_OBJECT;
                    }

                    fileprop = 1;
                    /* All we want is the play duration - uint64_t at offset 40 */
                    lseek(fd, 40, SEEK_CUR);

                    read_uint64le(fd, &play_duration);
                    id3->length = play_duration / 10000;

                    DEBUGF("****** length = %lums\n", id3->length);

                    /* Read the packet size - uint32_t at offset 68 */
                    lseek(fd, 20, SEEK_CUR);
                    read_uint32le(fd, &wfx.packet_size);

                    /* Skip bytes remaining in object */
                    lseek(fd, current.size - 24 - 72, SEEK_CUR);
            } else if (asf_guid_match(&current.guid, &asf_guid_stream_properties)) {
                    guid_t guid;
                    uint32_t propdatalen;

                    if (current.size < 78)
                        return ASF_ERROR_OBJECT_SIZE;

#if 0
                    asf_byteio_getGUID(&guid, current->data);
                    datalen = asf_byteio_getDWLE(current->data + 40);
                    flags = asf_byteio_getWLE(current->data + 48);
#endif

                    asf_readGUID(fd, &guid);

                    lseek(fd, 24, SEEK_CUR);
                    read_uint32le(fd, &propdatalen);
                    lseek(fd, 4, SEEK_CUR);
                    read_uint16le(fd, &flags);

                    if (!asf_guid_match(&guid, &asf_guid_stream_type_audio)) {
                        DEBUGF("Found stream properties for non audio stream, skipping\n");
                        lseek(fd,current.size - 24 - 50,SEEK_CUR);
                    } else {
                        lseek(fd, 4, SEEK_CUR);
                        DEBUGF("Found stream properties for audio stream %d\n",flags&0x7f);

                        /* TODO: Check codec_id and find the lowest numbered audio stream in the file */
                        wfx.audiostream = flags&0x7f;

                        if (propdatalen < 18) {
                            return ASF_ERROR_INVALID_LENGTH;
                        }

#if 0
                        if (asf_byteio_getWLE(data + 16) > datalen - 16) {
                            return ASF_ERROR_INVALID_LENGTH;
                        }
#endif
                        read_uint16le(fd, &wfx.codec_id);
                        read_uint16le(fd, &wfx.channels);
                        read_uint32le(fd, &wfx.rate);
                        read_uint32le(fd, &wfx.bitrate);
                        wfx.bitrate *= 8;
                        read_uint16le(fd, &wfx.blockalign);
                        read_uint16le(fd, &wfx.bitspersample);
                        read_uint16le(fd, &wfx.datalen);

                        /* Round bitrate to the nearest kbit */
                        id3->bitrate = (wfx.bitrate + 500) / 1000;
                        id3->frequency = wfx.rate;

                        if (wfx.codec_id == ASF_CODEC_ID_WMAV1) {
                            read(fd, wfx.data, 4);
                            lseek(fd,current.size - 24 - 72 - 4,SEEK_CUR);
                            /* A hack - copy the wfx struct to the MP3 TOC field in the id3 struct */
                            memcpy(id3->toc, &wfx, sizeof(wfx));
                        } else if (wfx.codec_id == ASF_CODEC_ID_WMAV2) {
                            read(fd, wfx.data, 6);
                            lseek(fd,current.size - 24 - 72 - 6,SEEK_CUR);
                            /* A hack - copy the wfx struct to the MP3 TOC field in the id3 struct */
                            memcpy(id3->toc, &wfx, sizeof(wfx));
                        } else {
                            lseek(fd,current.size - 24 - 72,SEEK_CUR);
                        }

                    }
            } else if (asf_guid_match(&current.guid, &asf_guid_content_description)) {
                    /* Object contains five 16-bit string lengths, followed by the five strings:
                       title, artist, copyright, description, rating
                     */
                    uint16_t strlength[5];
                    int i;

                    DEBUGF("Found GUID_CONTENT_DESCRIPTION - size=%d\n",(int)(current.size - 24));

                    /* Read the 5 string lengths - number of bytes included trailing zero */
                    for (i=0; i<5; i++) {
                        read_uint16le(fd, &strlength[i]);
                        DEBUGF("strlength = %u\n",strlength[i]);
                    }

                    for (i=0; i<5 ; i++) {
                        if (strlength[i] > 0) {
                            read(fd, utf16buf, strlength[i]);
                            utf16LEdecode(utf16buf, utf8buf, strlength[i]);
                            DEBUGF("TAG %d = %s\n",i,utf8buf);
                        }
                    }
            } else if (asf_guid_match(&current.guid, &asf_guid_extended_content_description)) {
                    uint16_t count;
                    int i;
                    int bytesleft = current.size - 24;
                    DEBUGF("Found GUID_EXTENDED_CONTENT_DESCRIPTION\n");
                    
                    read_uint16le(fd, &count);
                    bytesleft -= 2;
                    DEBUGF("extended metadata count = %u\n",count);

                    for (i=0; i < count; i++) {
                        uint16_t length, type;

                        read_uint16le(fd, &length);
                        read(fd, utf16buf, length);
                        utf16LEdecode(utf16buf, utf8buf, length);
                        DEBUGF("Key=\"%s\" ",utf8buf);
                        bytesleft -= 2 + length;

                        read_uint16le(fd, &type);
                        read_uint16le(fd, &length);
                        switch(type)
                        {
                            case 0: /* String */
                                read(fd, utf16buf, length);
                                utf16LEdecode(utf16buf, utf8buf, length);
                                DEBUGF("Value=\"%s\"\n",utf8buf);
                                break;

                            case 1: /* Hex string */
                                DEBUGF("Value=NOT YET IMPLEMENTED (HEX STRING)\n");
                                lseek(fd,length,SEEK_CUR);
                                break;

                            case 2: /* Bool */
                                read(fd, &tmp8, 1);
                                DEBUGF("Value=%s\n",(tmp8 ? "TRUE" : "FALSE"));
                                lseek(fd,length - 1,SEEK_CUR);
                                break;

                            case 3: /* 32-bit int */
                                read_uint32le(fd, &tmp32);
                                DEBUGF("Value=%lu\n",tmp32);
                                lseek(fd,length - 4,SEEK_CUR);
                                break;

                            case 4: /* 64-bit int */
                                read_uint64le(fd, &tmp64);
                                DEBUGF("Value=%llu\n",tmp64);
                                lseek(fd,length - 8,SEEK_CUR);
                                break;

                            case 5: /* 16-bit int */
                                read_uint16le(fd, &tmp16);
                                DEBUGF("Value=%u\n",tmp16);
                                lseek(fd,length - 2,SEEK_CUR);
                                break;

                            default:
                                lseek(fd,length,SEEK_CUR);
                                break;
                        }
                        bytesleft -= 4 + length;
                    }

                    lseek(fd, bytesleft, SEEK_CUR);
            } else {
                DEBUGF("Skipping %d bytes of object\n",(int)(current.size - 24));
                lseek(fd,current.size - 24,SEEK_CUR);
            }

            DEBUGF("Parsed object - size = %d\n",(int)current.size);
            datalen -= current.size;
        }

        if (i != (int)subobjects || datalen != 0) {
            DEBUGF("header data doesn't match given subobject count\n");
            return ASF_ERROR_INVALID_VALUE;
        }

        DEBUGF("%d subobjects read successfully\n", i);
    }

#if 0
    tmp = asf_parse_header_validate(file, &header);
    if (tmp < 0) {
        /* header read ok but doesn't validate correctly */
        return tmp;
    }
#endif

    DEBUGF("header validated correctly\n");

    return 0;
}

bool get_asf_metadata(int fd, struct mp3entry* id3)
{
    int res;
    asf_object_t obj;

    wfx.audiostream = -1;

    res = asf_parse_header(fd, id3);

    if (res < 0) {
        DEBUGF("ASF: parsing error - %d\n",res);
        return false;
    }

    if (wfx.audiostream == -1) {
        DEBUGF("ASF: No WMA streams found\n");
        return false;
    }

    asf_read_object_header(&obj, fd);

    if (!asf_guid_match(&obj.guid, &asf_guid_data)) {
        DEBUGF("ASF: No data object found\n");
        return false;
    }

    /* Store the current file position - no need to parse the header
       again in the codec.  The +26 skips the rest of the data object
       header.
     */
    id3->first_frame_offset = lseek(fd, 0, SEEK_CUR) + 26;

    return true;
}
