/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Daniel Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/*
 * Parts of this code has been stolen from the Ample project and was written
 * by David Härdeman. It has since been extended and enhanced pretty much by
 * all sorts of friendly Rockbox people.
 *
 * A nice reference for MPEG header info:
 * http://rockbox.haxx.se/docs/mpeghdr.html
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "debug.h"
#include "logf.h"
#include "mp3data.h"
#include "file.h"
#include "buffer.h"

#define DEBUG_VERBOSE

#define BYTES2INT(b1,b2,b3,b4) (((long)(b1 & 0xFF) << (3*8)) |      \
                                ((long)(b2 & 0xFF) << (2*8)) | \
                                ((long)(b3 & 0xFF) << (1*8)) | \
                                ((long)(b4 & 0xFF) << (0*8)))

#define SYNC_MASK (0x7ffL << 21)
#define VERSION_MASK (3L << 19)
#define LAYER_MASK (3L << 17)
#define PROTECTION_MASK (1L << 16)
#define BITRATE_MASK (0xfL << 12)
#define SAMPLERATE_MASK (3L << 10)
#define PADDING_MASK (1L << 9)
#define PRIVATE_MASK (1L << 8)
#define CHANNELMODE_MASK (3L << 6)
#define MODE_EXT_MASK (3L << 4)
#define COPYRIGHT_MASK (1L << 3)
#define ORIGINAL_MASK (1L << 2)
#define EMPHASIS_MASK 3L

/* Table of bitrates for MP3 files, all values in kilo.
 * Indexed by version, layer and value of bit 15-12 in header.
 */
const int bitrate_table[2][3][16] =
{
    {
        {0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,0},
        {0,32,48,56, 64,80, 96, 112,128,160,192,224,256,320,384,0},
        {0,32,40,48, 56,64, 80, 96, 112,128,160,192,224,256,320,0}
    },
    {
        {0,32,48,56,64,80,96,112,128,144,160,176,192,224,256,0},
        {0, 8,16,24,32,40,48, 56, 64, 80, 96,112,128,144,160,0},
        {0, 8,16,24,32,40,48, 56, 64, 80, 96,112,128,144,160,0}
    }
};

/* Table of samples per frame for MP3 files.
 * Indexed by layer. Multiplied with 1000.
 */
const long bs[3] = {384000, 1152000, 1152000};

/* Table of sample frequency for MP3 files.
 * Indexed by version and layer.
 */

const int freqtab[][4] =
{
    {11025, 12000, 8000, 0},  /* MPEG version 2.5 */
    {44100, 48000, 32000, 0}, /* MPEG Version 1 */
    {22050, 24000, 16000, 0}, /* MPEG version 2 */
};

/* check if 'head' is a valid mp3 frame header */
static bool is_mp3frameheader(unsigned long head)
{
    if ((head & SYNC_MASK) != (unsigned long)SYNC_MASK) /* bad sync? */
        return false;
    if ((head & VERSION_MASK) == (1L << 19)) /* bad version? */
        return false;
    if (!(head & LAYER_MASK)) /* no layer? */
        return false;
#if CONFIG_CODEC != SWCODEC
    /* The MAS can't decode layer 1, so treat layer 1 data as invalid */
    if ((head & LAYER_MASK) == LAYER_MASK)
        return false;
#endif
    if ((head & BITRATE_MASK) == BITRATE_MASK) /* bad bitrate? */
        return false;
    if (!(head & BITRATE_MASK)) /* no bitrate? */
        return false;
    if ((head & SAMPLERATE_MASK) == SAMPLERATE_MASK) /* bad sample rate? */
        return false;
    
    return true;
}

