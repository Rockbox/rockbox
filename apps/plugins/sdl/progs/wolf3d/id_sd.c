//
//      ID Engine
//      ID_SD.c - Sound Manager for Wolfenstein 3D
//      v1.2
//      By Jason Blochowiak
//

//
//      This module handles dealing with generating sound on the appropriate
//              hardware
//
//      Depends on: User Mgr (for parm checking)
//
//      Globals:
//              For User Mgr:
//                      SoundBlasterPresent - SoundBlaster card present?
//                      AdLibPresent - AdLib card present?
//                      SoundMode - What device is used for sound effects
//                              (Use SM_SetSoundMode() to set)
//                      MusicMode - What device is used for music
//                              (Use SM_SetMusicMode() to set)
//                      DigiMode - What device is used for digitized sound effects
//                              (Use SM_SetDigiDevice() to set)
//
//              For Cache Mgr:
//                      NeedsDigitized - load digitized sounds?
//                      NeedsMusic - load music?
//

#include "wl_def.h"
#include "fmopl.h"

#include "fixedpoint.h"

#pragma hdrstop

#define ORIGSAMPLERATE 7042

typedef struct
{
	char RIFF[4];
	longword filelenminus8;
	char WAVE[4];
	char fmt_[4];
	longword formatlen;
	word val0x0001;
	word channels;
	longword samplerate;
	longword bytespersec;
	word bytespersample;
	word bitspersample;
} headchunk;

typedef struct
{
	char chunkid[4];
	longword chunklength;
} wavechunk;

typedef struct
{
    uint32_t startpage;
    uint32_t length;
} digiinfo;

static Mix_Chunk *SoundChunks[ STARTMUSIC - STARTDIGISOUNDS];
static byte      *SoundBuffers[STARTMUSIC - STARTDIGISOUNDS];

globalsoundpos channelSoundPos[MIX_CHANNELS];

//      Global variables
        boolean         AdLibPresent,
                        SoundBlasterPresent,SBProPresent,
                        SoundPositioned;
        SDMode          SoundMode;
        SMMode          MusicMode;
        SDSMode         DigiMode;
static  byte          **SoundTable;
        int             DigiMap[LASTSOUND];
        int             DigiChannel[STARTMUSIC - STARTDIGISOUNDS];

//      Internal variables
static  boolean                 SD_Started;
static  boolean                 nextsoundpos;
static  soundnames              SoundNumber;
static  soundnames              DigiNumber;
static  word                    SoundPriority;
static  word                    DigiPriority;
static  int                     LeftPosition;
static  int                     RightPosition;

        word                    NumDigi;
static  digiinfo               *DigiList;
static  boolean                 DigiPlaying;

//      PC Sound variables
static  volatile byte           pcLastSample;
static  byte * volatile         pcSound;
static  longword                pcLengthLeft;

//      AdLib variables
static  byte * volatile         alSound;
static  byte                    alBlock;
static  longword                alLengthLeft;
static  longword                alTimeCount;
static  Instrument              alZeroInst;

//      Sequencer variables
static  volatile boolean        sqActive;
static  word                   *sqHack;
static  word                   *sqHackPtr;
static  int                     sqHackLen;
static  int                     sqHackSeqLen;
static  longword                sqHackTime;


static void SDL_SoundFinished(void)
{
#ifdef SOUND_ENABLE
	SoundNumber   = (soundnames)0;
	SoundPriority = 0;
#endif
}


#ifdef NOTYET

void SDL_turnOnPCSpeaker(word timerval);
#pragma aux SDL_turnOnPCSpeaker = \
        "mov    al,0b6h" \
        "out    43h,al" \
        "mov    al,bl" \
        "out    42h,al" \
        "mov    al,bh" \
        "out    42h,al" \
        "in     al,61h" \
        "or     al,3"   \
        "out    61h,al" \
        parm [bx] \
        modify exact [al]

void SDL_turnOffPCSpeaker();
#pragma aux SDL_turnOffPCSpeaker = \
        "in     al,61h" \
        "and    al,0fch" \
        "out    61h,al" \
        modify exact [al]

void SDL_setPCSpeaker(byte val);
#pragma aux SDL_setPCSpeaker = \
        "in     al,61h" \
        "and    al,0fch" \
        "or     al,ah" \
        "out    61h,al" \
        parm [ah] \
        modify exact [al]

void SDL_DoFX()
{
        if(pcSound)
        {
                if(*pcSound!=pcLastSample)
                {
                        pcLastSample=*pcSound;

                        if(pcLastSample)
                                SDL_turnOnPCSpeaker(pcLastSample*60);
                        else
                                SDL_turnOffPCSpeaker();
                }
                pcSound++;
                pcLengthLeft--;
                if(!pcLengthLeft)
                {
                        pcSound=0;
                        SoundNumber=(soundnames)0;
                        SoundPriority=0;
                        SDL_turnOffPCSpeaker();
                }
        }

        // [adlib sound stuff removed...]
}

static void SDL_DigitizedDoneInIRQ(void);

