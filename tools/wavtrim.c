/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 by Jörg Hohensohn
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * Details at http://www.rockbox.org/twiki/bin/view/Main/VoiceBuilding
 *
 ****************************************************************************/


#include <stdio.h>  /* for file I/O */
#include <stdlib.h> /* for malloc */

#include "wavtrim.h"

/* place a 32 bit value into memory, little endian */
void Write32(unsigned char* pByte, unsigned long value)
{
    pByte[0] = (unsigned char)value;    
    pByte[1] = (unsigned char)(value >> 8);    
    pByte[2] = (unsigned char)(value >> 16);
    pByte[3] = (unsigned char)(value >> 24) ;    
}


/* read a 32 bit value from memory, little endian */
unsigned long Read32(unsigned char* pByte)
{
    unsigned long value = 0;

    value |= (unsigned long)pByte[0];
    value |= (unsigned long)pByte[1] << 8;
    value |= (unsigned long)pByte[2] << 16;
    value |= (unsigned long)pByte[3] << 24;

    return value;
}


/* place a 16 bit value into memory, little endian */
void Write16(unsigned char* pByte, unsigned short value)
{
    pByte[0] = (unsigned char)value;    
    pByte[1] = (unsigned char)(value >> 8);    
}


/* read a 16 bit value from memory, little endian */
unsigned long Read16(unsigned char* pByte)
{
    unsigned short value = 0;

    value |= (unsigned short)pByte[0];
    value |= (unsigned short)pByte[1] << 8;

    return value;
}

int wavtrim(char * filename, int maxsilence ,char* errstring,int errsize)
{    
    FILE* pFile;
    long lFileSize, lGot;
    unsigned char* pBuf;
    int bps; /* byte per sample */
    int sps; /* samples per second */
    int datapos; /* where the payload starts */
    int datalen; /* Length of the data chunk */
    unsigned char *databuf; /* Pointer to the data chunk payload */
    int skip_head, skip_tail, pad_head, pad_tail;
    int i;
    int max_silence = maxsilence;
    signed char sample8;
    short sample16;

    pFile = fopen(filename, "rb");
    if (pFile == NULL)
    {
        snprintf(errstring,errsize,"Error opening file %s for reading\n", filename);
        return -1;
    }
    
    fseek(pFile, 0, SEEK_END);
    lFileSize = ftell(pFile);
    fseek(pFile, 0, SEEK_SET);

    pBuf = malloc(lFileSize);
    if (pBuf == NULL)
    {
        snprintf(errstring,errsize,"Out of memory to allocate %ld bytes for file.\n", lFileSize);
        fclose(pFile);
        return -1;
    }

    lGot = fread(pBuf, 1, lFileSize, pFile);
    fclose(pFile);
    if (lGot != lFileSize)
    {
        snprintf(errstring,errsize,"File read error, got only %ld bytes out of %ld.\n", lGot, lFileSize);
        free(pBuf);
        return -1;
    }
    
    
    bps = Read16(pBuf + 32);
    datapos = 28 + Read16(pBuf + 16);
    databuf = &pBuf[datapos];

    if (Read32(pBuf) != 0x46464952 /* "RIFF" */
     || Read32(pBuf+8) != 0x45564157 /* "WAVE" */
     || Read32(pBuf+12) != 0x20746d66 /* "fmt " */
     || Read32(pBuf+datapos-8) != 0x61746164) /* "data" */
    {
        snprintf(errstring,errsize,"No valid input WAV file?\n");
        free(pBuf);
        return -1;
    }

    datalen = Read32(pBuf+datapos-4);
    
    sps = Read32(pBuf + 24);
    pad_head = sps * 10 / 1000; /* 10 ms */
    pad_tail = sps * 10 / 1000; /* 10 ms */

    if (bps == 1) /* 8 bit samples */
    {
        
        max_silence >>= 8;

        /* clip the start */
        for (i=0; i<datalen; i++)
        {
            sample8 = databuf[i] - 0x80;
            if (abs(sample8) > max_silence)
                break;
        }
        skip_head = i;
        skip_head = (skip_head > pad_head) ? skip_head - pad_head : 0;
        
        /* clip the end */
        for (i=datalen-1; i>skip_head; i--)
        {
            sample8 = databuf[i] - 0x80;
            if (abs(sample8) > max_silence)
                break;
        }
        skip_tail = datalen - 1 - i;
        skip_tail = (skip_tail > pad_tail) ? skip_tail - pad_tail : 0;
    }
    else if (bps == 2) /* 16 bit samples */
    {

        /* clip the start */
        for (i=0; i<datalen; i+=2)
        {
            sample16 = *(short *)(databuf + i);
            if (abs(sample16) > max_silence)
                break;
        }
        skip_head = i;
        skip_head = (skip_head > 2 * pad_head) ?
                     skip_head - 2 * pad_head : 0;

        /* clip the end */
        for (i=datalen-2; i>skip_head; i-=2)
        {
            sample16 = *(short *)(databuf + i);
            if (abs(sample16) > max_silence)
                break;
        }
        skip_tail = datalen - 2 - i;
        skip_tail = (skip_tail > 2 * pad_tail) ?
                     skip_tail - 2 * pad_tail : 0;
    }

    /* update the size in the headers */
    Write32(pBuf+4, Read32(pBuf+4) - skip_head - skip_tail);
    Write32(pBuf+datapos-4, datalen - skip_head - skip_tail);

    pFile = fopen(filename, "wb");
    if (pFile == NULL)
    {
        snprintf(errstring,errsize,"Error opening file %s for writing\n",filename);
        return -1;
    }

    /* write the new file */
    fwrite(pBuf, 1, datapos, pFile); /* write header */
    fwrite(pBuf + datapos + skip_head, 1, datalen - skip_head - skip_tail, pFile);
    fclose(pFile);

    free(pBuf);
    return 0;

}

#ifndef RBUTIL
int main (int argc, char** argv)
{
    int max_silence = 0;
    char errbuffer[255];
    int ret=0;
    
    if (argc < 2)
    {
        printf("wavtrim removes silence at the begin and end of a WAV file.\n");
        printf("usage: wavtrim <filename.wav> [<max_silence>]\n");
        return 0;
    }
    
    if (argc == 3)
    {
        max_silence = atoi(argv[2]);
    }
    
    
    ret = wavtrim(argv[1],max_silence,errbuffer,255 ); 
    if( ret< 0)
    {
        printf(errbuffer);
    }
    return ret;
}
#endif
/*
RIFF Chunk (12 bytes in length total) 
0 - 3  "RIFF" (ASCII Characters)
4 - 7  Total Length Of Package To Follow (Binary, little endian)
8 - 11  "WAVE" (ASCII Characters)


FORMAT Chunk (24 or 26 bytes in length total) Byte Number
12 - 15  "fmt_" (ASCII Characters)
16 - 19  Length Of FORMAT Chunk (Binary, 0x10 or 0x12 seen)
20 - 21  Always 0x01
22 - 23  Channel Numbers (Always 0x01=Mono, 0x02=Stereo)
24 - 27 Sample Rate (Binary, in Hz)
28 - 31 Bytes Per Second
32 - 33 Bytes Per Sample: 1=8 bit Mono, 2=8 bit Stereo or 16 bit Mono, 4=16 bit Stereo
34 - 35 Bits Per Sample
 

DATA Chunk Byte Number
36 - 39 "data" (ASCII Characters)
40 - 43  Length Of Data To Follow
44 - end
 Data (Samples)
*/