static bool mp3headerinfo(struct mp3info *info, unsigned long header)
{
    int bittable = 0;
    int bitindex;
    int freqindex;
    
    /* MPEG Audio Version */
    switch((header & VERSION_MASK) >> 19) {
    case 0:
        /* MPEG version 2.5 is not an official standard */
        info->version = MPEG_VERSION2_5;
        bittable = MPEG_VERSION2 - 1; /* use the V2 bit rate table */
        break;

    case 1:
        return false;

    case 2:
        /* MPEG version 2 (ISO/IEC 13818-3) */
        info->version = MPEG_VERSION2;
        bittable = MPEG_VERSION2 - 1;
        break;
        
    case 3:
        /* MPEG version 1 (ISO/IEC 11172-3) */
        info->version = MPEG_VERSION1;
        bittable = MPEG_VERSION1 - 1;
        break;
    }

    switch((header & LAYER_MASK) >> 17) {
    case 0:
        return false;
    case 1:
        info->layer = 2;
        break;
    case 2:
        info->layer = 1;
        break;
    case 3:
        info->layer = 0;
        break;
    }

    info->protection = (header & PROTECTION_MASK)?true:false;
    
    /* Bitrate */
    bitindex = (header & 0xf000) >> 12;
    info->bitrate = bitrate_table[bittable][info->layer][bitindex];
    if(info->bitrate == 0)
        return false;
    
    /* Sampling frequency */
    freqindex = (header & 0x0C00) >> 10;
    info->frequency = freqtab[info->version][freqindex];
    if(info->frequency == 0)
        return false;

    info->padding = (header & 0x0200)?1:0;

    /* Calculate number of bytes, calculation depends on layer */
    switch(info->layer) {
    case 0:
        info->frame_size = info->bitrate * 48000;
        info->frame_size /=
            freqtab[info->version][freqindex] << bittable;
        break;
    case 1:
    case 2:
        info->frame_size = info->bitrate * 144000;
        info->frame_size /=
            freqtab[info->version][freqindex] << bittable;
        break;
    default:
        info->frame_size = 1;
    }
    
    info->frame_size += info->padding;

    /* Calculate time per frame */
    info->frame_time = bs[info->layer] /
        (freqtab[info->version][freqindex] << bittable);

    info->channel_mode = (header & 0xc0) >> 6;
    info->mode_extension = (header & 0x30) >> 4;
    info->emphasis = header & 3;

#ifdef DEBUG_VERBOSE
    DEBUGF( "Header: %08x, Ver %d, lay %d, bitr %d, freq %d, "
            "chmode %d, mode_ext %d, emph %d, bytes: %d time: %d\n",
            header, info->version, info->layer+1, info->bitrate,
            info->frequency, info->channel_mode, info->mode_extension,
            info->emphasis, info->frame_size, info->frame_time);
#endif
    return true;
}

static unsigned long __find_next_frame(int fd, long *offset, long max_offset,
                                       unsigned long last_header,
                                       int(*getfunc)(int fd, unsigned char *c))
{
    unsigned long header=0;
    unsigned char tmp;
    int i;

    long pos = 0;

    /* We remember the last header we found, to use as a template to see if
       the header we find has the same frequency, layer etc */
    last_header &= 0xffff0c00;
    
    /* Fill up header with first 24 bits */
    for(i = 0; i < 3; i++) {
        header <<= 8;
        if(!getfunc(fd, &tmp))
            return 0;
        header |= tmp;
        pos++;
    }

    do {
        header <<= 8;
        if(!getfunc(fd, &tmp))
            return 0;
        header |= tmp;
        pos++;
        if(max_offset > 0 && pos > max_offset)
            return 0;
    } while(!is_mp3frameheader(header) ||
            (last_header?((header & 0xffff0c00) != last_header):false));

    *offset = pos - 4;

#if defined(DEBUG) || defined(SIMULATOR)
    if(*offset)
        DEBUGF("Warning: skipping %d bytes of garbage\n", *offset);
#endif
    
    return header;
}

static int fileread(int fd, unsigned char *c)
{    
    return read(fd, c, 1);
}

unsigned long find_next_frame(int fd, long *offset, long max_offset, unsigned long last_header)
{
    return __find_next_frame(fd, offset, max_offset, last_header, fileread);
}

static int fnf_read_index;
static int fnf_buf_len;

static int buf_getbyte(int fd, unsigned char *c)
{
    if(fnf_read_index < fnf_buf_len)
    {
        *c = audiobuf[fnf_read_index++];
        return 1;
    }
    else
    {
        fnf_buf_len = read(fd, audiobuf, audiobufend - audiobuf);
        if(fnf_buf_len < 0)
            return -1;

        fnf_read_index = 0;

        if(fnf_buf_len > 0)
        {
            *c = audiobuf[fnf_read_index++];
            return 1;
        }
        else
            return 0;
    }
    return 0;
}

static int buf_seek(int fd, int len)
{
    fnf_read_index += len;
    if(fnf_read_index > fnf_buf_len)
    {
        len = fnf_read_index - fnf_buf_len;
        
        fnf_buf_len = read(fd, audiobuf, audiobufend - audiobuf);
        if(fnf_buf_len < 0)
            return -1;

        fnf_read_index = 0;
        fnf_read_index += len;
    }
    
    if(fnf_read_index > fnf_buf_len)
    {
        return -1;
    }
    else
        return 0;
}

static void buf_init(void)
{
    fnf_buf_len = 0;
    fnf_read_index = 0;
}

