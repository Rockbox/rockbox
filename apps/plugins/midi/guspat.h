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

extern const uint32_t gustable[];

struct GWaveform
{
    unsigned char * name;
    unsigned char fractions;
    unsigned int wavSize;
    unsigned int numSamples;
    unsigned int startLoop;
    unsigned int endLoop;
    unsigned int sampRate;
    unsigned int lowFreq;
    unsigned int highFreq;
    unsigned int rootFreq;
    unsigned int tune;
    unsigned int balance;
    unsigned char * envRate;
    unsigned char * envOffset;

    unsigned char tremSweep;
    unsigned char tremRate;
    unsigned char tremDepth;
    unsigned char vibSweep;
    unsigned char vibRate;
    unsigned char vibDepth;
    unsigned char mode;

    unsigned int scaleFreq;
    unsigned int scaleFactor;

    unsigned char * res;
    signed char * data;
};


struct GPatch
{
    unsigned int patchNumber;
    unsigned char * header;
    unsigned char * gravisid;
    unsigned char * desc;
    unsigned char inst, voc, chan;
    unsigned int numWaveforms;
    unsigned int datSize;
    unsigned int vol;
    unsigned char * res;


    unsigned int instrID;
    unsigned char * instrName;
    unsigned int instrSize;
    unsigned int layers;
    unsigned char * instrRes;

    unsigned char layerDup;
    unsigned char layerID;
    unsigned int layerSize;
    unsigned char numWaves;
    unsigned char * layerRes;

    unsigned char noteTable[128];
    struct GWaveform * waveforms[255];
};

