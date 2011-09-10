/*
  * This file is based on:
  *    Y8950.cc -- Y8950 emulator from the openMSX team
  *    ported to c by gama
  *
  * The openMSX version is based on:
  *    emu8950.c -- Y8950 emulator written by Mitsutaka Okazaki 2001
  *    heavily rewritten to fit openMSX structure
  */

#include <math.h>
#include "emu8950.h"

#ifdef _MSC_VER
#pragma warning( disable : 4355 )
#endif

#if !defined(ROCKBOX)
	#define EMU8950_CALCUL_TABLES
#else
	#include "opltables.h"
#endif

// dB to Liner table
static short dB2LinTab[(2*DB_MUTE)*2];
// Dynamic range
static unsigned int dphaseNoiseTable[1024][8];
// LFO Table
int pmtable[2][PM_PG_WIDTH];
int amtable[2][AM_PG_WIDTH];

/** WaveTable for each envelope amp. */
static int sintable[PG_WIDTH];
   /** Phase incr table for Attack. */
static unsigned int dphaseARTable[16][16];
/** Phase incr table for Decay and Release. */
static unsigned int dphaseDRTable[16][16];
/** KSL + TL Table. */
#if !defined(ROCKBOX)
static unsigned char tllTable[16][8][1<<TL_BITS][4];
#else
/* Use the table calculated in emu2413 which is identical. */
extern unsigned char tllTable[16][8][1<<TL_BITS][4];
#endif
static int rksTable[2][8][2];
/** Since we wont change clock rate in rockbox we can
    skip this table */
#if !defined(ROCKBOX)
/** Phase incr table for PG. */
static unsigned int dphaseTable[1024][8][16];
#endif

/** Liner to Log curve conversion table (for Attack rate). */
static int AR_ADJUST_TABLE[1<<EG_BITS];

//**************************************************//
//                                                  //
//  Helper functions                                //
//                                                  //
//**************************************************//

#define ALIGN(d, SS, SD) (d*(int)(SS/SD))

inline static int DB_POS(int x)
{
	return (int)(x/DB_STEP);
}

inline static int DB_NEG(int x)
{
	return (int)(2*DB_MUTE+x/DB_STEP);
}

// Cut the lower b bits off
inline static int HIGHBITS(int c, int b)
{
	return c >> b;
}
// Leave the lower b bits
inline static int LOWBITS(int c, int b)
{
	return c & ((1<<b)-1);
}
// Expand x which is s bits to d bits
inline static int EXPAND_BITS(int x, int s, int d)
{
	return x << (d-s);
}

//**************************************************//
//                                                  //
//                  Create tables                   //
//                                                  //
//**************************************************//

// Table for AR to LogCurve. 
static void makeAdjustTable(void)
{
	AR_ADJUST_TABLE[0] = 1 << EG_BITS;
	for (int i = 1; i < (1 << EG_BITS); i++)
	#ifdef EMU8950_CALCUL_TABLES
		AR_ADJUST_TABLE[i] = (int)((double)(1 << EG_BITS) - 1 -
		         (1 << EG_BITS) * log((double)i) / log((double)(1 << EG_BITS))) >> 1;        
	#else
		AR_ADJUST_TABLE[i] = ar_adjust_coeff[i-1];
	#endif
}

// Table for dB(0 -- (1<<DB_BITS)) to Liner(0 -- DB2LIN_AMP_WIDTH) 
static void makeDB2LinTable(void)
{
	int i;
	for (i=0; i < 2*DB_MUTE; i++) {
		dB2LinTab[i] = (i<DB_MUTE) ?
		#ifdef EMU8950_CALCUL_TABLES
			(int)((double)((1<<DB2LIN_AMP_BITS)-1)*pow((double)10,-(double)i*DB_STEP/20)) :
		#else
			db2lin_coeff[i] :
		#endif
			0;
		dB2LinTab[i + 2*DB_MUTE] = -dB2LinTab[i];
	}
}

// Sin Table 
static void makeSinTable(void)
{
	int i;
	for (i=0; i < PG_WIDTH/4; i++)
	#ifdef EMU8950_CALCUL_TABLES
		sintable[i] = lin2db(sin(2.0*MPI*i/PG_WIDTH));
	#else
		sintable[i] = sin_coeff[i];
	#endif
	for (int i=0; i < PG_WIDTH/4; i++)
		sintable[PG_WIDTH/2 - 1 - i] = sintable[i];
	for (int i=0; i < PG_WIDTH/2; i++)
		sintable[PG_WIDTH/2 + i] = 2*DB_MUTE + sintable[i];
}

static void makeDphaseNoiseTable(int sampleRate, int clockRate)
{
	for (int i=0; i<1024; i++)
		for (int j=0; j<8; j++)
			dphaseNoiseTable[i][j] = rate_adjust(i<<j, sampleRate, clockRate);
}

// Table for Pitch Modulator 
static void makePmTable(void)
{
	int i;
	for (i=0; i < PM_PG_WIDTH; i++)
	#ifdef EMU8950_CALCUL_TABLES 
		pmtable[0][i] = (int)((double)PM_AMP * pow(2.,(double)PM_DEPTH*sin(2.0*MPI*i/PM_PG_WIDTH)/1200));
	#else
		pmtable[0][i] = pm0_coeff[i];
	#endif
	for (i=0; i < PM_PG_WIDTH; i++)
	#ifdef EMU8950_CALCUL_TABLES
		pmtable[1][i] = (int)((double)PM_AMP * pow(2.,(double)PM_DEPTH2*sin(2.0*MPI*i/PM_PG_WIDTH)/1200));
	#else
		pmtable[1][i] = pm1_coeff[i];
	#endif
}