unsigned long buf_find_next_frame(int fd, long *offset, long max_offset,
                                  unsigned long last_header)
{
    return __find_next_frame(fd, offset, max_offset, last_header, buf_getbyte);
}

static int audiobuflen;
static int mem_pos;
static int mem_cnt;
static int mem_maxlen;

static int mem_getbyte(int dummy, unsigned char *c)
{
    dummy = dummy;
    
    *c = audiobuf[mem_pos++];
    if(mem_pos >= audiobuflen)
        mem_pos = 0;

    if(mem_cnt++ >= mem_maxlen)
        return 0;
    else
        return 1;
}

unsigned long mem_find_next_frame(int startpos, long *offset, long max_offset,
                                  unsigned long last_header)
{
    audiobuflen = audiobufend - audiobuf;
    mem_pos = startpos;
    mem_cnt = 0;
    mem_maxlen = max_offset;

    return __find_next_frame(0, offset, max_offset, last_header, mem_getbyte);
}

int get_mp3file_info(int fd, struct mp3info *info)
{
    unsigned char frame[1800];
    unsigned char *vbrheader;
    unsigned long header;
    long bytecount;
    int num_offsets;
    int frames_per_entry;
    int i;
    long offset;
    int j;
    long tmp;

    header = find_next_frame(fd, &bytecount, 0x20000, 0);
    /* Quit if we haven't found a valid header within 128K */
    if(header == 0)
        return -1;

    memset(info, 0, sizeof(struct mp3info));
    /* These two are needed for proper LAME gapless MP3 playback */
    /* TODO: These can be found in a LAME Info header as well, but currently
       they are only looked for in a Xing header. Xing and Info headers have
       the exact same format, but Info headers are used for CBR files. */
    info->enc_delay = -1;
    info->enc_padding = -1;
    if(!mp3headerinfo(info, header))
        return -2;

    /* OK, we have found a frame. Let's see if it has a Xing header */
    if(read(fd, frame, info->frame_size-4) < 0)
        return -3;

    /* calculate position of VBR header */
    if ( info->version == MPEG_VERSION1 ) {
        if (info->channel_mode == 3) /* mono */
            vbrheader = frame + 17;
        else
            vbrheader = frame + 32;
    }
    else {
        if (info->channel_mode == 3) /* mono */
            vbrheader = frame + 9;
        else
            vbrheader = frame + 17;
    }

    if (!memcmp(vbrheader, "Xing", 4)
        || !memcmp(vbrheader, "Info", 4))
    {
        int i = 8; /* Where to start parsing info */

        DEBUGF("Xing/Info header\n");

        /* Remember where in the file the Xing header is */
        info->vbr_header_pos = lseek(fd, 0, SEEK_CUR) - info->frame_size;
        
        /* We want to skip the Xing frame when playing the stream */
        bytecount += info->frame_size;
        
        /* Now get the next frame to find out the real info about
           the mp3 stream */
        header = find_next_frame(fd, &tmp, 0x20000, 0);
        if(header == 0)
            return -4;

        if(!mp3headerinfo(info, header))
            return -5;

        /* Is it a VBR file? */
        info->is_vbr = info->is_xing_vbr = !memcmp(vbrheader, "Xing", 4);

        if(vbrheader[7] & VBR_FRAMES_FLAG) /* Is the frame count there? */
        {
            info->frame_count = BYTES2INT(vbrheader[i], vbrheader[i+1],
                                          vbrheader[i+2], vbrheader[i+3]);
            info->file_time = info->frame_count * info->frame_time;
            i += 4;
        }

        if(vbrheader[7] & VBR_BYTES_FLAG) /* Is byte count there? */
        {
            info->byte_count = BYTES2INT(vbrheader[i], vbrheader[i+1],
                                         vbrheader[i+2], vbrheader[i+3]);
            i += 4;
        }

        if(info->file_time && info->byte_count)
            info->bitrate = info->byte_count * 8 / info->file_time;
        else
            info->bitrate = 0;
        
        if(vbrheader[7] & VBR_TOC_FLAG) /* Is table-of-contents there? */
        {
            memcpy( info->toc, vbrheader+i, 100 );
            i += 100;
        }
        if (vbrheader[7] & VBR_QUALITY_FLAG)
        {
            /* We don't care about this, but need to skip it */
            i += 4;
        }
        i += 21;
        info->enc_delay = (vbrheader[i] << 4) | (vbrheader[i + 1] >> 4);
        info->enc_padding = ((vbrheader[i + 1] & 0x0f) << 8) | vbrheader[i + 2];
        if (!(info->enc_delay >= 0 && info->enc_delay <= 1152 && 
            info->enc_padding >= 0 && info->enc_padding <= 2*1152))
        {
           /* Invalid data */
           info->enc_delay = -1;
           info->enc_padding = -1;
        }
    }

    if (!memcmp(vbrheader, "VBRI", 4))
    {
        DEBUGF("VBRI header\n");

        /* We want to skip the VBRI frame when playing the stream */
        bytecount += info->frame_size;
        
        /* Now get the next frame to find out the real info about
           the mp3 stream */
        header = find_next_frame(fd, &tmp, 0x20000, 0);
        if(header == 0)
            return -6;

        bytecount += tmp;
        
        if(!mp3headerinfo(info, header))
            return -7;

        DEBUGF("%04x: %04x %04x ", 0, header >> 16, header & 0xffff);
        for(i = 4;i < (int)sizeof(frame)-4;i+=2) {
            if(i % 16 == 0) {
                DEBUGF("\n%04x: ", i-4);
            }
            DEBUGF("%04x ", (frame[i-4] << 8) | frame[i-4+1]);
        }

        DEBUGF("\n");
        
        /* Yes, it is a FhG VBR file */
        info->is_vbr = true;
        info->is_vbri_vbr = true;
        info->has_toc = false; /* We don't parse the TOC (yet) */

        info->byte_count = BYTES2INT(vbrheader[10], vbrheader[11],
                                     vbrheader[12], vbrheader[13]);
        info->frame_count = BYTES2INT(vbrheader[14], vbrheader[15],
                                      vbrheader[16], vbrheader[17]);
        info->file_time = info->frame_count * info->frame_time;
        info->bitrate = info->byte_count * 8 / info->file_time;

        /* We don't parse the TOC, since we don't yet know how to (FIXME) */
        num_offsets = BYTES2INT(0, 0, vbrheader[18], vbrheader[19]);
        frames_per_entry = BYTES2INT(0, 0, vbrheader[24], vbrheader[25]);
        DEBUGF("Frame size (%dkpbs): %d bytes (0x%x)\n",
               info->bitrate, info->frame_size, info->frame_size);
        DEBUGF("Frame count: %x\n", info->frame_count);
        DEBUGF("Byte count: %x\n", info->byte_count);
        DEBUGF("Offsets: %d\n", num_offsets);
        DEBUGF("Frames/entry: %d\n", frames_per_entry);

        offset = 0;

        for(i = 0;i < num_offsets;i++)
        {
           j = BYTES2INT(0, 0, vbrheader[26+i*2], vbrheader[27+i*2]);
           offset += j;
           DEBUGF("%03d: %x (%x)\n", i, offset - bytecount, j);
        }
    }

    return bytecount;
}

