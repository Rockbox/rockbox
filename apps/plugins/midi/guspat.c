/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Stepan Moskovchenko
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
#include "plugin.h"
#include "guspat.h"
#include "midiutil.h"

/* This came from one of the Gravis documents */
const uint32_t gustable[]=
{
  8175, 8661, 9177, 9722, 10300, 10913, 11562, 12249, 12978, 13750, 14567, 15433,
  16351, 17323, 18354, 19445, 20601, 21826, 23124, 24499, 25956, 27500, 29135, 30867,
  32703, 34647, 36708, 38890, 41203, 43653, 46249, 48999, 51913, 54999, 58270, 61735,
  65406, 69295, 73416, 77781, 82406, 87306, 92498, 97998, 103826, 109999, 116540, 123470,
  130812, 138591, 146832, 155563, 164813, 174614, 184997, 195997, 207652, 219999, 233081, 246941,
  261625, 277182, 293664, 311126, 329627, 349228, 369994, 391995, 415304, 440000, 466163, 493883,
  523251, 554365, 587329, 622254, 659255, 698456, 739989, 783991, 830609, 880000, 932328, 987767,
  1046503, 1108731, 1174660, 1244509, 1318511, 1396914, 1479979, 1567983, 1661220, 1760002, 1864657, 1975536,
  2093007, 2217464, 2349321, 2489019, 2637024, 2793830, 2959960, 3135968, 3322443, 3520006, 3729316, 3951073,
  4186073, 4434930, 4698645, 4978041, 5274051, 5587663, 5919922, 6271939, 6644889, 7040015, 7458636, 7902150
};

unsigned int readWord(int file)
{
    return (readChar(file)<<0) | (readChar(file)<<8); // | (readChar(file)<<8) | (readChar(file)<<0);
}

unsigned int readDWord(int file)
{
    return (readChar(file)<<0) | (readChar(file)<<8) | (readChar(file)<<16) | (readChar(file)<<24);
}

int curr_waveform;
struct GWaveform waveforms[256] IBSS_ATTR;

struct GWaveform * loadWaveform(int file)
{
    struct GWaveform * wav = &waveforms[curr_waveform++];
    rb->memset(wav, 0, sizeof(struct GWaveform));

    wav->name=readData(file, 7);
/*    printf("\nWAVE NAME = [%s]", wav->name); */
    wav->fractions=readChar(file);
    wav->wavSize=readDWord(file);
    wav->startLoop=readDWord(file);
    wav->endLoop=readDWord(file);
    wav->sampRate=readWord(file);

    wav->lowFreq=readDWord(file);
    wav->highFreq=readDWord(file);
    wav->rootFreq=readDWord(file);

    wav->tune=readWord(file);

    wav->balance=readChar(file);
    wav->envRate=readData(file, 6);
    wav->envOffset=readData(file, 6);

    wav->tremSweep=readChar(file);
    wav->tremRate=readChar(file);
    wav->tremDepth=readChar(file);
    wav->vibSweep=readChar(file);
    wav->vibRate=readChar(file);
    wav->vibDepth=readChar(file);
    wav->mode=readChar(file);

    wav->scaleFreq=readWord(file);
    wav->scaleFactor=readWord(file);
/*    printf("\nScaleFreq = %d   ScaleFactor = %d   RootFreq = %d", wav->scaleFreq, wav->scaleFactor, wav->rootFreq); */
    wav->res=readData(file, 36);
    wav->data=readData(file, wav->wavSize);

    wav->numSamples = wav->wavSize / 2;
    wav->startLoop = wav->startLoop >> 1;
    wav->endLoop = wav->endLoop >> 1;
    unsigned int a=0;

    /* half baked 8 bit conversion  UNFINISHED*/
    /*
    if(wav->mode & 1 == 0)  //Whoops, 8 bit
    {
        wav->numSamples = wav->wavSize;

        //Allocate a block for the rest of it
        //It should end up right after the previous one.
        wav->wavSize = wav->wavSize * 2;
        void * foo = allocate(wav->wavSize);


        for(a=0; a<1000; a++)
            printf("\n!!!!!!!!!!!!!!!!!!!!!!!!!!!");


        for(a=wav->wavSize-1; a>0; a-=2)
        {

        }
    //  int b1=wf->data[s]+((wf->mode & 2) << 6);
    //  return b1<<8;
    }
    */


#ifdef ROCKBOX_BIG_ENDIAN
    /* Byte-swap if necessary. Gus files are little endian */
    for(a=0; a<wav->numSamples; a++)
    {
        ((unsigned short *) wav->data)[a] = letoh16(((unsigned short *) wav->data)[a]);
    }
#endif

    /*  Convert unsigned to signed by subtracting 32768 */
    if(wav->mode & 2)
    {
        for(a=0; a<wav->numSamples; a++)
            ((short *) wav->data)[a] = ((unsigned short *) wav->data)[a] - 32768;

    }

    return wav;
}

int selectWaveform(struct GPatch * pat, int midiNote)
{
    /* We divide by 100 here because everyone's freq formula is slightly different */
    unsigned int tabFreq = gustable[midiNote]/100; /* Comparison */
    unsigned int a=0;
    for(a=0; a<pat->numWaveforms; a++)
    {
        if(pat->waveforms[a]->lowFreq/100 <= tabFreq &&
           pat->waveforms[a]->highFreq/100 >= tabFreq)
        {
            return a;
        }
    }
    return 0;
}

struct GPatch * gusload(char * filename)
{
    struct GPatch * gp = (struct GPatch *)malloc(sizeof(struct GPatch));
    rb->memset(gp, 0, sizeof(struct GPatch));

    int file = rb->open(filename, O_RDONLY);

    if(file == -1)
    {
        char message[50];
        rb->snprintf(message, 50, "Error opening %s", filename);
        rb->splash(HZ*2, message);
        return NULL;
    }

    gp->header=readData(file, 12);
    gp->gravisid=readData(file, 10);
    gp->desc=readData(file, 60);
    gp->inst=readChar(file);
    gp->voc=readChar(file);
    gp->chan=readChar(file);
    gp->numWaveforms=readWord(file);
    gp->vol=readWord(file);
    gp->datSize=readDWord(file);
    gp->res=readData(file, 36);

    gp->instrID=readWord(file);
    gp->instrName=readData(file,16);
    gp->instrSize=readDWord(file);
    gp->layers=readChar(file);
    gp->instrRes=readData(file,40);


    gp->layerDup=readChar(file);
    gp->layerID=readChar(file);
    gp->layerSize=readDWord(file);
    gp->numWaves=readChar(file);
    gp->layerRes=readData(file,40);


/*    printf("\nFILE: %s", filename); */
/*    printf("\nlayerSamples=%d", gp->numWaves); */

    int a=0;
    for(a=0; a<gp->numWaves; a++)
        gp->waveforms[a] = loadWaveform(file);


/*    printf("\nPrecomputing note table"); */

    for(a=0; a<128; a++)
    {
        gp->noteTable[a] = selectWaveform(gp, a);
    }
    rb->close(file);

    return gp;
}

