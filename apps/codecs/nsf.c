/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2006 Adam Gashlin (hcs)
 * Copyright (C) 2004 Disch
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
 * This is a perversion of Disch's excellent NotSoFatso.
 */

#include "codeclib.h"
#include "inttypes.h"
#include "system.h"

CODEC_HEADER

/* arm doesn't benefit from IRAM? */
#ifdef CPU_ARM
#undef ICODE_ATTR
#define ICODE_ATTR
#undef IDATA_ATTR
#define IDATA_ATTR
#else
#define ICODE_INSTEAD_OF_INLINE
#endif

/* Maximum number of bytes to process in one iteration */
#define WAV_CHUNK_SIZE (1024*2)

static int16_t samples[WAV_CHUNK_SIZE] IBSS_ATTR;

#define ZEROMEMORY(addr,size) memset(addr,0,size)

/* simple profiling with USEC_TIMER

#define NSF_PROFILE

*/

#ifdef NSF_PROFILE

#define CREATE_TIMER(name) uint32_t nsf_timer_##name##_start,\
    nsf_timer_##name##_total
#define ENTER_TIMER(name) nsf_timer_##name##_start=USEC_TIMER
#define EXIT_TIMER(name) nsf_timer_##name##_total+=\
    (USEC_TIMER-nsf_timer_##name##_start)
#define READ_TIMER(name) (nsf_timer_##name##_total)
#define RESET_TIMER(name) nsf_timer_##name##_total=0

#define PRINT_TIMER_PCT(bname,tname,nstr) ci->fdprintf(
    logfd,"%10ld ",READ_TIMER(bname));\
    ci->fdprintf(logfd,"(%3d%%) " nstr "\t",\
    ((uint64_t)READ_TIMER(bname))*100/READ_TIMER(tname))

CREATE_TIMER(total);
CREATE_TIMER(cpu);
CREATE_TIMER(apu);
CREATE_TIMER(squares);
CREATE_TIMER(tnd);
CREATE_TIMER(tnd_enter);
CREATE_TIMER(tnd_tri);
CREATE_TIMER(tnd_noise);
CREATE_TIMER(tnd_dmc);
CREATE_TIMER(fds);
CREATE_TIMER(frame);
CREATE_TIMER(mix);

void reset_profile_timers(void) {
    RESET_TIMER(total);
    RESET_TIMER(cpu);
    RESET_TIMER(apu);
    RESET_TIMER(squares);
    RESET_TIMER(tnd);
    RESET_TIMER(tnd_enter);
    RESET_TIMER(tnd_tri);
    RESET_TIMER(tnd_noise);
    RESET_TIMER(tnd_dmc);
    RESET_TIMER(fds);
    RESET_TIMER(frame);
    RESET_TIMER(mix);
}

int logfd=-1;

void print_timers(char * path, int track) {
    logfd = ci->open("/nsflog.txt",O_WRONLY|O_CREAT|O_APPEND);
    ci->fdprintf(logfd,"%s[%d]:\t",path,track);
    ci->fdprintf(logfd,"%10ld total\t",READ_TIMER(total));
    PRINT_TIMER_PCT(cpu,total,"CPU");
    PRINT_TIMER_PCT(apu,total,"APU");
    ci->fdprintf(logfd,"\n\t");
    PRINT_TIMER_PCT(squares,apu,"squares");
    PRINT_TIMER_PCT(frame,apu,"frame");
    PRINT_TIMER_PCT(mix,apu,"mix");
    PRINT_TIMER_PCT(fds,apu,"FDS");
    PRINT_TIMER_PCT(tnd,apu,"tnd");
    ci->fdprintf(logfd,"\n\t\t");
    PRINT_TIMER_PCT(tnd_enter,tnd,"enter");
    PRINT_TIMER_PCT(tnd_tri,tnd,"triangle");
    PRINT_TIMER_PCT(tnd_noise,tnd,"noise");
    PRINT_TIMER_PCT(tnd_dmc,tnd,"DMC");
    ci->fdprintf(logfd,"\n");

    ci->close(logfd);
    logfd=-1;
}

#else

#define CREATE_TIMER(name)
#define ENTER_TIMER(name)
#define EXIT_TIMER(name)
#define READ_TIMER(name)
#define RESET_TIMER(name)
#define print_timers(path,track)
#define reset_profile_timers()

#endif

/* proper handling of multibyte values */
#ifdef ROCKBOX_LITTLE_ENDIAN
union TWIN
{
    uint16_t                        W;
    struct{ uint8_t l; uint8_t h; } B;
};

union QUAD
{
    uint32_t                                    D;
    struct{ uint8_t l; uint8_t h; uint16_t w; } B;
};
#else

union TWIN
{
    uint16_t                        W;
    struct{ uint8_t h; uint8_t l; } B;
};

union QUAD
{
    uint32_t                                    D;
    struct{uint16_t w; uint8_t h; uint8_t l; }  B;
};

#endif

#define NTSC_FREQUENCY           1789772.727273f
#define PAL_FREQUENCY            1652097.692308f
#define NTSC_NMIRATE                  60.098814f
#define PAL_NMIRATE                   50.006982f

#define NES_FREQUENCY           21477270
#define NTSC_FRAME_COUNTER_FREQ (NTSC_FREQUENCY / (NES_FREQUENCY / 89490.0f))
#define PAL_FRAME_COUNTER_FREQ  (PAL_FREQUENCY / (NES_FREQUENCY / 89490.0f))

/****************** tables */
static const int32_t ModulationTable[8] ICONST_ATTR = {0,1,2,4,0,-4,-2,-1};
const uint16_t  DMC_FREQ_TABLE[2][0x10] = {
    /* NTSC */
    {0x1AC,0x17C,0x154,0x140,0x11E,0x0FE,0x0E2,0x0D6,0x0BE,0x0A0,0x08E,0x080,
    0x06A,0x054,0x048,0x036},
    /* PAL */
    {0x18C,0x160,0x13A,0x128,0x108,0x0EA,0x0D0,0x0C6,0x0B0,0x094,0x082,0x076,
    0x062,0x04E,0x042,0x032}
};

const uint8_t DUTY_CYCLE_TABLE[4] = {2,4,8,12};

const uint8_t LENGTH_COUNTER_TABLE[0x20] = {
    0x0A,0xFE,0x14,0x02,0x28,0x04,0x50,0x06,0xA0,0x08,0x3C,0x0A,0x0E,0x0C,0x1A,
    0x0E,0x0C,0x10,0x18,0x12,0x30,0x14,0x60,0x16,0xC0,0x18,0x48,0x1A,0x10,0x1C,
    0x20,0x1E
};

const uint16_t NOISE_FREQ_TABLE[0x10] = {
    0x004,0x008,0x010,0x020,0x040,0x060,0x080,0x0A0,0x0CA,0x0FE,0x17C,0x1FC,
    0x2FA,0x3F8,0x7F2,0xFE4
};

/****************** NSF loading ******************/

/* file format structs (both are little endian) */

struct NESM_HEADER
{
    uint32_t        nHeader;
    uint8_t         nHeaderExtra;
    uint8_t         nVersion;
    uint8_t         nTrackCount;
    uint8_t         nInitialTrack;
    uint16_t        nLoadAddress;
    uint16_t        nInitAddress;
    uint16_t        nPlayAddress;
    uint8_t         szGameTitle[32];
    uint8_t         szArtist[32];
    uint8_t         szCopyright[32];
    uint16_t        nSpeedNTSC;
    uint8_t         nBankSwitch[8];
    uint16_t        nSpeedPAL;
    uint8_t         nNTSC_PAL;
    uint8_t         nExtraChip;
    uint8_t         nExpansion[4];
};

struct NSFE_INFOCHUNK
{
    uint16_t        nLoadAddress;
    uint16_t        nInitAddress;
    uint16_t        nPlayAddress;
    uint8_t         nIsPal;
    uint8_t         nExt;
    uint8_t         nTrackCount;
    uint8_t         nStartingTrack;
};

int32_t     LoadFile(uint8_t *,size_t);

int32_t     LoadFile_NESM(uint8_t *,size_t);
int32_t     LoadFile_NSFE(uint8_t *,size_t);

/* NSF file info */

/* basic NSF info */
int32_t     bIsExtended=0;      /* 0 = NSF, 1 = NSFE */
uint8_t     nIsPal=0;           /* 0 = NTSC, 1 = PAL,
                                 2,3 = mixed NTSC/PAL (interpretted as NTSC) */
int32_t     nfileLoadAddress=0; /* The address to which the NSF code is
                                   loaded */
int32_t     nfileInitAddress=0; /* The address of the Init routine
                                   (called at track change) */
int32_t     nfilePlayAddress=0; /* The address of the Play routine
                                   (called several times a second) */
uint8_t     nChipExtensions=0;  /* Bitwise representation of the external chips
                                   used by this NSF.  */
    
/* old NESM speed stuff (blarg) */
int32_t     nNTSC_PlaySpeed=0;
int32_t     nPAL_PlaySpeed=0;

/* track info */
/* The number of tracks in the NSF (1 = 1 track, 5 = 5 tracks, etc) */
int32_t     nTrackCount=0;
/* The initial track (ZERO BASED:  0 = 1st track, 4 = 5th track, etc) */
int32_t     nInitialTrack=0;

/* nsf data */
uint8_t*    pDataBuffer=0;      /* the buffer containing NSF code. */
int32_t     nDataBufferSize=0;  /* the size of the above buffer. */

/* playlist */
uint8_t     nPlaylist[256];     /* Each entry is the zero based index of the
                                   song to play */
int32_t     nPlaylistSize=0;    /* the number of tracks in the playlist */

/* track time / fade */
int32_t     nTrackTime[256];    /* track times -1 if no track times specified */
int32_t     nTrackFade[256];    /* track fade times -1 if none are specified */

/* string info */
uint8_t     szGameTitle[0x101];
uint8_t     szArtist[0x101];
uint8_t     szCopyright[0x101];
uint8_t     szRipper[0x101];

/* bankswitching info */
uint8_t     nBankswitch[8]={0}; /* The initial bankswitching registers needed
                                 * for some NSFs.  If the NSF does not use
                                 * bankswitching, these values will all be zero
                                 */

int32_t     LoadFile(uint8_t * inbuffer, size_t size)
{
    if(!inbuffer) return -1;

    int32_t ret = -1;

    if(!memcmp(inbuffer,"NESM",4)) ret = LoadFile_NESM(inbuffer,size);
    if(!memcmp(inbuffer,"NSFE",4)) ret = LoadFile_NSFE(inbuffer,size);

    /*
     * Snake's revenge puts '00' for the initial track,
     * which (after subtracting 1) makes it 256 or -1 (bad!)
     * This prevents that crap
     */
    if(nInitialTrack >= nTrackCount)
        nInitialTrack = 0;
    if(nInitialTrack < 0)
        nInitialTrack = 0;

    /* if there's no tracks... this is a crap NSF */
    if(nTrackCount < 1)
    {
        return -1;
    }

    return ret;
}

int32_t LoadFile_NESM(uint8_t* inbuffer, size_t size)
{
    uint8_t ignoreversion=1;
    uint8_t needdata=1;

    /* read the info */
    struct NESM_HEADER hdr;
    
    memcpy(&hdr,inbuffer,sizeof(hdr));

    /* confirm the header */
    if(memcmp("NESM",&(hdr.nHeader),4))         return -1;
    if(hdr.nHeaderExtra != 0x1A)                return -1;
    /* stupid NSFs claim to be above version 1  >_> */
    if((!ignoreversion) && (hdr.nVersion != 1)) return -1;

    /* 
     * NESM is generally easier to work with (but limited!)
     * just move the data over from NESM_HEADER over to our member data
     */

    bIsExtended =               0;
    nIsPal =                    hdr.nNTSC_PAL & 0x03;
    nPAL_PlaySpeed =            letoh16(hdr.nSpeedPAL);
    nNTSC_PlaySpeed =           letoh16(hdr.nSpeedNTSC);
    nfileLoadAddress =          letoh16(hdr.nLoadAddress);
    nfileInitAddress =          letoh16(hdr.nInitAddress);
    nfilePlayAddress =          letoh16(hdr.nPlayAddress);
    nChipExtensions =           hdr.nExtraChip;


    nTrackCount =               hdr.nTrackCount;
    nInitialTrack =             hdr.nInitialTrack - 1;

    memcpy(nBankswitch,hdr.nBankSwitch,8);

    memcpy(szGameTitle,hdr.szGameTitle,32);
    memcpy(szArtist   ,hdr.szArtist   ,32);
    memcpy(szCopyright,hdr.szCopyright,32);

    /* read the NSF data */
    if(needdata)
    {
        pDataBuffer=inbuffer+0x80;
        nDataBufferSize=size-0x80;
    }

    /* if we got this far... it was a successful read */
    return 0;
}

int32_t LoadFile_NSFE(uint8_t* inbuffer, size_t size)
{
    /* the vars we'll be using */
    uint32_t nChunkType;
    int32_t  nChunkSize;
    int32_t  nChunkUsed;
    int32_t i;
    uint8_t *  nDataPos = 0;
    uint8_t bInfoFound = 0;
    uint8_t bEndFound = 0;
    uint8_t bBankFound = 0;
    nPlaylistSize=-1;

    struct NSFE_INFOCHUNK   info;
    ZEROMEMORY(&info,sizeof(struct NSFE_INFOCHUNK));
    ZEROMEMORY(nBankswitch,8);
    info.nTrackCount = 1;       /* default values */
    
    if (size < 8) return -1;    /* must have at least NSFE,NEND */

    /* confirm the header! */
    memcpy(&nChunkType,inbuffer,4);
    inbuffer+=4;
    if(memcmp(&nChunkType,"NSFE",4))            return -1;

    for (i=0;i<256;i++) {
        nTrackTime[i]=-1;
        nTrackFade[i]=-1;
    }

    /* begin reading chunks */
    while(!bEndFound)
    {
        memcpy(&nChunkSize,inbuffer,4);
        nChunkSize=letoh32(nChunkSize);
        inbuffer+=4;
        memcpy(&nChunkType,inbuffer,4);
        inbuffer+=4;

        if(!memcmp(&nChunkType,"INFO",4)) {
            /* only one info chunk permitted */
            if(bInfoFound)                      return -1;
            if(nChunkSize < 8)                  return -1;  /* minimum size */

            bInfoFound = 1;
            nChunkUsed = MIN((int32_t)sizeof(struct NSFE_INFOCHUNK),
                             nChunkSize);

            memcpy(&info,inbuffer,nChunkUsed);
            inbuffer+=nChunkSize;

            bIsExtended =           1;
            nIsPal =                info.nIsPal & 3;
            nfileLoadAddress =      letoh16(info.nLoadAddress);
            nfileInitAddress =      letoh16(info.nInitAddress);
            nfilePlayAddress =      letoh16(info.nPlayAddress);
            nChipExtensions =       info.nExt;
            nTrackCount =           info.nTrackCount;
            nInitialTrack =         info.nStartingTrack;

            nPAL_PlaySpeed =        (uint16_t)(1000000 / PAL_NMIRATE);
            nNTSC_PlaySpeed =       (uint16_t)(1000000 / NTSC_NMIRATE);
        } else if (!memcmp(&nChunkType,"DATA",4)) {
            if(!bInfoFound)                     return -1;
            if(nDataPos)                        return -1;
            if(nChunkSize < 1)                  return -1;

            nDataBufferSize = nChunkSize;
            nDataPos = inbuffer;

            inbuffer+=nChunkSize;
        } else if (!memcmp(&nChunkType,"NEND",4)) {
            bEndFound = 1;
        } else if (!memcmp(&nChunkType,"time",4)) {
            if(!bInfoFound)                     return -1;
            for (nChunkUsed=0; nChunkUsed < MIN(nChunkSize / 4,nTrackCount);
                 nChunkUsed++,inbuffer+=4) {
                nTrackTime[nChunkUsed]=
                    ((uint32_t)inbuffer[0])|
                    ((uint32_t)inbuffer[1]<<8)|
                    ((uint32_t)inbuffer[2]<<16)|
                    ((uint32_t)inbuffer[3]<<24);
            }

            inbuffer+=nChunkSize-(nChunkUsed*4);

            /* negative signals to use default time */
            for(; nChunkUsed < nTrackCount; nChunkUsed++)
                nTrackTime[nChunkUsed] = -1;
        } else if (!memcmp(&nChunkType,"fade",4)) {
            if(!bInfoFound)                     return -1;
            for (nChunkUsed=0; nChunkUsed < MIN(nChunkSize / 4,nTrackCount);
                 nChunkUsed++,inbuffer+=4) {
                nTrackFade[nChunkUsed]=
                    ((uint32_t)inbuffer[0])|
                    ((uint32_t)inbuffer[1]<<8)|
                    ((uint32_t)inbuffer[2]<<16)|
                    ((uint32_t)inbuffer[3]<<24);
            }

            inbuffer+=nChunkSize-(nChunkUsed*4);

            /* negative signals to use default time */
            for(; nChunkUsed < nTrackCount; nChunkUsed++)
                nTrackFade[nChunkUsed] = -1;
        } else if (!memcmp(&nChunkType,"BANK",4)) {
            if(bBankFound)                      return -1;

            bBankFound = 1;
            nChunkUsed = MIN(8,nChunkSize);
            memcpy(nBankswitch,inbuffer,nChunkUsed);

            inbuffer+=nChunkSize;
        } else if (!memcmp(&nChunkType,"plst",4)) {

            nPlaylistSize = nChunkSize;
            if(nPlaylistSize >= 1) {

                memcpy(nPlaylist,inbuffer,nChunkSize);
                inbuffer+=nChunkSize;
            }
        } else if (!memcmp(&nChunkType,"auth",4)) {
            uint8_t*        ptr;

            ptr = inbuffer;

            uint8_t*    ar[4] = {szGameTitle,szArtist,szCopyright,szRipper};
            int32_t     i;
            for(i = 0; (ptr-inbuffer)<nChunkSize && i < 4; i++)
            {
                nChunkUsed = strlen(ptr) + 1;
                memcpy(ar[i],ptr,nChunkUsed);
                ptr += nChunkUsed;
            }
            inbuffer+=nChunkSize;
        } else if (!memcmp(&nChunkType,"tlbl",4)) {
            /* we unfortunately can't use these anyway */
            inbuffer+=nChunkSize;
        } else { /* unknown chunk */
            nChunkType = letoh32(nChunkType)>>24;  /* check the first byte */
            /* chunk is vital... don't continue */
            if((nChunkType >= 'A') && (nChunkType <= 'Z'))
                return -1;
            /* otherwise, just skip it */
            inbuffer+=nChunkSize;
        }       /* end if series */
    }           /* end while */

    /*
     * if we exited the while loop without a 'return', we must have hit an NEND
     *  chunk if this is the case, the file was layed out as it was expected.
     *  now.. make sure we found both an info chunk, AND a data chunk... since
     *  these are minimum requirements for a valid NSFE file
     */

    if(!bInfoFound)         return -1;
    if(!nDataPos)           return -1;

    /* if both those chunks existed, this file is valid.
       Load the data if it's needed */

    pDataBuffer=nDataPos;

    /* return success! */
    return 0;
}


/****************** Audio Device Structures ******************/

struct FDSWave
{
    /*  Envelope Unit   */
    uint8_t     bEnvelopeEnable;
    uint8_t     nEnvelopeSpeed;

    /*  Volume Envelope */
    uint8_t     nVolEnv_Mode;
    uint8_t     nVolEnv_Decay;
    uint8_t     nVolEnv_Gain;
    int32_t     nVolEnv_Timer;
    int32_t     nVolEnv_Count;
    uint8_t     nVolume;
    uint8_t     bVolEnv_On;

    /*  Sweep Envenlope */
    uint8_t     nSweep_Mode;
    uint8_t     nSweep_Decay;
    int32_t     nSweep_Timer;
    int32_t     nSweep_Count;
    uint8_t     nSweep_Gain;
    uint8_t     bSweepEnv_On;

    /*  Effector / LFO / Modulation Unit    */
    int32_t     nSweepBias;
    uint8_t     bLFO_Enabled;
    union TWIN  nLFO_Freq;
    /*float       fLFO_Timer;*/
    /*float       fLFO_Count;*/
    int32_t    nLFO_Timer; /* -17.14*/
    int32_t    nLFO_Count; /* -17.14*/
    uint8_t     nLFO_Addr;
    uint8_t     nLFO_Table[0x40];
    uint8_t     bLFO_On;

    /*  Main Output     */
    uint8_t     nMainVolume;
    uint8_t     bEnabled;
    union TWIN  nFreq;
    /*float       fFreqCount;*/
    int32_t    nFreqCount; /* -17.14 */
    uint8_t     nMainAddr;
    uint8_t     nWaveTable[0x40];
    uint8_t     bWaveWrite;
    uint8_t     bMain_On;

    /*  Output and Downsampling */
    int32_t     nMixL;
    
