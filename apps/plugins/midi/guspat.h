/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2005 Stepan Moskovchenko
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* This came from one of the Gravis documents */
static const unsigned int gustable[]=
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

