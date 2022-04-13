// cRSID lightweight RealSID (integer-only) library-header (with API-calls) by Hermit (Mihaly Horvath)

#ifndef LIBCRSIDC64_HEADER
#define LIBCRSIDC64_HEADER //used  to prevent double inclusion of this header-file

typedef struct cRSID_CPUinstance cRSID_CPUinstance;
typedef struct cRSID_SIDinstance cRSID_SIDinstance;
typedef struct cRSID_CIAinstance cRSID_CIAinstance;
typedef struct cRSID_VICinstance cRSID_VICinstance;


//Internal functions

// C64/C64.c
cRSID_C64instance*  cRSID_createC64     (cRSID_C64instance* C64, unsigned short samplerate);
void                cRSID_setC64        (cRSID_C64instance* C64); //configure hardware (SIDs) for SID-tune
void                cRSID_initC64       (cRSID_C64instance* C64); //hard-reset
int                 cRSID_emulateC64    (cRSID_C64instance* C64);
static inline short cRSID_playPSIDdigi  (cRSID_C64instance* C64);
// C64/MEM.c
static inline unsigned char* cRSID_getMemReadPtr     (register unsigned short address); //for global cSID_C64 fast-access
static inline unsigned char* cRSID_getMemReadPtrC64  (cRSID_C64instance* C64, register unsigned short address); //maybe slower
static inline unsigned char* cRSID_getMemWritePtr    (register unsigned short address); //for global cSID_C64 fast-access
static inline unsigned char* cRSID_getMemWritePtrC64 (cRSID_C64instance* C64, register unsigned short address); //maybe slower
static inline unsigned char  cRSID_readMem     (register unsigned short address); //for global cSID_C64 fast-access
static inline unsigned char  cRSID_readMemC64  (cRSID_C64instance* C64, register unsigned short address); //maybe slower
static inline void           cRSID_writeMem    (register unsigned short address, register unsigned char data); //for global cSID_C64 fast-access
static inline void           cRSID_writeMemC64 (cRSID_C64instance* C64, register unsigned short address, register unsigned char data); //maybe slower
void                         cRSID_setROMcontent (cRSID_C64instance* C64); //KERNAL, BASIC
void                         cRSID_initMem       (cRSID_C64instance* C64);
// C64/CPU.c
void               cRSID_initCPU       (cRSID_CPUinstance* CPU, unsigned short mempos);
unsigned char      cRSID_emulateCPU    (void); //direct instances inside for hopefully faster operation
static inline char cRSID_handleCPUinterrupts (cRSID_CPUinstance* CPU);
// C64/SID.c
void               cRSID_createSIDchip (cRSID_C64instance* C64, cRSID_SIDinstance* SID, unsigned short model, unsigned short baseaddress);
void               cRSID_initSIDchip   (cRSID_SIDinstance* SID);
void               cRSID_emulateADSRs  (cRSID_SIDinstance *SID, char cycles);
int                cRSID_emulateWaves  (cRSID_SIDinstance* SID);
// C64/CIA.c
void               cRSID_createCIAchip (cRSID_C64instance* C64, cRSID_CIAinstance* CIA, unsigned short baseaddress);
void               cRSID_initCIAchip   (cRSID_CIAinstance* CIA);
static inline char cRSID_emulateCIA    (cRSID_CIAinstance* CIA, char cycles);
static inline void cRSID_writeCIAIRQmask   (cRSID_CIAinstance* CIA, unsigned char value);
static inline void cRSID_acknowledgeCIAIRQ (cRSID_CIAinstance* CIA);
// C64/VIC.c
void               cRSID_createVICchip (cRSID_C64instance* C64, cRSID_VICinstance* VIC, unsigned short baseaddress);
void               cRSID_initVICchip   (cRSID_VICinstance* VIC);
static inline char cRSID_emulateVIC    (cRSID_VICinstance* VIC, char cycles);
static inline void cRSID_acknowledgeVICrasterIRQ (cRSID_VICinstance* VIC);

// host/file.c
#ifdef CRSID_PLATFORM_PC
int                cRSID_loadSIDfile   (unsigned char* SIDfileData, char* filename, int maxlen); //load SID-file to a memory location (and return size)
#endif
// host/audio.c
#ifdef CRSID_PLATFORM_PC
void*              cRSID_initSound     (cRSID_C64instance* C64, unsigned short samplerate, unsigned short buflen);
void               cRSID_startSound    (void);
void               cRSID_stopSound     (void);
void               cRSID_closeSound    (void);
void               cRSID_generateSound (cRSID_C64instance* C64, unsigned char* buf, unsigned short len);
#endif


struct cRSID_CPUinstance {
 cRSID_C64instance* C64; //reference to the containing C64
 unsigned int       PC;
 short int          A, SP;
 unsigned char      X, Y, ST;  //STATUS-flags: N V - B D I Z C
 unsigned char      PrevNMI; //used for NMI leading edge detection
};