    /*  Pop Reducer             */
    uint8_t     bPopReducer;
    uint8_t     nPopOutput;
    int32_t     nPopCount;
    
};
int16_t     FDS_nOutputTable_L[4][0x21][0x40];

struct FME07Wave
{
    /* Frequency Control */
    union TWIN  nFreqTimer;
    int32_t     nFreqCount;

    /* Channel Disabling */
    uint8_t     bChannelEnabled;

    /* Volume */
    uint8_t     nVolume;

    /* Duty Cycle */
    uint8_t     nDutyCount;

    /* Output and Downsampling */
    int32_t     nMixL;
};

int16_t     FME07_nOutputTable_L[0x10] IDATA_ATTR;

struct N106Wave
{
    /*  All Channel Stuff */

    uint8_t     nActiveChannels;
    uint8_t     bAutoIncrement;
    uint8_t     nCurrentAddress;
    uint8_t     nRAM[0x100];      /* internal memory for registers/wave data */
    int32_t     nFrequencyLookupTable[8]; /* lookup tbl for freq conversions */

    /*
     *  Individual channel stuff
     */
    /*  Wavelength / Frequency */
    union QUAD  nFreqReg[8];
    int32_t     nFreqTimer[8];
    int32_t     nFreqCount[8];

    /*  Wave data length / remaining */
    uint8_t     nWaveSize[8];
    uint8_t     nWaveRemaining[8];

    /*  Wave data position */
    uint8_t     nWavePosStart[8];
    uint8_t     nWavePos[8];
    uint8_t     nOutput[8];

    /*  Volume */
    uint8_t     nVolume[8];

    /*  Pop Reducer */
    uint8_t     nPreVolume[8];
    uint8_t     nPopCheck[8];

    /* Mixing */
    int32_t     nMixL[8];
};

int16_t     N106_nOutputTable_L[0x10][0x10];

struct VRC6PulseWave
{

    /* Frequency Control */
    union TWIN  nFreqTimer;
    int32_t     nFreqCount;

    /* Flags */
    uint8_t     bChannelEnabled;
    uint8_t     bDigitized;

    /* Volume */
    uint8_t     nVolume;

    /* Duty Cycle */
    uint8_t     nDutyCycle;
    uint8_t     nDutyCount;

    /* Output and Downsampling */
    int32_t     nMixL;
    
};

int16_t     VRC6Pulse_nOutputTable_L[0x10] IDATA_ATTR;

struct VRC6SawWave
{

    /* Frequency Control */
    union TWIN  nFreqTimer;
    int32_t     nFreqCount;

    /* Flags */
    uint8_t     bChannelEnabled;

    /* Phase Accumulator */
    uint8_t     nAccumRate;
    uint8_t     nAccum;
    uint8_t     nAccumStep;

    /* Output and Downsampling */
    int32_t     nMixL;
    
};

int16_t     VRC6Saw_nOutputTable_L[0x20] IDATA_ATTR;

struct Wave_Squares
{

    /* Programmable Timer */
    union TWIN  nFreqTimer[2];
    int32_t     nFreqCount[2];

    /* Length Counter */
    uint8_t     nLengthCount[2];
    uint8_t     bLengthEnabled[2];
    uint8_t     bChannelEnabled[2];

    /* Volume / Decay */
    uint8_t     nVolume[2];
    uint8_t     nDecayVolume[2];
    uint8_t     bDecayEnable[2];
    uint8_t     bDecayLoop[2];
    uint8_t     nDecayTimer[2];
    uint8_t     nDecayCount[2];

    /* Sweep Unit */
    uint8_t     bSweepEnable[2];
    uint8_t     bSweepMode[2];
    uint8_t     bSweepForceSilence[2];
    uint8_t     nSweepTimer[2];
    uint8_t     nSweepCount[2];
    uint8_t     nSweepShift[2];

    /* Duty Cycle */
    uint8_t     nDutyCount[2];
    uint8_t     nDutyCycle[2];

    /* Output and Downsampling */
    int32_t         nMixL;
};

int16_t     Squares_nOutputTable_L[0x10][0x10] IDATA_ATTR;

struct Wave_TND
{

    /*
     * Triangle
     */

    /* Programmable Timer */
    union TWIN  nTriFreqTimer;
    int32_t     nTriFreqCount;

    /* Length Counter */
    uint8_t     nTriLengthCount;
    uint8_t     bTriLengthEnabled;
    uint8_t     bTriChannelEnabled;

    /* Linear Counter */
    uint8_t     nTriLinearCount;
    uint8_t     nTriLinearLoad;
    uint8_t     bTriLinearHalt;
    uint8_t     bTriLinearControl;

    /* Tri-Step Generator / Output */
    uint8_t     nTriStep;
    uint8_t     nTriOutput;

    /*
     * Noise
     */

    /* Programmable Timer */
    uint16_t    nNoiseFreqTimer;
    int32_t     nNoiseFreqCount;

    /* Length Counter */
    uint8_t     nNoiseLengthCount;
    uint8_t     bNoiseLengthEnabled;
    uint8_t     bNoiseChannelEnabled;

    /* Volume / Decay */
    uint8_t     nNoiseVolume;
    uint8_t     nNoiseDecayVolume;
    uint8_t     bNoiseDecayEnable;
    uint8_t     bNoiseDecayLoop;
    uint8_t     nNoiseDecayTimer;
    uint8_t     nNoiseDecayCount;

    /* Random Number Generator */
    uint16_t    nNoiseRandomShift;
    uint8_t     bNoiseRandomMode;           /* 1 = 32k, 6 = 93-bit */
    uint8_t     bNoiseRandomOut;

    /*
     * DMC
     */

    /* Play Mode */
    uint8_t     bDMCLoop;
    uint8_t     bDMCIRQEnabled;
    uint8_t     bDMCIRQPending;

    /* Address / DMA */
    uint8_t     nDMCDMABank_Load;
    uint16_t    nDMCDMAAddr_Load;
    uint8_t     nDMCDMABank;
    uint16_t    nDMCDMAAddr;
    uint8_t*    pDMCDMAPtr[8];

    /* Length / Input */
    uint16_t    nDMCLength;
    uint16_t    nDMCBytesRemaining;
    uint8_t     nDMCDelta;
    uint8_t     nDMCDeltaBit;
    uint8_t     bDMCDeltaSilent;
    uint8_t     nDMCSampleBuffer;
    uint8_t     bDMCSampleBufferEmpty;

    /* Frequency */
    uint16_t    nDMCFreqTimer;
    int32_t     nDMCFreqCount;

    /* Output */
    uint8_t     bDMCActive;
    uint8_t     nDMCOutput;

    int32_t     nMixL;
};

/* channels */
struct Wave_Squares mWave_Squares IDATA_ATTR; /* Square channels 1 and 2 */
struct Wave_TND     mWave_TND IDATA_ATTR;     /* Triangle/Noise/DMC channels */
struct VRC6PulseWave    mWave_VRC6Pulse[2] IDATA_ATTR;
struct VRC6SawWave  mWave_VRC6Saw IDATA_ATTR;
struct N106Wave     mWave_N106 IDATA_ATTR;
struct FDSWave      mWave_FDS IDATA_ATTR;
struct FME07Wave    mWave_FME07[3] IDATA_ATTR; /* FME-07's 3 pulse channels */


/****************** MMC5 ******************/
/* will include MMC5 sound channels some day,
   currently only multiply is supported */

/****************** N106 (Disch loves this chip) ******************/

#ifdef ICODE_INSTEAD_OF_INLINE
void Wave_N106_DoTicks(const int32_t ticks) ICODE_ATTR;
void Wave_N106_DoTicks(const int32_t ticks)
#else
inline void Wave_N106_DoTicks(const int32_t ticks);
inline void Wave_N106_DoTicks(const int32_t ticks)
#endif
{
    register int32_t i;

    for(i = (7 - mWave_N106.nActiveChannels); i < 8; i++)
    {
        if(!mWave_N106.nFreqReg[i].D)
        {
            /* written frequency of zero will cause divide by zero error
               makes me wonder if the formula was supposed to be Reg+1 */
            mWave_N106.nVolume[i] = mWave_N106.nPreVolume[i];
            continue;
        }

        {
            mWave_N106.nMixL[i] = 
                N106_nOutputTable_L[mWave_N106.nVolume[i]]
                                      [mWave_N106.nOutput[i]];
            
            if(mWave_N106.nFreqTimer[i] < 0)
                mWave_N106.nFreqTimer[i] =
                 (mWave_N106.nFrequencyLookupTable[mWave_N106.nActiveChannels] /
                 mWave_N106.nFreqReg[i].D);
            if(mWave_N106.nFreqCount[i] > mWave_N106.nFreqTimer[i])
                mWave_N106.nFreqCount[i] = mWave_N106.nFreqTimer[i];

            mWave_N106.nFreqCount[i] -= ticks << 8;
            while(mWave_N106.nFreqCount[i] <= 0)
            {
                mWave_N106.nFreqCount[i] += mWave_N106.nFreqTimer[i];
                if(mWave_N106.nWaveRemaining[i])
                {
                    mWave_N106.nWaveRemaining[i]--;
                    mWave_N106.nWavePos[i]++;
                }
                if(!mWave_N106.nWaveRemaining[i])
                {
                    mWave_N106.nWaveRemaining[i] = mWave_N106.nWaveSize[i];
                    mWave_N106.nWavePos[i] = mWave_N106.nWavePosStart[i];
                    if(mWave_N106.nVolume[i] != mWave_N106.nPreVolume[i])
                    {
                        if(++mWave_N106.nPopCheck[i] >= 2)
                        {
                            mWave_N106.nPopCheck[i] = 0;
                            mWave_N106.nVolume[i] = mWave_N106.nPreVolume[i];
                        }
                    }
                }

                mWave_N106.nOutput[i] =
                    mWave_N106.nRAM[mWave_N106.nWavePos[i]];
                    
                if(!mWave_N106.nOutput[i])
                {
                    mWave_N106.nPopCheck[i] = 0;
                    mWave_N106.nVolume[i] = mWave_N106.nPreVolume[i];
                }
                    
            }
        }
    }
}
/****************** VRC6 ******************/

#ifdef ICODE_INSTEAD_OF_INLINE
void Wave_VRC6_DoTicks(const int32_t ticks) ICODE_ATTR;
void Wave_VRC6_DoTicks(const int32_t ticks)
#else
inline void Wave_VRC6_DoTicks(const int32_t ticks);
inline void Wave_VRC6_DoTicks(const int32_t ticks)
#endif
{
    register int32_t i;

    for(i = 0; i < 2; i++) {

        if(mWave_VRC6Pulse[i].bChannelEnabled) {

            mWave_VRC6Pulse[i].nFreqCount -= ticks;

            if(mWave_VRC6Pulse[i].nDutyCount <=
               mWave_VRC6Pulse[i].nDutyCycle)
            {
                mWave_VRC6Pulse[i].nMixL =
                    VRC6Pulse_nOutputTable_L[mWave_VRC6Pulse[i].nVolume];
            }
            else
                mWave_VRC6Pulse[i].nMixL = 0;

            while(mWave_VRC6Pulse[i].nFreqCount <= 0) {
                mWave_VRC6Pulse[i].nFreqCount +=
                    mWave_VRC6Pulse[i].nFreqTimer.W + 1;

                if(!mWave_VRC6Pulse[i].bDigitized)
                    mWave_VRC6Pulse[i].nDutyCount =
                        (mWave_VRC6Pulse[i].nDutyCount + 1) & 0x0F;
            }
        }
    }

    if(mWave_VRC6Saw.bChannelEnabled) {

        mWave_VRC6Saw.nFreqCount -= ticks;

        mWave_VRC6Saw.nMixL =
            VRC6Saw_nOutputTable_L[mWave_VRC6Saw.nAccum >> 3];

        while(mWave_VRC6Saw.nFreqCount <= 0) {

            mWave_VRC6Saw.nFreqCount += mWave_VRC6Saw.nFreqTimer.W + 1;

            mWave_VRC6Saw.nAccumStep++;
            if(mWave_VRC6Saw.nAccumStep == 14)
            {
                mWave_VRC6Saw.nAccumStep = 0;
                mWave_VRC6Saw.nAccum = 0;
            }
            else if(!(mWave_VRC6Saw.nAccumStep & 1))
                mWave_VRC6Saw.nAccum += mWave_VRC6Saw.nAccumRate;
        }
    }
}

/****************** Square waves ******************/

/* decay */
#ifdef ICODE_INSTEAD_OF_INLINE
void Wave_Squares_ClockMajor(void) ICODE_ATTR;
void Wave_Squares_ClockMajor()
#else
inline void Wave_Squares_ClockMajor(void);
inline void Wave_Squares_ClockMajor()
#endif
{
    if(mWave_Squares.nDecayCount[0])
        mWave_Squares.nDecayCount[0]--;
    else
    {
        mWave_Squares.nDecayCount[0] = mWave_Squares.nDecayTimer[0];
        if(mWave_Squares.nDecayVolume[0])
            mWave_Squares.nDecayVolume[0]--;
        else
        {
            if(mWave_Squares.bDecayLoop[0])
                mWave_Squares.nDecayVolume[0] = 0x0F;
        }

        if(mWave_Squares.bDecayEnable[0])
            mWave_Squares.nVolume[0] = mWave_Squares.nDecayVolume[0];
    }
        
    if(mWave_Squares.nDecayCount[1])
        mWave_Squares.nDecayCount[1]--;
    else
        {
        mWave_Squares.nDecayCount[1] = mWave_Squares.nDecayTimer[1];
        if(mWave_Squares.nDecayVolume[1])
            mWave_Squares.nDecayVolume[1]--;
        else
        {
            if(mWave_Squares.bDecayLoop[1])
                mWave_Squares.nDecayVolume[1] = 0x0F;
        }

        if(mWave_Squares.bDecayEnable[1])
            mWave_Squares.nVolume[1] = mWave_Squares.nDecayVolume[1];
    }
        
}


#ifdef ICODE_INSTEAD_OF_INLINE
void Wave_Squares_CheckSweepForcedSilence(const int32_t i) ICODE_ATTR;
void Wave_Squares_CheckSweepForcedSilence(const int32_t i)
#else
inline void Wave_Squares_CheckSweepForcedSilence(const int32_t i);
inline void Wave_Squares_CheckSweepForcedSilence(const int32_t i)
#endif
{
    if(mWave_Squares.nFreqTimer[i].W < 8) {
        mWave_Squares.bSweepForceSilence[i] = 1; return;
    }
    if(!mWave_Squares.bSweepMode[i] &&
       (( mWave_Squares.nFreqTimer[i].W +
          (mWave_Squares.nFreqTimer[i].W >> mWave_Squares.nSweepShift[i]))
       >= 0x0800)) { mWave_Squares.bSweepForceSilence[i] = 1; return; }

    mWave_Squares.bSweepForceSilence[i] = 0;
}

/* sweep / length */
#ifdef ICODE_INSTEAD_OF_INLINE
void Wave_Squares_ClockMinor(void) ICODE_ATTR;
void Wave_Squares_ClockMinor()
#else
inline void Wave_Squares_ClockMinor(void);
inline void Wave_Squares_ClockMinor()
#endif
{
/* unrolled a little loop
   static int i = 0;
  for(i = 0; i < 2; i++)
  {
*/
    if(mWave_Squares.bLengthEnabled[0] && mWave_Squares.nLengthCount[0])
            mWave_Squares.nLengthCount[0]--;

    if(!mWave_Squares.bSweepEnable[0] || !mWave_Squares.nLengthCount[0] ||
        mWave_Squares.bSweepForceSilence[0] || !mWave_Squares.nSweepShift[0])
        goto other_square;

    if(mWave_Squares.nSweepCount[0])
        mWave_Squares.nSweepCount[0]--;
    else
    {
        mWave_Squares.nSweepCount[0] = mWave_Squares.nSweepTimer[0];
        if(mWave_Squares.bSweepMode[0])  mWave_Squares.nFreqTimer[0].W -=
            (mWave_Squares.nFreqTimer[0].W >> mWave_Squares.nSweepShift[0])+1;
        else mWave_Squares.nFreqTimer[0].W +=
            (mWave_Squares.nFreqTimer[0].W >> mWave_Squares.nSweepShift[0]);

        Wave_Squares_CheckSweepForcedSilence(0);
    }
        
    /* */
other_square:
    if(mWave_Squares.bLengthEnabled[1] && mWave_Squares.nLengthCount[1])
        mWave_Squares.nLengthCount[1]--;

    if(!mWave_Squares.bSweepEnable[1] || !mWave_Squares.nLengthCount[1] ||
        mWave_Squares.bSweepForceSilence[1] || !mWave_Squares.nSweepShift[1])
        return;

    if(mWave_Squares.nSweepCount[1])
        mWave_Squares.nSweepCount[1]--;
    else
    {
        mWave_Squares.nSweepCount[1] = mWave_Squares.nSweepTimer[1];
        if(mWave_Squares.bSweepMode[1])  mWave_Squares.nFreqTimer[1].W -=
            (mWave_Squares.nFreqTimer[1].W >> mWave_Squares.nSweepShift[1]);
        else mWave_Squares.nFreqTimer[1].W +=
            (mWave_Squares.nFreqTimer[1].W >> mWave_Squares.nSweepShift[1]);

        Wave_Squares_CheckSweepForcedSilence(1);
    }
}

/****************** Triangle/noise/DMC ******************/

/* decay (noise), linear (tri) */

#ifdef ICODE_INSTEAD_OF_INLINE
void Wave_TND_ClockMajor(void) ICODE_ATTR;
void Wave_TND_ClockMajor()
#else
inline void Wave_TND_ClockMajor(void);
inline void Wave_TND_ClockMajor()
#endif
{
    /* noise's decay */
    if(mWave_TND.nNoiseDecayCount)
        mWave_TND.nNoiseDecayCount--;
    else
    {
        mWave_TND.nNoiseDecayCount = mWave_TND.nNoiseDecayTimer;
        if(mWave_TND.nNoiseDecayVolume)
            mWave_TND.nNoiseDecayVolume--;
        else
        {
            if(mWave_TND.bNoiseDecayLoop)
                mWave_TND.nNoiseDecayVolume = 0x0F;
        }

        if(mWave_TND.bNoiseDecayEnable)
            mWave_TND.nNoiseVolume = mWave_TND.nNoiseDecayVolume;
    }

    /* triangle's linear */
    if(mWave_TND.bTriLinearHalt)
        mWave_TND.nTriLinearCount = mWave_TND.nTriLinearLoad;
    else if(mWave_TND.nTriLinearCount)
        mWave_TND.nTriLinearCount--;

    if(!mWave_TND.bTriLinearControl)
        mWave_TND.bTriLinearHalt = 0;
}

/* length */

#ifdef ICODE_INSTEAD_OF_INLINE
void Wave_TND_ClockMinor(void) ICODE_ATTR;
void Wave_TND_ClockMinor()
#else
inline void Wave_TND_ClockMinor(void);
inline void Wave_TND_ClockMinor()
#endif
{
    if(mWave_TND.bNoiseLengthEnabled && mWave_TND.nNoiseLengthCount)
        mWave_TND.nNoiseLengthCount--;
        
    if(mWave_TND.bTriLengthEnabled && mWave_TND.nTriLengthCount)
        mWave_TND.nTriLengthCount--;
}

/*#undef this*/

/****************** NSF Core ******************/

/* start globals */

/*
 *  Memory
 */
/* RAM:      0x0000 - 0x07FF */
uint8_t     pRAM[0x800] IDATA_ATTR;
/* SRAM:     0x6000 - 0x7FFF (non-FDS only) */
uint8_t     pSRAM[0x2000];
/* ExRAM:    0x5C00 - 0x5FF5 (MMC5 only)
 * Also holds NSF player code (at 0x5000 - 0x500F) */
uint8_t     pExRAM[0x1000];
/* Full ROM buffer */
uint8_t*    pROM_Full IDATA_ATTR;

uint16_t    main_nOutputTable_L[0x8000];

uint8_t*    pROM[10] IDATA_ATTR;/* ROM banks (point to areas in pROM_Full) */
                                /* 0x8000 - 0xFFFF */
                                /* also includes 0x6000 - 0x7FFF (FDS only) */
uint8_t*        pStack;         /* the stack (points to areas in pRAM) */
                                /* 0x0100 - 0x01FF */

int32_t         nROMSize;       /* size of this ROM file in bytes */
int32_t         nROMBankCount;  /* max number of 4k banks */
int32_t         nROMMaxSize;    /* size of allocated pROM_Full buffer */

/*
 *  Memory Proc Pointers
 */
 
typedef uint8_t ( *ReadProc)(uint16_t);
typedef void ( *WriteProc)(uint16_t,uint8_t);
ReadProc    ReadMemory[0x10] IDATA_ATTR;
WriteProc   WriteMemory[0x10] IDATA_ATTR;

/*
 *  6502 Registers / Mode
 */

