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
#include <limits.h>
#include "debug.h"
#include "logf.h"
#include "mp3data.h"
#include "file.h"
#include "buffer.h"

#define DEBUG_VERBOSE

#define BYTES2INT(b1,b2,b3,b4) (((long)(b1 & 0xFF) << (3*8)) | \
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
static const long freq_table[3][3] =
{
    {44100, 48000, 32000}, /* MPEG Version 1 */
    {22050, 24000, 16000}, /* MPEG version 2 */
    {11025, 12000,  8000}, /* MPEG version 2.5 */
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

    info->protection = (header & PROTECTION_MASK) ? true : false;
    
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
    info->mode_extension = (header & MODE_EXT_MASK) >> 4;
    info->emphasis = header & EMPHASIS_MASK;

#ifdef DEBUG_VERBOSE
    DEBUGF( "Header: %08x, Ver %d, lay %d, bitr %d, freq %ld, "
            "chmode %d, mode_ext %d, emph %d, bytes: %d time: %d/%d\n",
            header, info->version, info->layer+1, info->bitrate,
            info->frequency, info->channel_mode, info->mode_extension,
            info->emphasis, info->frame_size, info->ft_num, info->ft_den);
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
    info->enc_delay = -1;
    info->enc_padding = -1;
    if(!mp3headerinfo(info, header))
        return -2;

    /* OK, we have found a frame. Let's see if it has a Xing header */
    if (info->frame_size-4 >= sizeof(frame))
    {
#if defined(DEBUG) || defined(SIMULATOR)
        DEBUGF("Error: Invalid id3 header, frame_size: %d\n", info->frame_size);
#endif
        return -8;
    }
    
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

        if (vbrheader[7] & VBR_FRAMES_FLAG) /* Is the frame count there? */
        {
            info->frame_count = BYTES2INT(vbrheader[i], vbrheader[i+1],
                                          vbrheader[i+2], vbrheader[i+3]);
            if (info->frame_count <= ULONG_MAX / info->ft_num)
                info->file_time = info->frame_count * info->ft_num / info->ft_den;
            else
                info->file_time = info->frame_count / info->ft_den * info->ft_num;
            i += 4;
        }

        if (vbrheader[7] & VBR_BYTES_FLAG) /* Is byte count there? */
        {
            info->byte_count = BYTES2INT(vbrheader[i], vbrheader[i+1],
                                         vbrheader[i+2], vbrheader[i+3]);
            i += 4;
        }

        if (info->file_time && info->byte_count)
        {
            if (info->byte_count <= (ULONG_MAX/8))
                info->bitrate = info->byte_count * 8 / info->file_time;
            else
                info->bitrate = info->byte_count / (info->file_time >> 3);
        }
        else
            info->bitrate = 0;

        if (vbrheader[7] & VBR_TOC_FLAG) /* Is table-of-contents there? */
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
        /* TODO: This sanity checking is rather silly, seeing as how the LAME
           header contains a CRC field that can be used to verify integrity. */
        if (!(info->enc_delay >= 0 && info->enc_delay <= 2880 && 
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
        if (info->frame_count <= ULONG_MAX / info->ft_num)
            info->file_time = info->frame_count * info->ft_num / info->ft_den;
        else
            info->file_time = info->frame_count / info->ft_den * info->ft_num;

        if (info->byte_count <= (ULONG_MAX/8))
            info->bitrate = info->byte_count * 8 / info->file_time;
        else
            info->bitrate = info->byte_count / (info->file_time >> 3);

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

/* buf needs to be the audio buffer with TOC generation enabled,
   and at least MAX_XING_HEADER_SIZE bytes otherwise */
int create_xing_header(int fd, long startpos, long filesize,
                       unsigned char *buf, unsigned long num_frames,
                       unsigned long rec_time, unsigned long header_template,
                       void (*progressfunc)(int), bool generate_toc)
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
        buf_init();

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
            
            DEBUGF("Pos %d: %d  relpos: %d  filepos: %x tocentry: %x\n",
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