void SDL_DoFast()
{
        count_fx++;
        if(count_fx>=5)
        {
                count_fx=0;

                SDL_DoFX();

                count_time++;
                if(count_time>=2)
                {
                        TimeCount++;
                        count_time=0;
                }
        }

        // [adlib music and soundsource stuff removed...]

        TimerCount+=TimerDivisor;
        if(*((word *)&TimerCount+1))
        {
                *((word *)&TimerCount+1)=0;
                t0OldService();
        }
        else
        {
                outp(0x20,0x20);
        }
}

// Timer 0 ISR for 7000Hz interrupts
void __interrupt SDL_t0ExtremeAsmService(void)
{
        if(pcindicate)
        {
                if(pcSound)
                {
                        SDL_setPCSpeaker(((*pcSound++)&0x80)>>6);
                        pcLengthLeft--;
                        if(!pcLengthLeft)
                        {
                                pcSound=0;
                                SDL_turnOffPCSpeaker();
                                SDL_DigitizedDoneInIRQ();
                        }
                }
        }
        extreme++;
        if(extreme>=10)
        {
                extreme=0;
                SDL_DoFast();
        }
        else
                outp(0x20,0x20);
}

// Timer 0 ISR for 700Hz interrupts
void __interrupt SDL_t0FastAsmService(void)
{
        SDL_DoFast();
}

// Timer 0 ISR for 140Hz interrupts
void __interrupt SDL_t0SlowAsmService(void)
{
        count_time++;
        if(count_time>=2)
        {
                TimeCount++;
                count_time=0;
        }

        SDL_DoFX();

        TimerCount+=TimerDivisor;
        if(*((word *)&TimerCount+1))
        {
                *((word *)&TimerCount+1)=0;
                t0OldService();
        }
        else
                outp(0x20,0x20);
}