uint8_t     regA IDATA_ATTR;        /* Accumulator */
uint8_t     regX IDATA_ATTR;        /* X-Index */
uint8_t     regY IDATA_ATTR;        /* Y-Index */
uint8_t     regP IDATA_ATTR;        /* Processor Status */
uint8_t     regSP IDATA_ATTR;       /* Stack Pointer */
uint16_t    regPC IDATA_ATTR;       /* Program Counter */

uint8_t     bPALMode IDATA_ATTR;/* 1 if in PAL emulation mode, 0 if in NTSC */
uint8_t     bCPUJammed IDATA_ATTR;  /* 0 = not jammed.  1 = really jammed.
                                     * 2 = 'fake' jammed */
                                  /* fake jam caused by the NSF code to signal
                                   * the end of the play/init routine */

/* Multiplication Register, for MMC5 chip only (5205+5206) */
uint8_t     nMultIn_Low;
uint8_t     nMultIn_High;

/*
 *  NSF Preparation Information
 */

uint8_t     nBankswitchInitValues[10];  /* banks to swap to on tune init */
uint16_t    nPlayAddress;               /* Play routine address */
uint16_t    nInitAddress;               /* Init routine address */

uint8_t     nExternalSound;             /* external sound chips */
uint8_t     nCurTrack;

float       fNSFPlaybackSpeed;

/*
 *  pAPU
 */

uint8_t     nFrameCounter;      /* Frame Sequence Counter */
uint8_t     nFrameCounterMax;   /* Frame Sequence Counter Size
                                   (3 or 4 depending on $4017.7) */
uint8_t     bFrameIRQEnabled;   /* TRUE if frame IRQs are enabled */
uint8_t     bFrameIRQPending;   /* TRUE if the frame sequencer is holding down
                                   an IRQ */

uint8_t         nFME07_Address;

/*
 *  Timing and Counters
 */
/* fixed point -15.16 */

int32_t     nTicksUntilNextFrame;
int32_t     nTicksPerPlay;
int32_t     nTicksUntilNextPlay;
int32_t     nTicksPerSample;
int32_t     nTicksUntilNextSample;

uint32_t    nCPUCycle IDATA_ATTR;
uint32_t    nAPUCycle IDATA_ATTR;

   
uint32_t    nTotalPlays; /* number of times the play subroutine has been called
                            (for tracking output time) */
/*
 *  Silence Tracker
 */
int32_t     nSilentSamples;
int32_t     nSilentSampleMax;
int32_t     nSilenceTrackMS;
uint8_t     bNoSilenceIfTime;
uint8_t     bTimeNotDefault;

/*
 *  Sound output options
 */
const int32_t       nSampleRate=44100;

/*
 *  Volume/fading/filter tracking
 */

uint32_t        nStartFade; /* play call to start fading out */
uint32_t        nEndFade;   /* play call to stop fading out (song is over) */
uint8_t         bFade;      /* are we fading? */
float           fFadeVolume;
float           fFadeChange;

/*
 *  Designated Output Buffer
 */
uint8_t*        pOutput IDATA_ATTR;

const uint8_t   bDMCPopReducer=1;
uint8_t         nDMCPop_Prev IDATA_ATTR = 0;
uint8_t         bDMCPop_Skip IDATA_ATTR = 0;
uint8_t         bDMCPop_SamePlay IDATA_ATTR = 0;

const uint8_t   nForce4017Write=0;
const uint8_t   bN106PopReducer=0;
const uint8_t   bIgnore4011Writes=0;
    
const uint8_t   bIgnoreBRK=0;
const uint8_t   bIgnoreIllegalOps=0;
const uint8_t   bNoWaitForReturn=0;
const uint8_t   bPALPreference=0;
const uint8_t   bCleanAXY=0;
const uint8_t   bResetDuty=0;

/*
 *  Sound Filter
 */

int64_t     nFilterAccL IDATA_ATTR;
int64_t     nHighPass IDATA_ATTR;

int32_t     nHighPassBase IDATA_ATTR;

uint8_t     bHighPassEnabled IDATA_ATTR;

/* end globals */

#define CLOCK_MAJOR() { Wave_Squares_ClockMajor(); Wave_TND_ClockMajor(); }
#define CLOCK_MINOR() { Wave_Squares_ClockMinor(); Wave_TND_ClockMinor(); }

#define EXTSOUND_VRC6           0x01
#define EXTSOUND_VRC7           0x02
#define EXTSOUND_FDS            0x04
#define EXTSOUND_MMC5           0x08
#define EXTSOUND_N106           0x10
#define EXTSOUND_FME07          0x20

#define SILENCE_THRESHOLD       3

/*
 *  prototypes
 */
 
uint32_t Emulate6502(uint32_t runto) ICODE_ATTR;
void EmulateAPU(uint8_t bBurnCPUCycles) ICODE_ATTR;

int     NSFCore_Initialize(void); /* 1 = initialized ok,
                           0 = couldn't initialize (memory allocation error) */

/*
 *  Song Loading
 */
int     LoadNSF(int32_t);   /* grab data from an existing file
                               1 = loaded ok, 0 = error loading */

/*
 *  Track Control
 */
void    SetTrack(uint8_t track);  /* Change tracks */

/*
 *  Getting Samples
 */
/* fill a buffer with samples */
int32_t     GetSamples(uint8_t* buffer, int32_t buffersize);

/*
 *  Playback options
 */
/* Set desired playback options (0 = bad options couldn't be set) */
int     SetPlaybackOptions(int32_t samplerate);
/* Speed throttling (0 = uses NSF specified speed) */
void    SetPlaybackSpeed(float playspersec);

float   GetPlaybackSpeed(void);
float   GetMasterVolume(void);

/*
 *  Seeking
 */
/* gets the number of 'play' routine calls executed */
float   GetPlayCalls(void);

/* gets the output time (based on the given play rate,
   if basedplayspersec is zero, current playback speed is used */
uint32_t    GetWrittenTime(float basedplayspersec);
/* sets the number of 'plays' routines executed (for precise seeking) */
void    SetPlayCalls(float plays);
/* sets the written time (approx. seeking) */
void    SetWrittenTime(uint32_t ms,float basedplays);

/*
 *  Fading
 */

void    StopFade(void);         /* stops all fading (plays indefinitely) */
uint8_t SongCompleted(void);    /* song has faded out (samples have stopped
                                   being generated) */
/* parameters are play calls */
void    SetFade(int32_t fadestart,int32_t fadestop,uint8_t bNotDefault);
void    SetFadeTime(uint32_t fadestart,uint32_t fadestop,float basedplays,
            uint8_t bNotDefault); /* parameters are in milliseconds */

/*
 *  Internal Functions
 */
void    RebuildOutputTables(void);
void    RecalculateFade(void);  /* called when fade status is changed. */
void    RecalcFilter(void);
void    RecalcSilenceTracker(void);

void    WriteMemory_VRC6(uint16_t a,uint8_t v) ICODE_ATTR;
void    WriteMemory_MMC5(uint16_t a,uint8_t v) ICODE_ATTR;
void    WriteMemory_N106(uint16_t a,uint8_t v) ICODE_ATTR;
void    WriteMemory_FME07(uint16_t a,uint8_t v) ICODE_ATTR;

/*
 *  Memory Read/Write routines
 */

uint8_t     ReadMemory_RAM(uint16_t a) ICODE_ATTR;
uint8_t     ReadMemory_ExRAM(uint16_t a) ICODE_ATTR;
uint8_t     ReadMemory_SRAM(uint16_t a) ICODE_ATTR;
uint8_t     ReadMemory_pAPU(uint16_t a) ICODE_ATTR;
uint8_t     ReadMemory_ROM(uint16_t a) ICODE_ATTR;
uint8_t     ReadMemory_Default(uint16_t a) ICODE_ATTR;

uint8_t     ReadMemory_N106(uint16_t a) ICODE_ATTR;

void        WriteMemory_RAM(uint16_t a,uint8_t v) ICODE_ATTR;
void        WriteMemory_ExRAM(uint16_t a,uint8_t v) ICODE_ATTR;
void        WriteMemory_SRAM(uint16_t a,uint8_t v) ICODE_ATTR;
void        WriteMemory_pAPU(uint16_t a,uint8_t v) ICODE_ATTR;
void        WriteMemory_FDSRAM(uint16_t a,uint8_t v) ICODE_ATTR;
void        WriteMemory_Default(uint16_t a,uint8_t v) ICODE_ATTR;

uint8_t         ReadMemory_RAM(uint16_t a)      { return pRAM[a & 0x07FF]; }
uint8_t         ReadMemory_ExRAM(uint16_t a)    { return pExRAM[a & 0x0FFF]; }
uint8_t         ReadMemory_SRAM(uint16_t a)     { return pSRAM[a & 0x1FFF]; }
uint8_t         ReadMemory_ROM(uint16_t a)
    { return pROM[(a >> 12) - 6][a & 0x0FFF]; }
uint8_t         ReadMemory_Default(uint16_t a)  { return (a >> 8); }

void        WriteMemory_RAM(uint16_t a,uint8_t v)
    { pRAM[a & 0x07FF] = v; }
void        WriteMemory_ExRAM(uint16_t a,uint8_t v);
void        WriteMemory_SRAM(uint16_t a,uint8_t v)
    { pSRAM[a & 0x1FFF] = v; }
void        WriteMemory_FDSRAM(uint16_t a,uint8_t v)
    { pROM[(a >> 12) - 6][a & 0x0FFF] = v; }
void        WriteMemory_Default(uint16_t a,uint8_t v)   { (void)a; (void)v; }


/* Read Memory Procs */

uint8_t  ReadMemory_pAPU(uint16_t a)
{
    EmulateAPU(1);

    if(a == 0x4015)
    {
        uint8_t ret = 0;
        if(mWave_Squares.nLengthCount[0])       ret |= 0x01;
        if(mWave_Squares.nLengthCount[1])       ret |= 0x02;
        if(mWave_TND.nTriLengthCount)           ret |= 0x04;
        if(mWave_TND.nNoiseLengthCount)         ret |= 0x08;
        if(mWave_TND.nDMCBytesRemaining)        ret |= 0x10;

        if(bFrameIRQPending)            ret |= 0x40;
        if(mWave_TND.bDMCIRQPending)            ret |= 0x80;

        bFrameIRQPending = 0;
        return ret;
    }

    if(!(nExternalSound & EXTSOUND_FDS))        return 0x40;
    if(bPALMode)                                return 0x40;

    if((a >= 0x4040) && (a <= 0x407F))
        return mWave_FDS.nWaveTable[a & 0x3F] | 0x40;
    if(a == 0x4090)
        return (mWave_FDS.nVolEnv_Gain & 0x3F) | 0x40;
    if(a == 0x4092)
        return (mWave_FDS.nSweep_Gain & 0x3F) | 0x40;

    return 0x40;
}

uint8_t  ReadMemory_N106(uint16_t a)
{
    if(a != 0x4800)
        return ReadMemory_pAPU(a);

    uint8_t ret = mWave_N106.nRAM[(mWave_N106.nCurrentAddress << 1)] |
        (mWave_N106.nRAM[(mWave_N106.nCurrentAddress << 1) + 1] << 4);
    if(mWave_N106.bAutoIncrement)
        mWave_N106.nCurrentAddress = (mWave_N106.nCurrentAddress + 1) & 0x7F;

    return ret;
}


/* Write Memory Procs */

void  WriteMemory_ExRAM(uint16_t a,uint8_t v)
{
    if(a < 0x5FF6)              /* Invalid */
        return;

    a -= 0x5FF6;

    /* Swap out banks */

    EmulateAPU(1);
    /* stop it from swapping to a bank that doesn't exist */
    if(v >= nROMBankCount)
        v = 0;

    pROM[a] = pROM_Full + (v << 12);

    /* Update the DMC's DMA pointer, as well */
    if(a >= 2)
        mWave_TND.pDMCDMAPtr[a - 2] = pROM[a];
}

void  WriteMemory_pAPU(uint16_t a,uint8_t v)
{
    EmulateAPU(1);
    switch(a)
    {
    /* Square 1 */
    case 0x4000:
        mWave_Squares.nDutyCycle[0] = DUTY_CYCLE_TABLE[v >> 6];
        mWave_Squares.bLengthEnabled[0] =
            !(mWave_Squares.bDecayLoop[0] = (v & 0x20));
        mWave_Squares.bDecayEnable[0] = !(v & 0x10);
        mWave_Squares.nDecayTimer[0] = (v & 0x0F);

        if(!mWave_Squares.bDecayEnable[0])
            mWave_Squares.nVolume[0] = mWave_Squares.nDecayTimer[0];
        break;

    case 0x4001:
        mWave_Squares.bSweepEnable[0] = (v & 0x80);
        mWave_Squares.nSweepTimer[0] = (v & 0x70) >> 4;
        mWave_Squares.bSweepMode[0] = v & 0x08;
        mWave_Squares.nSweepShift[0] = v & 0x07;
        Wave_Squares_CheckSweepForcedSilence(0);
        break;
        
    case 0x4002:
        mWave_Squares.nFreqTimer[0].B.l = v;
        Wave_Squares_CheckSweepForcedSilence(0);
        break;
        
    case 0x4003:
        mWave_Squares.nFreqTimer[0].B.h = v & 0x07;
        Wave_Squares_CheckSweepForcedSilence(0);

        mWave_Squares.nDecayVolume[0] = 0x0F;

        if(mWave_Squares.bChannelEnabled[0])
            mWave_Squares.nLengthCount[0] = LENGTH_COUNTER_TABLE[v >> 3];

        if(bResetDuty)
            mWave_Squares.nDutyCount[0] = 0;
        break;
        

    /* Square 2 */
    case 0x4004:
        mWave_Squares.nDutyCycle[1] = DUTY_CYCLE_TABLE[v >> 6];
        mWave_Squares.bLengthEnabled[1] =
            !(mWave_Squares.bDecayLoop[1] = (v & 0x20));
        mWave_Squares.bDecayEnable[1] = !(v & 0x10);
        mWave_Squares.nDecayTimer[1] = (v & 0x0F);

        if(!mWave_Squares.bDecayEnable[1])
            mWave_Squares.nVolume[1] = mWave_Squares.nDecayTimer[1];
        break;

    case 0x4005:
        mWave_Squares.bSweepEnable[1] = (v & 0x80);
        mWave_Squares.nSweepTimer[1] = (v & 0x70) >> 4;
        mWave_Squares.bSweepMode[1] = v & 0x08;
        mWave_Squares.nSweepShift[1] = v & 0x07;
        Wave_Squares_CheckSweepForcedSilence(1);
        break;
        
    case 0x4006:
        mWave_Squares.nFreqTimer[1].B.l = v;
        Wave_Squares_CheckSweepForcedSilence(1);
        break;
        
    case 0x4007:
        mWave_Squares.nFreqTimer[1].B.h = v & 0x07;
        Wave_Squares_CheckSweepForcedSilence(1);

        mWave_Squares.nDecayVolume[1] = 0x0F;

        if(mWave_Squares.bChannelEnabled[1])
            mWave_Squares.nLengthCount[1] = LENGTH_COUNTER_TABLE[v >> 3];

        if(bResetDuty)
            mWave_Squares.nDutyCount[1] = 0;
        break;

        
    /* Triangle */
    case 0x4008:
        mWave_TND.nTriLinearLoad = v & 0x7F;
        mWave_TND.bTriLinearControl = v & 0x80;
        mWave_TND.bTriLengthEnabled = !(v & 0x80);
        break;

    case 0x400A:
        mWave_TND.nTriFreqTimer.B.l = v;
        break;

    case 0x400B:
        mWave_TND.nTriFreqTimer.B.h = v & 0x07;
        mWave_TND.bTriLinearHalt = 1;
        
        if(mWave_TND.bTriChannelEnabled)
            mWave_TND.nTriLengthCount = LENGTH_COUNTER_TABLE[v >> 3];
        break;

    /* Noise */
    case 0x400C:
        mWave_TND.bNoiseLengthEnabled =
            !(mWave_TND.bNoiseDecayLoop = (v & 0x20));
        mWave_TND.bNoiseDecayEnable = !(v & 0x10);
        mWave_TND.nNoiseDecayTimer = (v & 0x0F);

        if(mWave_TND.bNoiseDecayEnable)
            mWave_TND.nNoiseVolume = mWave_TND.nNoiseDecayVolume;
        else
            mWave_TND.nNoiseVolume = mWave_TND.nNoiseDecayTimer;
        break;

    case 0x400E:
        mWave_TND.nNoiseFreqTimer = NOISE_FREQ_TABLE[v & 0x0F];
        mWave_TND.bNoiseRandomMode = (v & 0x80) ? 6 : 1;
        break;

    case 0x400F:
        if(mWave_TND.bNoiseChannelEnabled)
            mWave_TND.nNoiseLengthCount = LENGTH_COUNTER_TABLE[v >> 3];

        mWave_TND.nNoiseDecayVolume = 0x0F;
        if(mWave_TND.bNoiseDecayEnable)
            mWave_TND.nNoiseVolume = 0x0F;
        break;

    /* DMC */
    case 0x4010:
        mWave_TND.bDMCLoop = v & 0x40;
        mWave_TND.bDMCIRQEnabled = v & 0x80;
        /* IRQ can't be pending if disabled */
        if(!mWave_TND.bDMCIRQEnabled)
            mWave_TND.bDMCIRQPending = 0;

        mWave_TND.nDMCFreqTimer = DMC_FREQ_TABLE[bPALMode][v & 0x0F];
        break;

    case 0x4011:
        if(bIgnore4011Writes)
            break;
        v &= 0x7F;
        if(bDMCPopReducer)
        {
            if(bDMCPop_SamePlay)
                mWave_TND.nDMCOutput = v;
            else
            {
                if(bDMCPop_Skip)
                {
                    bDMCPop_Skip = 0;
                    break;
                }
                if(nDMCPop_Prev == v) break;
                if(mWave_TND.nDMCOutput == v) break;
                mWave_TND.nDMCOutput = nDMCPop_Prev;
                nDMCPop_Prev = v;
                bDMCPop_SamePlay = 1;
            }
        }
        else
            mWave_TND.nDMCOutput = v;
        break;

    case 0x4012:
        mWave_TND.nDMCDMABank_Load = (v >> 6) | 0x04;
        mWave_TND.nDMCDMAAddr_Load = (v << 6) & 0x0FFF;
        break;

    case 0x4013:
        mWave_TND.nDMCLength = (v << 4) + 1;
        break;

    /* All / General Purpose */
    case 0x4015:
        mWave_TND.bDMCIRQPending = 0;

        if(v & 0x01){   mWave_Squares.bChannelEnabled[0] =  1;  }
        else        {   mWave_Squares.bChannelEnabled[0] =
                        mWave_Squares.nLengthCount[0] =     0;  }
        if(v & 0x02){   mWave_Squares.bChannelEnabled[1] =  1;  }
        else        {   mWave_Squares.bChannelEnabled[1] =
                        mWave_Squares.nLengthCount[1] =     0;  }
        if(v & 0x04){   mWave_TND.bTriChannelEnabled =      1;  }
        else        {   mWave_TND.bTriChannelEnabled =
                        mWave_TND.nTriLengthCount =         0;  }
        if(v & 0x08){   mWave_TND.bNoiseChannelEnabled =    1;  }
        else        {   mWave_TND.bNoiseChannelEnabled =
                        mWave_TND.nNoiseLengthCount =       0;  }

        if(v & 0x10)
        {
            if(!mWave_TND.nDMCBytesRemaining)
            {
                bDMCPop_Skip = 1;
                mWave_TND.nDMCDMAAddr = mWave_TND.nDMCDMAAddr_Load;
                mWave_TND.nDMCDMABank = mWave_TND.nDMCDMABank_Load;
                mWave_TND.nDMCBytesRemaining = mWave_TND.nDMCLength;
                mWave_TND.bDMCActive = 1;
            }
        }
        else
            mWave_TND.nDMCBytesRemaining = 0;
        break;

    case 0x4017:
        bFrameIRQEnabled = !(v & 0x40);
        bFrameIRQPending = 0;
        nFrameCounter = 0;
        nFrameCounterMax = (v & 0x80) ? 4 : 3;
        nTicksUntilNextFrame =
            (bPALMode ? PAL_FRAME_COUNTER_FREQ : NTSC_FRAME_COUNTER_FREQ)
            * 0x10000;

        CLOCK_MAJOR();
        if(v & 0x80) CLOCK_MINOR();
        break;
    }

    if(!(nExternalSound & EXTSOUND_FDS))        return;
    if(bPALMode)                                return;

    /* FDS Sound registers */

    if(a < 0x4040)      return;

    /* wave table */
    if(a <= 0x407F)
    {
        if(mWave_FDS.bWaveWrite)
            mWave_FDS.nWaveTable[a - 0x4040] = v;
    }
    else
    {
        switch(a)
        {
        case 0x4080:
            mWave_FDS.nVolEnv_Mode = (v >> 6);
            if(v & 0x80)
            {
                mWave_FDS.nVolEnv_Gain = v & 0x3F;
                if(!mWave_FDS.nMainAddr)
                {
                    if(mWave_FDS.nVolEnv_Gain < 0x20)
                        mWave_FDS.nVolume = mWave_FDS.nVolEnv_Gain;
                    else mWave_FDS.nVolume = 0x20;
                }
            }
            mWave_FDS.nVolEnv_Decay = v & 0x3F;
            mWave_FDS.nVolEnv_Timer =
                ((mWave_FDS.nVolEnv_Decay + 1) * mWave_FDS.nEnvelopeSpeed * 8);

            mWave_FDS.bVolEnv_On = mWave_FDS.bEnvelopeEnable &&
                mWave_FDS.nEnvelopeSpeed && !(v & 0x80);
            break;

        case 0x4082:
            mWave_FDS.nFreq.B.l = v;
            mWave_FDS.bMain_On = mWave_FDS.nFreq.W && mWave_FDS.bEnabled &&
                !mWave_FDS.bWaveWrite;
            break;

        case 0x4083:
            mWave_FDS.bEnabled =        !(v & 0x80);
            mWave_FDS.bEnvelopeEnable = !(v & 0x40);
            if(v & 0x80)
            {
                if(mWave_FDS.nVolEnv_Gain < 0x20)
                    mWave_FDS.nVolume = mWave_FDS.nVolEnv_Gain;
                else mWave_FDS.nVolume = 0x20;
            }
            mWave_FDS.nFreq.B.h = v & 0x0F;
            mWave_FDS.bMain_On = mWave_FDS.nFreq.W && mWave_FDS.bEnabled &&
                !mWave_FDS.bWaveWrite;

            mWave_FDS.bVolEnv_On = mWave_FDS.bEnvelopeEnable &&
                mWave_FDS.nEnvelopeSpeed && !(mWave_FDS.nVolEnv_Mode & 2);
            mWave_FDS.bSweepEnv_On = mWave_FDS.bEnvelopeEnable &&
                mWave_FDS.nEnvelopeSpeed && !(mWave_FDS.nSweep_Mode & 2);
            break;


        case 0x4084:
            mWave_FDS.nSweep_Mode = v >> 6;
            if(v & 0x80)
                mWave_FDS.nSweep_Gain = v & 0x3F;
            mWave_FDS.nSweep_Decay = v & 0x3F;
            mWave_FDS.nSweep_Timer =
                ((mWave_FDS.nSweep_Decay + 1) * mWave_FDS.nEnvelopeSpeed * 8);
            mWave_FDS.bSweepEnv_On =
                mWave_FDS.bEnvelopeEnable && mWave_FDS.nEnvelopeSpeed &&
                !(v & 0x80);
            break;


        case 0x4085:
            if(v & 0x40)    mWave_FDS.nSweepBias = (v & 0x3F) - 0x40;
            else            mWave_FDS.nSweepBias = v & 0x3F;
            mWave_FDS.nLFO_Addr = 0;
            break;


        case 0x4086:
            mWave_FDS.nLFO_Freq.B.l = v;
            mWave_FDS.bLFO_On =
                mWave_FDS.bLFO_Enabled && mWave_FDS.nLFO_Freq.W;
            if(mWave_FDS.nLFO_Freq.W)
                mWave_FDS.nLFO_Timer = (0x10000<<14) / mWave_FDS.nLFO_Freq.W;
            break;

        case 0x4087:
            mWave_FDS.bLFO_Enabled = !(v & 0x80);
            mWave_FDS.nLFO_Freq.B.h = v & 0x0F;
            mWave_FDS.bLFO_On =
                mWave_FDS.bLFO_Enabled && mWave_FDS.nLFO_Freq.W;
            if(mWave_FDS.nLFO_Freq.W)
                mWave_FDS.nLFO_Timer = (0x10000<<14) / mWave_FDS.nLFO_Freq.W;
            break;

        case 0x4088:
            if(mWave_FDS.bLFO_Enabled)  break;
            register int32_t i;
            for(i = 0; i < 62; i++)
                mWave_FDS.nLFO_Table[i] = mWave_FDS.nLFO_Table[i + 2];
            mWave_FDS.nLFO_Table[62] = mWave_FDS.nLFO_Table[63] = v & 7;
            break;

        case 0x4089:
            mWave_FDS.nMainVolume = v & 3;
            mWave_FDS.bWaveWrite = v & 0x80;
            mWave_FDS.bMain_On = mWave_FDS.nFreq.W && mWave_FDS.bEnabled &&
                !mWave_FDS.bWaveWrite;
            break;

        case 0x408A:
            mWave_FDS.nEnvelopeSpeed = v;
            mWave_FDS.bVolEnv_On =
                mWave_FDS.bEnvelopeEnable &&
                mWave_FDS.nEnvelopeSpeed && !(mWave_FDS.nVolEnv_Mode & 2);
            mWave_FDS.bSweepEnv_On =
                mWave_FDS.bEnvelopeEnable &&
                mWave_FDS.nEnvelopeSpeed && !(mWave_FDS.nSweep_Mode & 2);
            break;
        }
    }
}

