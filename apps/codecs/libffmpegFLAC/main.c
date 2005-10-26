/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Dave Chapman
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* A test program for the Rockbox version of the ffmpeg FLAC decoder.

   Compile using Makefile.test - run it as "./test file.flac" to decode the
   FLAC file to the file "test.wav" in the current directory

   This test program should support 16-bit and 24-bit mono and stereo files.

   The resulting "test.wav" should have the same md5sum as a WAV file created
   by the official FLAC decoder (it produces the same 44-byte canonical WAV 
   header.
*/

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "decoder.h"

static unsigned char wav_header[44]={
    'R','I','F','F',//  0 - ChunkID
    0,0,0,0,        //  4 - ChunkSize (filesize-8)
    'W','A','V','E',//  8 - Format
    'f','m','t',' ',// 12 - SubChunkID
    16,0,0,0,       // 16 - SubChunk1ID  // 16 for PCM
    1,0,            // 20 - AudioFormat (1=Uncompressed)
    2,0,            // 22 - NumChannels
    0,0,0,0,        // 24 - SampleRate in Hz
    0,0,0,0,        // 28 - Byte Rate (SampleRate*NumChannels*(BitsPerSample/8)
    4,0,            // 32 - BlockAlign (== NumChannels * BitsPerSample/8)
    16,0,           // 34 - BitsPerSample
    'd','a','t','a',// 36 - Subchunk2ID
    0,0,0,0         // 40 - Subchunk2Size
};

int open_wav(char* filename) {
    int fd;

    fd=open(filename,O_CREAT|O_WRONLY|O_TRUNC,S_IRUSR|S_IWUSR);
    if (fd >= 0) {
        write(fd,wav_header,sizeof(wav_header));
    }
    return(fd);
}

void close_wav(int fd, FLACContext* fc) {
    int x;
    int filesize;
    int bytespersample;

    bytespersample=fc->bps/8;

    filesize=fc->totalsamples*bytespersample*fc->channels+44;

    // ChunkSize
    x=filesize-8;
    wav_header[4]=(x&0xff);
    wav_header[5]=(x&0xff00)>>8;
    wav_header[6]=(x&0xff0000)>>16;
    wav_header[7]=(x&0xff000000)>>24;

    // Number of channels
    wav_header[22]=fc->channels;

    // Samplerate
    wav_header[24]=fc->samplerate&0xff;
    wav_header[25]=(fc->samplerate&0xff00)>>8;
    wav_header[26]=(fc->samplerate&0xff0000)>>16;
    wav_header[27]=(fc->samplerate&0xff000000)>>24;

    // ByteRate
    x=fc->samplerate*(fc->bps/8)*fc->channels;
    wav_header[28]=(x&0xff);
    wav_header[29]=(x&0xff00)>>8;
    wav_header[30]=(x&0xff0000)>>16;
    wav_header[31]=(x&0xff000000)>>24;

    // BlockAlign
    wav_header[32]=(fc->bps/8)*fc->channels;

    // Bits per sample
    wav_header[34]=fc->bps;
    
    // Subchunk2Size
    x=filesize-44;
    wav_header[40]=(x&0xff);
    wav_header[41]=(x&0xff00)>>8;
    wav_header[42]=(x&0xff0000)>>16;
    wav_header[43]=(x&0xff000000)>>24;

    lseek(fd,0,SEEK_SET);
    write(fd,wav_header,sizeof(wav_header));
    close(fd);
}

static void dump_headers(FLACContext *s)
{
    fprintf(stderr,"  Blocksize: %d .. %d\n", s->min_blocksize, 
                   s->max_blocksize);
    fprintf(stderr,"  Framesize: %d .. %d\n", s->min_framesize, 
                   s->max_framesize);
    fprintf(stderr,"  Samplerate: %d\n", s->samplerate);
    fprintf(stderr,"  Channels: %d\n", s->channels);
    fprintf(stderr,"  Bits per sample: %d\n", s->bps);
    fprintf(stderr,"  Metadata length: %d\n", s->metadatalength);
    fprintf(stderr,"  Total Samples: %lu\n",s->totalsamples);
    fprintf(stderr,"  Duration: %d ms\n",s->length);
    fprintf(stderr,"  Bitrate: %d kbps\n",s->bitrate);
}

static bool flac_init(int fd, FLACContext* fc)
{
    unsigned char buf[255];
    struct stat statbuf;
    bool found_streaminfo=false;
    int endofmetadata=0;
    int blocklength;

    if (lseek(fd, 0, SEEK_SET) < 0) 
    {
        return false;
    }

    if (read(fd, buf, 4) < 4)
    {
        return false;
    }

    if (memcmp(buf,"fLaC",4) != 0) 
    {
        return false;
    }
    fc->metadatalength = 4;

    while (!endofmetadata) {
        if (read(fd, buf, 4) < 4)
        {
            return false;
        }

        endofmetadata=(buf[0]&0x80);
        blocklength = (buf[1] << 16) | (buf[2] << 8) | buf[3];
        fc->metadatalength+=blocklength+4;

        if ((buf[0] & 0x7f) == 0)       /* 0 is the STREAMINFO block */
        {
            /* FIXME: Don't trust the value of blocklength */
            if (read(fd, buf, blocklength) < 0)
            {
                return false;
            }
          
            fstat(fd,&statbuf);
            fc->filesize = statbuf.st_size;
            fc->min_blocksize = (buf[0] << 8) | buf[1];
            fc->max_blocksize = (buf[2] << 8) | buf[3];
            fc->min_framesize = (buf[4] << 16) | (buf[5] << 8) | buf[6];
            fc->max_framesize = (buf[7] << 16) | (buf[8] << 8) | buf[9];
            fc->samplerate = (buf[10] << 12) | (buf[11] << 4) 
                             | ((buf[12] & 0xf0) >> 4);
            fc->channels = ((buf[12]&0x0e)>>1) + 1;
            fc->bps = (((buf[12]&0x01) << 4) | ((buf[13]&0xf0)>>4) ) + 1;

            /* totalsamples is a 36-bit field, but we assume <= 32 bits are 
               used */
            fc->totalsamples = (buf[14] << 24) | (buf[15] << 16) 
                               | (buf[16] << 8) | buf[17];

            /* Calculate track length (in ms) and estimate the bitrate 
               (in kbit/s) */
            fc->length = (fc->totalsamples / fc->samplerate) * 1000;

            found_streaminfo=true;
        } else if ((buf[0] & 0x7f) == 3) { /* 3 is the SEEKTABLE block */
            fprintf(stderr,"Seektable length = %d bytes\n",blocklength);
            if (lseek(fd, blocklength, SEEK_CUR) < 0) {
                return false;
          }
        } else {
            /* Skip to next metadata block */
            if (lseek(fd, blocklength, SEEK_CUR) < 0)
            {
                return false;
            }
        }
    }

   if (found_streaminfo) {
       fc->bitrate = ((fc->filesize-fc->metadatalength) * 8) / fc->length;
       return true;
   } else {
       return false;
   }
}

/* Dummy function needed to pass to flac_decode_frame() */
void yield() {
}

int main(int argc, char* argv[]) {
    FLACContext fc;
    int fd,fdout;
    int n;
    int i;
    int bytesleft;
    int consumed;
    char buf[MAX_FRAMESIZE]; /* The input buffer */
    /* The output buffers containing the decoded samples (channels 0 and 1) */
    int32_t decoded0[MAX_BLOCKSIZE];
    int32_t decoded1[MAX_BLOCKSIZE];

    /* For testing */
    int8_t wavbuf[MAX_CHANNELS*MAX_BLOCKSIZE*3];
    int8_t* p;
    int scale;

    fd=open(argv[1],O_RDONLY);

    if (fd < 0) {
        fprintf(stderr,"Can not parse %s\n",argv[1]);
        return(1);
    }

    /* Read the metadata and position the file pointer at the start of the 
       first audio frame */
    flac_init(fd,&fc);

    dump_headers(&fc);

    fdout=open_wav("test.wav");
    bytesleft=read(fd,buf,sizeof(buf));
    while (bytesleft) {
        if(flac_decode_frame(&fc,decoded0,decoded1,buf,bytesleft,yield) < 0) {
             fprintf(stderr,"DECODE ERROR, ABORTING\n");
             break;
        }
        consumed=fc.gb.index/8;

        scale=FLAC_OUTPUT_DEPTH-fc.bps;
        p=wavbuf;
        for (i=0;i<fc.blocksize;i++) {
             /* Left sample */
             decoded0[i]=decoded0[i]>>scale;
             *(p++)=decoded0[i]&0xff;
             *(p++)=(decoded0[i]&0xff00)>>8;
             if (fc.bps==24) *(p++)=(decoded0[i]&0xff0000)>>16;

             if (fc.channels==2) {
                 /* Right sample */
                 decoded1[i]=decoded1[i]>>scale;
                 *(p++)=decoded1[i]&0xff;
                 *(p++)=(decoded1[i]&0xff00)>>8;
                 if (fc.bps==24) *(p++)=(decoded1[i]&0xff0000)>>16;
             }
        }
        write(fdout,wavbuf,fc.blocksize*fc.channels*(fc.bps/8));

        memmove(buf,&buf[consumed],bytesleft-consumed);
        bytesleft-=consumed;

        n=read(fd,&buf[bytesleft],sizeof(buf)-bytesleft);
        if (n > 0) {
            bytesleft+=n;
        }
    }
    close_wav(fdout,&fc);
    close(fd);
    return(0);
}