void SDL_IndicatePC(boolean ind)
{
        pcindicate=ind;
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_SetTimer0() - Sets system timer 0 to the specified speed
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_SetTimer0(word speed)
{
#ifndef TPROF   // If using Borland's profiling, don't screw with the timer
//      _asm pushfd
        _asm cli

        outp(0x43,0x36);                                // Change timer 0
        outp(0x40,(byte)speed);
        outp(0x40,speed >> 8);
        // Kludge to handle special case for digitized PC sounds
        if (TimerDivisor == (1192030 / (TickBase * 100)))
                TimerDivisor = (1192030 / (TickBase * 10));
        else
                TimerDivisor = speed;

//      _asm popfd
        _asm    sti
#else
        TimerDivisor = 0x10000;
#endif
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_SetIntsPerSec() - Uses SDL_SetTimer0() to set the number of
//              interrupts generated by system timer 0 per second
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_SetIntsPerSec(word ints)
{
        TimerRate = ints;
        SDL_SetTimer0(1192030 / ints);
}

static void
SDL_SetTimerSpeed(void)
{
        word    rate;
        void (_interrupt *isr)(void);

        if ((DigiMode == sds_PC) && DigiPlaying)
        {
                rate = TickBase * 100;
                isr = SDL_t0ExtremeAsmService;
        }
        else if ((MusicMode == smm_AdLib) || ((DigiMode == sds_SoundSource) && DigiPlaying)     )
        {
                rate = TickBase * 10;
                isr = SDL_t0FastAsmService;
        }
        else
        {
                rate = TickBase * 2;
                isr = SDL_t0SlowAsmService;
        }

        if (rate != TimerRate)
        {
                _dos_setvect(8,isr);
                SDL_SetIntsPerSec(rate);
                TimerRate = rate;
        }
}

//
//      PC Sound code
//

///////////////////////////////////////////////////////////////////////////
//
//      SDL_PCPlaySample() - Plays the specified sample on the PC speaker
//
///////////////////////////////////////////////////////////////////////////
#ifdef  _MUSE_
void
#else
static void
#endif
SDL_PCPlaySample(byte *data,longword len,boolean inIRQ)
{
        if(!inIRQ)
        {
//              _asm    pushfd
                _asm    cli
        }

        SDL_IndicatePC(true);

        pcLengthLeft = len;
        pcSound = (volatile byte *)data;

        if(!inIRQ)
        {
//              _asm    popfd
                _asm    sti
        }
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_PCStopSample() - Stops a sample playing on the PC speaker
//
///////////////////////////////////////////////////////////////////////////
#ifdef  _MUSE_
void
#else
static void
#endif
SDL_PCStopSampleInIRQ(void)
{
        pcSound = 0;

        SDL_IndicatePC(false);

        _asm    in      al,0x61                 // Turn the speaker off
        _asm    and     al,0xfd                 // ~2
        _asm    out     0x61,al
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_PCPlaySound() - Plays the specified sound on the PC speaker
//
///////////////////////////////////////////////////////////////////////////
#ifdef  _MUSE_
void
#else
static void
#endif
SDL_PCPlaySound(PCSound *sound)
{
//      _asm    pushfd
        _asm    cli

        pcLastSample = -1;
        pcLengthLeft = sound->common.length;
        pcSound = sound->data;

//      _asm    popfd
        _asm    sti
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_PCStopSound() - Stops the current sound playing on the PC Speaker
//
///////////////////////////////////////////////////////////////////////////
#ifdef  _MUSE_
void
#else
static void
#endif
SDL_PCStopSound(void)
{
//      _asm    pushfd
        _asm    cli

        pcSound = 0;

        _asm    in      al,0x61                 // Turn the speaker off
        _asm    and     al,0xfd                 // ~2
        _asm    out     0x61,al

//      _asm    popfd
        _asm    sti
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_ShutPC() - Turns off the pc speaker
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_ShutPC(void)
{
//      _asm    pushfd
        _asm    cli

        pcSound = 0;

        _asm    in      al,0x61                 // Turn the speaker & gate off
        _asm    and     al,0xfc                 // ~3
        _asm    out     0x61,al

//      _asm    popfd
        _asm    sti
}

#endif

void
SD_StopDigitized(void)
{
#ifdef SOUND_ENABLE
    DigiPlaying = false;
    DigiNumber = (soundnames) 0;
    DigiPriority = 0;
    SoundPositioned = false;
    if ((DigiMode == sds_PC) && (SoundMode == sdm_PC))
        SDL_SoundFinished();

    switch (DigiMode)
    {
        case sds_PC:
//            SDL_PCStopSampleInIRQ();
            break;
        case sds_SoundBlaster:
//            SDL_SBStopSampleInIRQ();
            Mix_HaltChannel(-1);
            break;
    }
#endif
}

int SD_GetChannelForDigi(int which)
{
#ifdef SOUND_ENABLE
    if(DigiChannel[which] != -1) return DigiChannel[which];

    int channel = Mix_GroupAvailable(1);
    if(channel == -1) channel = Mix_GroupOldest(1);
    if(channel == -1)           // All sounds stopped in the meantime?
        return Mix_GroupAvailable(1);
    return channel;
#endif
    return 0;
}

void SD_SetPosition(int channel, int leftpos, int rightpos)
{
#ifdef SOUND_ENABLE
    if((leftpos < 0) || (leftpos > 15) || (rightpos < 0) || (rightpos > 15)
            || ((leftpos == 15) && (rightpos == 15)))
        Quit("SD_SetPosition: Illegal position");

    switch (DigiMode)
    {
        case sds_SoundBlaster:
//            SDL_PositionSBP(leftpos,rightpos);
            Mix_SetPanning(channel, ((15 - leftpos) << 4) + 15,
                ((15 - rightpos) << 4) + 15);
            break;
    }
#endif
}

#define FP_FLOOR(x, fracbits) (x & (~((1<<(fracbits))-1)))

/* more precise than original samples */
#define FRACBITS 10
#define FP_MATH

#ifdef FP_MATH
/* Looks to be some sort of interpolation - FW19 */
Sint16 GetSample(long csample_fp, byte *samples, int size)
{
    //float s0=0, s1=0, s2=0;
    long s0_fp = 0, s1_fp = 0, s2_fp = 0;
    long cursample_fp = FP_FLOOR(csample_fp, FRACBITS);
    long sf_fp = csample_fp - cursample_fp;
    const long one = 1 << FRACBITS;

    int cursample = cursample_fp >> FRACBITS;

    if(cursample-1 >= 0) s0_fp = (samples[cursample-1] - 128) << FRACBITS;
    s1_fp = (samples[cursample] - 128) << FRACBITS;
    if(cursample+1 < size) s2_fp = (samples[cursample+1] - 128) << FRACBITS;

    //float val = s0*sf*(sf-1)/2 - s1*(sf*sf-1) + s2*(sf+1)*sf/2;

    long val_fp = fp_mul(s0_fp, fp_mul(sf_fp, sf_fp - one, FRACBITS), FRACBITS) / 2 -
                  fp_mul(s1_fp, fp_mul(sf_fp, sf_fp, FRACBITS) - one, FRACBITS)     +
                  fp_mul(s2_fp, fp_mul(sf_fp + one, sf_fp, FRACBITS), FRACBITS) / 2;

    int32_t intval = (int32_t) (val_fp * 256) >> FRACBITS;
    if(intval < -32768) intval = -32768;
    else if(intval > 32767) intval = 32767;
    return (Sint16) intval;
}
#else
Sint16 GetSample(float csample, byte *samples, int size)
{
    float s0=0, s1=0, s2=0;
    int cursample = (int) csample;
    float sf = csample - (float) cursample;

    if(cursample-1 >= 0) s0 = (float) (samples[cursample-1] - 128);
    s1 = (float) (samples[cursample] - 128);
    if(cursample+1 < size) s2 = (float) (samples[cursample+1] - 128);

    float val = s0*sf*(sf-1)/2 - s1*(sf*sf-1) + s2*(sf+1)*sf/2;
    int32_t intval = (int32_t) (val * 256);
    if(intval < -32768) intval = -32768;
    else if(intval > 32767) intval = 32767;
    return (Sint16) intval;
}
#endif

void SD_PrepareSound(int which)
{
#ifdef SOUND_ENABLE
#ifdef FP_MATH
    if(DigiList == NULL)
        Quit("SD_PrepareSound(%i): DigiList not initialized!\n", which);

    int page = DigiList[which].startpage;
    int size = DigiList[which].length;

    byte *origsamples = PM_GetSound(page);
    if(origsamples + size >= PM_GetEnd())
        Quit("SD_PrepareSound(%i): Sound reaches out of page file!\n", which);

    /* this is fine to keep as floating-point */
    int destsamples = (int) ((float) size * (float) param_samplerate
                             / (float) ORIGSAMPLERATE);

    byte *wavebuffer = (byte *) malloc(sizeof(headchunk) + sizeof(wavechunk)
        + destsamples * 2);     // dest are 16-bit samples
    if(wavebuffer == NULL)
        Quit("Unable to allocate wave buffer for sound %i size %d %d %d %d!\n", which, destsamples, size, param_samplerate, ORIGSAMPLERATE);

    headchunk head = {{'R','I','F','F'}, 0, {'W','A','V','E'},
        {'f','m','t',' '}, 0x10, 0x0001, 1, param_samplerate, param_samplerate*2, 2, 16};
    wavechunk dhead = {{'d', 'a', 't', 'a'}, destsamples*2};
    head.filelenminus8 = sizeof(head) + destsamples*2;  // (sizeof(dhead)-8 = 0)
    memcpy(wavebuffer, &head, sizeof(head));
    memcpy(wavebuffer+sizeof(head), &dhead, sizeof(dhead));

    // alignment is correct, as wavebuffer comes from malloc
    // and sizeof(headchunk) % 4 == 0 and sizeof(wavechunk) % 4 == 0
    Sint16 *newsamples = (Sint16 *)(void *) (wavebuffer + sizeof(headchunk)
        + sizeof(wavechunk));

    long scale_fac_fp = fp_div(size << FRACBITS, destsamples << FRACBITS, FRACBITS);

    for(int i=0; i<destsamples; i++)
    {
        newsamples[i] = GetSample(fp_mul(i << FRACBITS, scale_fac_fp, FRACBITS),
                                  origsamples, size);
    }
    SoundBuffers[which] = wavebuffer;

    SoundChunks[which] = Mix_LoadWAV_RW(SDL_RWFromMem(wavebuffer,
        sizeof(headchunk) + sizeof(wavechunk) + destsamples * 2), 1);
#else
        if(DigiList == NULL)
        Quit("SD_PrepareSound(%i): DigiList not initialized!\n", which);

    int page = DigiList[which].startpage;
    int size = DigiList[which].length;

    byte *origsamples = PM_GetSound(page);
    if(origsamples + size >= PM_GetEnd())
        Quit("SD_PrepareSound(%i): Sound reaches out of page file!\n", which);

    int destsamples = (int) ((float) size * (float) param_samplerate
        / (float) ORIGSAMPLERATE);

    byte *wavebuffer = (byte *) malloc(sizeof(headchunk) + sizeof(wavechunk)
        + destsamples * 2);     // dest are 16-bit samples
    if(wavebuffer == NULL)
        Quit("Unable to allocate wave buffer for sound %i!\n", which);

    headchunk head = {{'R','I','F','F'}, 0, {'W','A','V','E'},
        {'f','m','t',' '}, 0x10, 0x0001, 1, param_samplerate, param_samplerate*2, 2, 16};
    wavechunk dhead = {{'d', 'a', 't', 'a'}, destsamples*2};
    head.filelenminus8 = sizeof(head) + destsamples*2;  // (sizeof(dhead)-8 = 0)
    memcpy(wavebuffer, &head, sizeof(head));
    memcpy(wavebuffer+sizeof(head), &dhead, sizeof(dhead));

    // alignment is correct, as wavebuffer comes from malloc
    // and sizeof(headchunk) % 4 == 0 and sizeof(wavechunk) % 4 == 0
    Sint16 *newsamples = (Sint16 *)(void *) (wavebuffer + sizeof(headchunk)
        + sizeof(wavechunk));
    float cursample = 0.F;
    float samplestep = (float) ORIGSAMPLERATE / (float) param_samplerate;
    for(int i=0; i<destsamples; i++, cursample+=samplestep)
    {
        newsamples[i] = GetSample((float)size * (float)i / (float)destsamples,
            origsamples, size);
    }
    SoundBuffers[which] = wavebuffer;

    SoundChunks[which] = Mix_LoadWAV_RW(SDL_RWFromMem(wavebuffer,
        sizeof(headchunk) + sizeof(wavechunk) + destsamples * 2), 1);
#endif
#endif
}

int SD_PlayDigitized(word which,int leftpos,int rightpos)
{
#ifdef SOUND_ENABLE
    if (!DigiMode)
        return 0;

    if (which >= NumDigi)
        Quit("SD_PlayDigitized: bad sound number %i", which);

    int channel = SD_GetChannelForDigi(which);
    SD_SetPosition(channel, leftpos,rightpos);

    DigiPlaying = true;

    Mix_Chunk *sample = SoundChunks[which];
    if(sample == NULL)
    {
        printf("SoundChunks[%i] is NULL!\n", which);
        return 0;
    }

    if(Mix_PlayChannel(channel, sample, 0) == -1)
    {
        printf("Unable to play sound: %s\n", Mix_GetError());
        return 0;
    }

    return channel;
#endif
    return 0;
}

void SD_ChannelFinished(int channel)
{
#ifdef SOUND_ENABLE
    channelSoundPos[channel].valid = 0;
#endif
}

void
SD_SetDigiDevice(SDSMode mode)
{
#ifdef SOUND_ENABLE
    boolean devicenotpresent;

    if (mode == DigiMode)
        return;

    SD_StopDigitized();

    devicenotpresent = false;
    switch (mode)
    {
        case sds_SoundBlaster:
            if (!SoundBlasterPresent)
                devicenotpresent = true;
            break;
    }

    if (!devicenotpresent)
    {
        DigiMode = mode;

#ifdef NOTYET
        SDL_SetTimerSpeed();
#endif
    }
#endif
}

void
SDL_SetupDigi(void)
{
#ifdef SOUND_ENABLE
    // Correct padding enforced by PM_Startup()
    word *soundInfoPage = (word *) (void *) PM_GetPage(ChunksInFile-1);
    NumDigi = (word) PM_GetPageSize(ChunksInFile - 1) / 4;

    DigiList = (digiinfo *) malloc(NumDigi * sizeof(digiinfo));
    int i;
    for(i = 0; i < NumDigi; i++)
    {
        // Calculate the size of the digi from the sizes of the pages between
        // the start page and the start page of the next sound

        DigiList[i].startpage = soundInfoPage[i * 2];
        if((int) DigiList[i].startpage >= ChunksInFile - 1)
        {
            NumDigi = i;
            break;
        }

        int lastPage;
        if(i < NumDigi - 1)
        {
            lastPage = soundInfoPage[i * 2 + 2];
            if(lastPage == 0 || lastPage + PMSoundStart > ChunksInFile - 1) lastPage = ChunksInFile - 1;
            else lastPage += PMSoundStart;
        }
        else lastPage = ChunksInFile - 1;

        int size = 0;
        for(int page = PMSoundStart + DigiList[i].startpage; page < lastPage; page++)
            size += PM_GetPageSize(page);

        // Don't include padding of sound info page, if padding was added
        if(lastPage == ChunksInFile - 1 && PMSoundInfoPagePadded) size--;

        // Patch lower 16-bit of size with size from sound info page.
        // The original VSWAP contains padding which is included in the page size,
        // but not included in the 16-bit size. So we use the more precise value.
        if((size & 0xffff0000) != 0 && (size & 0xffff) < soundInfoPage[i * 2 + 1])
            size -= 0x10000;
        size = (size & 0xffff0000) | soundInfoPage[i * 2 + 1];

        DigiList[i].length = size;
    }

    for(i = 0; i < LASTSOUND; i++)
    {
        DigiMap[i] = -1;
        DigiChannel[i] = -1;
    }
#endif
}

//      AdLib Code

///////////////////////////////////////////////////////////////////////////
//
//      SDL_ALStopSound() - Turns off any sound effects playing through the
//              AdLib card
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_ALStopSound(void)
{
#ifdef SOUND_ENABLE
    alSound = 0;
    alOut(alFreqH + 0, 0);
#endif
}

static void
SDL_AlSetFXInst(Instrument *inst)
{
#ifdef SOUND_ENABLE
    byte c,m;

    m = 0;      // modulator cell for channel 0
    c = 3;      // carrier cell for channel 0
    alOut(m + alChar,inst->mChar);
    alOut(m + alScale,inst->mScale);
    alOut(m + alAttack,inst->mAttack);
    alOut(m + alSus,inst->mSus);
    alOut(m + alWave,inst->mWave);
    alOut(c + alChar,inst->cChar);
    alOut(c + alScale,inst->cScale);
    alOut(c + alAttack,inst->cAttack);
    alOut(c + alSus,inst->cSus);
    alOut(c + alWave,inst->cWave);

    // Note: Switch commenting on these lines for old MUSE compatibility
//    alOutInIRQ(alFeedCon,inst->nConn);
    alOut(alFeedCon,0);
#endif
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_ALPlaySound() - Plays the specified sound on the AdLib card
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_ALPlaySound(AdLibSound *sound)
{
#ifdef SOUND_ENABLE
    Instrument      *inst;
    byte            *data;

    SDL_ALStopSound();

    alLengthLeft = sound->common.length;
    data = sound->data;
    alBlock = ((sound->block & 7) << 2) | 0x20;
    inst = &sound->inst;

    if (!(inst->mSus | inst->cSus))
    {
        Quit("SDL_ALPlaySound() - Bad instrument");
    }

    SDL_AlSetFXInst(inst);
    alSound = (byte *)data;
#endif
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_ShutAL() - Shuts down the AdLib card for sound effects
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_ShutAL(void)
{
#ifdef SOUND_ENABLE
    alSound = 0;
    alOut(alEffects,0);
    alOut(alFreqH + 0,0);
    SDL_AlSetFXInst(&alZeroInst);
#endif
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_CleanAL() - Totally shuts down the AdLib card
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_CleanAL(void)
{
#ifdef SOUND_ENABLE
    int     i;

    alOut(alEffects,0);
    for (i = 1; i < 0xf5; i++)
        alOut(i, 0);
#endif
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_StartAL() - Starts up the AdLib card for sound effects
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_StartAL(void)
{
#ifdef SOUND_ENABLE
    alOut(alEffects, 0);
    SDL_AlSetFXInst(&alZeroInst);
#endif
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_DetectAdLib() - Determines if there's an AdLib (or SoundBlaster
//              emulating an AdLib) present
//
///////////////////////////////////////////////////////////////////////////
static boolean
SDL_DetectAdLib(void)
{
#ifdef SOUND_ENABLE
    for (int i = 1; i <= 0xf5; i++)       // Zero all the registers
        alOut(i, 0);

    alOut(1, 0x20);             // Set WSE=1
//    alOut(8, 0);                // Set CSM=0 & SEL=0

    return true;
#endif
}

////////////////////////////////////////////////////////////////////////////
//
//      SDL_ShutDevice() - turns off whatever device was being used for sound fx
//
////////////////////////////////////////////////////////////////////////////
static void
SDL_ShutDevice(void)
{
#ifdef SOUND_ENABLE
    switch (SoundMode)
    {
        case sdm_PC:
//            SDL_ShutPC();
            break;
        case sdm_AdLib:
            SDL_ShutAL();
            break;
    }
    SoundMode = sdm_Off;
#endif
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_CleanDevice() - totally shuts down all sound devices
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_CleanDevice(void)
{
#ifdef SOUND_ENABLE
    if ((SoundMode == sdm_AdLib) || (MusicMode == smm_AdLib))
        SDL_CleanAL();
#endif
}

///////////////////////////////////////////////////////////////////////////
//
//      SDL_StartDevice() - turns on whatever device is to be used for sound fx
//
///////////////////////////////////////////////////////////////////////////
static void
SDL_StartDevice(void)
{
#ifdef SOUND_ENABLE
    switch (SoundMode)
    {
        case sdm_AdLib:
            SDL_StartAL();
            break;
    }
    SoundNumber = (soundnames) 0;
    SoundPriority = 0;
#endif
}

//      Public routines

///////////////////////////////////////////////////////////////////////////
//
//      SD_SetSoundMode() - Sets which sound hardware to use for sound effects
//
///////////////////////////////////////////////////////////////////////////
boolean
SD_SetSoundMode(SDMode mode)
{
#ifdef SOUND_ENABLE
    boolean result = false;
    word    tableoffset;

    SD_StopSound();

    if ((mode == sdm_AdLib) && !AdLibPresent)
        mode = sdm_PC;

    switch (mode)
    {
        case sdm_Off:
            tableoffset = STARTADLIBSOUNDS;
            result = true;
            break;
        case sdm_PC:
            tableoffset = STARTPCSOUNDS;
            result = true;
            break;
        case sdm_AdLib:
            tableoffset = STARTADLIBSOUNDS;
            if (AdLibPresent)
                result = true;
            break;
        default:
            Quit("SD_SetSoundMode: Invalid sound mode %i", mode);
            return false;
    }
    SoundTable = &audiosegs[tableoffset];

    if (result && (mode != SoundMode))
    {
        SDL_ShutDevice();
        SoundMode = mode;
        SDL_StartDevice();
    }

    return(result);
#endif
    return true;
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_SetMusicMode() - sets the device to use for background music
//
///////////////////////////////////////////////////////////////////////////
boolean
SD_SetMusicMode(SMMode mode)
{
#ifdef SOUND_ENABLE
    boolean result = false;

    SD_FadeOutMusic();
    while (SD_MusicPlaying())
        SDL_Delay(5);

    switch (mode)
    {
        case smm_Off:
            result = true;
            break;
        case smm_AdLib:
            if (AdLibPresent)
                result = true;
            break;
    }

    if (result)
        MusicMode = mode;

//    SDL_SetTimerSpeed();

    return(result);
#endif
}

int numreadysamples = 0;
byte *curAlSound = 0;
byte *curAlSoundPtr = 0;
longword curAlLengthLeft = 0;
int soundTimeCounter = 5;
int samplesPerMusicTick;

void *OPL_ptr = NULL;

void SDL_IMFMusicPlayer(void *udata, Uint8 *stream, int len)
{
#ifdef SOUND_ENABLE
    int stereolen = len>>1;
    int sampleslen = stereolen>>1;
    INT16 *stream16 = (INT16 *) (void *) stream;    // expect correct alignment

    while(1)
    {
        if(numreadysamples)
        {
            if(numreadysamples<sampleslen)
            {
                YM3812UpdateOne(OPL_ptr, stream16, numreadysamples);
                stream16 += numreadysamples*2;
                sampleslen -= numreadysamples;
            }
            else
            {
                YM3812UpdateOne(OPL_ptr, stream16, sampleslen);
                numreadysamples -= sampleslen;
                return;
            }
        }
        soundTimeCounter--;
        if(!soundTimeCounter)
        {
            soundTimeCounter = 5;
            if(curAlSound != alSound)
            {
                curAlSound = curAlSoundPtr = alSound;
                curAlLengthLeft = alLengthLeft;
            }
            if(curAlSound)
            {
                if(*curAlSoundPtr)
                {
                    alOut(alFreqL, *curAlSoundPtr);
                    alOut(alFreqH, alBlock);
                }
                else alOut(alFreqH, 0);
                curAlSoundPtr++;
                curAlLengthLeft--;
                if(!curAlLengthLeft)
                {
                    curAlSound = alSound = 0;
                    SoundNumber = (soundnames) 0;
                    SoundPriority = 0;
                    alOut(alFreqH, 0);
                }
            }
        }
        if(sqActive)
        {
            do
            {
                if(sqHackTime > alTimeCount) break;
                sqHackTime = alTimeCount + *(sqHackPtr+1);
                alOut(*(byte *) sqHackPtr, *(((byte *) sqHackPtr)+1));
                sqHackPtr += 2;
                sqHackLen -= 4;
            }
            while(sqHackLen>0);
            alTimeCount++;
            if(!sqHackLen)
            {
                sqHackPtr = sqHack;
                sqHackLen = sqHackSeqLen;
                sqHackTime = 0;
                alTimeCount = 0;
            }
        }
        numreadysamples = samplesPerMusicTick;
    }
#endif
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_Startup() - starts up the Sound Mgr
//              Detects all additional sound hardware and installs my ISR
//
///////////////////////////////////////////////////////////////////////////
void
SD_Startup(void)
{
#ifdef SOUND_ENABLE
    int     i;

    if (SD_Started)
        return;

    if(Mix_OpenAudio(param_samplerate, AUDIO_S16, 2, param_audiobuffer))
    {
        printf("Unable to open audio: %s\n", Mix_GetError());
        return;
    }

    Mix_ReserveChannels(2);  // reserve player and boss weapon channels
    Mix_GroupChannels(2, MIX_CHANNELS-1, 1); // group remaining channels

    // Init music

    samplesPerMusicTick = param_samplerate / 700;    // SDL_t0FastAsmService played at 700Hz

    if(!(OPL_ptr = YM3812Init((void*)1,3579545,param_samplerate)))
    {
        //printf("Unable to create virtual OPL!!\n");
    }

    for(i=1;i<0xf6;i++)
        YM3812Write(OPL_ptr,i,0);

    YM3812Write(OPL_ptr,1,0x20); // Set WSE=1
//    YM3812Write(0,8,0); // Set CSM=0 & SEL=0		 // already set in for statement

    Mix_HookMusic(SDL_IMFMusicPlayer, 0);
    Mix_ChannelFinished(SD_ChannelFinished);
    AdLibPresent = true;
    SoundBlasterPresent = true;

    alTimeCount = 0;

    SD_SetSoundMode(sdm_Off);
    SD_SetMusicMode(smm_Off);

    SDL_SetupDigi();

    SD_Started = true;
#endif
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_Shutdown() - shuts down the Sound Mgr
//              Removes sound ISR and turns off whatever sound hardware was active
//
///////////////////////////////////////////////////////////////////////////
void
SD_Shutdown(void)
{
#ifdef SOUND_ENABLE
    if (!SD_Started)
        return;

    SD_MusicOff();
    SD_StopSound();

    for(int i = 0; i < STARTMUSIC - STARTDIGISOUNDS; i++)
    {
        if(SoundChunks[i]) Mix_FreeChunk(SoundChunks[i]);
        if(SoundBuffers[i]) free(SoundBuffers[i]);
    }

    free(DigiList);

    SD_Started = false;
#endif
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_PositionSound() - Sets up a stereo imaging location for the next
//              sound to be played. Each channel ranges from 0 to 15.
//
///////////////////////////////////////////////////////////////////////////
void
SD_PositionSound(int leftvol,int rightvol)
{
#ifdef SOUND_ENABLE
    LeftPosition = leftvol;
    RightPosition = rightvol;
    nextsoundpos = true;
#endif
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_PlaySound() - plays the specified sound on the appropriate hardware
//
///////////////////////////////////////////////////////////////////////////
boolean
SD_PlaySound(soundnames sound)
{
#ifdef SOUND_ENABLE
    boolean         ispos;
    SoundCommon     *s;
    int             lp,rp;

    lp = LeftPosition;
    rp = RightPosition;
    LeftPosition = 0;
    RightPosition = 0;

    ispos = nextsoundpos;
    nextsoundpos = false;

    if (sound == -1 || (DigiMode == sds_Off && SoundMode == sdm_Off))
        return 0;

    s = (SoundCommon *) SoundTable[sound];

    if ((SoundMode != sdm_Off) && !s)
            Quit("SD_PlaySound() - Uncached sound");

    if ((DigiMode != sds_Off) && (DigiMap[sound] != -1))
    {
        if ((DigiMode == sds_PC) && (SoundMode == sdm_PC))
        {
#ifdef NOTYET
            if (s->priority < SoundPriority)
                return 0;

            SDL_PCStopSound();

            SD_PlayDigitized(DigiMap[sound],lp,rp);
            SoundPositioned = ispos;
            SoundNumber = sound;
            SoundPriority = s->priority;
#else
            return 0;
#endif
        }
        else
        {
#ifdef NOTYET
            if (s->priority < DigiPriority)
                return(false);
#endif

            int channel = SD_PlayDigitized(DigiMap[sound], lp, rp);
            SoundPositioned = ispos;
            DigiNumber = sound;
            DigiPriority = s->priority;
            return channel + 1;
        }

        return(true);
    }

    if (SoundMode == sdm_Off)
        return 0;

    if (!s->length)
        Quit("SD_PlaySound() - Zero length sound");
    if (s->priority < SoundPriority)
        return 0;

    switch (SoundMode)
    {
        case sdm_PC:
//            SDL_PCPlaySound((PCSound *)s);
            break;
        case sdm_AdLib:
            SDL_ALPlaySound((AdLibSound *)s);
            break;
    }

    SoundNumber = sound;
    SoundPriority = s->priority;

    return 0;
#endif
    return 0;
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_SoundPlaying() - returns the sound number that's playing, or 0 if
//              no sound is playing
//
///////////////////////////////////////////////////////////////////////////
word
SD_SoundPlaying(void)
{
#ifdef SOUND_ENABLE
    boolean result = false;

    switch (SoundMode)
    {
        case sdm_PC:
            result = pcSound? true : false;
            break;
        case sdm_AdLib:
            result = alSound? true : false;
            break;
    }

    if (result)
        return(SoundNumber);
    else
        return(false);
#endif
    return false;
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_StopSound() - if a sound is playing, stops it
//
///////////////////////////////////////////////////////////////////////////
void
SD_StopSound(void)
{
#ifdef SOUND_ENABLE
    if (DigiPlaying)
        SD_StopDigitized();

    switch (SoundMode)
    {
        case sdm_PC:
//            SDL_PCStopSound();
            break;
        case sdm_AdLib:
            SDL_ALStopSound();
            break;
    }

    SoundPositioned = false;

    SDL_SoundFinished();
#endif
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_WaitSoundDone() - waits until the current sound is done playing
//
///////////////////////////////////////////////////////////////////////////
void
SD_WaitSoundDone(void)
{
#ifdef SOUND_ENABLE
    while (SD_SoundPlaying())
        SDL_Delay(5);
#endif
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_MusicOn() - turns on the sequencer
//
///////////////////////////////////////////////////////////////////////////
void
SD_MusicOn(void)
{
#ifdef SOUND_ENABLE
    sqActive = true;
#endif
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_MusicOff() - turns off the sequencer and any playing notes
//      returns the last music offset for music continue
//
///////////////////////////////////////////////////////////////////////////
int
SD_MusicOff(void)
{
#ifdef SOUND_ENABLE
    word    i;

    sqActive = false;
    switch (MusicMode)
    {
        case smm_AdLib:
            alOut(alEffects, 0);
            for (i = 0;i < sqMaxTracks;i++)
                alOut(alFreqH + i + 1, 0);
            break;
    }

    return (int) (sqHackPtr-sqHack);
#endif
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_StartMusic() - starts playing the music pointed to
//
///////////////////////////////////////////////////////////////////////////
void
SD_StartMusic(int chunk)
{
#ifdef SOUND_ENABLE
    SD_MusicOff();

    if (MusicMode == smm_AdLib)
    {
        int32_t chunkLen = CA_CacheAudioChunk(chunk);
        sqHack = (word *)(void *) audiosegs[chunk];     // alignment is correct
        if(*sqHack == 0) sqHackLen = sqHackSeqLen = chunkLen;
        else sqHackLen = sqHackSeqLen = *sqHack++;
        sqHackPtr = sqHack;
        sqHackTime = 0;
        alTimeCount = 0;
        SD_MusicOn();
    }
#endif
}

void
SD_ContinueMusic(int chunk, int startoffs)
{
#ifdef SOUND_ENABLE
    SD_MusicOff();

    if (MusicMode == smm_AdLib)
    {
        int32_t chunkLen = CA_CacheAudioChunk(chunk);
        sqHack = (word *)(void *) audiosegs[chunk];     // alignment is correct
        if(*sqHack == 0) sqHackLen = sqHackSeqLen = chunkLen;
        else sqHackLen = sqHackSeqLen = *sqHack++;
        sqHackPtr = sqHack;

        if(startoffs >= sqHackLen)
        {
            Quit("SD_StartMusic: Illegal startoffs provided!");
        }

        // fast forward to correct position
        // (needed to reconstruct the instruments)

        for(int i = 0; i < startoffs; i += 2)
        {
            byte reg = *(byte *)sqHackPtr;
            byte val = *(((byte *)sqHackPtr) + 1);
            if(reg >= 0xb1 && reg <= 0xb8) val &= 0xdf;           // disable play note flag
            else if(reg == 0xbd) val &= 0xe0;                     // disable drum flags

            alOut(reg,val);
            sqHackPtr += 2;
            sqHackLen -= 4;
        }
        sqHackTime = 0;
        alTimeCount = 0;

        SD_MusicOn();
    }
#endif
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_FadeOutMusic() - starts fading out the music. Call SD_MusicPlaying()
//              to see if the fadeout is complete
//
///////////////////////////////////////////////////////////////////////////
void
SD_FadeOutMusic(void)
{
#ifdef SOUND_ENABLE
    switch (MusicMode)
    {
        case smm_AdLib:
            // DEBUG - quick hack to turn the music off
            SD_MusicOff();
            break;
    }
#endif
}

///////////////////////////////////////////////////////////////////////////
//
//      SD_MusicPlaying() - returns true if music is currently playing, false if
//              not
//
///////////////////////////////////////////////////////////////////////////
boolean
SD_MusicPlaying(void)
{
#ifdef SOUND_ENABLE
    boolean result;

    switch (MusicMode)
    {
        case smm_AdLib:
            result = sqActive;
            break;
        default:
            result = false;
            break;
    }

    return(result);
#endif
    return false;
}