void  WriteMemory_VRC6(uint16_t a,uint8_t v)
{
    EmulateAPU(1);

    if((a < 0xA000) && (nExternalSound & EXTSOUND_VRC7)) return;
    else if(nExternalSound & EXTSOUND_FDS)
        WriteMemory_FDSRAM(a,v);

    switch(a)
    {
    /* Pulse 1 */
    case 0x9000:
        mWave_VRC6Pulse[0].nVolume = v & 0x0F;
        mWave_VRC6Pulse[0].nDutyCycle = (v >> 4) & 0x07;
        mWave_VRC6Pulse[0].bDigitized = v & 0x80;
        if(mWave_VRC6Pulse[0].bDigitized)
            mWave_VRC6Pulse[0].nDutyCount = 0;
        break;

    case 0x9001:
        mWave_VRC6Pulse[0].nFreqTimer.B.l = v;
        break;

    case 0x9002:
        mWave_VRC6Pulse[0].nFreqTimer.B.h = v & 0x0F;
        mWave_VRC6Pulse[0].bChannelEnabled = v & 0x80;
        break;
        

    /* Pulse 2 */
    case 0xA000:
        mWave_VRC6Pulse[1].nVolume = v & 0x0F;
        mWave_VRC6Pulse[1].nDutyCycle = (v >> 4) & 0x07;
        mWave_VRC6Pulse[1].bDigitized = v & 0x80;
        if(mWave_VRC6Pulse[1].bDigitized)
            mWave_VRC6Pulse[1].nDutyCount = 0;
        break;

    case 0xA001:
        mWave_VRC6Pulse[1].nFreqTimer.B.l = v;
        break;

    case 0xA002:
        mWave_VRC6Pulse[1].nFreqTimer.B.h = v & 0x0F;
        mWave_VRC6Pulse[1].bChannelEnabled = v & 0x80;
        break;
        
    /* Sawtooth */
    case 0xB000:
        mWave_VRC6Saw.nAccumRate = (v & 0x3F);
        break;

    case 0xB001:
        mWave_VRC6Saw.nFreqTimer.B.l = v;
        break;

    case 0xB002:
        mWave_VRC6Saw.nFreqTimer.B.h = v & 0x0F;
        mWave_VRC6Saw.bChannelEnabled = v & 0x80;
        break;
    }
}

void  WriteMemory_MMC5(uint16_t a,uint8_t v)
{
    if((a <= 0x5015) && !bPALMode)
    {
        /* no audio emulation */
        return;
    }

    if(a == 0x5205)
    {
        nMultIn_Low = v;
        goto multiply;
    }
    if(a == 0x5206)
    {
        nMultIn_High = v;
multiply:
        a = nMultIn_Low * nMultIn_High;
        pExRAM[0x205] = a & 0xFF;
        pExRAM[0x206] = a >> 8;
        return;
    }

    if(a < 0x5C00) return;

    pExRAM[a & 0x0FFF] = v;
    if(a >= 0x5FF6)
        WriteMemory_ExRAM(a,v);
}

void  WriteMemory_N106(uint16_t a,uint8_t v)
{
    if(a < 0x4800)
    {
        WriteMemory_pAPU(a,v);
        return;
    }

    if(a == 0xF800)
    {
        mWave_N106.nCurrentAddress = v & 0x7F;
        mWave_N106.bAutoIncrement = (v & 0x80);
        return;
    }

    if(a == 0x4800)
    {
        EmulateAPU(1);
        mWave_N106.nRAM[mWave_N106.nCurrentAddress << 1] = v & 0x0F;
        mWave_N106.nRAM[(mWave_N106.nCurrentAddress << 1) + 1] = v >> 4;
        a = mWave_N106.nCurrentAddress;
        if(mWave_N106.bAutoIncrement)
            mWave_N106.nCurrentAddress =
                (mWave_N106.nCurrentAddress + 1) & 0x7F;

#define N106REGWRITE(ch,r0,r1,r2,r3,r4)                         \
    case r0:    if(mWave_N106.nFreqReg[ch].B.l == v) break;     \
                mWave_N106.nFreqReg[ch].B.l = v;                \
                mWave_N106.nFreqTimer[ch] = -1;              \
                break;                                          \
    case r1:    if(mWave_N106.nFreqReg[ch].B.h == v) break;     \
                mWave_N106.nFreqReg[ch].B.h = v;                \
                mWave_N106.nFreqTimer[ch] = -1;              \
                break;                                          \
    case r2:    if(mWave_N106.nFreqReg[ch].B.w != (v & 3)){     \
                    mWave_N106.nFreqReg[ch].B.w = v & 0x03;     \
                    mWave_N106.nFreqTimer[ch] = -1;}         \
                mWave_N106.nWaveSize[ch] = 0x20 - (v & 0x1C);   \
                break;                                          \
    case r3:    mWave_N106.nWavePosStart[ch] = v;               \
                break;                                          \
    case r4:    mWave_N106.nPreVolume[ch] = v & 0x0F;           \
                if(!bN106PopReducer)                            \
                    mWave_N106.nVolume[ch] = v & 0x0F

        switch(a)
        {
            N106REGWRITE(0,0x40,0x42,0x44,0x46,0x47); break;
            N106REGWRITE(1,0x48,0x4A,0x4C,0x4E,0x4F); break;
            N106REGWRITE(2,0x50,0x52,0x54,0x56,0x57); break;
            N106REGWRITE(3,0x58,0x5A,0x5C,0x5E,0x5F); break;
            N106REGWRITE(4,0x60,0x62,0x64,0x66,0x67); break;
            N106REGWRITE(5,0x68,0x6A,0x6C,0x6E,0x6F); break;
            N106REGWRITE(6,0x70,0x72,0x74,0x76,0x77); break;
            N106REGWRITE(7,0x78,0x7A,0x7C,0x7E,0x7F);
                v = (v >> 4) & 7;
                if(mWave_N106.nActiveChannels == v) break;
                mWave_N106.nActiveChannels = v;
                mWave_N106.nFreqTimer[0] = -1;
                mWave_N106.nFreqTimer[1] = -1;
                mWave_N106.nFreqTimer[2] = -1;
                mWave_N106.nFreqTimer[3] = -1;
                mWave_N106.nFreqTimer[4] = -1;
                mWave_N106.nFreqTimer[5] = -1;
                mWave_N106.nFreqTimer[6] = -1;
                mWave_N106.nFreqTimer[7] = -1;
                break;
        }
#undef N106REGWRITE
    }
}

void WriteMemory_FME07(uint16_t a,uint8_t v)
{
    if((a < 0xD000) && (nExternalSound & EXTSOUND_FDS))
        WriteMemory_FDSRAM(a,v);

    if(a == 0xC000)
        nFME07_Address = v;
    if(a == 0xE000)
    {
        switch(nFME07_Address)
        {
        case 0x00:  mWave_FME07[0].nFreqTimer.B.l = v;          break;
        case 0x01:  mWave_FME07[0].nFreqTimer.B.h = v & 0x0F;   break;
        case 0x02:  mWave_FME07[1].nFreqTimer.B.l = v;          break;
        case 0x03:  mWave_FME07[1].nFreqTimer.B.h = v & 0x0F;   break;
        case 0x04:  mWave_FME07[2].nFreqTimer.B.l = v;          break;
        case 0x05:  mWave_FME07[2].nFreqTimer.B.h = v & 0x0F;   break;
        case 0x07:
            mWave_FME07[0].bChannelEnabled = !(v & 0x01);
            mWave_FME07[1].bChannelEnabled = !(v & 0x02);
            mWave_FME07[2].bChannelEnabled = !(v & 0x03);
            break;
        case 0x08:  mWave_FME07[0].nVolume = v & 0x0F; break;
        case 0x09:  mWave_FME07[1].nVolume = v & 0x0F; break;
        case 0x0A:  mWave_FME07[2].nVolume = v & 0x0F; break;
        }
    }
}

/*
 * Emulate APU
 */

