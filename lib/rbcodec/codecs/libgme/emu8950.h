#ifndef __Y8950_HH__
#define __Y8950_HH__

#include "blargg_common.h"
#include "emuadpcm.h"

#define AUDIO_MONO_BUFFER_SIZE    1024

// Dynamic range of envelope
static const double EG_STEP = 0.1875;
#define EG_BITS  9
#define EG_MUTE  (1<<EG_BITS)
// Dynamic range of sustine level
static const double SL_STEP = 3.0;
static const int SL_BITS = 4;
#define SL_MUTE  (1<<SL_BITS)
// Size of Sintable ( 1 -- 18 can be used, but 7 -- 14 recommended.)
#define PG_BITS  10
#define PG_WIDTH  (1<<PG_BITS)
// Phase increment counter
static const int DP_BITS = 19;
#define DP_WIDTH  (1<<DP_BITS)
#define DP_BASE_BITS  (DP_BITS - PG_BITS)
// Bits for envelope phase incremental counter
static const int EG_DP_BITS = 23;
#define EG_DP_WIDTH  (1<<EG_DP_BITS)
// Dynamic range of total level
static const double TL_STEP = 0.75;
#define TL_BITS  6
#define TL_MUTE  (1<<TL_BITS)

static const double DB_STEP = 0.1875;
#define DB_BITS  9
#define DB_MUTE  (1<<DB_BITS)
// PM table is calcurated by PM_AMP * pow(2,PM_DEPTH*sin(x)/1200)
static const int PM_AMP_BITS = 8;
#define PM_AMP  (1<<PM_AMP_BITS)



static const int CLK_FREQ = 3579545;
static const double MPI = 3.14159265358979;
// PM speed(Hz) and depth(cent)
static const double PM_SPEED = 6.4;
static const double PM_DEPTH = (13.75/2);
static const double PM_DEPTH2 = 13.75;
// AM speed(Hz) and depth(dB)
static const double AM_SPEED = 3.7;
static const double AM_DEPTH = 1.0;
static const double AM_DEPTH2 = 4.8;
// Bits for liner value
static const int DB2LIN_AMP_BITS = 11;
#define SLOT_AMP_BITS  DB2LIN_AMP_BITS

// Bits for Pitch and Amp modulator
#define PM_PG_BITS  8
#define PM_PG_WIDTH  (1<<PM_PG_BITS)
static const int PM_DP_BITS = 16;
#define PM_DP_WIDTH  (1<<PM_DP_BITS)
#define AM_PG_BITS  8
#define AM_PG_WIDTH  (1<<AM_PG_BITS)
static const int AM_DP_BITS = 16;
#define AM_DP_WIDTH  (1<<AM_DP_BITS)

// Bitmask for register 0x04
/** Timer1 Start. */
static const int R04_ST1          = 0x01;
/** Timer2 Start. */
static const int R04_ST2          = 0x02;
// not used
//static const int R04            = 0x04;
/** Mask 'Buffer Ready'. */
static const int R04_MASK_BUF_RDY = 0x08;
/** Mask 'End of sequence'. */
static const int R04_MASK_EOS     = 0x10;
/** Mask Timer2 flag. */
static const int R04_MASK_T2      = 0x20;
/** Mask Timer1 flag. */
static const int R04_MASK_T1      = 0x40;
/** IRQ RESET. */
static const int R04_IRQ_RESET    = 0x80;

// Bitmask for status register
#define STATUS_EOS      (R04_MASK_EOS)
#define STATUS_BUF_RDY  (R04_MASK_BUF_RDY)
#define STATUS_T2       (R04_MASK_T2)
#define STATUS_T1       (R04_MASK_T1)

// Definition of envelope mode
enum { ATTACK,DECAY,SUSHOLD,SUSTINE,RELEASE,FINISH };