// Table for Amp Modulator 
static void makeAmTable(void)
{
	int i;
	for (i=0; i<AM_PG_WIDTH; i++)
	#ifdef EMU8950_CALCUL_TABLES 
		amtable[0][i] = (int)((double)AM_DEPTH/2/DB_STEP * (1.0 + sin(2.0*MPI*i/PM_PG_WIDTH)));
	#else
		amtable[0][i] = am0_coeff[i];
	#endif
	for (i=0; i<AM_PG_WIDTH; i++)
	#ifdef EMU8950_CALCUL_TABLES 
		amtable[1][i] = (int)((double)AM_DEPTH2/2/DB_STEP * (1.0 + sin(2.0*MPI*i/PM_PG_WIDTH)));
	#else
		amtable[1][i] = am1_coeff[i];
	#endif
}

#if !defined(ROCKBOX)
// Phase increment counter table  
static void makeDphaseTable(int sampleRate, int clockRate)
{
	int mltable[16] = {
		1,1*2,2*2,3*2,4*2,5*2,6*2,7*2,8*2,9*2,10*2,10*2,12*2,12*2,15*2,15*2
	};

	int fnum, block, ML;
	for (fnum=0; fnum<1024; fnum++)
		for (block=0; block<8; block++)
			for (ML=0; ML<16; ML++)
				dphaseTable[fnum][block][ML] = 
					rate_adjust((((fnum * mltable[ML]) << block) >> (21 - DP_BITS)), sampleRate, clockRate);
}
#endif

#if !defined(ROCKBOX)
static void makeTllTable(void)
{
	#define dB2(x) (int)((x)*2)
	static int kltable[16] = {
		dB2( 0.000),dB2( 9.000),dB2(12.000),dB2(13.875),
		dB2(15.000),dB2(16.125),dB2(16.875),dB2(17.625),
		dB2(18.000),dB2(18.750),dB2(19.125),dB2(19.500),
		dB2(19.875),dB2(20.250),dB2(20.625),dB2(21.000)
	};

	int fnum, block, TL, KL;
	for (fnum=0; fnum<16; fnum++)
		for (block=0; block<8; block++)
			for (TL=0; TL<64; TL++)
				for (KL=0; KL<4; KL++) {
					if (KL==0) {
						tllTable[fnum][block][TL][KL] = (ALIGN(TL, TL_STEP, EG_STEP) ) >> 1;
					} else {
						int tmp = kltable[fnum] - dB2(3.000) * (7 - block);
						if (tmp <= 0)
							tllTable[fnum][block][TL][KL] = (ALIGN(TL, TL_STEP, EG_STEP) ) >> 1;
						else 
							tllTable[fnum][block][TL][KL] = ((int)((tmp>>(3-KL))/EG_STEP) + ALIGN(TL, TL_STEP, EG_STEP) ) >> 1;
					}
				}
}
#endif

// Rate Table for Attack 
static void makeDphaseARTable(int sampleRate, int clockRate)
{
	int AR, Rks;
	for (AR=0; AR<16; AR++)
		for (Rks=0; Rks<16; Rks++) {
			int RM = AR + (Rks>>2);
			int RL = Rks&3;
			if (RM>15) RM=15;
			switch (AR) { 
			case 0:
				dphaseARTable[AR][Rks] = 0;
				break;
			case 15:
				dphaseARTable[AR][Rks] = EG_DP_WIDTH;
				break;
			default:
				dphaseARTable[AR][Rks] = rate_adjust((3*(RL+4) << (RM+1)), sampleRate, clockRate);
				break;
			}
		}
}

// Rate Table for Decay 
static void makeDphaseDRTable(int sampleRate, int clockRate)
{
	int DR, Rks;
	for (DR=0; DR<16; DR++)
		for (Rks=0; Rks<16; Rks++) {
			int RM = DR + (Rks>>2);
			int RL = Rks&3;
			if (RM>15) RM=15;
			switch (DR) { 
			case 0:
				dphaseDRTable[DR][Rks] = 0;
				break;
			default:
				dphaseDRTable[DR][Rks] = rate_adjust((RL+4) << (RM-1), sampleRate, clockRate);
				break;
			}
		}
}

static void makeRksTable(void)
{
	int fnum9, block, KR;
	for (fnum9=0; fnum9<2; fnum9++)
		for (block=0; block<8; block++)
			for (KR=0; KR<2; KR++) {
				rksTable[fnum9][block][KR] = (KR != 0) ?
					(block<<1) + fnum9:
					 block>>1;
			}
}

//**********************************************************//
//                                                          //
//  Patch                                                   //
//                                                          //
//**********************************************************//


void patchReset(struct Patch* patch)
{
	patch->AM = patch->PM = patch->EG = false;
	patch->KR = patch->ML = patch->KL = patch->TL =
	patch->FB = patch->AR = patch->DR = patch->SL = patch->RR = 0;
}


//**********************************************************//
//                                                          //
//  Slot                                                    //
//                                                          //
//**********************************************************//