int32_t fulltick;
void EmulateAPU(uint8_t bBurnCPUCycles)
{
    int32_t tick;
    int64_t diff;
    
    int32_t tnd_out;
    int square_out1;
    int square_out2;
    
    ENTER_TIMER(apu);
    
    fulltick += (signed)(nCPUCycle - nAPUCycle);

    int32_t burned;
    int32_t mixL;

    if(bFade && nSilentSampleMax && (nSilentSamples >= nSilentSampleMax))
        fulltick = 0;

    while(fulltick>0)
    {
        tick = (nTicksUntilNextSample+0xffff)>>16;

        fulltick -= tick;

        /*
         * Sample Generation
         */

        ENTER_TIMER(squares);
        /* Square generation */

        mWave_Squares.nFreqCount[0] -= tick;
        mWave_Squares.nFreqCount[1] -= tick;

        if((mWave_Squares.nDutyCount[0] < mWave_Squares.nDutyCycle[0]) &&
            mWave_Squares.nLengthCount[0] &&
            !mWave_Squares.bSweepForceSilence[0])
            square_out1 = mWave_Squares.nVolume[0];
        else
            square_out1 = 0;

        if((mWave_Squares.nDutyCount[1] < mWave_Squares.nDutyCycle[1]) &&
            mWave_Squares.nLengthCount[1] &&
            !mWave_Squares.bSweepForceSilence[1])
            square_out2 = mWave_Squares.nVolume[1];
        else
            square_out2 = 0;

        mWave_Squares.nMixL = Squares_nOutputTable_L[square_out1][square_out2];

        if(mWave_Squares.nFreqCount[0]<=0)
        {
            int cycles =
                (-mWave_Squares.nFreqCount[0])/
                (mWave_Squares.nFreqTimer[0].W + 1) + 1;
            mWave_Squares.nFreqCount[0] =
                (mWave_Squares.nFreqTimer[0].W + 1)-
                (-mWave_Squares.nFreqCount[0])%
                (mWave_Squares.nFreqTimer[0].W + 1);
            mWave_Squares.nDutyCount[0] =
                (mWave_Squares.nDutyCount[0]+cycles)%0x10;
        }
        if(mWave_Squares.nFreqCount[1]<=0)
        {
            int cycles =
                (-mWave_Squares.nFreqCount[1])/
                (mWave_Squares.nFreqTimer[1].W + 1) + 1;
            mWave_Squares.nFreqCount[1] = 
                (mWave_Squares.nFreqTimer[1].W + 1)-
                (-mWave_Squares.nFreqCount[1])%
                (mWave_Squares.nFreqTimer[1].W + 1);
            mWave_Squares.nDutyCount[1] = (mWave_Squares.nDutyCount[1]+cycles)%
                0x10;
        }
        /* end of Square generation */
        EXIT_TIMER(squares);
        ENTER_TIMER(tnd);
        
        ENTER_TIMER(tnd_enter);
    
        burned=0;
    
        /* TND generation */
    
        if(mWave_TND.nNoiseFreqTimer) mWave_TND.nNoiseFreqCount -= tick;
            
        if(mWave_TND.nTriFreqTimer.W > 8)
            mWave_TND.nTriFreqCount -= tick;

        tnd_out = mWave_TND.nTriOutput << 11;

        if(mWave_TND.bNoiseRandomOut && mWave_TND.nNoiseLengthCount)
            tnd_out |= mWave_TND.nNoiseVolume << 7;

        tnd_out |= mWave_TND.nDMCOutput;

        mWave_TND.nMixL = main_nOutputTable_L[tnd_out];

        EXIT_TIMER(tnd_enter);
    
        ENTER_TIMER(tnd_tri);
    
        /* Tri */

        if(mWave_TND.nTriFreqCount<=0)
        {
            if(mWave_TND.nTriLengthCount && mWave_TND.nTriLinearCount)
            {
                do mWave_TND.nTriStep++;
                while ((mWave_TND.nTriFreqCount +=
                    mWave_TND.nTriFreqTimer.W + 1) <= 0);
                mWave_TND.nTriStep &= 0x1F;

                if(mWave_TND.nTriStep & 0x10)
                    mWave_TND.nTriOutput = mWave_TND.nTriStep ^ 0x1F;
                else mWave_TND.nTriOutput = mWave_TND.nTriStep;
            } else mWave_TND.nTriFreqCount=mWave_TND.nTriFreqTimer.W+1;
        }

        EXIT_TIMER(tnd_tri);
    
        ENTER_TIMER(tnd_noise);
    
        /* Noise */

        if(mWave_TND.nNoiseFreqTimer &&
           mWave_TND.nNoiseVolume && mWave_TND.nNoiseFreqCount<=0)
        {
            mWave_TND.nNoiseFreqCount = mWave_TND.nNoiseFreqTimer;
            mWave_TND.nNoiseRandomShift <<= 1;
            mWave_TND.bNoiseRandomOut = (((mWave_TND.nNoiseRandomShift <<
                mWave_TND.bNoiseRandomMode) ^
                mWave_TND.nNoiseRandomShift) & 0x8000 ) ? 1 : 0;
            if(mWave_TND.bNoiseRandomOut)
                mWave_TND.nNoiseRandomShift |= 0x01;
        }
    
        EXIT_TIMER(tnd_noise);
    
        ENTER_TIMER(tnd_dmc);

        /* DMC */
        if(mWave_TND.bDMCActive)
        {
            mWave_TND.nDMCFreqCount -= tick;
            while (mWave_TND.nDMCFreqCount <= 0) {
                if (!mWave_TND.bDMCActive) {
                    mWave_TND.nDMCFreqCount = mWave_TND.nDMCFreqTimer;
                    break;
                }

                mWave_TND.nDMCFreqCount += mWave_TND.nDMCFreqTimer;

                if(mWave_TND.bDMCSampleBufferEmpty &&
                   mWave_TND.nDMCBytesRemaining)
                {
                    burned += 4;        /* 4 cycle burn! */
                    mWave_TND.nDMCSampleBuffer =
                        mWave_TND.pDMCDMAPtr[mWave_TND.nDMCDMABank]
                                            [mWave_TND.nDMCDMAAddr];
                    mWave_TND.nDMCDMAAddr++;
                    if(mWave_TND.nDMCDMAAddr & 0x1000)
                    {
                        mWave_TND.nDMCDMAAddr &= 0x0FFF;
                        mWave_TND.nDMCDMABank =
                            (mWave_TND.nDMCDMABank + 1) & 0x07;
                    }

                    mWave_TND.bDMCSampleBufferEmpty = 0;
                    mWave_TND.nDMCBytesRemaining--;
                    if(!mWave_TND.nDMCBytesRemaining)
                    {
                        if(mWave_TND.bDMCLoop)
                        {
                            mWave_TND.nDMCDMABank = mWave_TND.nDMCDMABank_Load;
                            mWave_TND.nDMCDMAAddr = mWave_TND.nDMCDMAAddr_Load;
                            mWave_TND.nDMCBytesRemaining =mWave_TND.nDMCLength;
                        }
                        else if(mWave_TND.bDMCIRQEnabled)
                            mWave_TND.bDMCIRQPending = 1;
                    }
                }

                if(!mWave_TND.nDMCDeltaBit)
                {
                    mWave_TND.nDMCDeltaBit = 8;
                    mWave_TND.bDMCDeltaSilent =mWave_TND.bDMCSampleBufferEmpty;
                    mWave_TND.nDMCDelta = mWave_TND.nDMCSampleBuffer;
                    mWave_TND.bDMCSampleBufferEmpty = 1;
                }
                
                if(mWave_TND.nDMCDeltaBit) {
                    mWave_TND.nDMCDeltaBit--;
                    if(!mWave_TND.bDMCDeltaSilent)
                    {
                        if(mWave_TND.nDMCDelta & 0x01)
                        {
                            if(mWave_TND.nDMCOutput < 0x7E)
                                mWave_TND.nDMCOutput += 2;
                        }
                        else if(mWave_TND.nDMCOutput > 1)
                            mWave_TND.nDMCOutput -= 2;
                    }
                    mWave_TND.nDMCDelta >>= 1;
                }

                if(!mWave_TND.nDMCBytesRemaining &&
                    mWave_TND.bDMCSampleBufferEmpty &&
                    mWave_TND.bDMCDeltaSilent)
                    mWave_TND.bDMCActive = mWave_TND.nDMCDeltaBit = 0;
            }
        }
    
        EXIT_TIMER(tnd_dmc);
   
        /* end of TND generation */
        EXIT_TIMER(tnd);

        if(nExternalSound && !bPALMode)
        {
            if(nExternalSound & EXTSOUND_VRC6)
                Wave_VRC6_DoTicks(tick);
            if(nExternalSound & EXTSOUND_N106)
                Wave_N106_DoTicks(tick);
            if(nExternalSound & EXTSOUND_FME07)
            {
                if (mWave_FME07[0].bChannelEnabled &&
                    mWave_FME07[0].nFreqTimer.W) {
                    mWave_FME07[0].nFreqCount -= tick;

                    if(mWave_FME07[0].nDutyCount < 16)
                    {
                        mWave_FME07[0].nMixL =
                            FME07_nOutputTable_L[mWave_FME07[0].nVolume];
                    } else mWave_FME07[0].nMixL = 0;
                    while(mWave_FME07[0].nFreqCount <= 0) {
                        mWave_FME07[0].nFreqCount +=
                            mWave_FME07[0].nFreqTimer.W;

                        mWave_FME07[0].nDutyCount=
                            (mWave_FME07[0].nDutyCount+1)&0x1f;
                    }
                }

                if (mWave_FME07[1].bChannelEnabled &&
                    mWave_FME07[1].nFreqTimer.W) {
                    mWave_FME07[1].nFreqCount -= tick;

                    if(mWave_FME07[1].nDutyCount < 16)
                    {
                        mWave_FME07[1].nMixL =
                            FME07_nOutputTable_L[mWave_FME07[1].nVolume];
                    } else mWave_FME07[1].nMixL = 0;
                    while(mWave_FME07[1].nFreqCount <= 0) {
                        mWave_FME07[1].nFreqCount +=
                            mWave_FME07[1].nFreqTimer.W;

                        mWave_FME07[1].nDutyCount=
                            (mWave_FME07[1].nDutyCount+1)&0x1f;
                    }
                }

                if (mWave_FME07[2].bChannelEnabled &&
                    mWave_FME07[2].nFreqTimer.W) {
                    mWave_FME07[2].nFreqCount -= tick;

                    if(mWave_FME07[2].nDutyCount < 16)
                    {
                        mWave_FME07[2].nMixL =
                            FME07_nOutputTable_L[mWave_FME07[2].nVolume];
                    } else mWave_FME07[2].nMixL = 0;
                    while(mWave_FME07[2].nFreqCount <= 0) {
                        mWave_FME07[2].nFreqCount +=
                            mWave_FME07[2].nFreqTimer.W;

                        mWave_FME07[2].nDutyCount=
                            (mWave_FME07[2].nDutyCount+1)&0x1f;
                    }
                }

            } /* end FME07 */
            ENTER_TIMER(fds);
            if(nExternalSound & EXTSOUND_FDS) {

                /*  Volume Envelope Unit    */
                if(mWave_FDS.bVolEnv_On)
                {
                    mWave_FDS.nVolEnv_Count -= tick;
                    while(mWave_FDS.nVolEnv_Count <= 0)
                    {
                        mWave_FDS.nVolEnv_Count += mWave_FDS.nVolEnv_Timer;
                        if(mWave_FDS.nVolEnv_Mode) {
                            if(mWave_FDS.nVolEnv_Gain < 0x20)
                                mWave_FDS.nVolEnv_Gain++;
                            }
                        else {
                            if(mWave_FDS.nVolEnv_Gain)
                                mWave_FDS.nVolEnv_Gain--;
                        }
                    }
                }
    
                /*  Sweep Envelope Unit */
                if(mWave_FDS.bSweepEnv_On)
                {
                    mWave_FDS.nSweep_Count -= tick;
                    while(mWave_FDS.nSweep_Count <= 0)
                    {
                        mWave_FDS.nSweep_Count += mWave_FDS.nSweep_Timer;
                        if(mWave_FDS.nSweep_Mode)    {
                            if(mWave_FDS.nSweep_Gain < 0x20)
                                mWave_FDS.nSweep_Gain++;
                        } else {
                            if(mWave_FDS.nSweep_Gain) mWave_FDS.nSweep_Gain--;
                        }
                    }
                }
            
                /*  Effector / LFO      */
                int32_t     subfreq = 0;
                if(mWave_FDS.bLFO_On)
                {
                    mWave_FDS.nLFO_Count -= tick<<14;
                    while(mWave_FDS.nLFO_Count <= 0)
                    {
                        mWave_FDS.nLFO_Count += mWave_FDS.nLFO_Timer;
                        if(mWave_FDS.nLFO_Table[mWave_FDS.nLFO_Addr] == 4)
                            mWave_FDS.nSweepBias = 0;
                        else 
                            mWave_FDS.nSweepBias +=
                                ModulationTable[ 
                                    mWave_FDS.nLFO_Table[mWave_FDS.nLFO_Addr]
                                ];
                        mWave_FDS.nLFO_Addr = (mWave_FDS.nLFO_Addr + 1) & 0x3F;
                    }
            
                    while(mWave_FDS.nSweepBias >  63)
                        mWave_FDS.nSweepBias -= 128;
                    while(mWave_FDS.nSweepBias < -64)
                        mWave_FDS.nSweepBias += 128;
            
                    register int32_t temp =
                        mWave_FDS.nSweepBias * mWave_FDS.nSweep_Gain;
                    if(temp & 0x0F)
                    {
                        temp /= 16;
                        if(mWave_FDS.nSweepBias < 0) temp--;
                        else                temp += 2;
                    }
                    else
                        temp /= 16;
            
                    if(temp > 193)  temp -= 258;
                    if(temp < -64)  temp += 256;
            
                    subfreq = mWave_FDS.nFreq.W * temp / 64;
                }
            
                /*  Main Unit       */
                if(mWave_FDS.bMain_On)
                {
                    mWave_FDS.nMixL =
                        FDS_nOutputTable_L[mWave_FDS.nMainVolume]
                                          [mWave_FDS.nVolume]
                                 [mWave_FDS.nWaveTable[mWave_FDS.nMainAddr] ];
            
                    if((subfreq + mWave_FDS.nFreq.W) > 0)
                    {
                        int32_t freq = (0x10000<<14) / (subfreq + mWave_FDS.nFreq.W);
            
                        mWave_FDS.nFreqCount -= tick<<14;
                        while(mWave_FDS.nFreqCount <= 0)
                        {
                            mWave_FDS.nFreqCount += freq;
            
                            mWave_FDS.nMainAddr =
                                (mWave_FDS.nMainAddr + 1) & 0x3F;
                            mWave_FDS.nPopOutput =
                                mWave_FDS.nWaveTable[mWave_FDS.nMainAddr];
                            if(!mWave_FDS.nMainAddr)
                            {
                                if(mWave_FDS.nVolEnv_Gain < 0x20)
                                    mWave_FDS.nVolume = mWave_FDS.nVolEnv_Gain;
                                else mWave_FDS.nVolume = 0x20;
                            }
                        }
                    }
                    else
                        mWave_FDS.nFreqCount = mWave_FDS.nLFO_Count;
                }
                else if(mWave_FDS.bPopReducer && mWave_FDS.nPopOutput)
                {
                    mWave_FDS.nMixL = FDS_nOutputTable_L[mWave_FDS.nMainVolume]
                                                        [mWave_FDS.nVolume]
                                                        [mWave_FDS.nPopOutput];
            
                    mWave_FDS.nPopCount -= tick;
                    while(mWave_FDS.nPopCount <= 0)
                    {
                        mWave_FDS.nPopCount += 500;
                        mWave_FDS.nPopOutput--;
                        if(!mWave_FDS.nPopOutput)
                            mWave_FDS.nMainAddr = 0;
                    }
                } /* end FDS */
            }
            EXIT_TIMER(fds);
        } /* end while fulltick */

        if(bBurnCPUCycles)
        {
            nCPUCycle += burned;
            fulltick += burned;
        }
        
        /* Frame Sequencer */

        ENTER_TIMER(frame);
        nTicksUntilNextFrame -= tick<<16;
        while(nTicksUntilNextFrame <= 0)
        {
            nTicksUntilNextFrame +=
                (bPALMode ? PAL_FRAME_COUNTER_FREQ : NTSC_FRAME_COUNTER_FREQ) *
                0x10000;
            nFrameCounter++;
            if(nFrameCounter > nFrameCounterMax)
                nFrameCounter = 0;

            if(nFrameCounterMax == 4)
            {
                if(nFrameCounter < 4)
                {
                    CLOCK_MAJOR();
                    if(!(nFrameCounter & 1))
                        CLOCK_MINOR();
                }
            }
            else
            {
                CLOCK_MAJOR();
                if(nFrameCounter & 1)
                    CLOCK_MINOR();

                if((nFrameCounter == 3) && bFrameIRQEnabled)
                    bFrameIRQPending = 1;
            }
        }
        EXIT_TIMER(frame);

        ENTER_TIMER(mix);
        nTicksUntilNextSample -= tick<<16;
        if(nTicksUntilNextSample <= 0)
        {
            nTicksUntilNextSample += nTicksPerSample;
            
            mixL = mWave_Squares.nMixL;
            mixL += mWave_TND.nMixL;

            if(nExternalSound && !bPALMode)
            {
                if(nExternalSound & EXTSOUND_VRC6)
                {
                    mixL += (mWave_VRC6Pulse[0].nMixL);
                    mixL += (mWave_VRC6Pulse[1].nMixL);
                    mixL += (mWave_VRC6Saw.nMixL);
                }
                if(nExternalSound & EXTSOUND_N106) {
                    mixL += (mWave_N106.nMixL[0]);
                    mixL += (mWave_N106.nMixL[1]);
                    mixL += (mWave_N106.nMixL[2]);
                    mixL += (mWave_N106.nMixL[3]);
                    mixL += (mWave_N106.nMixL[4]);
                    mixL += (mWave_N106.nMixL[5]);
                    mixL += (mWave_N106.nMixL[6]);
                    mixL += (mWave_N106.nMixL[7]);
                }
                if(nExternalSound & EXTSOUND_FME07)
                {
                    mixL += (mWave_FME07[0].nMixL);
                    mixL += (mWave_FME07[1].nMixL);
                    mixL += (mWave_FME07[2].nMixL);
                }
                if(nExternalSound & EXTSOUND_FDS)
                    mixL += mWave_FDS.nMixL;
            }

            /*  Filter  */
            diff = ((int64_t)mixL << 25) - nFilterAccL;
            nFilterAccL += (diff * nHighPass) >> 16;
            mixL = (int32_t)(diff >> 23);
            /*  End Filter  */
                
            if(bFade && (fFadeVolume < 1))
                mixL = (int32_t)(mixL * fFadeVolume);

            if(mixL < -32768)   mixL = -32768;
            if(mixL >  32767)   mixL =  32767;

            *((uint16_t*)pOutput) = (uint16_t)mixL;
            pOutput += 2;
        }
        
    }
    EXIT_TIMER(mix);

    nAPUCycle = nCPUCycle;
    
    EXIT_TIMER(apu);
}


/*
 *  Initialize
 *
 *      Initializes Memory
 */

int NSFCore_Initialize()
{
    int32_t i;
    /* clear globals */
    /* why, yes, this was easier when they were in a struct */

    /*
     *  Memory
     */

    ZEROMEMORY(pRAM,0x800);
    ZEROMEMORY(pSRAM,0x2000);
    ZEROMEMORY(pExRAM,0x1000);
    pROM_Full=0;

    ZEROMEMORY(pROM,10);
    pStack=0;

    nROMSize=0;
    nROMBankCount=0;
    nROMMaxSize=0;

    /*
     *  Memory Proc Pointers
     */

    ZEROMEMORY(ReadMemory,sizeof(ReadProc)*0x10);
    ZEROMEMORY(WriteMemory,sizeof(WriteProc)*0x10);
    
    /*
     *  6502 Registers / Mode
     */

    regA=0;
    regX=0;
    regY=0;
    regP=0;
    regSP=0;
    regPC=0;

    bPALMode=0;
    bCPUJammed=0;

    nMultIn_Low=0;
    nMultIn_High=0;

    /*
     *  NSF Preparation Information
     */

    ZEROMEMORY(nBankswitchInitValues,10);
    nPlayAddress=0;
    nInitAddress=0;

    nExternalSound=0;
    nCurTrack=0;

    fNSFPlaybackSpeed=0;

    /*
     * pAPU
     */

    nFrameCounter=0;
    nFrameCounterMax=0;
    bFrameIRQEnabled=0;
    bFrameIRQPending=0;

    /*
     *  Timing and Counters
     */
    nTicksUntilNextFrame=0;

    nTicksPerPlay=0;
    nTicksUntilNextPlay=0;

    nTicksPerSample=0;
    nTicksUntilNextSample=0;

    nCPUCycle=0;
    nAPUCycle=0;
    nTotalPlays=0;

    /*
     * Silence Tracker
     */
    nSilentSamples=0;
    nSilentSampleMax=0;
    nSilenceTrackMS=0;
    bNoSilenceIfTime=0;
    bTimeNotDefault=0;

    /*
     * Volume/fading/filter tracking
     */

    nStartFade=0;
    nEndFade=0;
    bFade=0;
    fFadeVolume=0;
    fFadeChange=0;

    pOutput=0;

    nDMCPop_Prev=0;
    bDMCPop_Skip=0;
    bDMCPop_SamePlay=0;

    /*
     * Sound Filter
     */

    nFilterAccL=0;
    nHighPass=0;

    nHighPassBase=0;

    bHighPassEnabled=0;

    /* channels */
    
    ZEROMEMORY(&mWave_Squares,sizeof(struct Wave_Squares));
    ZEROMEMORY(&mWave_TND,sizeof(struct Wave_TND));
    ZEROMEMORY(mWave_VRC6Pulse,sizeof(struct VRC6PulseWave)*2);
    ZEROMEMORY(&mWave_VRC6Saw,sizeof(struct VRC6SawWave));
    ZEROMEMORY(&mWave_N106,sizeof(struct N106Wave));
    ZEROMEMORY(mWave_FME07,sizeof(struct FME07Wave)*3);
    ZEROMEMORY(&mWave_FDS,sizeof(struct FDSWave));
    
    /* end clear globals */

    // Default filter bases
    nHighPassBase = 150;

    bHighPassEnabled = 1;

    mWave_TND.nNoiseRandomShift =   1;
    for(i = 0; i < 8; i++)
        mWave_TND.pDMCDMAPtr[i] = pROM[i + 2];


    SetPlaybackOptions(nSampleRate);

    for(i = 0; i < 8; i++)
        mWave_N106.nFrequencyLookupTable[i] =
            ((((i + 1) * 45 * 0x40000) / (float)NES_FREQUENCY) *
            (float)NTSC_FREQUENCY) * 256.0;

    ZEROMEMORY(pRAM,0x800);
    ZEROMEMORY(pSRAM,0x2000);
    ZEROMEMORY(pExRAM,0x1000);
    pStack = pRAM + 0x100;
    return 1;
}

/*
 *  LoadNSF
 */

int LoadNSF(int32_t datasize)
{
    if(!pDataBuffer)                return 0;

    int32_t i;

    nExternalSound = nChipExtensions;
    if(nIsPal & 2)
        bPALMode = bPALPreference;
    else
        bPALMode = nIsPal & 1;

    SetPlaybackOptions(nSampleRate);
    
    int32_t neededsize = datasize + (nfileLoadAddress & 0x0FFF);
    if(neededsize & 0x0FFF)     neededsize += 0x1000 - (neededsize & 0x0FFF);
    if(neededsize < 0x1000)     neededsize = 0x1000;

    uint8_t specialload = 0;
    
    for(i = 0; (i < 8) && (!nBankswitch[i]); i++);
    if(i < 8)       /* uses bankswitching */
    {
        memcpy(&nBankswitchInitValues[2],nBankswitch,8);
        nBankswitchInitValues[0] = nBankswitch[6];
        nBankswitchInitValues[1] = nBankswitch[7];
        if(nExternalSound & EXTSOUND_FDS)
        {
            if(!(nBankswitchInitValues[0] || nBankswitchInitValues[1]))
            {
                /*
                 * FDS sound with '00' specified for both $6000 and $7000 banks.
                 * point this to an area of fresh RAM (sort of hackish solution
                 * for those FDS tunes that don't quite follow the nsf specs.
                 */
                nBankswitchInitValues[0] = (uint8_t)(neededsize >> 12);
                nBankswitchInitValues[1] = (uint8_t)(neededsize >> 12) + 1;
                neededsize += 0x2000;
            }
        }
    }
    else            /* doesn't use bankswitching */
    {
        if(nExternalSound & EXTSOUND_FDS)
        {
            /* bad load address */
            if(nfileLoadAddress < 0x6000)       return 0;

            if(neededsize < 0xA000)
                neededsize = 0xA000;
            specialload = 1;
            for(i = 0; i < 10; i++)
                nBankswitchInitValues[i] = (uint8_t)i;
        }
        else
        {
            /* bad load address */
            if(nfileLoadAddress < 0x8000)       return 0;

            int32_t j = (nfileLoadAddress >> 12) - 6;
            for(i = 0; i < j; i++)
                nBankswitchInitValues[i] = 0;
            for(j = 0; i < 10; i++, j++)
                nBankswitchInitValues[i] = (uint8_t)j;
        }
    }

    nROMSize = neededsize;
    nROMBankCount = neededsize >> 12;

    if(specialload)
        pROM_Full = pDataBuffer-(nfileLoadAddress-0x6000);
    else
        pROM_Full = pDataBuffer-(nfileLoadAddress&0x0FFF);

    ZEROMEMORY(pRAM,0x0800);
    ZEROMEMORY(pExRAM,0x1000);
    ZEROMEMORY(pSRAM,0x2000);

    nExternalSound = nChipExtensions;
    fNSFPlaybackSpeed = (bPALMode ? PAL_NMIRATE : NTSC_NMIRATE);
    
    SetPlaybackSpeed(0);

    nPlayAddress = nfilePlayAddress;
    nInitAddress = nfileInitAddress;

    pExRAM[0x00] = 0x20;                        /* JSR */
    pExRAM[0x01] = nInitAddress&0xff;           /* Init Address */
    pExRAM[0x02] = (nInitAddress>>8)&0xff;
    pExRAM[0x03] = 0xF2;                        /* JAM */
    pExRAM[0x04] = 0x20;                        /* JSR */
    pExRAM[0x05] = nPlayAddress&0xff;           /* Play Address */
    pExRAM[0x06] = (nPlayAddress>>8)&0xff;
    pExRAM[0x07] = 0x4C;                        /* JMP */
    pExRAM[0x08] = 0x03;/* $5003  (JAM right before the JSR to play address) */
    pExRAM[0x09] = 0x50;

    regA = regX = regY = 0;
    regP = 0x04;            /* I_FLAG */
    regSP = 0xFF;

    nFilterAccL = 0;

    /*  Reset Read/Write Procs          */
    
    ReadMemory[0] = ReadMemory[1] = ReadMemory_RAM;
    ReadMemory[2] = ReadMemory[3] = ReadMemory_Default;
    ReadMemory[4] =                 ReadMemory_pAPU;
    ReadMemory[5] =                 ReadMemory_ExRAM;
    ReadMemory[6] = ReadMemory[7] = ReadMemory_SRAM;

    WriteMemory[0] = WriteMemory[1] =   WriteMemory_RAM;
    WriteMemory[2] = WriteMemory[3] =   WriteMemory_Default;
    WriteMemory[4] =                    WriteMemory_pAPU;
    WriteMemory[5] =                    WriteMemory_ExRAM;
    WriteMemory[6] = WriteMemory[7] =   WriteMemory_SRAM;

    for(i = 8; i < 16; i++)
    {
        ReadMemory[i] = ReadMemory_ROM;
        WriteMemory[i] = WriteMemory_Default;
    }

    if(nExternalSound & EXTSOUND_FDS)
    {
        WriteMemory[0x06] = WriteMemory_FDSRAM;
        WriteMemory[0x07] = WriteMemory_FDSRAM;
        WriteMemory[0x08] = WriteMemory_FDSRAM;
        WriteMemory[0x09] = WriteMemory_FDSRAM;
        WriteMemory[0x0A] = WriteMemory_FDSRAM;
        WriteMemory[0x0B] = WriteMemory_FDSRAM;
        WriteMemory[0x0C] = WriteMemory_FDSRAM;
        WriteMemory[0x0D] = WriteMemory_FDSRAM;
        ReadMemory[0x06] = ReadMemory_ROM;
        ReadMemory[0x07] = ReadMemory_ROM;
    }

    if(!bPALMode)   /* no expansion sound available on a PAL system */
    {
        if(nExternalSound & EXTSOUND_VRC6)
        {
            /* if both VRC6+VRC7... it MUST go to WriteMemory_VRC6
             * or register writes will be lost (WriteMemory_VRC6 calls
             * WriteMemory_VRC7 if needed) */
            WriteMemory[0x09] = WriteMemory_VRC6;   
            WriteMemory[0x0A] = WriteMemory_VRC6;   
            WriteMemory[0x0B] = WriteMemory_VRC6;   
        }
        if(nExternalSound & EXTSOUND_N106)
        {
            WriteMemory[0x04] = WriteMemory_N106;
            ReadMemory[0x04] = ReadMemory_N106;
            WriteMemory[0x0F] = WriteMemory_N106;
        }
        if(nExternalSound & EXTSOUND_FME07)
        {
            WriteMemory[0x0C] = WriteMemory_FME07;
            WriteMemory[0x0E] = WriteMemory_FME07;
        }
    }
    
    /* MMC5 still has a multiplication reg that needs to be available on
       PAL tunes */
    if(nExternalSound & EXTSOUND_MMC5)
        WriteMemory[0x05] = WriteMemory_MMC5;

    return 1;
}

