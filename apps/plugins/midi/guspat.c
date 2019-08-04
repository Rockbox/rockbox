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

/* Note frequencies in milliHz, base A = 440000
 * Calculated using:
 *  double base_a=440000;
 *  double offset;
 *  for(offset=-69;offset<=58;offset++)
 *  {
 *      int value = (int)round(base_a*pow(2,offset/12));
 *      printf("%d, ",value);
 *  }
 */
const uint32_t freqtable[]=
{
/* C,       C#,      D,       D#,      E,        F,        F#,       G,       G#,      A,       A#,      B */
  8176,    8662,    9177,    9723,    10301,    10913,    11562,    12250,   12978,   13750,   14568,   15434,
  16352,   17324,   18354,   19445,   20602,    21827,    23125,    24500,   25957,   27500,   29135,   30868,
  32703,   34648,   36708,   38891,   41203,    43654,    46249,    48999,   51913,   55000,   58270,   61735,
  65406,   69296,   73416,   77782,   82407,    87307,    92499,    97999,   103826,  110000,  116541,  123471,
  130813,  138591,  146832,  155563,  164814,   174614,   184997,   195998,  207652,  220000,  233082,  246942,
  261626,  277183,  293665,  311127,  329628,   349228,   369994,   391995,  415305,  440000,  466164,  493883,
  523251,  554365,  587330,  622254,  659255,   698456,   739989,   783991,  830609,  880000,  932328,  987767,
  1046502, 1108731, 1174659, 1244508, 1318510,  1396913,  1479978,  1567982, 1661219, 1760000, 1864655, 1975533,
  2093005, 2217461, 2349318, 2489016, 2637020,  2793826,  2959955,  3135963, 3322438, 3520000, 3729310, 3951066,
  4186009, 4434922, 4698636, 4978032, 5274041,  5587652,  5919911,  6271927, 6644875, 7040000, 7458620, 7902133,
  8372018, 8869844, 9397273, 9956063, 10548082, 11175303, 11839822, 12543854 };

static unsigned int readWord(int file)
{
    return (readChar(file)<<0) | (readChar(file)<<8); // | (readChar(file)<<8) | (readChar(file)<<0);
}

static unsigned int readDWord(int file)
{
    return (readChar(file)<<0) | (readChar(file)<<8) | (readChar(file)<<16) | (readChar(file)<<24);
}

int curr_waveform;
struct GWaveform waveforms[256] IBSS_ATTR;

static struct GWaveform * loadWaveform(int file)
{
    struct GWaveform * wav = &waveforms[curr_waveform++];
    rb->memset(wav, 0, sizeof(struct GWaveform));

    wav->name=readData(file, 7);
    if (!wav->name)
        return NULL;
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
    if (!wav->envRate)
        return NULL;
    wav->envOffset=readData(file, 6);
    if (!wav->envOffset)
        return NULL;

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
    if (!wav->res)
        return NULL;
    wav->data=readData(file, wav->wavSize);
    if (!wav->data)
        return NULL;

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
        ((uint16_t*) wav->data)[a] = letoh16(((uint16_t *) wav->data)[a]);
    }
#endif

    /*  Convert unsigned to signed by subtracting 32768 */
    if(wav->mode & 2)
    {
        for(a=0; a<wav->numSamples; a++)
            ((int16_t *) wav->data)[a] = ((uint16_t *) wav->data)[a] - 32768;

    }

    return wav;
}

static int selectWaveform(struct GPatch * pat, int midiNote)
{
    /* We divide by 100 here because everyone's freq formula is slightly different */
    unsigned int tabFreq = freqtable[midiNote]/100; /* Comparison */
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

    if (gp)
        rb->memset(gp, 0, sizeof(struct GPatch));
    else return NULL;

    int file = rb->open(filename, O_RDONLY);

    if(file < 0)
    {
        char message[50];
        rb->snprintf(message, 50, "Error opening %s", filename);
        rb->splash(HZ*2, message);
        return NULL;
    }

    gp->header=readData(file, 12);
    if (!gp->header)
    {
        rb->close(file);
        return NULL;
    }
    gp->gravisid=readData(file, 10);
    if (!gp->gravisid)
    {
        rb->close(file);
        return NULL;
    }
    gp->desc=readData(file, 60);
    if (!gp->desc)
    {
        rb->close(file);
        return NULL;
    }
    gp->inst=readChar(file);
    gp->voc=readChar(file);
    gp->chan=readChar(file);
    gp->numWaveforms=readWord(file);
    gp->vol=readWord(file);
    gp->datSize=readDWord(file);
    gp->res=readData(file, 36);
    if (!gp->res)
    {
        rb->close(file);
        return NULL;
    }

    gp->instrID=readWord(file);
    gp->instrName=readData(file,16);
    if (!gp->instrName)
    {
        rb->close(file);
        return NULL;
    }
    gp->instrSize=readDWord(file);
    gp->layers=readChar(file);
    gp->instrRes=readData(file,40);
    if (!gp->instrRes)
    {
        rb->close(file);
        return NULL;
    }

    gp->layerDup=readChar(file);
    gp->layerID=readChar(file);
    gp->layerSize=readDWord(file);
    gp->numWaves=readChar(file);
    gp->layerRes=readData(file,40);
    if (!gp->layerRes)
    {
        rb->close(file);
        return NULL;
    }

/*    printf("\nFILE: %s", filename); */
/*    printf("\nlayerSamples=%d", gp->numWaves); */

    int a=0;
    for(a=0; a<gp->numWaves; a++)
    {
        gp->waveforms[a] = loadWaveform(file);
        if (!gp->waveforms[a])
        {
            rb->close(file);
            return NULL;
        }
    }

/*    printf("\nPrecomputing note table"); */

    for(a=0; a<128; a++)
    {
        gp->noteTable[a] = selectWaveform(gp, a);
    }
    rb->close(file);

    return gp;
}

