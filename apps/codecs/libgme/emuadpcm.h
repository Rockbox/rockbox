#ifndef __Y8950ADPCM_HH__
#define __Y8950ADPCM_HH__

#include "blargg_common.h"
#include "blargg_source.h"
#include "msxtypes.h"

typedef unsigned short word;
typedef unsigned __int64 uint64;
struct Y8950;

struct Y8950Adpcm
{
	struct Y8950* y8950;

	int sampleRate;
	int clockRate;
	
	int ramSize;
	int startAddr;
	int stopAddr;
	int playAddr;
	int addrMask;
	int memPntr;
	bool romBank;
	byte* ramBank;

	bool playing;
	int volume;
	word delta;
	unsigned int nowStep, step;
	int out, output;
	int diff;
	int nextLeveling;
	int sampleStep;
	int volumeWStep;

	byte reg7;
	byte reg15;
};


void ADPCM_init(struct Y8950Adpcm* this_, struct Y8950* y8950, byte* ramBank, int sampleRam);
void ADPCM_reset(struct Y8950Adpcm* this_);
void ADPCM_setSampleRate(struct Y8950Adpcm* this_, int sr, int clk);
bool ADPCM_muted(struct Y8950Adpcm* this_);
void ADPCM_writeReg(struct Y8950Adpcm* this_, byte rg, byte data);
byte ADPCM_readReg(struct Y8950Adpcm* this_, byte rg);
int ADPCM_calcSample(struct Y8950Adpcm* this_);


#endif 