/*
 *  SetTrack
 */

void SetTrack(uint8_t track)
{
    int32_t i;
    
    nCurTrack = track;

    regPC = 0x5000;
    regA = track;
    regX = bPALMode;
    regY = bCleanAXY ? 0 : 0xCD;
    regSP = 0xFF;
    if(bCleanAXY)
        regP = 0x04;
    bCPUJammed = 0;

    nCPUCycle = nAPUCycle = 0;
    nDMCPop_Prev = 0;
    bDMCPop_Skip = 0;

    for(i = 0x4000; i < 0x400F; i++)
        WriteMemory_pAPU(i,0);
    WriteMemory_pAPU(0x4010,0);
    WriteMemory_pAPU(0x4012,0);
    WriteMemory_pAPU(0x4013,0);
    WriteMemory_pAPU(0x4014,0);
    WriteMemory_pAPU(0x4015,0);
    WriteMemory_pAPU(0x4015,0x0F);
    WriteMemory_pAPU(0x4017,0);

    for(i = 0; i < 10; i++)
        WriteMemory_ExRAM(0x5FF6 + i,nBankswitchInitValues[i]);

    ZEROMEMORY(pRAM,0x0800);
    ZEROMEMORY(pSRAM,0x2000);
    ZEROMEMORY(&pExRAM[0x10],0x0FF0);
    bFade = 0;


    nTicksUntilNextSample = nTicksPerSample;
    nTicksUntilNextFrame =
        (bPALMode ? PAL_FRAME_COUNTER_FREQ : NTSC_FRAME_COUNTER_FREQ)*0x10000;
    nTicksUntilNextPlay = nTicksPerPlay;
    nTotalPlays = 0;
    
    /*  Clear mixing vals   */
    mWave_Squares.nMixL = 0;
    mWave_TND.nMixL = 0;
    mWave_VRC6Pulse[0].nMixL = 0;
    mWave_VRC6Pulse[1].nMixL = 0;
    mWave_VRC6Saw.nMixL = 0;

    /*  Reset Tri/Noise/DMC */
    mWave_TND.nTriStep = mWave_TND.nTriOutput = 0;
    mWave_TND.nDMCOutput = 0;
    mWave_TND.bNoiseRandomOut = 0;
    mWave_Squares.nDutyCount[0] = mWave_Squares.nDutyCount[1] = 0;
    mWave_TND.bDMCActive = 0;
    mWave_TND.nDMCBytesRemaining = 0;
    mWave_TND.bDMCSampleBufferEmpty = 1;
    mWave_TND.bDMCDeltaSilent = 1;

    /*  Reset VRC6  */
    mWave_VRC6Pulse[0].nVolume = 0;
    mWave_VRC6Pulse[1].nVolume = 0;
    mWave_VRC6Saw.nAccumRate = 0;

    /*  Reset N106  */
    ZEROMEMORY(mWave_N106.nRAM,0x100);
    ZEROMEMORY(mWave_N106.nVolume,8);
    ZEROMEMORY(mWave_N106.nOutput,8);
    ZEROMEMORY(mWave_N106.nMixL,32);

    /*  Reset FME-07    */
    mWave_FME07[0].nVolume = 0;
    mWave_FME07[1].nVolume = 0;
    mWave_FME07[2].nVolume = 0;

    /*  Clear FDS crap      */

    mWave_FDS.bEnvelopeEnable = 0;
    mWave_FDS.nEnvelopeSpeed = 0xFF;
    mWave_FDS.nVolEnv_Mode = 2;
    mWave_FDS.nVolEnv_Decay = 0;
    mWave_FDS.nVolEnv_Gain = 0;
    mWave_FDS.nVolume = 0;
    mWave_FDS.bVolEnv_On = 0;
    mWave_FDS.nSweep_Mode = 2;
    mWave_FDS.nSweep_Decay = 0;
    mWave_FDS.nSweep_Gain = 0;
    mWave_FDS.bSweepEnv_On = 0;
    mWave_FDS.nSweepBias = 0;
    mWave_FDS.bLFO_Enabled = 0;
    mWave_FDS.nLFO_Freq.W = 0;
/*    mWave_FDS.fLFO_Timer = 0;
    mWave_FDS.fLFO_Count = 0;*/
    mWave_FDS.nLFO_Timer = 0;
    mWave_FDS.nLFO_Count = 0;
    mWave_FDS.nLFO_Addr = 0;
    mWave_FDS.bLFO_On = 0;
    mWave_FDS.nMainVolume = 0;
    mWave_FDS.bEnabled = 0;
    mWave_FDS.nFreq.W = 0;
/*    mWave_FDS.fFreqCount = 0;*/
    mWave_FDS.nFreqCount = 0;
    mWave_FDS.nMainAddr = 0;
    mWave_FDS.bWaveWrite = 0;
    mWave_FDS.bMain_On = 0;
    mWave_FDS.nMixL = 0;
    ZEROMEMORY(mWave_FDS.nWaveTable,0x40);
    ZEROMEMORY(mWave_FDS.nLFO_Table,0x40);

    mWave_FDS.nSweep_Count = mWave_FDS.nSweep_Timer =
        ((mWave_FDS.nSweep_Decay + 1) * mWave_FDS.nEnvelopeSpeed * 8);
    mWave_FDS.nVolEnv_Count = mWave_FDS.nVolEnv_Timer =
        ((mWave_FDS.nVolEnv_Decay + 1) * mWave_FDS.nEnvelopeSpeed * 8);

    nSilentSamples = 0;

    nFilterAccL = 0;

    nSilentSamples = 0;

    fulltick=0;
}

/*
 *  SetPlaybackOptions
 */

int SetPlaybackOptions(int32_t samplerate)
{
    if(samplerate < 2000)                   return 0;
    if(samplerate > 96000)                  return 0;

    nTicksPerSample =
        (bPALMode ? PAL_FREQUENCY : NTSC_FREQUENCY) / samplerate * 0x10000;
    nTicksUntilNextSample = nTicksPerSample;

    RecalcFilter();
    RecalcSilenceTracker();

    return 1;
}

/*
 *  SetPlaybackSpeed
 */

void SetPlaybackSpeed(float playspersec)
{
    if(playspersec < 1)
    {
        playspersec = fNSFPlaybackSpeed;
    }

    nTicksPerPlay = nTicksUntilNextPlay =
        (bPALMode ? PAL_FREQUENCY : NTSC_FREQUENCY) / playspersec * 0x10000;
}

/*
*   GetPlaybackSpeed
*/

float GetPlaybackSpeed()
{
    if(nTicksPerPlay <= 0)  return 0;
    return ((bPALMode ? PAL_FREQUENCY : NTSC_FREQUENCY) / (nTicksPerPlay>>16));
}

/*
 *  RecalcFilter
 */

void RecalcFilter()
{
    if(!nSampleRate) return;

    nHighPass = ((int64_t)nHighPassBase << 16) / nSampleRate;

    if(nHighPass > (1<<16)) nHighPass = 1<<16;
}

/*
 *  RecalcSilenceTracker
 */

void RecalcSilenceTracker()
{
    if(nSilenceTrackMS <= 0 || !nSampleRate ||
       (bNoSilenceIfTime && bTimeNotDefault))
    {
        nSilentSampleMax = 0;
        return;
    }

    nSilentSampleMax = nSilenceTrackMS * nSampleRate / 500;
    nSilentSampleMax /= 2;
}

void RebuildOutputTables(void) {
    int32_t i,j;
    float l[3];
    int32_t temp;
    float ftemp;
    
    /* tnd */
    for(i = 0; i < 3; i++)
    {
        l[i] = 255;
    }

    for(i = 0; i < 0x8000; i++)
    {
        ftemp = (l[0] * (i >> 11)) / 2097885;
        ftemp += (l[1] * ((i >> 7) & 0x0F)) / 3121455;
        ftemp += (l[2] * (i & 0x7F)) / 5772690;

        if(!ftemp)
            main_nOutputTable_L[i] = 0;
        else
            main_nOutputTable_L[i] =
                (int16_t)(2396850 / ((1.0f / ftemp) + 100));
    }
    
    /* squares */
    for(i = 0; i < 2; i++)
    {
        l[i] = 255;
    }

    for(j = 0; j < 0x10; j++)
    {
        for(i = 0; i < 0x10; i++)
        {
            temp = (int32_t)(l[0] * j);
            temp += (int32_t)(l[1] * i);

            if(!temp)
                Squares_nOutputTable_L[j][i] = 0;
            else
                Squares_nOutputTable_L[j][i] = 1438200 / ((2072640 / temp) + 100);
        }
    }

    /* VRC6 Pulse 1,2 */
    for(i = 0; i < 0x10; i++)
    {
        VRC6Pulse_nOutputTable_L[i] =
            1875 * i / 0x0F;
    }
    /* VRC6 Saw */
    for(i = 0; i < 0x20; i++)
    {
        VRC6Saw_nOutputTable_L[i] = 3750 * i / 0x1F;
    }

    /* N106 channels */
    /* this amplitude is just a guess */

    for(i = 0; i < 0x10; i++)
    {
        for(j = 0; j < 0x10; j++)
        {
            N106_nOutputTable_L[i][j] = (3000 * i * j) / 0xE1;
        }
    }
    
    /* FME-07 Square A,B,C */
    FME07_nOutputTable_L[15] = 3000;
    FME07_nOutputTable_L[0] = 0;
    for(i = 14; i > 0; i--)
    {
        FME07_nOutputTable_L[i] = FME07_nOutputTable_L[i + 1] * 80 / 100;
    }

    /*
     *  FDS
     */
    /*  this base volume (4000) is just a guess to what sounds right.
     *  Given the number of steps available in an FDS wave... it seems like
     *  it should be much much more... but then it's TOO loud.
     */
    for(i = 0; i < 0x21; i++)
    {
        for(j = 0; j < 0x40; j++)
        {
            FDS_nOutputTable_L[0][i][j] =
                (4000 * i * j * 30) / (0x21 * 0x40 * 30);
            FDS_nOutputTable_L[1][i][j] =
                (4000 * i * j * 20) / (0x21 * 0x40 * 30);
            FDS_nOutputTable_L[2][i][j] =
                (4000 * i * j * 15) / (0x21 * 0x40 * 30);
            FDS_nOutputTable_L[3][i][j] =
                (4000 * i * j * 12) / (0x21 * 0x40 * 30);
        }
    }
}

/*
 *  GetPlayCalls
 */

float GetPlayCalls()
{
    if(!nTicksPerPlay)  return 0;

    return ((float)nTotalPlays) +
        (1.0f - (nTicksUntilNextPlay*1.0f / nTicksPerPlay));
}

/*
 *  GetWrittenTime
 */
uint32_t GetWrittenTime(float basedplayspersec /* = 0 */)
{
    if(basedplayspersec <= 0)
        basedplayspersec = GetPlaybackSpeed();

    if(basedplayspersec <= 0)
        return 0;

    return (uint32_t)((GetPlayCalls() * 1000) / basedplayspersec);
}

/*
 *  StopFade
 */
void StopFade()
{
    bFade = 0;
    fFadeVolume = 1;
}

/*
 *  SongCompleted
 */

uint8_t SongCompleted()
{
    if(!bFade)                      return 0;
    if(nTotalPlays >= nEndFade)     return 1;
    if(nSilentSampleMax)            return (nSilentSamples >= nSilentSampleMax);

    return 0;
}

/*
 *  SetFade
 */

void SetFade(int32_t fadestart,int32_t fadestop,
             uint8_t bNotDefault) /* play routine calls */
{
    if(fadestart < 0)   fadestart = 0;
    if(fadestop < fadestart) fadestop = fadestart;

    nStartFade = (uint32_t)fadestart;
    nEndFade = (uint32_t)fadestop;
    bFade = 1;
    bTimeNotDefault = bNotDefault;

    RecalcSilenceTracker();
    RecalculateFade();
}

/*
 *  SetFadeTime
 */

void SetFadeTime(uint32_t fadestart,uint32_t fadestop,float basedplays,
                 uint8_t bNotDefault) /* time in MS */
{
    if(basedplays <= 0)
        basedplays = GetPlaybackSpeed();
    if(basedplays <= 0)
        return;

    SetFade((int32_t)(fadestart * basedplays / 1000),
           (int32_t)(fadestop * basedplays / 1000),bNotDefault);
}

/*
 *  RecalculateFade
 */

void RecalculateFade()
{
    if(!bFade)  return;

    /* make it hit silence a little before the song ends...
       otherwise we're not really fading OUT, we're just fading umm...
       quieter =P */
    int32_t temp = (int32_t)(GetPlaybackSpeed() / 4);

    if(nEndFade <= nStartFade)
    {
        nEndFade = nStartFade;
        fFadeChange = 1.0f;
    }
    else if((nEndFade - temp) <= nStartFade)
        fFadeChange = 1.0f;
    else
        fFadeChange = 1.0f / (nEndFade - nStartFade - temp);

    if(nTotalPlays < nStartFade)
        fFadeVolume = 1.0f;
    else if(nTotalPlays >= nEndFade)
        fFadeVolume = 0.0f;
    else
    {
        fFadeVolume = 1.0f - ( (nTotalPlays - nStartFade + 1) * fFadeChange );
        if(fFadeVolume < 0)
            fFadeVolume = 0;
    }

}

int32_t GetSamples(uint8_t* buffer,int32_t buffersize)
{
    if(!buffer)                             return 0;
    if(buffersize < 16)                     return 0;
    if(bFade && (nTotalPlays >= nEndFade))  return 0;
    
    pOutput = buffer;
    uint32_t runtocycle =
        (uint32_t)((buffersize / 2) * nTicksPerSample / 0x10000);
    nCPUCycle = nAPUCycle = 0;
    uint32_t tick;

    while(1)
    {
        /*tick = (uint32_t)ceil(fTicksUntilNextPlay);*/
        tick = (nTicksUntilNextPlay+0xffff)>>16;
        if((tick + nCPUCycle) > runtocycle)
            tick = runtocycle - nCPUCycle;

        if(bCPUJammed)
        {
            nCPUCycle += tick;
            EmulateAPU(0);
        }
        else
        {
            tick = Emulate6502(tick + nCPUCycle);
            EmulateAPU(1);
        }

        nTicksUntilNextPlay -= tick<<16;
        if(nTicksUntilNextPlay <= 0)
        {
            nTicksUntilNextPlay += nTicksPerPlay;
            if((bCPUJammed == 2) || bNoWaitForReturn)
            {
                regX = regY = regA = (bCleanAXY ? 0 : 0xCD);
                regPC = 0x5004;
                nTotalPlays++;
                bDMCPop_SamePlay = 0;
                bCPUJammed = 0;
                if(nForce4017Write == 1)    WriteMemory_pAPU(0x4017,0x00);
                if(nForce4017Write == 2)    WriteMemory_pAPU(0x4017,0x80);
            }
            
            if(bFade && (nTotalPlays >= nStartFade))
            {
                fFadeVolume -= fFadeChange;
                if(fFadeVolume < 0)
                    fFadeVolume = 0;
                if(nTotalPlays >= nEndFade)
                    break;
            }
        }

        if(nCPUCycle >= runtocycle)
            break;
    }

    nCPUCycle = nAPUCycle = 0;

    if(nSilentSampleMax && bFade)
    {
        int16_t* tempbuf = (int16_t*)buffer;
        while( ((uint8_t*)tempbuf) < pOutput)
        {
            if( (*tempbuf < -SILENCE_THRESHOLD) ||
                (*tempbuf > SILENCE_THRESHOLD) )
                nSilentSamples = 0;
            else
            {
                if(++nSilentSamples >= nSilentSampleMax)
                    return (int32_t)( ((uint8_t*)tempbuf) - buffer);
            }
            tempbuf++;
        }
    }

    return (int32_t)(pOutput - buffer);
}

/****************** 6502 emulation ******************/

/*  Memory reading/writing and other defines */

/* reads zero page memory */
#define     Zp(a)           pRAM[a]
/* reads zero page memory in word form */
#define     ZpWord(a)       (Zp(a) | (Zp((uint8_t)(a + 1)) << 8))
/* reads memory */
#define     Rd(a)           ((ReadMemory[((uint16_t)(a)) >> 12])(a))
/* reads memory in word form */
#define     RdWord(a)       (Rd(a) | (Rd(a + 1) << 8))
/* writes memory */
#define     Wr(a,v)         (WriteMemory[((uint16_t)(a)) >> 12])(a,v)
/* writes zero paged memory */
#define     WrZ(a,v)        pRAM[a] = v
/* pushes a value onto the stack */
#define     PUSH(v)         pStack[SP--] = v
/* pulls a value from the stack */
#define     PULL(v)         v = pStack[++SP]

/*  Addressing Modes */

/* first set - gets the value that's being addressed */
/*Immediate*/
#define Ad_VlIm()   val = Rd(PC.W); PC.W++
/*Zero Page*/
#define Ad_VlZp()   final.W = Rd(PC.W); val = Zp(final.W); PC.W++
/*Zero Page, X*/
#define Ad_VlZx()   front.W = final.W = Rd(PC.W); final.B.l += X;           \
                    val = Zp(final.B.l); PC.W++
/*Zero Page, Y*/
#define Ad_VlZy()   front.W = final.W = Rd(PC.W); final.B.l += Y;           \
                    val = Zp(final.B.l); PC.W++
/*Absolute*/
#define Ad_VlAb()   final.W = RdWord(PC.W); val = Rd(final.W); PC.W += 2
/*Absolute, X [uses extra cycle if crossed page]*/
#define Ad_VlAx()   front.W = final.W = RdWord(PC.W); final.W += X; PC.W += 2;\
                    if(front.B.h != final.B.h) nCPUCycle++; val = Rd(final.W)
/*Absolute, X [uses extra cycle if crossed page]*/
#define Ad_VlAy()   front.W = final.W = RdWord(PC.W); final.W += Y; PC.W += 2;\
                    if(front.B.h != final.B.h) nCPUCycle++; val = Rd(final.W)
/*(Indirect, X)*/
#define Ad_VlIx()   front.W = final.W = Rd(PC.W); final.B.l += X; PC.W++;   \
                    final.W = ZpWord(final.B.l); val = Rd(final.W)
/*(Indirect), Y [uses extra cycle if crossed page]*/
#define Ad_VlIy()   val = Rd(PC.W); front.W = final.W = ZpWord(val); PC.W++;\
                    final.W += Y; if(final.B.h != front.B.h) nCPUCycle++;    \
                    front.W = val; val = Rd(final.W)

/* second set - gets the ADDRESS that the mode is referring to (for operators
 *              that write to memory) note that AbsoluteX, AbsoluteY, and
 *              IndirectY modes do NOT check for page boundary crossing here
 *              since that extra cycle isn't added for operators that write to
 *              memory (it only applies to ones that only read from memory.. in
 *              which case the 1st set should be used)
 */
/*Zero Page*/
#define Ad_AdZp()   final.W = Rd(PC.W); PC.W++
/*Zero Page, X*/
#define Ad_AdZx()   final.W = front.W = Rd(PC.W); final.B.l += X; PC.W++
/*Zero Page, Y*/
#define Ad_AdZy()   final.W = front.W = Rd(PC.W); final.B.l += Y; PC.W++
/*Absolute*/
#define Ad_AdAb()   final.W = RdWord(PC.W); PC.W += 2
/*Absolute, X*/
#define Ad_AdAx()   front.W = final.W = RdWord(PC.W); PC.W += 2;            \
                    final.W += X
/*Absolute, Y*/
#define Ad_AdAy()   front.W = final.W = RdWord(PC.W); PC.W += 2;            \
                    final.W += Y
/*(Indirect, X)*/
#define Ad_AdIx()   front.W = final.W = Rd(PC.W); PC.W++; final.B.l += X;   \
                    final.W = ZpWord(final.B.l)