static inline void slotUpdatePG(struct Slot* slot)
{
#if defined(ROCKBOX)
	static const int mltable[16] = {
		1,1*2,2*2,3*2,4*2,5*2,6*2,7*2,8*2,9*2,10*2,10*2,12*2,12*2,15*2,15*2
	};

	slot->dphase = ((slot->fnum * mltable[slot->patch.ML]) << slot->block) >> (21 - DP_BITS);
#else
	slot->dphase = dphaseTable[slot->fnum][slot->block][slot->patch.ML];
#endif
}

static inline void slotUpdateTLL(struct Slot* slot)
{
	slot->tll = (int)(tllTable[slot->fnum>>6][slot->block][slot->patch.TL][slot->patch.KL]) << 1;
}

static inline void slotUpdateRKS(struct Slot* slot)
{
	slot->rks = rksTable[slot->fnum>>9][slot->block][slot->patch.KR];
}

static inline void slotUpdateEG(struct Slot* slot)
{
	switch (slot->eg_mode) {
		case ATTACK:
			slot->eg_dphase = dphaseARTable[slot->patch.AR][slot->rks];
			break;
		case DECAY:
			slot->eg_dphase = dphaseDRTable[slot->patch.DR][slot->rks];
			break;
		case SUSTINE:
			slot->eg_dphase = dphaseDRTable[slot->patch.RR][slot->rks];
			break;
		case RELEASE:
			slot->eg_dphase = slot->patch.EG ?
			            dphaseDRTable[slot->patch.RR][slot->rks]:
			            dphaseDRTable[7]       [slot->rks];
			break;
		case SUSHOLD:
		case FINISH:
			slot->eg_dphase = 0;
			break;
	}
}

static inline void slotUpdateAll(struct Slot* slot)
{
	slotUpdatePG(slot);
	slotUpdateTLL(slot);
	slotUpdateRKS(slot);
	slotUpdateEG(slot); // EG should be last 
}

void slotReset(struct Slot* slot)
{
	slot->phase = 0;
	slot->dphase = 0;
	slot->output[0] = 0;
	slot->output[1] = 0;
	slot->feedback = 0;
	slot->eg_mode = FINISH;
	slot->eg_phase = EG_DP_WIDTH;
	slot->eg_dphase = 0;
	slot->rks = 0;
	slot->tll = 0;
	slot->fnum = 0;
	slot->block = 0;
	slot->pgout = 0;
	slot->egout = 0;
	slot->slotStatus = false;
	patchReset(&slot->patch);
	slotUpdateAll(slot);
}

// Slot key on  
static inline void slotOn(struct Slot* slot)
{
	if (!slot->slotStatus) {
		slot->slotStatus = true;
		slot->eg_mode = ATTACK;
		slot->phase = 0;
		slot->eg_phase = 0;
	}
}

// Slot key off 
static inline void slotOff(struct Slot* slot)
{
	if (slot->slotStatus) {
		slot->slotStatus = false;
		if (slot->eg_mode == ATTACK)
			slot->eg_phase = EXPAND_BITS(AR_ADJUST_TABLE[HIGHBITS(slot->eg_phase, EG_DP_BITS-EG_BITS)], EG_BITS, EG_DP_BITS);
		slot->eg_mode = RELEASE;
	}
}


//**********************************************************//
//                                                          //
//  OPLChannel                                                 //
//                                                          //
//**********************************************************//


void channelReset(struct OPLChannel* ch)
{
	slotReset(&ch->mod);
	slotReset(&ch->car);
	ch->alg = false;
}

// Set F-Number ( fnum : 10bit ) 
static void channelSetFnumber(struct OPLChannel* ch, int fnum)
{
	ch->car.fnum = fnum;
	ch->mod.fnum = fnum;
}

// Set Block data (block : 3bit ) 
static void channelSetBlock(struct OPLChannel* ch, int block)
{
	ch->car.block = block;
	ch->mod.block = block;
}

// OPLChannel key on 
static void keyOn(struct OPLChannel* ch)
{
	slotOn(&ch->mod);
	slotOn(&ch->car);
}

// OPLChannel key off 
static void keyOff(struct OPLChannel* ch)
{
	slotOff(&ch->mod);
	slotOff(&ch->car);
}


//**********************************************************//
//                                                          //
//  Y8950                                                   //
//                                                          //
//**********************************************************//

void OPL_init(struct Y8950* this, byte* ramBank, int sampleRam)
{
	this->clockRate = CLK_FREQ;
	
	ADPCM_init(&this->adpcm, this, ramBank, sampleRam);
	
	makePmTable();
	makeAmTable();
	
	makeAdjustTable();
	makeDB2LinTable();
#if !defined(ROCKBOX)
	makeTllTable();
#endif
	makeRksTable();
	makeSinTable();

	int i;
	for (i=0; i<9; i++) {
		// TODO cleanup
		this->slot[i*2+0] = &(this->ch[i].mod);
		this->slot[i*2+1] = &(this->ch[i].car);
		this->ch[i].mod.plfo_am = &this->lfo_am;
		this->ch[i].mod.plfo_pm = &this->lfo_pm;
		this->ch[i].car.plfo_am = &this->lfo_am;
		this->ch[i].car.plfo_pm = &this->lfo_pm;
	}

	OPL_reset(this);
}

