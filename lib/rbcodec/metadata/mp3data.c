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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
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
#include <limits.h>
#include "debug.h"
#include "logf.h"
#include "mp3data.h"
#include "platform.h"

#include "metadata.h"
#include "metadata/metadata_parsers.h"

//#define DEBUG_VERBOSE

#ifdef DEBUG_VERBOSE
#define VDEBUGF DEBUGF
#else
#define VDEBUGF(...) do { } while(0)
#endif

#define SYNC_MASK           (0x7ffL << 21)
#define VERSION_MASK        (3L << 19)
#define LAYER_MASK          (3L << 17)
#define PROTECTION_MASK     (1L << 16)
#define BITRATE_MASK        (0xfL << 12)
#define SAMPLERATE_MASK     (3L << 10)
#define PADDING_MASK        (1L << 9)
#define PRIVATE_MASK        (1L << 8)
#define CHANNELMODE_MASK    (3L << 6)
#define MODE_EXT_MASK       (3L << 4)
#define COPYRIGHT_MASK      (1L << 3)
#define ORIGINAL_MASK       (1L << 2)
#define EMPHASIS_MASK       (3L)

/* Maximum number of bytes needed by Xing/Info/VBRI parser. */
#define VBR_HEADER_MAX_SIZE (180)

/* MPEG Version table, sorted by version index */
static const signed char version_table[4] = {
    MPEG_VERSION2_5, -1, MPEG_VERSION2, MPEG_VERSION1
};

/* Bitrate table for mpeg audio, indexed by row index and birate index */
static const short bitrates[5][16] = {
    {0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,0}, /* V1 L1 */
    {0,32,48,56, 64, 80, 96,112,128,160,192,224,256,320,384,0}, /* V1 L2 */
    {0,32,40,48, 56, 64, 80, 96,112,128,160,192,224,256,320,0}, /* V1 L3 */
    {0,32,48,56, 64, 80, 96,112,128,144,160,176,192,224,256,0}, /* V2 L1 */
    {0, 8,16,24, 32, 40, 48, 56, 64, 80, 96,112,128,144,160,0}  /* V2 L2+L3 */
};

/* Bitrate pointer table, indexed by version and layer */
static const short *bitrate_table[3][3] =
{
    {bitrates[0], bitrates[1], bitrates[2]},
    {bitrates[3], bitrates[4], bitrates[4]},
    {bitrates[3], bitrates[4], bitrates[4]}
};

/* Sampling frequency table, indexed by version and frequency index */
static const unsigned short freq_table[3][3] =
{
    {44100, 48000, 32000}, /* MPEG Version 1 */
    {22050, 24000, 16000}, /* MPEG version 2 */
    {11025, 12000,  8000}, /* MPEG version 2.5 */
};

unsigned long bytes2int(unsigned long b0, unsigned long b1,
                        unsigned long b2, unsigned long b3)
{
   return (b0 & 0xFF) << (3*8) |
          (b1 & 0xFF) << (2*8) |
          (b2 & 0xFF) << (1*8) |
          (b3 & 0xFF) << (0*8);
}

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
    int bitindex, freqindex;

    /* MPEG Audio Version */
    if ((header & VERSION_MASK) >> 19 >= sizeof(version_table))
        return false;
    
    info->version = version_table[(header & VERSION_MASK) >> 19];
    if (info->version < 0)
        return false;

    /* Layer */
    info->layer = 3 - ((header & LAYER_MASK) >> 17);
    if (info->layer == 3)
        return false;

/* Rockbox: not used
    info->protection = (header & PROTECTION_MASK) ? true : false;
*/

    /* Bitrate */
    bitindex = (header & BITRATE_MASK) >> 12;
    info->bitrate = bitrate_table[info->version][info->layer][bitindex];
    if(info->bitrate == 0)
        return false;

    /* Sampling frequency */
    freqindex = (header & SAMPLERATE_MASK) >> 10;
    if (freqindex == 3)
        return false;
    info->frequency = freq_table[info->version][freqindex];

    info->padding = (header & PADDING_MASK) ? 1 : 0;

    /* Calculate number of bytes, calculation depends on layer */
    if (info->layer == 0) {
        info->frame_samples = 384;
        info->frame_size = (12000 * info->bitrate / info->frequency 
                            + info->padding) * 4;
    }
    else {
        if ((info->version > MPEG_VERSION1) && (info->layer == 2))
            info->frame_samples = 576;
        else
            info->frame_samples = 1152;
        info->frame_size = (1000/8) * info->frame_samples * info->bitrate
                           / info->frequency + info->padding;
    }
    
    /* Frametime fraction denominator */
    if (freqindex != 0) {      /* 48/32/24/16/12/8 kHz */
        info->ft_den = 1;      /* integer number of milliseconds */
    }
    else {                     /* 44.1/22.05/11.025 kHz */
        if (info->layer == 0)     /* layer 1 */
            info->ft_den = 147;
        else                      /* layer 2+3 */
            info->ft_den = 49;
    }
    /* Frametime fraction numerator */
    info->ft_num = 1000 * info->ft_den * info->frame_samples / info->frequency;

    info->channel_mode = (header & CHANNELMODE_MASK) >> 6;