static void long2bytes(unsigned char *buf, long val)
{
    buf[0] = (val >> 24) & 0xff;
    buf[1] = (val >> 16) & 0xff;
    buf[2] = (val >> 8) & 0xff;
    buf[3] = val & 0xff;
}

int count_mp3_frames(int fd, int startpos, int filesize,
                     void (*progressfunc)(int))
{
    unsigned long header = 0;
    struct mp3info info;
    int num_frames;
    long bytes;
    int cnt;
    long progress_chunk = filesize / 50; /* Max is 50%, in 1% increments */
    int progress_cnt = 0;
    bool is_vbr = false;
    int last_bitrate = 0;
    int header_template = 0;

    if(lseek(fd, startpos, SEEK_SET) < 0)
        return -1;

    buf_init();

    /* Find out the total number of frames */
    num_frames = 0;
    cnt = 0;
    
    while((header = buf_find_next_frame(fd, &bytes, -1, header_template))) {
        mp3headerinfo(&info, header);

        if(!header_template)
            header_template = header;
        
        /* See if this really is a VBR file */
        if(last_bitrate && info.bitrate != last_bitrate)
        {
            is_vbr = true;
        }
        last_bitrate = info.bitrate;
        
        buf_seek(fd, info.frame_size-4);
        num_frames++;
        if(progressfunc)
        {
            cnt += bytes + info.frame_size;
            if(cnt > progress_chunk)
            {
                progress_cnt++;
                progressfunc(progress_cnt);
                cnt = 0;
            }
        }
    }
    DEBUGF("Total number of frames: %d\n", num_frames);

    if(is_vbr)
        return num_frames;
    else
    {
        DEBUGF("Not a VBR file\n");
        return 0;
    }
}