void OPL_setSampleRate(struct Y8950* this, int sampleRate, int clockRate)
{
	this->clockRate = clockRate;
	ADPCM_setSampleRate(&this->adpcm, sampleRate, clockRate);
	
#if !defined(ROCKBOX)
	makeDphaseTable(sampleRate, clockRate);
#endif
	makeDphaseARTable(sampleRate, clockRate);
	makeDphaseDRTable(sampleRate, clockRate);
	makeDphaseNoiseTable(sampleRate, clockRate);
	this->pm_dphase = rate_adjust( (int)(PM_SPEED * PM_DP_WIDTH) / (clockRate/72), sampleRate, clockRate);
	this->am_dphase = rate_adjust( (int)(AM_SPEED * AM_DP_WIDTH) / (clockRate/72), sampleRate, clockRate);
}

// Reset whole of opl except patch datas.
void OPL_reset(struct Y8950* this)
{
	int i;
	for (i=0; i<9; i++)
		channelReset(&this->ch[i]);
	this->output[0] = 0;
	this->output[1] = 0;


    this->dacSampleVolume = 0;
    this->dacOldSampleVolume = 0;
    this->dacSampleVolumeSum = 0;
    this->dacCtrlVolume = 0;
    this->dacDaVolume = 0;
    this->dacEnabled = 0;

	this->rythm_mode = false;
	this->am_mode = 0;
	this->pm_mode = 0;
	this->pm_phase = 0;
	this->am_phase = 0;
	this->noise_seed = 0xffff;
	this->noiseA = 0;
	this->noiseB = 0;
	this->noiseA_phase = 0;
	this->noiseB_phase = 0;
	this->noiseA_dphase = 0;
	this->noiseB_dphase = 0;

	for (i = 0; i < 0x100; ++i) 
		this->reg[i] = 0x00;

	this->reg[0x04] = 0x18;
	this->reg[0x19] = 0x0F; // fixes 'Thunderbirds are Go'
	this->status = 0x00;
	this->statusMask = 0;
	/* irq.reset(); */
	
	ADPCM_reset(&this->adpcm);
	OPL_setInternalMute(this, true);	// muted
}


// Drum key on
static inline void keyOn_BD(struct Y8950* this)  { keyOn(&this->ch[6]); }
static inline void keyOn_HH(struct Y8950* this)  { slotOn(&this->ch[7].mod); }
static inline void keyOn_SD(struct Y8950* this)  { slotOn(&this->ch[7].car); }
static inline void keyOn_TOM(struct Y8950* this) { slotOn(&this->ch[8].mod); }
static inline void keyOn_CYM(struct Y8950* this) { slotOn(&this->ch[8].car); }

// Drum key off
static inline void keyOff_BD(struct Y8950* this) { keyOff(&this->ch[6]); }
static inline void keyOff_HH(struct Y8950* this) { slotOff(&this->ch[7].mod); }
static inline void keyOff_SD(struct Y8950* this) { slotOff(&this->ch[7].car); }
static inline void keyOff_TOM(struct Y8950* this){ slotOff(&this->ch[8].mod); }
static inline void keyOff_CYM(struct Y8950* this){ slotOff(&this->ch[8].car); }

// Change Rhythm Mode
static inline void setRythmMode(struct Y8950* this, int data)
{
	bool newMode = (data & 32) != 0;
	if (this->rythm_mode != newMode) {
		this->rythm_mode = newMode;
		if (!this->rythm_mode) {
			// ON->OFF
			this->ch[6].mod.eg_mode = FINISH; // BD1
			this->ch[6].mod.slotStatus = false;
			this->ch[6].car.eg_mode = FINISH; // BD2
			this->ch[6].car.slotStatus = false;
			this->ch[7].mod.eg_mode = FINISH; // HH
			this->ch[7].mod.slotStatus = false;
			this->ch[7].car.eg_mode = FINISH; // SD
			this->ch[7].car.slotStatus = false;
			this->ch[8].mod.eg_mode = FINISH; // TOM
			this->ch[8].mod.slotStatus = false;
			this->ch[8].car.eg_mode = FINISH; // CYM
			this->ch[8].car.slotStatus = false;
		}
	}
}

//********************************************************//
//                                                        //
// Generate wave data                                     //
//                                                        //
//********************************************************//

// Convert Amp(0 to EG_HEIGHT) to Phase(0 to 4PI). 
inline static int wave2_4pi(int e)
{
	int shift =  SLOT_AMP_BITS - PG_BITS - 1;
	if (shift > 0)
		return e >> shift;
	else
		return e << -shift;
}

// Convert Amp(0 to EG_HEIGHT) to Phase(0 to 8PI). 
inline static int wave2_8pi(int e)
{
	int shift = SLOT_AMP_BITS - PG_BITS - 2;
	if (shift > 0)
		return e >> shift;
	else
		return e << -shift;
}

static inline void update_noise(struct Y8950* this)
{
	if (this->noise_seed & 1)
		this->noise_seed ^= 0x24000;
	this->noise_seed >>= 1;
	this->whitenoise = this->noise_seed&1 ? DB_POS(6) : DB_NEG(6);

	this->noiseA_phase += this->noiseA_dphase;
	this->noiseB_phase += this->noiseB_dphase;

	this->noiseA_phase &= (0x40<<11) - 1;
	if ((this->noiseA_phase>>11)==0x3f)
		this->noiseA_phase = 0;
	this->noiseA = this->noiseA_phase&(0x03<<11)?DB_POS(6):DB_NEG(6);

	this->noiseB_phase &= (0x10<<11) - 1;
	this->noiseB = this->noiseB_phase&(0x0A<<11)?DB_POS(6):DB_NEG(6);
}