/*(Indirect), Y*/
#define Ad_AdIy()   front.W = Rd(PC.W); final.W = ZpWord(front.W) + Y;      \
                    PC.W++

/* third set - reads memory, performs the desired operation on the value, then
 * writes back to memory
 *       used for operators that directly change memory (ASL, INC, DEC, etc)
 */
/*Zero Page*/
#define MRW_Zp(cmd) Ad_AdZp(); val = Zp(final.W); cmd(val); WrZ(final.W,val)
/*Zero Page, X*/
#define MRW_Zx(cmd) Ad_AdZx(); val = Zp(final.W); cmd(val); WrZ(final.W,val)
/*Zero Page, Y*/
#define MRW_Zy(cmd) Ad_AdZy(); val = Zp(final.W); cmd(val); WrZ(final.W,val)
/*Absolute*/
#define MRW_Ab(cmd) Ad_AdAb(); val = Rd(final.W); cmd(val); Wr(final.W,val)
/*Absolute, X*/
#define MRW_Ax(cmd) Ad_AdAx(); val = Rd(final.W); cmd(val); Wr(final.W,val)
/*Absolute, Y*/
#define MRW_Ay(cmd) Ad_AdAy(); val = Rd(final.W); cmd(val); Wr(final.W,val)
/*(Indirect, X)*/
#define MRW_Ix(cmd) Ad_AdIx(); val = Rd(final.W); cmd(val); Wr(final.W,val)
/*(Indirect), Y*/
#define MRW_Iy(cmd) Ad_AdIy(); val = Rd(final.W); cmd(val); Wr(final.W,val)

/* Relative modes are special in that they're only used by branch commands
 *  this macro handles the jump, and should only be called if the branch
 *  condition was true if the branch condition was false, the PC must be
 *  incremented
 */

#define RelJmp(cond)    val = Rd(PC.W); PC.W++; final.W = PC.W + (int8_t)(val);\
                        if(cond) {\
                        nCPUCycle += ((final.B.h != PC.B.h) ? 2 : 1);\
                        PC.W = final.W; }

/* Status Flags */

#define     C_FLAG      0x01    /* carry flag */
#define     Z_FLAG      0x02    /* zero flag */
#define     I_FLAG      0x04    /* mask interrupt flag */
#define     D_FLAG      0x08    /* decimal flag (decimal mode is unsupported on
                                   NES) */
#define     B_FLAG      0x10    /* break flag (not really in the status register
                                   It's value in ST is never used.  When ST is
                                   put in memory (by an interrupt or PHP), this
                                   flag is set only if BRK was called)
                                   ** also when PHP is called due to a bug */
#define     R_FLAG      0x20    /* reserved flag (not really in the register.
                                   It's value is never used.
                                   Whenever ST is put in memory,
                                   this flag is always set) */
#define     V_FLAG      0x40    /* overflow flag */
#define     N_FLAG      0x80    /* sign flag */


/*  Lookup Tables */

/* the number of CPU cycles used for each instruction */
static const uint8_t CPU_Cycles[0x100] = {
7,6,0,8,3,3,5,5,3,2,2,2,4,4,6,6,
2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
6,6,0,8,3,3,5,5,4,2,2,2,4,4,6,6,
2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
6,6,0,8,3,3,5,5,3,2,2,2,3,4,6,6,
2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
6,6,0,8,3,3,5,5,4,2,2,2,5,4,6,6,
2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
2,6,0,6,4,4,4,4,2,5,2,5,5,5,5,5,
2,6,2,6,3,3,3,3,2,2,2,2,4,4,4,4,
2,5,0,5,4,4,4,4,2,4,2,4,4,4,4,4,
2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,
2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7,
2,6,2,8,3,3,5,5,2,2,2,2,4,4,6,6,
2,5,0,8,4,4,6,6,2,4,2,7,4,4,7,7     };

/* the status of the NZ flags for the given value */
static const uint8_t NZTable[0x100] = {
Z_FLAG,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,
N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,
N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG,N_FLAG };

/* A quick macro for working with the above table */
#define UpdateNZ(v) ST = (ST & ~(N_FLAG|Z_FLAG)) | NZTable[v]


/*
 *  Opcodes
 *
 *      These opcodes perform the action with the given value (changing that
 *  value if necessary).  Registers and flags associated with the operation
 *  are changed accordingly.  There are a few exceptions which will be noted
 *  when they arise
 */


/*  ADC
        Adds the value to the accumulator with carry
        Changes:  A, NVZC
        - Decimal mode not supported on the NES
        - Due to a bug, NVZ flags are not altered if the Decimal flag is on
          --(taken out)-- */
#define ADC()                                                           \
    tw.W = A + val + (ST & C_FLAG);                                     \
    ST = (ST & (I_FLAG|D_FLAG)) | tw.B.h | NZTable[tw.B.l] |            \
        ( (0x80 & ~(A ^ val) & (A ^ tw.B.l)) ? V_FLAG : 0 );            \
    A = tw.B.l

/*  AND
        Combines the value with the accumulator using a bitwise AND operation
        Changes:  A, NZ     */
#define AND()                                                           \
    A &= val;                                                           \
    UpdateNZ(A)

/*  ASL
        Left shifts the value 1 bit.  The bit that gets shifted out goes to
        the carry flag.
        Changes:  value, NZC        */
#define ASL(value)                                                      \
    tw.W = value << 1;                                                  \
    ST = (ST & ~(N_FLAG|Z_FLAG|C_FLAG)) | tw.B.h | NZTable[tw.B.l];     \
    value = tw.B.l

/*  BIT
        Compares memory with the accumulator with an AND operation, but changes
        neither.
        The two high bits of memory get transferred to the status reg
        Z is set if the AND operation yielded zero, otherwise it's cleared
        Changes:  NVZ               */
#define BIT()                                                           \
    ST = (ST & ~(N_FLAG|V_FLAG|Z_FLAG)) | (val & (N_FLAG|V_FLAG)) |     \
            ((A & val) ? 0 : Z_FLAG)

/*  CMP, CPX, CPY
        Compares memory with the given register with a subtraction operation.
        Flags are set accordingly depending on the result:
        Reg < Memory:  Z=0, C=0
        Reg = Memory:  Z=1, C=1
        Reg > Memory:  Z=0, C=1
        N is set according to the result of the subtraction operation
        Changes:  NZC

        NOTE -- CMP, CPX, CPY all share this same routine, so the desired
                register (A, X, or Y respectively) must be given when calling
                this macro... as well as the memory to compare it with. */
#define CMP(reg)                                                        \
    tw.W = reg - val;                                                   \
    ST = (ST & ~(N_FLAG|Z_FLAG|C_FLAG)) | (tw.B.h ? 0 : C_FLAG) |       \
            NZTable[tw.B.l]

/*  DEC, DEX, DEY
        Decriments a value by one.
        Changes:  value, NZ             */
#define DEC(value)                                                      \
    value--;                                                            \
    UpdateNZ(value)

/*  EOR
        Combines a value with the accumulator using a bitwise exclusive-OR
        operation
        Changes:  A, NZ                 */
#define EOR()                                                           \
    A ^= val;                                                           \
    UpdateNZ(A)

/*  INC, INX, INY
        Incriments a value by one.
        Changes:  value, NZ             */
#define INC(value)                                                      \
    value++;                                                            \
    UpdateNZ(value)

/*  LSR
        Shifts value one bit to the right.  Bit that gets shifted out goes to
        the Carry flag.
        Changes:  value, NZC            */
#define LSR(value)                                                      \
    tw.W = value >> 1;                                                  \
    ST = (ST & ~(N_FLAG|Z_FLAG|C_FLAG)) | NZTable[tw.B.l] |             \
        (value & 0x01);                                                 \
    value = tw.B.l

/*  ORA
        Combines a value with the accumulator using a bitwise inclusive-OR
        operation
        Changes:  A, NZ                 */
#define ORA()                                                           \
    A |= val;                                                           \
    UpdateNZ(A)

/*  ROL
        Rotates a value one bit to the left:
        C <-   7<-6<-5<-4<-3<-2<-1<-0    <- C
        Changes:  value, NZC            */
#define ROL(value)                                                      \
    tw.W = (value << 1) | (ST & 0x01);                                  \
    ST = (ST & ~(N_FLAG|Z_FLAG|C_FLAG)) | NZTable[tw.B.l] | tw.B.h;     \
    value = tw.B.l

/*  ROR
        Rotates a value one bit to the right:
        C ->   7->6->5->4->3->2->1->0   -> C
        Changes:  value, NZC            */
#define ROR(value)                                                      \
    tw.W = (value >> 1) | (ST << 7);                                    \
    ST = (ST & ~(N_FLAG|Z_FLAG|C_FLAG)) | NZTable[tw.B.l] |             \
        (value & 0x01);                                                 \
    value = tw.B.l

/*  SBC
        Subtracts a value from the accumulator with borrow (inverted carry)
        Changes:  A, NVZC
        - Decimal mode not supported on the NES
        - Due to a bug, NVZ flags are not altered if the Decimal flag is on
           --(taken out)-- */
#define SBC()                                                               \
    tw.W = A - val - ((ST & C_FLAG) ? 0 : 1);                               \
    ST = (ST & (I_FLAG|D_FLAG)) | (tw.B.h ? 0 : C_FLAG) | NZTable[tw.B.l] | \
                    (((A ^ val) & (A ^ tw.B.l) & 0x80) ? V_FLAG : 0);       \
    A = tw.B.l

/*  Undocumented Opcodes
 *
 *      These opcodes are not included in the official specifications.  However,
 *  some of the unused opcode values perform operations which have since been
 *  documented.
 */


/*  ASO
        Left shifts a value, then ORs the result with the accumulator
        Changes:  value, A, NZC                                         */
#define ASO(value)                                                      \
    tw.W = value << 1;                                                  \
    A |= tw.B.l;                                                        \
    ST = (ST & ~(N_FLAG|Z_FLAG|C_FLAG)) | NZTable[A] | tw.B.h;          \
    value = tw.B.l

/*  RLA
        Roll memory left 1 bit, then AND the result with the accumulator
        Changes:  value, A, NZC                                         */
#define RLA(value)                                                      \
    tw.W = (value << 1) | (ST & 0x01);                                  \
    A &= tw.B.l;                                                        \
    ST = (ST & ~(N_FLAG|Z_FLAG|C_FLAG)) | NZTable[A] | tw.B.h;          \
    value = tw.B.l

/*  LSE
        Right shifts a value one bit, then EORs the result with the accumulator
        Changes:  value, A, NZC                                         */
#define LSE(value)                                                      \
    tw.W = value >> 1;                                                  \
    A ^= tw.B.l;                                                        \
    ST = (ST & ~(N_FLAG|Z_FLAG|C_FLAG)) | NZTable[A] | (value & 0x01);  \
    value = tw.B.l

/*  RRA
        Roll memory right one bit, then ADC the result
        Changes:  value, A, NVZC                                        */
#define RRA(value)                                                      \
    tw.W = (value >> 1) | (ST << 7);                                    \
    ST = (ST & ~C_FLAG) | (value & 0x01);                               \
    value = tw.B.l;                                                     \
    ADC()

/*  AXS
        ANDs the contents of the X and A registers and stores the result
        int memory.
        Changes:  value  [DOES NOT CHANGE X, A, or any flags]           */
#define AXS(value)                                                      \
    value = A & X

/*  DCM
        Decriments a value and compares it with the A register.
        Changes:  value, NZC                                            */
#define DCM(value)                                                          \
    value--;                                                                \
    CMP(A)

/*  INS
        Incriments a value then SBCs it
        Changes:  value, A, NVZC                                        */
#define INS(value)                                                      \
    value++;                                                            \
    SBC()

/*  AXA     */
#define AXA(value)                                                      \
    value = A & X & (Rd(PC.W - 1) + 1)


/* The 6502 emulation function! */

union TWIN front;
union TWIN final;
uint8_t val;
uint8_t op;