static const char cooltext[] = "Rockbox - rocks your box";

int create_xing_header(int fd, int startpos, int filesize,
                       unsigned char *buf, /* must be at least 288 bytes */
                       int num_frames, unsigned long header_template,
                       void (*progressfunc)(int), bool generate_toc)
{
    unsigned long header = 0;
    struct mp3info info;
    int pos, last_pos;
    int i, j;
    long bytes;
    unsigned long filepos;
    int x;
    int index;
    unsigned char toc[100];
    unsigned long xing_header_template = 0;

    DEBUGF("create_xing_header()\n");

    if(header_template)
        xing_header_template = header_template;
    
    if(generate_toc)
    {
        lseek(fd, startpos, SEEK_SET);
        buf_init();
    
        /* Generate filepos table */
        last_pos = 0;
        filepos = 0;
        header = 0;
        x = 0;
        for(i = 0;i < 100;i++) {
            /* Calculate the absolute frame number for this seek point */
            pos = i * num_frames / 100;
            
            /* Advance from the last seek point to this one */
            for(j = 0;j < pos - last_pos;j++)
            {
                header = buf_find_next_frame(fd, &bytes, -1, header_template);
                filepos += bytes;
                mp3headerinfo(&info, header);
                buf_seek(fd, info.frame_size-4);
                filepos += info.frame_size;

                if(!header_template)
                    header_template = header;
            }

            /* Save a header for later use if header_template is empty.
               We only save one header, and we want to save one in the
               middle of the stream, just in case the first and the last
               headers are corrupt. */
            if(!xing_header_template && i == 1)
                xing_header_template = header;
            
            if(progressfunc)
            {
                progressfunc(50 + i/2);
            }
            
            /* Fill in the TOC entry */
            /* each toc is a single byte indicating how many 256ths of the
             * way through the file, is that percent of the way through the
             * song. the easy method, filepos*256/filesize, chokes when
             * the upper 8 bits of the file position are nonzero 
             * (i.e. files over 16mb in size).
             */
            if (filepos > 0xFFFFFF)
            {
                /* instead of multiplying filepos by 256, we divide
                 * filesize by 256.
                 */
                toc[i] = filepos / (filesize >> 8);
            }
            else
            {
                toc[i] = filepos * 256 / filesize;
            }
            
            DEBUGF("Pos %d: %d  relpos: %d  filepos: %x tocentry: %x\n",
                   i, pos, pos-last_pos, filepos, toc[i]);
            
            last_pos = pos;
        }
    }
    
    /* Check the template header for validity and get some preliminary info. */
    if (!mp3headerinfo(&info, xing_header_template))
        return 0;  /* invalid header */

    /* Clear the frame */
    memset(buf, 0, MAX_XING_HEADER_SIZE);

    /* Use the template header and create a new one. */
    header = xing_header_template & ~(BITRATE_MASK | PROTECTION_MASK);

    /* Calculate position of VBR header and required frame bitrate */
    if (info.version == MPEG_VERSION1) {
        header |= 5 << 12;
        if (info.channel_mode == 3) /* mono */
            index = 21;
        else
            index = 36;
    }
    else {
        if (info.version == MPEG_VERSION2)
            header |= 8 << 12;
        else  /* MPEG_VERSION2_5 */
            header |= 4 << 12;
        if (info.channel_mode == 3) /* mono */
            index = 13;
        else
            index = 21;
    }
    mp3headerinfo(&info, header);  /* Get final header info */
    /* Size is now always one of 192, 208 or 288 bytes */

    /* Write the header to the buffer */
    long2bytes(buf, header);

    /* Create the Xing data */
    memcpy(&buf[index], "Xing", 4);
    long2bytes(&buf[index+4], ((num_frames?VBR_FRAMES_FLAG:0) |
                              (filesize?VBR_BYTES_FLAG:0) |
                              (generate_toc?VBR_TOC_FLAG:0)));
    index = index+8;
    if(num_frames)
    {
        long2bytes(&buf[index], num_frames);
        index += 4;
    }

    if(filesize)
    {
        long2bytes(&buf[index], filesize - startpos);
        index += 4;
    }

    /* Copy the TOC */
    memcpy(buf + index, toc, 100);

    /* And some extra cool info */
    memcpy(buf + index + 100, cooltext, sizeof(cooltext));

#ifdef DEBUG
    for(i = 0;i < info.frame_size;i++)
    {
        if(i && !(i % 16))
            DEBUGF("\n");

        DEBUGF("%02x ", buf[i]);
    }
#endif
    
    return info.frame_size;
}