static inline void update_ampm(struct Y8950* this)
{
	this->pm_phase = (this->pm_phase + this->pm_dphase)&(PM_DP_WIDTH - 1);
	this->am_phase = (this->am_phase + this->am_dphase)&(AM_DP_WIDTH - 1);
	this->lfo_am = amtable[this->am_mode][HIGHBITS(this->am_phase, AM_DP_BITS - AM_PG_BITS)];
	this->lfo_pm = pmtable[this->pm_mode][HIGHBITS(this->pm_phase, PM_DP_BITS - PM_PG_BITS)];
}

static inline void calc_phase(struct Slot* slot)
{
	if (slot->patch.PM)
		slot->phase += (slot->dphase * (*slot->plfo_pm)) >> PM_AMP_BITS;
	else
		slot->phase += slot->dphase;
	slot->phase &= (DP_WIDTH - 1);
	slot->pgout = HIGHBITS(slot->phase, DP_BASE_BITS);
}

static inline void calc_envelope(struct Slot* slot)
{
	#define S2E(x) (ALIGN((unsigned int)(x/SL_STEP),SL_STEP,EG_STEP)<<(EG_DP_BITS-EG_BITS)) 
	static unsigned int SL[16] = {
		S2E( 0), S2E( 3), S2E( 6), S2E( 9), S2E(12), S2E(15), S2E(18), S2E(21),
		S2E(24), S2E(27), S2E(30), S2E(33), S2E(36), S2E(39), S2E(42), S2E(93)
	};
	
	switch (slot->eg_mode) {
	case ATTACK:
		slot->eg_phase += slot->eg_dphase;
		if (EG_DP_WIDTH & slot->eg_phase) {
			slot->egout = 0;
			slot->eg_phase= 0;
			slot->eg_mode = DECAY;
			slotUpdateEG(slot);
		} else {
			slot->egout = AR_ADJUST_TABLE[HIGHBITS(slot->eg_phase, EG_DP_BITS - EG_BITS)];
		}
		break;

	case DECAY:
		slot->eg_phase += slot->eg_dphase;
		slot->egout = HIGHBITS(slot->eg_phase, EG_DP_BITS - EG_BITS);
		if (slot->eg_phase >= SL[slot->patch.SL]) {
			if (slot->patch.EG) {
				slot->eg_phase = SL[slot->patch.SL];
				slot->eg_mode = SUSHOLD;
				slotUpdateEG(slot);
			} else {
				slot->eg_phase = SL[slot->patch.SL];
				slot->eg_mode = SUSTINE;
				slotUpdateEG(slot);
			}
			slot->egout = HIGHBITS(slot->eg_phase, EG_DP_BITS - EG_BITS);
		}
		break;

	case SUSHOLD:
		slot->egout = HIGHBITS(slot->eg_phase, EG_DP_BITS - EG_BITS);
		if (!slot->patch.EG) {
			slot->eg_mode = SUSTINE;
			slotUpdateEG(slot);
		}
		break;

	case SUSTINE:
	case RELEASE:
		slot->eg_phase += slot->eg_dphase;
		slot->egout = HIGHBITS(slot->eg_phase, EG_DP_BITS - EG_BITS); 
		if (slot->egout >= (1<<EG_BITS)) {
			slot->eg_mode = FINISH;
			slot->egout = (1<<EG_BITS) - 1;
		}
		break;

	case FINISH:
		slot->egout = (1<<EG_BITS) - 1;
		break;
	}

	if (slot->patch.AM)
		slot->egout = ALIGN(slot->egout+slot->tll,EG_STEP,DB_STEP) + (*slot->plfo_am);
	else 
		slot->egout = ALIGN(slot->egout+slot->tll,EG_STEP,DB_STEP);
	if (slot->egout >= DB_MUTE)
		slot->egout = DB_MUTE-1;
}

inline static int calc_slot_car(struct Slot* slot, int fm)
{
	calc_envelope(slot); 
	calc_phase(slot);
	if (slot->egout>=(DB_MUTE-1))
		return 0;
	return dB2LinTab[sintable[(slot->pgout+wave2_8pi(fm))&(PG_WIDTH-1)] + slot->egout];
}

inline static int calc_slot_mod(struct Slot* slot)
{
	slot->output[1] = slot->output[0];
	calc_envelope(slot); 
	calc_phase(slot);

	if (slot->egout>=(DB_MUTE-1)) {
		slot->output[0] = 0;
	} else if (slot->patch.FB!=0) {
		int fm = wave2_4pi(slot->feedback) >> (7-slot->patch.FB);
		slot->output[0] = dB2LinTab[sintable[(slot->pgout+fm)&(PG_WIDTH-1)] + slot->egout];
	} else
		slot->output[0] = dB2LinTab[sintable[slot->pgout] + slot->egout];
	
	slot->feedback = (slot->output[1] + slot->output[0])>>1;
	return slot->feedback;
}