struct cRSID_SIDinstance {
 //SID-chip data:
 cRSID_C64instance* C64;           //reference to the containing C64
 unsigned short     ChipModel;     //values: 8580 / 6581
 unsigned short     BaseAddress;   //SID-baseaddress location in C64-memory (IO)
 unsigned char*     BasePtr;       //SID-baseaddress location in host's memory
 //ADSR-related:
 unsigned char      ADSRstate[15];
 unsigned short     RateCounter[15];
 unsigned char      EnvelopeCounter[15];
 unsigned char      ExponentCounter[15];
 //Wave-related:
 int                PhaseAccu[15];       //28bit precision instead of 24bit
 int                PrevPhaseAccu[15];   //(integerized ClockRatio fractionals, WebSID has similar solution)
 unsigned char      SyncSourceMSBrise;
 unsigned int       RingSourceMSB;
 unsigned int       NoiseLFSR[15];
 unsigned int       PrevWavGenOut[15];
 unsigned char      PrevWavData[15];
 //Filter-related:
 int                PrevLowPass;
 int                PrevBandPass;
 //Output-stage:
 signed int         PrevVolume; //lowpass-filtered version of Volume-band register
};


struct cRSID_CIAinstance {
 cRSID_C64instance* C64;         //reference to the containing C64
 char               ChipModel;   //old or new CIA? (have 1 cycle difference in cases)
 unsigned short     BaseAddress; //CIA-baseaddress location in C64-memory (IO)
 unsigned char*     BasePtrWR;   //CIA-baseaddress location in host's memory for writing
 unsigned char*     BasePtrRD;   //CIA-baseaddress location in host's memory for reading
};


struct cRSID_VICinstance {
 cRSID_C64instance* C64;         //reference to the containing C64
 char               ChipModel;   //(timing differences between models?)
 unsigned short     BaseAddress; //VIC-baseaddress location in C64-memory (IO)
 unsigned char*     BasePtrWR;   //VIC-baseaddress location in host's memory for writing
 unsigned char*     BasePtrRD;   //VIC-baseaddress location in host's memory for reading
 unsigned short     RasterLines;
 unsigned char      RasterRowCycles;
 unsigned char      RowCycleCnt;
};


struct cRSID_C64instance {
 //platform-related:
 unsigned short    SampleRate;
 //C64-machine related:
 unsigned char     VideoStandard; //0:NTSC, 1:PAL (based on the SID-header field)
 unsigned int      CPUfrequency;
 unsigned short    SampleClockRatio; //ratio of CPU-clock and samplerate
 //SID-file related:
 union {
  cRSID_SIDheader*  SIDheader;
  char*             SIDfileData;
 };
 unsigned short    Attenuation;
 char              RealSIDmode;
 char              PSIDdigiMode;
 unsigned char     SubTune;
 unsigned short    LoadAddress;
 unsigned short    InitAddress;
 unsigned short    PlayAddress;
 unsigned short    EndAddress;
 char              TimerSource; //for current subtune, 0:VIC, 1:CIA (as in SID-header)
 //PSID-playback related:
 //char              CIAisSet; //for dynamic CIA setting from player-routine (RealSID substitution)
 int               FrameCycles;
 int               FrameCycleCnt; //this is a substitution in PSID-mode for CIA/VIC counters
 short             PrevRasterLine;
 short             SampleCycleCnt;
 short             TenthSecondCnt;
 char              Finished;
 char              Returned;
 unsigned char     IRQ; //collected IRQ line from devices
 unsigned char     NMI; //collected NMI line from devices

 //Hardware-elements:
 cRSID_CPUinstance CPU;
 cRSID_SIDinstance SID[CRSID_SIDCOUNT_MAX+1];
 cRSID_CIAinstance CIA[CRSID_CIACOUNT+1];
 cRSID_VICinstance VIC;
 //Overlapping system memories, which one is read/written in an address region depends on CPU-port bankselect-bits)
 //Address $00 and $01 - data-direction and data-register of port built into CPU (used as bank-selection) (overriding RAM on C64)
 unsigned char RAMbank[0x10100];  //$0000..$FFFF RAM (and RAM under IO/ROM/CPUport)
 unsigned char IObankWR[0x10100]; //$D000..$DFFF IO-RAM (registers) to write (VIC/SID/CIA/ColorRAM/IOexpansion)
 unsigned char IObankRD[0x10100]; //$D000..$DFFF IO-RAM (registers) to read from (VIC/SID/CIA/ColorRAM/IOexpansion)
 unsigned char ROMbanks[0x10100]; //$1000..$1FFF/$9000..$9FFF (CHARGEN), $A000..$BFFF (BASIC), $E000..$FFFF (KERNAL)
};

cRSID_C64instance cRSID_C64; //the only global object (for faster & simpler access than with struct-pointers, in some places)


#endif //LIBCRSIDC64_HEADER