/* Rockbox: not used
    info->mode_extension = (header & MODE_EXT_MASK) >> 4;
    info->emphasis = header & EMPHASIS_MASK;
*/
    VDEBUGF( "Header: %08lx, Ver %d, lay %d, bitr %d, freq %ld, "
            "chmode %d, bytes: %d time: %d/%d\n",
            header, info->version, info->layer+1, info->bitrate,
            info->frequency, info->channel_mode,
            info->frame_size, info->ft_num, info->ft_den);
    return true;
}

static bool headers_have_same_type(unsigned long header1, 
                                   unsigned long header2)
{
    /* Compare MPEG version, layer and sampling frequency. If header1 is zero
     * it is assumed both frame headers are of same type. */
    unsigned int mask = SYNC_MASK | VERSION_MASK | LAYER_MASK | SAMPLERATE_MASK;
    header1 &= mask;
    header2 &= mask;
    return header1 ? (header1 == header2) : true;
}

/* Helper function to read 4-byte in big endian format. */
static void read_uint32be_mp3data(int fd, unsigned long *data)
{
#ifdef ROCKBOX_BIG_ENDIAN
    (void)read(fd, (char*)data, 4);
#else
    (void)read(fd, (char*)data, 4);
    *data = betoh32(*data);
#endif
}

static unsigned long __find_next_frame(int fd, long *offset, long max_offset,
                                       unsigned long reference_header,
                                       int(*getfunc)(int fd, unsigned char *c),
                                       bool single_header)
{
    unsigned long header=0;
    unsigned char tmp;
    long pos      = 0;

    /* We will search until we find two consecutive MPEG frame headers with 
     * the same MPEG version, layer and sampling frequency. The first header
     * of this pair is assumed to be the first valid MPEG frame header of the
     * whole stream. */
    do {
        /* Read 1 new byte. */
        header <<= 8;
        if (!getfunc(fd, &tmp))
            return 0;
        header |= tmp;
        pos++;
        
        /* Abort if max_offset is reached. Stop parsing. */
        if (max_offset > 0 && pos > max_offset)
            return 0;
        
        if (is_mp3frameheader(header)) {
            if (single_header) {
                /* We search for one _single_ valid header that has the same
                 * type as the reference_header (if reference_header != 0). 
                 * In this case we are finished. */
                if (headers_have_same_type(reference_header, header))
                    break;
            } else {
                /* The current header is valid. Now gather the frame size,
                 * seek to this byte position and check if there is another
                 * valid MPEG frame header of the same type. */
                struct mp3info info;
                
                /* Gather frame size from given header and seek to next
                 * frame header. */
                mp3headerinfo(&info, header);
                lseek(fd, info.frame_size-4, SEEK_CUR);
                
                /* Read possible next frame header and seek back to last frame
                 * headers byte position. */
                reference_header = 0;
                read_uint32be_mp3data(fd, &reference_header);
                //
                lseek(fd, -info.frame_size, SEEK_CUR);
                
                /* If the current header is of the same type as the previous 
                 * header we are finished. */
                if (headers_have_same_type(header, reference_header))
                    break;
            }
        }
  
    } while (true);

    *offset = pos - 4;

    if(*offset)
        VDEBUGF("Warning: skipping %ld bytes of garbage\n", *offset);

    return header;
}

static int fileread(int fd, unsigned char *c)
{    
    return read(fd, c, 1);
}

unsigned long find_next_frame(int fd, 
                              long *offset, 
                              long max_offset,
                              unsigned long reference_header)
{
    return __find_next_frame(fd, offset, max_offset, reference_header, 
                             fileread, true);
}

#ifndef __PCTOOL__
static int fnf_read_index;
static int fnf_buf_len;
static unsigned char *fnf_buf;