// TOM
inline static int calc_slot_tom(struct Slot* slot)
{
	calc_envelope(slot); 
	calc_phase(slot);
	if (slot->egout>=(DB_MUTE-1))
		return 0;
	return dB2LinTab[sintable[slot->pgout] + slot->egout];
}

// SNARE
inline static int calc_slot_snare(struct Slot* slot, int whitenoise)
{
	calc_envelope(slot);
	calc_phase(slot);
	if (slot->egout>=(DB_MUTE-1))
		return 0;
	if (slot->pgout & (1<<(PG_BITS-1))) {
		return (dB2LinTab[slot->egout] + dB2LinTab[slot->egout+whitenoise]) >> 1;
	} else {
		return (dB2LinTab[2*DB_MUTE + slot->egout] + dB2LinTab[slot->egout+whitenoise]) >> 1;
	}
}

// TOP-CYM
inline static int calc_slot_cym(struct Slot* slot, int a, int b)
{
	calc_envelope(slot);
	if (slot->egout>=(DB_MUTE-1)) {
		return 0;
	} else {
		return (dB2LinTab[slot->egout+a] + dB2LinTab[slot->egout+b]) >> 1;
	}
}

// HI-HAT
inline static int calc_slot_hat(struct Slot* slot, int a, int b, int whitenoise)
{
	calc_envelope(slot);
	if (slot->egout>=(DB_MUTE-1)) {
		return 0;
	} else {
		return (dB2LinTab[slot->egout+whitenoise] + dB2LinTab[slot->egout+a] + dB2LinTab[slot->egout+b]) >>2;
	}
}


static inline int calcSample(struct Y8950* this, int channelMask)
{
	// while muted update_ampm() and update_noise() aren't called, probably ok
	update_ampm(this);
	update_noise(this);      

	int mix = 0;

	if (this->rythm_mode) {
		// TODO wasn't in original source either
		calc_phase(&this->ch[7].mod);
		calc_phase(&this->ch[8].car);

		if (channelMask & (1 << 6))
			mix += calc_slot_car(&this->ch[6].car, calc_slot_mod(&this->ch[6].mod));
		if (this->ch[7].mod.eg_mode != FINISH)
			mix += calc_slot_hat(&this->ch[7].mod, this->noiseA, this->noiseB, this->whitenoise);
		if (channelMask & (1 << 7))
			mix += calc_slot_snare(&this->ch[7].car, this->whitenoise);
		if (this->ch[8].mod.eg_mode != FINISH)
			mix += calc_slot_tom(&this->ch[8].mod);
		if (channelMask & (1 << 8))
			mix += calc_slot_cym(&this->ch[8].car, this->noiseA, this->noiseB);	

		channelMask &= (1<< 6) - 1;
		mix *= 2;
	}
	struct OPLChannel *cp;
	for (cp = this->ch; channelMask; channelMask >>=1, cp++) {
		if (channelMask & 1) {
			if (cp->alg)
				mix += calc_slot_car(&cp->car, 0) +
				       calc_slot_mod(&cp->mod);
			else
				mix += calc_slot_car(&cp->car, 
				         calc_slot_mod(&cp->mod));
		}
	}
	
	mix += ADPCM_calcSample(&this->adpcm);

	return (mix*this->maxVolume) >> (DB2LIN_AMP_BITS - 1);
}

static bool checkMuteHelper(struct Y8950* this)
{
	int i;
	struct OPLChannel *ch = this->ch;	
	for (i = 0; i < 6; i++) {
		if (ch[i].car.eg_mode != FINISH) return false;
	}
	if (!this->rythm_mode) {
		for(i = 6; i < 9; i++) {
			if (ch[i].car.eg_mode != FINISH) return false;
		}
	} else {
		if (ch[6].car.eg_mode != FINISH) return false;
		if (ch[7].mod.eg_mode != FINISH) return false;
		if (ch[7].car.eg_mode != FINISH) return false;
		if (ch[8].mod.eg_mode != FINISH) return false;
		if (ch[8].car.eg_mode != FINISH) return false;
	}
	
	return ADPCM_muted(&this->adpcm);
}

static void checkMute(struct Y8950* this)
{
	bool mute = checkMuteHelper(this);
	//PRT_DEBUG("Y8950: muted " << mute);
	OPL_setInternalMute(this, mute);
}

int* OPL_updateBuffer(struct Y8950* this, int length)
{
	//PRT_DEBUG("Y8950: update buffer");

	if (OPL_isInternalMuted(this) && !this->dacEnabled) {
		return 0;
	}

    this->dacCtrlVolume = this->dacSampleVolume - this->dacOldSampleVolume + 0x3fe7 * this->dacCtrlVolume / 0x4000;
    this->dacOldSampleVolume = this->dacSampleVolume;

	int channelMask = 0, i;
	struct OPLChannel *ch = this->ch;
	for (i = 9; i--; ) {
		channelMask <<= 1;
		if (ch[i].car.eg_mode != FINISH) channelMask |= 1;
	}

	int* buf = this->buffer;
	while (length--) {
        int sample = calcSample(this, channelMask);

        this->dacCtrlVolume = 0x3fe7 * this->dacCtrlVolume / 0x4000;
        this->dacDaVolume += 2 * (this->dacCtrlVolume - this->dacDaVolume) / 3;
        sample += 48 * this->dacDaVolume;
		*(buf++) = sample;
	}

    this->dacEnabled = this->dacDaVolume;

	checkMute(this);
	return this->buffer;
}