struct Patch {
	bool AM, PM, EG;
	byte KR; // 0-1
	byte ML; // 0-15
	byte KL; // 0-3
	byte TL; // 0-63
	byte FB; // 0-7
	byte AR; // 0-15
	byte DR; // 0-15
	byte SL; // 0-15
	byte RR; // 0-15
};

void patchReset(struct Patch* p);

struct Slot {
	// OUTPUT
	int feedback;
	/** Output value of slot. */
	int output[5];

	// for Phase Generator (PG)
	/** Phase. */
	unsigned int phase;
	/** Phase increment amount. */
	unsigned int dphase;
	/** Output. */
	int pgout;

	// for Envelope Generator (EG)
	/** F-Number. */
	int fnum;
	/** Block. */
	int block;
	/** Total Level + Key scale level. */
	int tll;
	/** Key scale offset (Rks). */
	int rks;
	/** Current state. */
	int eg_mode;
	/** Phase. */
	unsigned int eg_phase;
	/** Phase increment amount. */
	unsigned int eg_dphase;
	/** Output. */
	int egout;

	bool slotStatus;
	struct Patch patch;

	// refer to Y8950->
	int *plfo_pm;
	int *plfo_am;
};

void slotReset(struct Slot* slot);


struct OPLChannel {
	bool alg;
	struct Slot mod, car;
};

void channelReset(struct OPLChannel* ch);


struct Y8950
{
	int adr;
	int output[2];
	// Register
	byte reg[0x100];
	bool rythm_mode;
	// Pitch Modulator
	int pm_mode;
	unsigned int pm_phase;
	// Amp Modulator
	int am_mode;
	unsigned int am_phase;

	// Noise Generator
	int noise_seed;
	int whitenoise;
	int noiseA;
	int noiseB;
	unsigned int noiseA_phase;
	unsigned int noiseB_phase;
	unsigned int noiseA_dphase;
	unsigned int noiseB_dphase;

	// Channel & Slot
	struct OPLChannel ch[9];
	struct Slot *slot[18];

	unsigned int pm_dphase;
	int lfo_pm;
	unsigned int am_dphase;
	int lfo_am;

	int maxVolume;
	bool internalMuted;
	
	int clockRate;

	/** STATUS Register. */
	byte status;
	/** bit=0 -> masked. */
	byte statusMask;
	/* MsxAudioIRQHelper irq; */

	// ADPCM
	struct Y8950Adpcm adpcm;

	/** 13-bit (exponential) DAC. */
	/* DACSound16S dac13; */
    
    // DAC stuff
    int dacSampleVolume;
    int dacOldSampleVolume;
    int dacSampleVolumeSum;
    int dacCtrlVolume;
    int dacDaVolume;
    int dacEnabled;
    
    // Internal buffer
    int buffer[AUDIO_MONO_BUFFER_SIZE];
};

void OPL_init(struct Y8950* this_, byte* ramBank, int sampleRam);

void OPL_reset(struct Y8950* this_);
void OPL_writeReg(struct Y8950* this_, byte reg, byte data);
byte OPL_readReg(struct Y8950* this_, byte reg);
byte OPL_readStatus(struct Y8950* this_);
static inline void OPL_setInternalMute(struct Y8950* this_, bool muted) { this_->internalMuted = muted; }
static inline bool OPL_isInternalMuted(struct Y8950* this_) { return this_->internalMuted; }
    
void OPL_setSampleRate(struct Y8950* this_, int sampleRate, int clockRate);
int* OPL_updateBuffer(struct Y8950* this_, int length);

// SoundDevice
void OPL_setInternalVolume(struct Y8950* this_, short maxVolume);

void OPL_setStatus(struct Y8950* this_, byte flags);
void OPL_resetStatus(struct Y8950* this_, byte flags);
void OPL_changeStatusMask(struct Y8950* this_, byte newMask);


// Adjust envelope speed which depends on sampling rate
static inline unsigned int rate_adjust(int x, int rate, int clk)
{
	unsigned int tmp = (long long)x * clk / 72 / rate;
//	assert (tmp <= 4294967295U);
	return tmp;
}

#endif
