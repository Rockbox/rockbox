/*
  * This file is based on:
  *    Y8950Adpcm.cc -- Y8950 ADPCM emulator from the openMSX team
  *    ported to c by gama
  *
  * The openMSX version is based on:
  *    emuadpcm.c -- Y8950 ADPCM emulator written by Mitsutaka Okazaki 2001
  *    heavily rewritten to fit openMSX structure
  */

#include <string.h>

#include "emuadpcm.h"
#include "emu8950.h"

// Relative volume between ADPCM part and FM part, 
// value experimentally found by Manuel Bilderbeek
const int ADPCM_VOLUME = 356;

// Bitmask for register 0x07
static const int R07_RESET        = 0x01;
//static const int R07            = 0x02;.      // not used
//static const int R07            = 0x04;.      // not used
const int R07_SP_OFF       = 0x08;
const int R07_REPEAT       = 0x10;
const int R07_MEMORY_DATA  = 0x20;
const int R07_REC          = 0x40;
const int R07_START        = 0x80;

//Bitmask for register 0x08
const int R08_ROM          = 0x01;
const int R08_64K          = 0x02;
const int R08_DA_AD        = 0x04;
const int R08_SAMPL        = 0x08;
//const int R08            = 0x10;.      // not used
//const int R08            = 0x20;.      // not used
const int R08_NOTE_SET     = 0x40;
const int R08_CSM          = 0x80;

const int DMAX = 0x6000;
const int DMIN = 0x7F;
const int DDEF = 0x7F;

const int DECODE_MAX = 32767;
const int DECODE_MIN = -32768;

#define GETA_BITS  14
#define MAX_STEP   (1<<(16+GETA_BITS))


//**************************************************//
//                                                  //
//  Helper functions                                //
//                                                  //
//**************************************************//

static int CLAP(int min, int x, int max)
{
	return (x < min) ? min : ((max < x) ? max : x);
}

//**********************************************************//
//                                                          //
//  Y8950Adpcm                                              //
//                                                          //
//**********************************************************//


void ADPCM_init(struct Y8950Adpcm* this_, struct Y8950* y8950_, byte* ramBank, int sampleRam)

{
	this_->y8950 = y8950_;
	this_->ramBank = ramBank;
	this_->ramSize = sampleRam;
	memset(this_->ramBank, 0xFF, this_->ramSize);
	this_->volume = 0;
}

void restart(struct Y8950Adpcm* this_);
void ADPCM_reset(struct Y8950Adpcm* this_)
{
	this_->playing = false;
	this_->startAddr = 0;
	this_->stopAddr = 7;
	this_->memPntr = 0;
	this_->delta = 0;
	this_->step = 0;
	this_->addrMask = (1 << 19) - 1;
	this_->reg7 = 0;
	this_->reg15 = 0;
	ADPCM_writeReg(this_, 0x12, 255);	// volume
	restart(this_);
}

void ADPCM_setSampleRate(struct Y8950Adpcm* this_, int sr, int clk)
{
	this_->sampleRate = sr;
	this_->clockRate = clk;
}

bool ADPCM_muted(struct Y8950Adpcm* this_)
{
	return (!this_->playing) || (this_->reg7 & R07_SP_OFF);
}

//**************************************************//
//                                                  //
//                       I/O Ctrl                   //
//                                                  //
//**************************************************//

void restart(struct Y8950Adpcm* this_)
{
	this_->playAddr = this_->startAddr & this_->addrMask;
	this_->nowStep = MAX_STEP - this_->step;
	this_->out = this_->output = 0;
	this_->diff = DDEF;
	this_->nextLeveling = 0;
	this_->sampleStep = 0;
	this_->volumeWStep = (int)((long long)this_->volume * this_->step / MAX_STEP);
}