void OPL_setInternalVolume(struct Y8950* this, short newVolume)
{
	this->maxVolume = newVolume;
}

//**************************************************//
//                                                  //
//                       I/O Ctrl                   //
//                                                  //
//**************************************************//

void OPL_writeReg(struct Y8950* this, byte rg, byte data)
{
	//PRT_DEBUG("Y8950 write " << (int)rg << " " << (int)data);
	int stbl[32] = {
		 0, 2, 4, 1, 3, 5,-1,-1,
		 6, 8,10, 7, 9,11,-1,-1,
		12,14,16,13,15,17,-1,-1,
		-1,-1,-1,-1,-1,-1,-1,-1
	};

	//TODO only for registers that influence sound
	//TODO also ADPCM

	switch (rg & 0xe0) {
	case 0x00: {
		switch (rg) {
		case 0x01: // TEST
			// TODO
			// Y8950 MSX-AUDIO Test register $01 (write only)
			//
			// Bit	Description
			//
			// 7	Reset LFOs - seems to force the LFOs to their initial values (eg.
			//	maximum amplitude, zero phase deviation)
			//
			// 6	something to do with ADPCM - bit 0 of the status register is
			//	affected by setting this bit (PCM BSY)
			//
			// 5	No effect? - Waveform select enable in YM3812 OPL2 so seems
			//	reasonable that this bit wouldn't have been used in OPL
			//
			// 4	No effect?
			//
			// 3	Faster LFOs - increases the frequencies of the LFOs and (maybe)
			//	the timers (cf. YM2151 test register)
			//
			// 2	Reset phase generators - No phase generator output, but envelope
			//	generators still work (can hear a transient when they are gated)
			//
			// 1	No effect?
			//
			// 0	Reset envelopes - Envelope generator outputs forced to maximum,
			//	so all enabled voices sound at maximum
			this->reg[rg] = data;
			break;

		case 0x02: // TIMER1 (reso. 80us)
			this->reg[rg] = data;
			break;

		case 0x03: // TIMER2 (reso. 320us) 
			this->reg[rg] = data;
			break;

		case 0x04: // FLAG CONTROL 
			if (data & R04_IRQ_RESET) {
				OPL_resetStatus(this, 0x78);	// reset all flags
			} else {
				OPL_changeStatusMask(this, (~data) & 0x78);
				this->reg[rg] = data;
			}
			break;

		case 0x06: // (KEYBOARD OUT) 
			this->reg[rg] = data;
			break;
		
		case 0x07: // START/REC/MEM DATA/REPEAT/SP-OFF/-/-/RESET
		case 0x08: // CSM/KEY BOARD SPLIT/-/-/SAMPLE/DA AD/64K/ROM 
		case 0x09: // START ADDRESS (L) 
		case 0x0A: // START ADDRESS (H) 
		case 0x0B: // STOP ADDRESS (L) 
		case 0x0C: // STOP ADDRESS (H) 
		case 0x0D: // PRESCALE (L) 
		case 0x0E: // PRESCALE (H) 
		case 0x0F: // ADPCM-DATA 
		case 0x10: // DELTA-N (L) 
		case 0x11: // DELTA-N (H) 
		case 0x12: // ENVELOP CONTROL
		case 0x1A: // PCM-DATA
			this->reg[rg] = data;
			ADPCM_writeReg(&this->adpcm, rg, data);
			break;
		
		case 0x15: // DAC-DATA  (bit9-2)
			this->reg[rg] = data;
			if (this->reg[0x08] & 0x04) {
                static int damp[] = { 256, 279, 304, 332, 362, 395, 431, 470 };
				int sample = (short)(256 * this->reg[0x15] + this->reg[0x16]) * 128 / damp[this->reg[0x17]];
                this->dacSampleVolume = sample;
                this->dacEnabled = 1;
			}
			break;
		case 0x16: //           (bit1-0)
			this->reg[rg] = data & 0xC0;
			break;
		case 0x17: //           (exponent)
			this->reg[rg] = data & 0x07;
			break;
		
		case 0x18: // I/O-CONTROL (bit3-0)
			// TODO
			// 0 -> input
			// 1 -> output
			this->reg[rg] = data;
			break;
		
		case 0x19: // I/O-DATA (bit3-0)
			// TODO
			this->reg[rg] = data;
			break;
		}
		
		break;
	}
	case 0x20: {
		int s = stbl[rg&0x1f];
		if (s >= 0) {
			this->slot[s]->patch.AM = (data>>7)&1;
			this->slot[s]->patch.PM = (data>>6)&1;
			this->slot[s]->patch.EG = (data>>5)&1;
			this->slot[s]->patch.KR = (data>>4)&1;
			this->slot[s]->patch.ML = (data)&15;
			slotUpdateAll(this->slot[s]);
		}
		this->reg[rg] = data;
		break;
	}
	case 0x40: {
		int s = stbl[rg&0x1f];
		if (s >= 0) {
			this->slot[s]->patch.KL = (data>>6)&3;
			this->slot[s]->patch.TL = (data)&63;
			slotUpdateAll(this->slot[s]);
		}
		this->reg[rg] = data;
		break;
	} 
	case 0x60: {
		int s = stbl[rg&0x1f];
		if (s >= 0) {
			this->slot[s]->patch.AR = (data>>4)&15;
			this->slot[s]->patch.DR = (data)&15;
			slotUpdateEG(this->slot[s]);
		}
		this->reg[rg] = data;
		break;
	} 
	case 0x80: {
		int s = stbl[rg&0x1f];
		if (s >= 0) {
			this->slot[s]->patch.SL = (data>>4)&15;
			this->slot[s]->patch.RR = (data)&15;
			slotUpdateEG(this->slot[s]);
		}
		this->reg[rg] = data;
		break;
	} 
	case 0xa0: {
		if (rg==0xbd) {
			this->am_mode = (data>>7)&1;
			this->pm_mode = (data>>6)&1;
			
			setRythmMode(this, data);
			if (this->rythm_mode) {
				if (data&0x10) keyOn_BD(this);  else keyOff_BD(this);
				if (data&0x08) keyOn_SD(this);  else keyOff_SD(this);
				if (data&0x04) keyOn_TOM(this); else keyOff_TOM(this);
				if (data&0x02) keyOn_CYM(this); else keyOff_CYM(this);
				if (data&0x01) keyOn_HH(this);  else keyOff_HH(this);
			}
			slotUpdateAll(&this->ch[6].mod);
			slotUpdateAll(&this->ch[6].car);
			slotUpdateAll(&this->ch[7].mod);
			slotUpdateAll(&this->ch[7].car);
			slotUpdateAll(&this->ch[8].mod);
			slotUpdateAll(&this->ch[8].car);

			this->reg[rg] = data;
			break;
		}
		if ((rg&0xf) > 8) {
			// 0xa9-0xaf 0xb9-0xbf
			break;
		}
		if (!(rg&0x10)) {
			// 0xa0-0xa8
			int c = rg-0xa0;
			int fNum = data + ((this->reg[rg+0x10]&3)<<8);
			int block = (this->reg[rg+0x10]>>2)&7;
			channelSetFnumber(&this->ch[c], fNum);
			switch (c) {
				case 7: this->noiseA_dphase = dphaseNoiseTable[fNum][block];
					break;
				case 8: this->noiseB_dphase = dphaseNoiseTable[fNum][block];
					break;
			}
			slotUpdateAll(&this->ch[c].car);
			slotUpdateAll(&this->ch[c].mod);
			this->reg[rg] = data;
		} else {
			// 0xb0-0xb8
			int c = rg-0xb0;
			int fNum = ((data&3)<<8) + this->reg[rg-0x10];
			int block = (data>>2)&7;
			channelSetFnumber(&this->ch[c], fNum);
			channelSetBlock(&this->ch[c], block);
			switch (c) {
				case 7: this->noiseA_dphase = dphaseNoiseTable[fNum][block];
					break;
				case 8: this->noiseB_dphase = dphaseNoiseTable[fNum][block];
					break;
			}
			if (data&0x20)
				keyOn(&this->ch[c]);
			else
				keyOff(&this->ch[c]);
			slotUpdateAll(&this->ch[c].mod);
			slotUpdateAll(&this->ch[c].car);
			this->reg[rg] = data;
		}
		break;
	}
	case 0xc0: {
		if (rg > 0xc8)
			break;
		int c = rg-0xC0;
		this->slot[c*2]->patch.FB = (data>>1)&7;
		this->ch[c].alg = data&1;
		this->reg[rg] = data;
	}
	}

	//TODO only for registers that influence sound
	checkMute(this);
}