uint32_t Emulate6502(uint32_t runto)
{
    /* If the CPU is jammed... don't bother */
    if(bCPUJammed == 1)
        return 0;

    register union TWIN tw;     /* used in calculations */
    register uint8_t    ST = regP;
    register union TWIN PC;
    uint8_t         SP = regSP;
    register uint8_t    A = regA;
    register uint8_t    X = regX;
    register uint8_t    Y = regY;
    union TWIN          front;
    union TWIN          final;
    PC.W = regPC;

    uint32_t ret = nCPUCycle;
    
    ENTER_TIMER(cpu);
    
    /*  Start the loop */
    
    while(nCPUCycle < runto)
    {
        op = Rd(PC.W);
        PC.W++;

        nCPUCycle += CPU_Cycles[op];
        switch(op)
        {
            /* Documented Opcodes first */
            
        /*  Flag setting/clearing */
        case 0x18:  ST &= ~C_FLAG;  break;      /* CLC  */
        case 0x38:  ST |=  C_FLAG;  break;      /* SEC  */
        case 0x58:  ST &= ~I_FLAG;  break;      /* CLI  */
        case 0x78:  ST |=  I_FLAG;  break;      /* SEI  */
        case 0xB8:  ST &= ~V_FLAG;  break;      /* CLV  */
        case 0xD8:  ST &= ~D_FLAG;  break;      /* CLD  */
        case 0xF8:  ST |=  D_FLAG;  break;      /* SED  */

        /* Branch commands */
        case 0x10:  RelJmp(!(ST & N_FLAG)); break;  /* BPL  */
        case 0x30:  RelJmp( (ST & N_FLAG)); break;  /* BMI  */
        case 0x50:  RelJmp(!(ST & V_FLAG)); break;  /* BVC  */
        case 0x70:  RelJmp( (ST & V_FLAG)); break;  /* BVS  */
        case 0x90:  RelJmp(!(ST & C_FLAG)); break;  /* BCC  */
        case 0xB0:  RelJmp( (ST & C_FLAG)); break;  /* BCS  */
        case 0xD0:  RelJmp(!(ST & Z_FLAG)); break;  /* BNE  */
        case 0xF0:  RelJmp( (ST & Z_FLAG)); break;  /* BEQ  */

        /* Direct stack alteration commands (push/pull commands) */
        case 0x08:  PUSH(ST | R_FLAG | B_FLAG); break;  /* PHP  */
        case 0x28:  PULL(ST);                   break;  /* PLP  */
        case 0x48:  PUSH(A);                    break;  /* PHA  */
        case 0x68:  PULL(A); UpdateNZ(A);       break;  /* PLA  */

        /* Register Transfers */
        case 0x8A:  A = X;  UpdateNZ(A);    break;  /* TXA  */
        case 0x98:  A = Y;  UpdateNZ(A);    break;  /* TYA  */
        case 0x9A:  SP = X;                 break;  /* TXS  */
        case 0xA8:  Y = A;  UpdateNZ(A);    break;  /* TAY  */
        case 0xAA:  X = A;  UpdateNZ(A);    break;  /* TAX  */
        case 0xBA:  X = SP; UpdateNZ(X);    break;  /* TSX  */

        /*  Other commands */

        /* ADC  */
        case 0x61:  Ad_VlIx();  ADC();  break;
        case 0x65:  Ad_VlZp();  ADC();  break;
        case 0x69:  Ad_VlIm();  ADC();  break;
        case 0x6D:  Ad_VlAb();  ADC();  break;
        case 0x71:  Ad_VlIy();  ADC();  break;
        case 0x75:  Ad_VlZx();  ADC();  break;
        case 0x79:  Ad_VlAy();  ADC();  break;
        case 0x7D:  Ad_VlAx();  ADC();  break;

        /* AND  */
        case 0x21:  Ad_VlIx();  AND();  break;
        case 0x25:  Ad_VlZp();  AND();  break;
        case 0x29:  Ad_VlIm();  AND();  break;
        case 0x2D:  Ad_VlAb();  AND();  break;
        case 0x31:  Ad_VlIy();  AND();  break;
        case 0x35:  Ad_VlZx();  AND();  break;
        case 0x39:  Ad_VlAy();  AND();  break;
        case 0x3D:  Ad_VlAx();  AND();  break;

        /* ASL  */
        case 0x0A:  ASL(A);             break;
        case 0x06:  MRW_Zp(ASL);        break;
        case 0x0E:  MRW_Ab(ASL);        break;
        case 0x16:  MRW_Zx(ASL);        break;
        case 0x1E:  MRW_Ax(ASL);        break;

        /* BIT  */
        case 0x24:  Ad_VlZp();  BIT();  break;
        case 0x2C:  Ad_VlAb();  BIT();  break;

        /* BRK  */
        case 0x00:
            if(bIgnoreBRK)
                break;
            PC.W++;                     /*BRK has a padding byte*/
            PUSH(PC.B.h);               /*push high byte of the return address*/
            PUSH(PC.B.l);               /*push low byte of return address*/
            PUSH(ST | R_FLAG | B_FLAG); /*push processor status with R|B flags*/
            ST |= I_FLAG;               /*mask interrupts*/
            PC.W = RdWord(0xFFFE);      /*read the IRQ vector and jump to it*/

            /* extra check to make sure we didn't hit an infinite BRK loop */
            if(!Rd(PC.W))                   /* next command will be BRK */
            {
                /* the CPU will endlessly loop...
                   just jam it to ease processing power */
                bCPUJammed = 1;
                goto jammed;
            }
            break;

        /* CMP  */
        case 0xC1:  Ad_VlIx();  CMP(A); break;
        case 0xC5:  Ad_VlZp();  CMP(A); break;
        case 0xC9:  Ad_VlIm();  CMP(A); break;
        case 0xCD:  Ad_VlAb();  CMP(A); break;
        case 0xD1:  Ad_VlIy();  CMP(A); break;
        case 0xD5:  Ad_VlZx();  CMP(A); break;
        case 0xD9:  Ad_VlAy();  CMP(A); break;
        case 0xDD:  Ad_VlAx();  CMP(A); break;

        /* CPX  */
        case 0xE0:  Ad_VlIm();  CMP(X); break;
        case 0xE4:  Ad_VlZp();  CMP(X); break;
        case 0xEC:  Ad_VlAb();  CMP(X); break;

        /* CPY  */
        case 0xC0:  Ad_VlIm();  CMP(Y); break;
        case 0xC4:  Ad_VlZp();  CMP(Y); break;
        case 0xCC:  Ad_VlAb();  CMP(Y); break;

        /* DEC  */
        case 0xCA:  DEC(X);             break;      /* DEX  */
        case 0x88:  DEC(Y);             break;      /* DEY  */
        case 0xC6:  MRW_Zp(DEC);        break;
        case 0xCE:  MRW_Ab(DEC);        break;
        case 0xD6:  MRW_Zx(DEC);        break;
        case 0xDE:  MRW_Ax(DEC);        break;

        /* EOR  */
        case 0x41:  Ad_VlIx();  EOR();  break;
        case 0x45:  Ad_VlZp();  EOR();  break;
        case 0x49:  Ad_VlIm();  EOR();  break;
        case 0x4D:  Ad_VlAb();  EOR();  break;
        case 0x51:  Ad_VlIy();  EOR();  break;
        case 0x55:  Ad_VlZx();  EOR();  break;
        case 0x59:  Ad_VlAy();  EOR();  break;
        case 0x5D:  Ad_VlAx();  EOR();  break;

        /* INC  */
        case 0xE8:  INC(X);             break;      /* INX  */
        case 0xC8:  INC(Y);             break;      /* INY  */
        case 0xE6:  MRW_Zp(INC);        break;
        case 0xEE:  MRW_Ab(INC);        break;
        case 0xF6:  MRW_Zx(INC);        break;
        case 0xFE:  MRW_Ax(INC);        break;

        /* JMP  */
        /* Absolute JMP */
        case 0x4C:  final.W = RdWord(PC.W);  PC.W = final.W; val = 0;   break;
        /* Indirect JMP -- must take caution:
           Indirection at 01FF will read from 01FF and 0100 (not 0200) */
        case 0x6C:  front.W = final.W = RdWord(PC.W);
                    PC.B.l = Rd(final.W); final.B.l++;
                    PC.B.h = Rd(final.W); final.W = PC.W;
                    break;      
        /* JSR  */
        case 0x20:
            val = 0;
            final.W = RdWord(PC.W);
            PC.W++;         /* JSR only increments the return address by one.
                               It's incremented again upon RTS */
            PUSH(PC.B.h);   /* push high byte of return address */
            PUSH(PC.B.l);   /* push low byte of return address */
            PC.W = final.W;
            break;

        /* LDA  */
        case 0xA1:  Ad_VlIx(); A = val; UpdateNZ(A);    break;
        case 0xA5:  Ad_VlZp(); A = val; UpdateNZ(A);    break;
        case 0xA9:  Ad_VlIm(); A = val; UpdateNZ(A);    break;
        case 0xAD:  Ad_VlAb(); A = val; UpdateNZ(A);    break;
        case 0xB1:  Ad_VlIy(); A = val; UpdateNZ(A);    break;
        case 0xB5:  Ad_VlZx(); A = val; UpdateNZ(A);    break;
        case 0xB9:  Ad_VlAy(); A = val; UpdateNZ(A);    break;
        case 0xBD:  Ad_VlAx(); A = val; UpdateNZ(A);    break;

        /* LDX  */
        case 0xA2:  Ad_VlIm(); X = val; UpdateNZ(X);    break;
        case 0xA6:  Ad_VlZp(); X = val; UpdateNZ(X);    break;
        case 0xAE:  Ad_VlAb(); X = val; UpdateNZ(X);    break;
        case 0xB6:  Ad_VlZy(); X = val; UpdateNZ(X);    break;
        case 0xBE:  Ad_VlAy(); X = val; UpdateNZ(X);    break;

        /* LDY  */
        case 0xA0:  Ad_VlIm(); Y = val; UpdateNZ(Y);    break;
        case 0xA4:  Ad_VlZp(); Y = val; UpdateNZ(Y);    break;
        case 0xAC:  Ad_VlAb(); Y = val; UpdateNZ(Y);    break;
        case 0xB4:  Ad_VlZx(); Y = val; UpdateNZ(Y);    break;
        case 0xBC:  Ad_VlAx(); Y = val; UpdateNZ(Y);    break;

        /* LSR  */
        case 0x4A:  LSR(A);             break;
        case 0x46:  MRW_Zp(LSR);        break;
        case 0x4E:  MRW_Ab(LSR);        break;
        case 0x56:  MRW_Zx(LSR);        break;
        case 0x5E:  MRW_Ax(LSR);        break;

        /* NOP  */
        case 0xEA:

        /* --- Undocumented ---
            These opcodes perform the same action as NOP    */
        case 0x1A:  case 0x3A:  case 0x5A:
        case 0x7A:  case 0xDA:  case 0xFA:      break;

        /* ORA  */
        case 0x01:  Ad_VlIx();  ORA();  break;
        case 0x05:  Ad_VlZp();  ORA();  break;
        case 0x09:  Ad_VlIm();  ORA();  break;
        case 0x0D:  Ad_VlAb();  ORA();  break;
        case 0x11:  Ad_VlIy();  ORA();  break;
        case 0x15:  Ad_VlZx();  ORA();  break;
        case 0x19:  Ad_VlAy();  ORA();  break;
        case 0x1D:  Ad_VlAx();  ORA();  break;

        /* ROL  */
        case 0x2A:  ROL(A);             break;
        case 0x26:  MRW_Zp(ROL);        break;
        case 0x2E:  MRW_Ab(ROL);        break;
        case 0x36:  MRW_Zx(ROL);        break;
        case 0x3E:  MRW_Ax(ROL);        break;

        /* ROR  */
        case 0x6A:  ROR(A);             break;
        case 0x66:  MRW_Zp(ROR);        break;
        case 0x6E:  MRW_Ab(ROR);        break;
        case 0x76:  MRW_Zx(ROR);        break;
        case 0x7E:  MRW_Ax(ROR);        break;

        /* RTI  */
        case 0x40:
            PULL(ST);                   /*pull processor status*/
            PULL(PC.B.l);               /*pull low byte of return address*/
            PULL(PC.B.h);               /*pull high byte of return address*/
            break;

        /* RTS  */
        case 0x60:
            PULL(PC.B.l);
            PULL(PC.B.h);
            PC.W++; /* the return address is one less of what it needs */
            break;

        /* SBC  */
        case 0xE1:  Ad_VlIx();  SBC();  break;
        case 0xE5:  Ad_VlZp();  SBC();  break;
        /* - Undocumented -  EB performs the same operation as SBC immediate */
        case 0xEB:
        case 0xE9:  Ad_VlIm();  SBC();  break;
        case 0xED:  Ad_VlAb();  SBC();  break;
        case 0xF1:  Ad_VlIy();  SBC();  break;
        case 0xF5:  Ad_VlZx();  SBC();  break;
        case 0xF9:  Ad_VlAy();  SBC();  break;
        case 0xFD:  Ad_VlAx();  SBC();  break;

        /* STA  */
        case 0x81:  Ad_AdIx(); val = A; Wr(final.W,A);  break;
        case 0x85:  Ad_AdZp(); val = A; WrZ(final.W,A); break;
        case 0x8D:  Ad_AdAb(); val = A; Wr(final.W,A);  break;
        case 0x91:  Ad_AdIy(); val = A; Wr(final.W,A);  break;
        case 0x95:  Ad_AdZx(); val = A; WrZ(final.W,A); break;
        case 0x99:  Ad_AdAy(); val = A; Wr(final.W,A);  break;
        case 0x9D:  Ad_AdAx(); val = A; Wr(final.W,A);  break;

        /* STX  */
        case 0x86:  Ad_AdZp(); val = X; WrZ(final.W,X); break;
        case 0x8E:  Ad_AdAb(); val = X; Wr(final.W,X);  break;
        case 0x96:  Ad_AdZy(); val = X; WrZ(final.W,X); break;

        /* STY  */
        case 0x84:  Ad_AdZp(); val = Y; WrZ(final.W,Y); break;
        case 0x8C:  Ad_AdAb(); val = Y; Wr(final.W,Y);  break;
        case 0x94:  Ad_AdZx(); val = Y; WrZ(final.W,Y); break;

        /*  Undocumented Opcodes */
        /* ASO  */
        case 0x03:  if(bIgnoreIllegalOps) break;    MRW_Ix(ASO);    break;
        case 0x07:  if(bIgnoreIllegalOps) break;    MRW_Zp(ASO);    break;
        case 0x0F:  if(bIgnoreIllegalOps) break;    MRW_Ab(ASO);    break;
        case 0x13:  if(bIgnoreIllegalOps) break;    MRW_Iy(ASO);    break;
        case 0x17:  if(bIgnoreIllegalOps) break;    MRW_Zx(ASO);    break;
        case 0x1B:  if(bIgnoreIllegalOps) break;    MRW_Ay(ASO);    break;
        case 0x1F:  if(bIgnoreIllegalOps) break;    MRW_Ax(ASO);    break;

        /* RLA  */
        case 0x23:  if(bIgnoreIllegalOps) break;    MRW_Ix(RLA);    break;
        case 0x27:  if(bIgnoreIllegalOps) break;    MRW_Zp(RLA);    break;
        case 0x2F:  if(bIgnoreIllegalOps) break;    MRW_Ab(RLA);    break;
        case 0x33:  if(bIgnoreIllegalOps) break;    MRW_Iy(RLA);    break;
        case 0x37:  if(bIgnoreIllegalOps) break;    MRW_Zx(RLA);    break;
        case 0x3B:  if(bIgnoreIllegalOps) break;    MRW_Ay(RLA);    break;
        case 0x3F:  if(bIgnoreIllegalOps) break;    MRW_Ax(RLA);    break;

        /* LSE  */
        case 0x43:  if(bIgnoreIllegalOps) break;    MRW_Ix(LSE);    break;
        case 0x47:  if(bIgnoreIllegalOps) break;    MRW_Zp(LSE);    break;
        case 0x4F:  if(bIgnoreIllegalOps) break;    MRW_Ab(LSE);    break;
        case 0x53:  if(bIgnoreIllegalOps) break;    MRW_Iy(LSE);    break;
        case 0x57:  if(bIgnoreIllegalOps) break;    MRW_Zx(LSE);    break;
        case 0x5B:  if(bIgnoreIllegalOps) break;    MRW_Ay(LSE);    break;
        case 0x5F:  if(bIgnoreIllegalOps) break;    MRW_Ax(LSE);    break;

        /* RRA  */
        case 0x63:  if(bIgnoreIllegalOps) break;    MRW_Ix(RRA);    break;
        case 0x67:  if(bIgnoreIllegalOps) break;    MRW_Zp(RRA);    break;
        case 0x6F:  if(bIgnoreIllegalOps) break;    MRW_Ab(RRA);    break;
        case 0x73:  if(bIgnoreIllegalOps) break;    MRW_Iy(RRA);    break;
        case 0x77:  if(bIgnoreIllegalOps) break;    MRW_Zx(RRA);    break;
        case 0x7B:  if(bIgnoreIllegalOps) break;    MRW_Ay(RRA);    break;
        case 0x7F:  if(bIgnoreIllegalOps) break;    MRW_Ax(RRA);    break;

        /* AXS  */
        case 0x83:  if(bIgnoreIllegalOps) break;    MRW_Ix(AXS);    break;
        case 0x87:  if(bIgnoreIllegalOps) break;    MRW_Zp(AXS);    break;
        case 0x8F:  if(bIgnoreIllegalOps) break;    MRW_Ab(AXS);    break;
        case 0x97:  if(bIgnoreIllegalOps) break;    MRW_Zy(AXS);    break;

        /* LAX  */
        case 0xA3:  if(bIgnoreIllegalOps) break;
            Ad_VlIx();  X = A = val; UpdateNZ(A);   break;
        case 0xA7:  if(bIgnoreIllegalOps) break;
            Ad_VlZp();  X = A = val; UpdateNZ(A);   break;
        case 0xAF:  if(bIgnoreIllegalOps) break;
            Ad_VlAb();  X = A = val; UpdateNZ(A);   break;
        case 0xB3:  if(bIgnoreIllegalOps) break;
            Ad_VlIy();  X = A = val; UpdateNZ(A);   break;
        case 0xB7:  if(bIgnoreIllegalOps) break;
            Ad_VlZy();  X = A = val; UpdateNZ(A);   break;
        case 0xBF:  if(bIgnoreIllegalOps) break;
            Ad_VlAy();  X = A = val; UpdateNZ(A);   break;

        /* DCM  */
        case 0xC3:  if(bIgnoreIllegalOps) break;    MRW_Ix(DCM);    break;
        case 0xC7:  if(bIgnoreIllegalOps) break;    MRW_Zp(DCM);    break;
        case 0xCF:  if(bIgnoreIllegalOps) break;    MRW_Ab(DCM);    break;
        case 0xD3:  if(bIgnoreIllegalOps) break;    MRW_Iy(DCM);    break;
        case 0xD7:  if(bIgnoreIllegalOps) break;    MRW_Zx(DCM);    break;
        case 0xDB:  if(bIgnoreIllegalOps) break;    MRW_Ay(DCM);    break;
        case 0xDF:  if(bIgnoreIllegalOps) break;    MRW_Ax(DCM);    break;

        /* INS  */
        case 0xE3:  if(bIgnoreIllegalOps) break;    MRW_Ix(INS);    break;
        case 0xE7:  if(bIgnoreIllegalOps) break;    MRW_Zp(INS);    break;
        case 0xEF:  if(bIgnoreIllegalOps) break;    MRW_Ab(INS);    break;
        case 0xF3:  if(bIgnoreIllegalOps) break;    MRW_Iy(INS);    break;
        case 0xF7:  if(bIgnoreIllegalOps) break;    MRW_Zx(INS);    break;
        case 0xFB:  if(bIgnoreIllegalOps) break;    MRW_Ay(INS);    break;
        case 0xFF:  if(bIgnoreIllegalOps) break;    MRW_Ax(INS);    break;

        /* ALR
                AND Accumulator with memory and LSR the result  */
        case 0x4B:  if(bIgnoreIllegalOps) break;
                    Ad_VlIm();  A &= val;   LSR(A); break;

        /* ARR
                ANDs memory with the Accumulator and RORs the result    */
        case 0x6B:  if(bIgnoreIllegalOps) break;
                    Ad_VlIm();  A &= val;   ROR(A); break;

        /* XAA
                Transfers X -> A, then ANDs A with memory               */
        case 0x8B:  if(bIgnoreIllegalOps) break;
                    Ad_VlIm();  A = X & val; UpdateNZ(A);   break;

        /* OAL
                OR the Accumulator with #EE, AND Accumulator with Memory,
                Transfer A -> X   */
        case 0xAB:  if(bIgnoreIllegalOps) break;
                    Ad_VlIm();  X = (A &= (val | 0xEE));
                    UpdateNZ(A);    break;

        /* SAX
                ANDs A and X registers (does not change A), subtracts memory
                from result (CMP style, not SBC style) result is stored in X */
        case 0xCB:  if(bIgnoreIllegalOps) break;
                Ad_VlIm();  tw.W = (X & A) - val; X = tw.B.l;
                    ST = (ST & ~(N_FLAG|Z_FLAG|C_FLAG)) | NZTable[X] |
                        (tw.B.h ? C_FLAG : 0);   break;
        /* SKB
                Skip Byte... or DOP - Double No-Op
                These bytes do nothing, but take a parameter (which can be
                ignored) */
        case 0x04:  case 0x14:  case 0x34:  case 0x44:  case 0x54:  case 0x64:
        case 0x80:  case 0x82:  case 0x89:  case 0xC2:  case 0xD4:  case 0xE2:
        case 0xF4:
            if(bIgnoreIllegalOps) break;
            PC.W++;     /* skip unused byte */
            break;

        /* SKW
                Swip Word... or TOP - Tripple No-Op
            These bytes are the same as SKB, only they take a 2 byte parameter.
            This can be ignored in some cases, but the read needs to be
            performed in a some cases because an extra clock cycle may be used
            in the process     */
        /* Absolute address... no need for operator */
        case 0x0C:
            if(bIgnoreIllegalOps) break;
            PC.W += 2;  break;
        /* Absolute X address... may cross page, have to perform the read */
        case 0x1C:  case 0x3C:  case 0x5C:  case 0x7C:  case 0xDC:  case 0xFC:
            if(bIgnoreIllegalOps) break;
            Ad_VlAx(); break;

        /* HLT / JAM
                Jams up CPU operation           */
        case 0x02:  case 0x12:  case 0x22:  case 0x32:  case 0x42:  case 0x52:
        case 0x62:  case 0x72:  case 0x92:  case 0xB2:  case 0xD2:  case 0xF2:
            /*it's not -really- jammed... only the NSF code has ended*/
            if(PC.W == 0x5004)  bCPUJammed = 2;
            else
            {
                if(bIgnoreIllegalOps) break;
                bCPUJammed = 1;
            }
            goto jammed;

        /* TAS  */
        case 0x9B:
            if(bIgnoreIllegalOps) break;
            Ad_AdAy();
            SP = A & X & (Rd(PC.W - 1) + 1);
            Wr(final.W,SP);
            break;

        /* SAY  */
        case 0x9C:
            if(bIgnoreIllegalOps) break;
            Ad_AdAx();
            Y &= (Rd(PC.W - 1) + 1);
            Wr(final.W,Y);
            break;

        /* XAS  */
        case 0x9E:
            if(bIgnoreIllegalOps) break;
            Ad_AdAy();
            X &= (Rd(PC.W - 1) + 1);
            Wr(final.W,X);
            break;

        /* AXA  */
        case 0x93:  if(bIgnoreIllegalOps) break;    MRW_Iy(AXA);    break;
        case 0x9F:  if(bIgnoreIllegalOps) break;    MRW_Ay(AXA);    break;

        /* ANC  */
        case 0x0B:  case 0x2B:
            if(bIgnoreIllegalOps) break;
            Ad_VlIm();
            A &= val;
            ST = (ST & ~(N_FLAG|Z_FLAG|C_FLAG)) |
                NZTable[A] | ((A & 0x80) ? C_FLAG : 0);
            break;

        /* LAS  */
        case 0xBB:
            if(bIgnoreIllegalOps) break;
            Ad_VlAy();
            X = A = (SP &= val);
            UpdateNZ(A);
            break;
        }
    }

jammed:
    regPC = PC.W;
    regA = A;
    regX = X;
    regY = Y;
    regSP = SP;
    regP = ST;
    
    EXIT_TIMER(cpu);
    
    return (nCPUCycle - ret);
}

/****************** rockbox interface ******************/

static void set_codec_track(int t, int d) {
    int track,fade,def=0;
    SetTrack(t);

    /* for REPEAT_ONE we disable track limits */
    if (ci->global_settings->repeat_mode!=REPEAT_ONE) {
        if (!bIsExtended || nTrackTime[t]==-1) {track=60*2*1000; def=1;}
        else track=nTrackTime[t];
        if (!bIsExtended || nTrackFade[t]==-1) fade=5*1000;
        else fade=nTrackFade[t];
        nSilenceTrackMS=5000;
        SetFadeTime(track,track+fade, fNSFPlaybackSpeed,def);
    }
    ci->id3->elapsed=d*1000; /* d is track no to display */
}

/* this is the codec entry point */
enum codec_status codec_main(void)
{
    int written;
    uint8_t *buf;
    size_t n;
    int endofstream; /* end of stream flag */
    int track;
    int dontresettrack;
    char last_path[MAX_PATH];
    int usingplaylist;

    /* we only render 16 bits */
    ci->configure(DSP_SET_SAMPLE_DEPTH, 16);

    ci->configure(DSP_SET_FREQUENCY, 44100);
    ci->configure(DSP_SET_STEREO_MODE, STEREO_MONO);
    
    RebuildOutputTables();

    dontresettrack=0;
    last_path[0]='\0';
    track=0;
    
next_track:
    usingplaylist=0;
    DEBUGF("NSF: next_track\n");
    if (codec_init()) {
        return CODEC_ERROR;
    }
    DEBUGF("NSF: after init\n");
    

    /* wait for track info to load */
    while (!*ci->taginfo_ready && !ci->stop_codec)
        ci->sleep(1);

    codec_set_replaygain(ci->id3);
        
    /* Read the entire file */
    DEBUGF("NSF: request file\n");
    ci->seek_buffer(0);
    buf = ci->request_buffer(&n, ci->filesize);
    if (!buf || n < (size_t)ci->filesize) {
        DEBUGF("NSF: file load failed\n");
        return CODEC_ERROR;
    }
    
init_nsf:    
    if(!NSFCore_Initialize()) {
        DEBUGF("NSF: NSFCore_Initialize failed\n"); return CODEC_ERROR;}

    if(LoadFile(buf,ci->filesize)) {
        DEBUGF("NSF: LoadFile failed\n"); return CODEC_ERROR;}
    if(!SetPlaybackOptions(44100)) {
        DEBUGF("NSF: SetPlaybackOptions failed\n"); return CODEC_ERROR;}
    if(!LoadNSF(nDataBufferSize)) {
        DEBUGF("NSF: LoadNSF failed\n"); return CODEC_ERROR;}

    ci->id3->title=szGameTitle;
    ci->id3->artist=szArtist;
    ci->id3->album=szCopyright;
    if (usingplaylist) {
        ci->id3->length=nPlaylistSize*1000;
    } else {
        ci->id3->length=nTrackCount*1000;
    }

    if (!dontresettrack||strcmp(ci->id3->path,last_path)) {
        /* if this is the first time we're seeing this file, or if we haven't
           been asked to preserve the track number, default to the proper
           initial track */
        if (bIsExtended &&
            ci->global_settings->repeat_mode!=REPEAT_ONE && nPlaylistSize>0) {
            /* decide to use the playlist */
            usingplaylist=1;
            track=0;
            set_codec_track(nPlaylist[0],0);
        } else {
            /* simply use the initial track */
            track=nInitialTrack;
            set_codec_track(track,track);
        }
    } else {
        /* if we've already been running this file assume track is set
           already */
        if (usingplaylist) set_codec_track(nPlaylist[track],track);
        else set_codec_track(track,track);
    }
    strcpy(last_path,ci->id3->path);

    /* The main decoder loop */
    
    endofstream = 0;
    
    reset_profile_timers();
    
    while (!endofstream) {

        ci->yield();
        if (ci->stop_codec || ci->new_track) {
            break;
        }

        if (ci->seek_time >0) {
            track=ci->seek_time/1000;
            if (usingplaylist) {
                if (track>=nPlaylistSize) break;
            } else {
                if (track>=nTrackCount) break;
            }
            ci->seek_complete();
            dontresettrack=1;
            goto init_nsf;
        }

        ENTER_TIMER(total);
        written=GetSamples((uint8_t*)samples,WAV_CHUNK_SIZE/2);
        EXIT_TIMER(total);
        
        if (!written || SongCompleted()) {
            print_timers(last_path,track);
            reset_profile_timers();
            
            track++;
            if (usingplaylist) {
               if (track>=nPlaylistSize) break;
            } else {
               if (track>=nTrackCount) break;
            }
            dontresettrack=1;
            goto init_nsf;
        }

        ci->pcmbuf_insert(samples, NULL, written >> 1);
    }
    
    print_timers(last_path,track);
    
    if (ci->request_next_track()) {
    if (ci->global_settings->repeat_mode==REPEAT_ONE) {
        /* in repeat one mode just advance to the next track */
        track++;
        if (track>=nTrackCount) track=0;
        dontresettrack=1;
        /* at this point we can't tell if another file has been selected */
        goto next_track;
    } else {
        /* otherwise do a proper load of the next file */
        dontresettrack=0;
        last_path[0]='\0';
    }
    goto next_track; /* when we fall through here we'll reload the file */
    }
    
    return CODEC_OK;
}