void ADPCM_writeReg(struct Y8950Adpcm* this_, byte rg, byte data)
{
	switch (rg) {
		case 0x07: // START/REC/MEM DATA/REPEAT/SP-OFF/-/-/RESET
			this_->reg7 = data;
			if (this_->reg7 & R07_RESET) {
				this_->playing = false;
			} else if (data & R07_START) {
				this_->playing = true;
				restart(this_);
			}
			break;

		case 0x08: // CSM/KEY BOARD SPLIT/-/-/SAMPLE/DA AD/64K/ROM 
			this_->romBank = data & R08_ROM;
			this_->addrMask = data & R08_64K ? (1<<17)-1 : (1<<19)-1;
			break;

		case 0x09: // START ADDRESS (L)
			this_->startAddr = (this_->startAddr & 0x7F800) | (data << 3);
			this_->memPntr = 0;
			break;
		case 0x0A: // START ADDRESS (H) 
			this_->startAddr = (this_->startAddr & 0x007F8) | (data << 11);
			this_->memPntr = 0;
			break;

		case 0x0B: // STOP ADDRESS (L)
			this_->stopAddr = (this_->stopAddr & 0x7F807) | (data << 3);
			break;
		case 0x0C: // STOP ADDRESS (H) 
			this_->stopAddr = (this_->stopAddr & 0x007FF) | (data << 11);
			break;


		case 0x0F: // ADPCM-DATA
			// TODO check this
			//if ((reg7 & R07_REC) && (reg7 & R07_MEMORY_DATA)) {
			{
				int tmp = ((this_->startAddr + this_->memPntr) & this_->addrMask) / 2;
				tmp = (tmp < this_->ramSize) ? tmp : (tmp & (this_->ramSize - 1)); 
				if (!this_->romBank) {
					this_->ramBank[tmp] = data;
				}
				//PRT_DEBUG("Y8950Adpcm: mem " << tmp << " " << (int)data);
				this_->memPntr += 2;
				if ((this_->startAddr + this_->memPntr) > this_->stopAddr) {
					OPL_setStatus(this_->y8950, STATUS_EOS);
				}
			}
			OPL_setStatus(this_->y8950, STATUS_BUF_RDY);
			break;

		case 0x10: // DELTA-N (L) 
			this_->delta = (this_->delta & 0xFF00) | data;
			this_->step = rate_adjust(this_->delta<<GETA_BITS, this_->sampleRate, this_->clockRate);
			this_->volumeWStep = (int)((long long)this_->volume * this_->step / MAX_STEP);
			break;
		case 0x11: // DELTA-N (H) 
			this_->delta = (this_->delta & 0x00FF) | (data << 8);
			this_->step = rate_adjust(this_->delta<<GETA_BITS, this_->sampleRate, this_->clockRate);
			this_->volumeWStep = (int)((long long)this_->volume * this_->step / MAX_STEP);
			break;

		case 0x12: { // ENVELOP CONTROL 
			int oldVol = this_->volume;
			this_->volume = (data * ADPCM_VOLUME) >> 8;
			if (oldVol != 0) {
				this_->output =     (int)(((long long)this_->output     * this_->volume) / oldVol);
				this_->sampleStep = (int)(((long long)this_->sampleStep * this_->volume) / oldVol);
			}
			this_->volumeWStep = (int)((long long)this_->volume * this_->step / MAX_STEP);
			break;
		}
		case 0x0D: // PRESCALE (L) 
		case 0x0E: // PRESCALE (H) 
		case 0x15: // DAC-DATA  (bit9-2)
		case 0x16: //           (bit1-0)
		case 0x17: //           (exponent)
		case 0x1A: // PCM-DATA
			// not implemented
			break;
	}
}

byte ADPCM_readReg(struct Y8950Adpcm* this_, byte rg)
{
	byte result;
	switch (rg) {
		case 0x0F: { // ADPCM-DATA
			// TODO don't advance pointer when playing???
			int adr = ((this_->startAddr + this_->memPntr) & this_->addrMask) / 2;
			if (this_->romBank || (adr >= this_->ramSize)) {
				result = 0xFF;
			} else {
				result = this_->ramBank[adr];
			}
			this_->memPntr += 2;
			if ((this_->startAddr + this_->memPntr) > this_->stopAddr) {
				OPL_setStatus(this_->y8950, STATUS_EOS);
			}
			break;
		}
		case 0x13: // TODO check
			result = this_->out & 0xFF;
			break;
		case 0x14: // TODO check
			result = this_->out / 256;
			break;
		default:
			result = 255;
	}
	//PRT_DEBUG("Y8950Adpcm: read "<<(int)rg<<" "<<(int)result);
	return result;
}

int ADPCM_calcSample(struct Y8950Adpcm* this_)
{
	// This table values are from ymdelta.c by Tatsuyuki Satoh.
	static const int F1[16] = { 1,   3,   5,   7,   9,  11,  13,  15,
				   -1,  -3,  -5,  -7,  -9, -11, -13, -15};
	static const int F2[16] = {57,  57,  57,  57,  77, 102, 128, 153,
				   57,  57,  57,  57,  77, 102, 128, 153};
	
	if (ADPCM_muted(this_)) {
		return 0;
	}
	this_->nowStep += this_->step;
	if (this_->nowStep >= MAX_STEP) {
		int nowLeveling;
		do {
			this_->nowStep -= MAX_STEP;
			unsigned long val;
			if (!(this_->playAddr & 1)) {
				// n-th nibble
				int tmp = this_->playAddr / 2;
				if (this_->romBank || (tmp >= this_->ramSize)) {
					this_->reg15 = 0xFF;
				} else {
					this_->reg15 = this_->ramBank[tmp];
				}
				val = this_->reg15 >> 4;
			} else {
				// (n+1)-th nibble
				val = this_->reg15 & 0x0F;
			}
			int prevOut = this_->out;
			this_->out = CLAP(DECODE_MIN, this_->out + (this_->diff * F1[val]) / 8,
			           DECODE_MAX);
			this_->diff = CLAP(DMIN, (this_->diff * F2[val]) / 64, DMAX);
			int deltaNext = this_->out - prevOut;
			nowLeveling = this_->nextLeveling;
			this_->nextLeveling = prevOut + deltaNext / 2;
		
			this_->playAddr++;
			if (this_->playAddr > this_->stopAddr) {
				if (this_->reg7 & R07_REPEAT) {
					restart(this_);
				} else {
					this_->playing = false;
					//y8950.setStatus(Y8950::STATUS_EOS);
				}
			}
		} while (this_->nowStep >= MAX_STEP);
		this_->sampleStep = (this_->nextLeveling - nowLeveling) * this_->volumeWStep;
		this_->output = nowLeveling * this_->volume;
		
		/* TODO: Used fixed point math here */
	#if !defined(ROCKBOX)
		this_->output += (int)(((long long)this_->sampleStep * this_->nowStep) / this_->step);
	#endif
	}
	this_->output += this_->sampleStep;
	return this_->output >> 12;
}