static int buf_getbyte(int fd, unsigned char *c)
{
    if(fnf_read_index < fnf_buf_len)
    {
        *c = fnf_buf[fnf_read_index++];
        return 1;
    }
    else
    {
        fnf_buf_len = read(fd, fnf_buf, fnf_buf_len);
        if(fnf_buf_len < 0)
            return -1;

        fnf_read_index = 0;

        if(fnf_buf_len > 0)
        {
            *c = fnf_buf[fnf_read_index++];
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
    if(fnf_read_index >= fnf_buf_len)
    {
        len = fnf_read_index - fnf_buf_len;
        
        fnf_buf_len = read(fd, fnf_buf, fnf_buf_len);
        if(fnf_buf_len < 0)
            return -1;

        fnf_read_index = len;
    }
    
    if(fnf_read_index >= fnf_buf_len)
    {
        return -1;
    }

    return 0;
}

static void buf_init(unsigned char* buf, size_t buflen)
{
    fnf_buf = buf;
    fnf_buf_len = buflen;
    fnf_read_index = buflen;
}

static unsigned long buf_find_next_frame(int fd, long *offset, long max_offset)
{
    return __find_next_frame(fd, offset, max_offset, 0, buf_getbyte, true);
}

static size_t mem_buflen;
static unsigned char* mem_buf;
static size_t mem_pos;
static int mem_cnt;
static int mem_maxlen;

static int mem_getbyte(int dummy, unsigned char *c)
{
    (void)dummy;

    *c = mem_buf[mem_pos++];
    if(mem_pos >= mem_buflen)
        mem_pos = 0;

    if(mem_cnt++ >= mem_maxlen)
        return 0;
    else
        return 1;
}

unsigned long mem_find_next_frame(int startpos, 
                                  long *offset, 
                                  long max_offset,
                                  unsigned long reference_header,
                                  unsigned char* buf, size_t buflen)
{
    mem_buf = buf;
    mem_buflen = buflen;
    mem_pos = startpos;
    mem_cnt = 0;
    mem_maxlen = max_offset;

    return __find_next_frame(0, offset, max_offset, reference_header, 
                             mem_getbyte, true);
}
#endif

/* Extract information from a 'Xing' or 'Info' header. */
static void get_xing_info(struct mp3info *info, unsigned char *buf)
{
    int i = 8;

    /* Is it a VBR file? */
    info->is_vbr = !memcmp(buf, "Xing", 4);

    if (buf[7] & VBR_FRAMES_FLAG) /* Is the frame count there? */
    {
        info->frame_count = bytes2int(buf[i], buf[i+1], buf[i+2], buf[i+3]);
        if (info->frame_count <= ULONG_MAX / info->ft_num)
            info->file_time = info->frame_count * info->ft_num / info->ft_den;
        else
            info->file_time = info->frame_count / info->ft_den * info->ft_num;
        i += 4;
    }

    if (buf[7] & VBR_BYTES_FLAG) /* Is byte count there? */
    {
        info->byte_count = bytes2int(buf[i], buf[i+1], buf[i+2], buf[i+3]);
        i += 4;
    }

    if (info->file_time && info->byte_count)
    {
        if (info->byte_count <= (ULONG_MAX/8))
            info->bitrate = info->byte_count * 8 / info->file_time;
        else
            info->bitrate = info->byte_count / (info->file_time >> 3);
    }

    if (buf[7] & VBR_TOC_FLAG) /* Is table-of-contents there? */
    {
        info->has_toc = true;
        memcpy( info->toc, buf+i, 100 );
        i += 100;
    }
    if (buf[7] & VBR_QUALITY_FLAG)
    {
        /* We don't care about this, but need to skip it */
        i += 4;
    }
#if CONFIG_CODEC==SWCODEC
    i += 21;
    info->enc_delay   = ((int)buf[i  ] << 4) | (buf[i+1] >> 4);
    info->enc_padding = ((int)(buf[i+1]&0xF) << 8) |  buf[i+2];
    /* TODO: This sanity checking is rather silly, seeing as how the LAME
       header contains a CRC field that can be used to verify integrity. */
    if (!(info->enc_delay   >= 0 && info->enc_delay   <= 2880 &&
          info->enc_padding >= 0 && info->enc_padding <= 2*1152))
    {
       /* Invalid data */
       info->enc_delay   = -1;
       info->enc_padding = -1;
    }
#endif
}

/* Extract information from a 'VBRI' header. */
static void get_vbri_info(struct mp3info *info, unsigned char *buf)
{
    /* We don't parse the TOC, since we don't yet know how to (FIXME) */
    /* 
    int i, num_offsets, offset = 0;
    */

    info->is_vbr  = true;  /* Yes, it is a FhG VBR file */
    info->has_toc = false; /* We don't parse the TOC (yet) */

    info->byte_count  = bytes2int(buf[10], buf[11], buf[12], buf[13]);
    info->frame_count = bytes2int(buf[14], buf[15], buf[16], buf[17]);
    if (info->frame_count <= ULONG_MAX / info->ft_num)
        info->file_time = info->frame_count * info->ft_num / info->ft_den;
    else
        info->file_time = info->frame_count / info->ft_den * info->ft_num;

    if (info->byte_count <= (ULONG_MAX/8))
        info->bitrate = info->byte_count * 8 / info->file_time;
    else
        info->bitrate = info->byte_count / (info->file_time >> 3);

    VDEBUGF("Frame size (%dkpbs): %d bytes (0x%x)\n",
           info->bitrate, info->frame_size, info->frame_size);
    VDEBUGF("Frame count: %lx\n", info->frame_count);
    VDEBUGF("Byte count: %lx\n", info->byte_count);

    /* We don't parse the TOC, since we don't yet know how to (FIXME) */
    /*
    num_offsets = bytes2int(0, 0, buf[18], buf[19]);
    VDEBUGF("Offsets: %d\n", num_offsets);
    VDEBUGF("Frames/entry: %ld\n", bytes2int(0, 0, buf[24], buf[25]));

    for(i = 0; i < num_offsets; i++)
    {
       offset += bytes2int(0, 0, buf[26+i*2], buf[27+i*2]);;
       VDEBUGF("%03d: %lx\n", i, offset - bytecount,);
    }
    */
}

/* Seek to next mpeg header and extract relevant information. */
static int get_next_header_info(int fd, long *bytecount, struct mp3info *info,
                                bool single_header)
{
    long tmp;
    unsigned long header = 0;
    
    header = __find_next_frame(fd, &tmp, 0x20000, 0, fileread, single_header);
    if(header == 0)
        return -1;

    if(!mp3headerinfo(info, header))
        return -2;

    /* Next frame header is tmp bytes away. */
    *bytecount += tmp;
        
    return 0;
}

int get_mp3file_info(int fd, struct mp3info *info)
{
    unsigned char frame[VBR_HEADER_MAX_SIZE], *vbrheader;
    long bytecount = 0;
    int result, buf_size;

    /* Initialize info and frame */
    memset(info,  0, sizeof(struct mp3info));
    memset(frame, 0, sizeof(frame));
    
#if CONFIG_CODEC==SWCODEC
    /* These two are needed for proper LAME gapless MP3 playback */
    info->enc_delay   = -1;
    info->enc_padding = -1;
#endif

    /* Get the very first single MPEG frame. */
    result = get_next_header_info(fd, &bytecount, info, true);
    if(result)
        return result;
    
    /* Read the amount of frame data to the buffer that is required for the 
     * vbr tag parsing. Skip the rest. */
    buf_size = MIN(info->frame_size-4, (int)sizeof(frame));
    if(read(fd, frame, buf_size) < 0)
        return -3;
    lseek(fd, info->frame_size - 4 - buf_size, SEEK_CUR);

    /* Calculate position of a possible VBR header */
    if (info->version == MPEG_VERSION1) {
        if (info->channel_mode == 3) /* mono */
            vbrheader = frame + 17;
        else
            vbrheader = frame + 32;
    } else {
        if (info->channel_mode == 3) /* mono */
            vbrheader = frame + 9;
        else
            vbrheader = frame + 17;
    }

    if (!memcmp(vbrheader, "Xing", 4) || !memcmp(vbrheader, "Info", 4))
    {
        VDEBUGF("-- XING header --\n");

        /* We want to skip the Xing frame when playing the stream */
        bytecount += info->frame_size;
        
        /* Now get the next frame to read the real info about the mp3 stream */
        result = get_next_header_info(fd, &bytecount, info, false);
        if(result)
            return result;
            
        get_xing_info(info, vbrheader);
    }
    else if (!memcmp(vbrheader, "VBRI", 4))
    {
        VDEBUGF("-- VBRI header --\n");

        /* We want to skip the VBRI frame when playing the stream */
        bytecount += info->frame_size;
        
        /* Now get the next frame to read the real info about the mp3 stream */
        result = get_next_header_info(fd, &bytecount, info, false);
        if(result)
            return result;
            
        get_vbri_info(info, vbrheader);
    }
    else
    {
        long offset;

        VDEBUGF("-- No VBR header --\n");
        
        /* There was no VBR header found. So, we seek back to beginning and
         * search for the first MPEG frame header of the mp3 stream. */
        offset = lseek(fd, -info->frame_size, SEEK_CUR);
        result = get_next_header_info(fd, &bytecount, info, false);
        if(result)
            return result;

        info->byte_count = filesize(fd) - getid3v1len(fd) - offset - bytecount;
    }

    return bytecount;
}

#ifndef __PCTOOL__
static void long2bytes(unsigned char *buf, long val)
{
    buf[0] = (val >> 24) & 0xff;
    buf[1] = (val >> 16) & 0xff;
    buf[2] = (val >> 8) & 0xff;
    buf[3] = val & 0xff;
}

int count_mp3_frames(int fd,  int startpos,  int filesize,
                     void (*progressfunc)(int),
                     unsigned char* buf, size_t buflen)
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

    buf_init(buf, buflen);

    /* Find out the total number of frames */
    num_frames = 0;
    cnt = 0;
    
    while((header = buf_find_next_frame(fd, &bytes, startpos + filesize))) {
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
    VDEBUGF("Total number of frames: %d\n", num_frames);

    if(is_vbr)
        return num_frames;
    else
    {
        DEBUGF("Not a VBR file\n");
        return 0;
    }
}

static const char cooltext[] = "Rockbox - rocks your box";

/* buf needs to be the audio buffer with TOC generation enabled,
   and at least MAX_XING_HEADER_SIZE bytes otherwise */
int create_xing_header(int fd, long startpos, long filesize,
                       unsigned char *buf, unsigned long num_frames,
                       unsigned long rec_time, unsigned long header_template,
                       void (*progressfunc)(int), bool generate_toc,
                       unsigned char *tempbuf, size_t tempbuflen )
{   
    struct mp3info info;
    unsigned char toc[100];
    unsigned long header = 0;
    unsigned long xing_header_template = header_template;
    unsigned long filepos;
    long pos, last_pos;
    long j;
    long bytes;
    int i;
    int index;

    DEBUGF("create_xing_header()\n");

    if(generate_toc)
    {
        lseek(fd, startpos, SEEK_SET);
        buf_init(tempbuf, tempbuflen);

        /* Generate filepos table */
        last_pos = 0;
        filepos = 0;
        header = 0;
        for(i = 0;i < 100;i++) {
            /* Calculate the absolute frame number for this seek point */
            pos = i * num_frames / 100;
            
            /* Advance from the last seek point to this one */
            for(j = 0;j < pos - last_pos;j++)
            {
                header = buf_find_next_frame(fd, &bytes, startpos + filesize);
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
            if (filepos > (ULONG_MAX/256))
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
            
            VDEBUGF("Pos %d: %ld  relpos: %ld  filepos: %lx tocentry: %x\n",
                   i, pos, pos-last_pos, filepos, toc[i]);
            
            last_pos = pos;
        }
    }
    
    /* Use the template header and create a new one.
       We ignore the Protection bit even if the rest of the stream is
       protected. */
    header = xing_header_template & ~(BITRATE_MASK|PROTECTION_MASK|PADDING_MASK);
    header |= 8 << 12; /* This gives us plenty of space, 192..576 bytes */

    if (!mp3headerinfo(&info, header))
        return 0;  /* invalid header */

    if (num_frames == 0 && rec_time) {
        /* estimate the number of frames based on the recording time */
        if (rec_time <= ULONG_MAX / info.ft_den)
            num_frames = rec_time * info.ft_den / info.ft_num;
        else
            num_frames = rec_time / info.ft_num * info.ft_den;
    }

    /* Clear the frame */
    memset(buf, 0, MAX_XING_HEADER_SIZE);

    /* Write the header to the buffer */
    long2bytes(buf, header);

    /* Calculate position of VBR header */
    if (info.version == MPEG_VERSION1) {
        if (info.channel_mode == 3) /* mono */
            index = 21;
        else
            index = 36;
    }
    else {
        if (info.channel_mode == 3) /* mono */
            index = 13;
        else
            index = 21;
    }

    /* Create the Xing data */
    memcpy(&buf[index], "Xing", 4);
    long2bytes(&buf[index+4], (num_frames ? VBR_FRAMES_FLAG : 0)
                              | (filesize ? VBR_BYTES_FLAG : 0)
                              | (generate_toc ? VBR_TOC_FLAG : 0));
    index += 8;
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

#endif