byte OPL_readReg(struct Y8950* this, byte rg)
{	
	byte result;
	switch (rg) {
		case 0x05: // (KEYBOARD IN)
		    result = 0xff;
			break;
		
		case 0x0f: // ADPCM-DATA
		case 0x13: //  ???
		case 0x14: //  ???
		case 0x1a: // PCM-DATA
			result = ADPCM_readReg(&this->adpcm, rg);
			break;
		
		case 0x19: // I/O DATA   TODO
		    /* result =  ~(switchGetAudio() ?	0 :	0x04); */
		    result =  0;
            break;
		default:
			result = 255;
	}
	//PRT_DEBUG("Y8950 read " << (int)rg<<" "<<(int)result);
	return result;
}

byte OPL_readStatus(struct Y8950* this)
{
	OPL_setStatus(this, STATUS_BUF_RDY);	// temp hack
	byte tmp = this->status & (0x80 | this->statusMask);
	//PRT_DEBUG("Y8950 read status " << (int)tmp);
	return tmp | 0x06;	// bit 1 and 2 are always 1
}


void OPL_setStatus(struct Y8950* this, byte flags)
{
	this->status |= flags;
	if (this->status & this->statusMask) {
		this->status |= 0x80;
		/* irq.set(); */
	}
}
void OPL_resetStatus(struct Y8950* this, byte flags)
{
	this->status &= ~flags;
	if (!(this->status & this->statusMask)) {
		this->status &= 0x7f;
		/* irq.reset(); */
	}
}
void OPL_changeStatusMask(struct Y8950* this, byte newMask)
{
	this->statusMask = newMask;
	this->status &= this->statusMask;
	if (this->status) {
		this->status |= 0x80;
		/* irq.set(); */
	} else {
		this->status &= 0x7f;
		/* irq.reset(); */
	}
}
