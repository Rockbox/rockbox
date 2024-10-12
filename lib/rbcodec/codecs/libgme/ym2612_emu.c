// Game_Music_Emu $vers. http://www.slack.net/~ant/

// Based on Gens 2.10 ym2612.c

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdio.h>
#include <math.h>

#include "ym2612_emu.h"

/* Copyright (C) 2002 Stéphane Dallongeville (gens AT consolemul.com) */
/* Copyright (C) 2004-2007 Shay Green. This module is free software; you
can redistribute it and/or modify it under the terms of the GNU Lesser
General Public License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version. This
module is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
details. You should have received a copy of the GNU Lesser General Public
License along with this module; if not, write to the Free Software Foundation,
Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA */

// This is mostly the original source in its C style and all.
//
// Somewhat optimized and simplified. Uses a template to generate the many
// variants of Update_Chan. Rewrote header file. In need of full rewrite by
// someone more familiar with FM sound and the YM2612. Has some inaccuracies
// compared to the Sega Genesis sound, particularly being mixed at such a
// high sample accuracy (the Genesis sounds like it has only 8 bit samples).
// - Shay

// Ported again to c by gama.
// Not sure if performance is better than the original c version.

#if !defined(YM2612_CALCUL_TABLES)
	#include "ymtables.h"
#endif

#ifdef YM2612_CALCUL_TABLES
    #define FREQ_TAB_LOOKUP g->LFO_FREQ_TAB
    #define ENV_TAB_LOOKUP g->LFO_ENV_TAB
#else
    #define FREQ_TAB_LOOKUP lfo_freq_coeff
    #define ENV_TAB_LOOKUP lfo_env_coeff
#endif

const int output_bits = 14;

static const unsigned char DT_DEF_TAB [4 * 32] =
{
// FD = 0
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,

// FD = 1
  0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2,
  2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7, 8, 8, 8, 8,

// FD = 2
  1, 1, 1, 1, 2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5,
  5, 6, 6, 7, 8, 8, 9, 10, 11, 12, 13, 14, 16, 16, 16, 16,

// FD = 3
  2, 2, 2, 2, 2, 3, 3, 3, 4, 4, 4, 5, 5, 6, 6, 7,
  8 , 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 20, 22, 22, 22, 22
};

static const unsigned char FKEY_TAB [16] =
{ 
	0, 0, 0, 0,
	0, 0, 0, 1,
	2, 3, 3, 3,
	3, 3, 3, 3
};

static const unsigned char LFO_AMS_TAB [4] =
{
	31, 4, 1, 0
};

static const unsigned char LFO_FMS_TAB [8] =
{
	LFO_FMS_BASE * 0, LFO_FMS_BASE * 1,
	LFO_FMS_BASE * 2, LFO_FMS_BASE * 3,
	LFO_FMS_BASE * 4, LFO_FMS_BASE * 6,
	LFO_FMS_BASE * 12, LFO_FMS_BASE * 24
};

int in0, in1, in2, in3;            // current phase calculation
// int en0, en1, en2, en3;            // current enveloppe calculation

static inline void set_seg( struct slot_t* s, int seg )
{
	s->env_xor = 0;
	s->env_max = INT_MAX;
	s->SEG = seg;
	if ( seg & 4 )
	{
		s->env_xor = ENV_MASK;
		s->env_max = ENV_MASK;
	}
}

static inline void YM2612_Special_Update(void) { }

static void KEY_ON( struct channel_* ch, struct tables_t *g, int nsl )
{
	struct slot_t *SL = &(ch->SLOT [nsl]);  // on recupere le bon pointeur de slot
	
	if (SL->Ecurp == RELEASE)       // la touche est-elle rel'chee ?
	{
		SL->Fcnt = 0;

		// Fix Ecco 2 splash sound
		
		SL->Ecnt = (g->DECAY_TO_ATTACK [g->ENV_TAB [SL->Ecnt >> ENV_LBITS]] + ENV_ATTACK) & SL->ChgEnM;
		SL->ChgEnM = ~0;

//      SL->Ecnt = g.DECAY_TO_ATTACK [g.ENV_TAB [SL->Ecnt >> ENV_LBITS]] + ENV_ATTACK;
//      SL->Ecnt = 0;

		SL->Einc = SL->EincA;
		SL->Ecmp = ENV_DECAY;
		SL->Ecurp = ATTACK;
	}
}


static void KEY_OFF( struct channel_* ch, struct tables_t *g, int nsl )
{
	struct slot_t *SL = &(ch->SLOT [nsl]);  // on recupere le bon pointeur de slot
	
	if (SL->Ecurp != RELEASE)       // la touche est-elle appuyee ?
	{
		if (SL->Ecnt < ENV_DECAY)   // attack phase ?
		{
			SL->Ecnt = (g->ENV_TAB [SL->Ecnt >> ENV_LBITS] << ENV_LBITS) + ENV_DECAY;
		}

		SL->Einc = SL->EincR;
		SL->Ecmp = ENV_END;
		SL->Ecurp = RELEASE;
	}
}


static int SLOT_SET( struct Ym2612_Impl* impl, int Adr, int data )
{
	int nch = Adr & 3;
	if ( nch == 3 )
		return 1;
	
	struct tables_t *g = &impl->g;
	struct state_t *YM2612 = &impl->YM2612;
	struct channel_* ch = &YM2612->CHANNEL [nch + (Adr & 0x100 ? 3 : 0)];
	struct slot_t* sl = &ch->SLOT [(Adr >> 2) & 3];

	switch ( Adr & 0xF0 )
	{
		case 0x30:
			if ( (sl->MUL = (data & 0x0F)) != 0 ) sl->MUL <<= 1;
			else sl->MUL = 1;

			sl->DT = (int*) g->DT_TAB [(data >> 4) & 7];

			ch->SLOT [0].Finc = -1;

			break;

		case 0x40:
			sl->TL = data & 0x7F;

			// SOR2 do a lot of TL adjustement and this fix R.Shinobi jump sound...
			YM2612_Special_Update();

#if ((ENV_HBITS - 7) < 0)
			sl->TLL = sl->TL >> (7 - ENV_HBITS);
#else
			sl->TLL = sl->TL << (ENV_HBITS - 7);
#endif

			break;

		case 0x50:
			sl->KSR_S = 3 - (data >> 6);

			ch->SLOT [0].Finc = -1;

			if (data &= 0x1F) sl->AR = (int*) &g->AR_TAB [data << 1];
			else sl->AR = (int*) &g->NULL_RATE [0];

			sl->EincA = sl->AR [sl->KSR];
			if (sl->Ecurp == ATTACK) sl->Einc = sl->EincA;
			break;

		case 0x60:
			if ( (sl->AMSon = (data & 0x80)) != 0 ) sl->AMS = ch->AMS;
			else sl->AMS = 31;

			if (data &= 0x1F) sl->DR = (int*) &g->DR_TAB [data << 1];
			else sl->DR = (int*) &g->NULL_RATE [0];

			sl->EincD = sl->DR [sl->KSR];
			if (sl->Ecurp == DECAY) sl->Einc = sl->EincD;
			break;

		case 0x70:
			if (data &= 0x1F) sl->SR = (int*) &g->DR_TAB [data << 1];
			else sl->SR = (int*) &g->NULL_RATE [0];

			sl->EincS = sl->SR [sl->KSR];
			if ((sl->Ecurp == SUBSTAIN) && (sl->Ecnt < ENV_END)) sl->Einc = sl->EincS;
			break;

		case 0x80:
			sl->SLL = g->SL_TAB [data >> 4];

			sl->RR = (int*) &g->DR_TAB [((data & 0xF) << 2) + 2];

			sl->EincR = sl->RR [sl->KSR];
			if ((sl->Ecurp == RELEASE) && (sl->Ecnt < ENV_END)) sl->Einc = sl->EincR;
			break;

		case 0x90:
			// SSG-EG envelope shapes :
			/*
			   E  At Al H
			  
			   1  0  0  0  \\\\
			   1  0  0  1  \___
			   1  0  1  0  \/\/
			   1  0  1  1  \
			   1  1  0  0  ////
			   1  1  0  1  /
			   1  1  1  0  /\/\
			   1  1  1  1  /___
			  
			   E  = SSG-EG enable
			   At = Start negate
			   Al = Altern
			   H  = Hold */

			set_seg( sl, (data & 8) ? (data & 0x0F) : 0 );
			break;
	}

	return 0;
}


static int CHANNEL_SET( struct state_t* YM2612, int Adr, int data )
{
	int num = Adr & 3;
	if ( num == 3 )
		return 1;
	
	struct channel_* ch = &YM2612->CHANNEL [num + (Adr & 0x100 ? 3 : 0)];
	
	switch ( Adr & 0xFC )
	{
		case 0xA0:
			YM2612_Special_Update();

			ch->FNUM [0] = (ch->FNUM [0] & 0x700) + data;
			ch->KC [0] = (ch->FOCT [0] << 2) | FKEY_TAB [ch->FNUM [0] >> 7];

			ch->SLOT [0].Finc = -1;
			break;

		case 0xA4:
			YM2612_Special_Update();

			ch->FNUM [0] = (ch->FNUM [0] & 0x0FF) + ((data & 0x07) << 8);
			ch->FOCT [0] = (data & 0x38) >> 3;
			ch->KC [0] = (ch->FOCT [0] << 2) | FKEY_TAB [ch->FNUM [0] >> 7];

			ch->SLOT [0].Finc = -1;
			break;

		case 0xA8:
			if ( Adr < 0x100 )
			{
				num++;

				YM2612_Special_Update();

				YM2612->CHANNEL [2].FNUM [num] = (YM2612->CHANNEL [2].FNUM [num] & 0x700) + data;
				YM2612->CHANNEL [2].KC [num] = (YM2612->CHANNEL [2].FOCT [num] << 2) |
						FKEY_TAB [YM2612->CHANNEL [2].FNUM [num] >> 7];

				YM2612->CHANNEL [2].SLOT [0].Finc = -1;
			}
			break;

		case 0xAC:
			if ( Adr < 0x100 )
			{
				num++;

				YM2612_Special_Update();

				YM2612->CHANNEL [2].FNUM [num] = (YM2612->CHANNEL [2].FNUM [num] & 0x0FF) + ((data & 0x07) << 8);
				YM2612->CHANNEL [2].FOCT [num] = (data & 0x38) >> 3;
				YM2612->CHANNEL [2].KC [num] = (YM2612->CHANNEL [2].FOCT [num] << 2) |
						FKEY_TAB [YM2612->CHANNEL [2].FNUM [num] >> 7];

				YM2612->CHANNEL [2].SLOT [0].Finc = -1;
			}
			break;

		case 0xB0:
			if ( ch->ALGO != (data & 7) )
			{
				// Fix VectorMan 2 heli sound (level 1)
				YM2612_Special_Update();

				ch->ALGO = data & 7;
				
				ch->SLOT [0].ChgEnM = 0;
				ch->SLOT [1].ChgEnM = 0;
				ch->SLOT [2].ChgEnM = 0;
				ch->SLOT [3].ChgEnM = 0;
			}

			ch->FB = 9 - ((data >> 3) & 7);                              // Real thing ?

//          if (ch->FB = ((data >> 3) & 7)) ch->FB = 9 - ch->FB;       // Thunder force 4 (music stage 8), Gynoug, Aladdin bug sound...
//          else ch->FB = 31;
			break;

		case 0xB4: {
			YM2612_Special_Update();
			
			ch->LEFT = 0 - ((data >> 7) & 1);
			ch->RIGHT = 0 - ((data >> 6) & 1);
			
			ch->AMS = LFO_AMS_TAB [(data >> 4) & 3];
			ch->FMS = LFO_FMS_TAB [data & 7];
			
			int i;
			for ( i = 0; i < 4; i++ )
			{
				struct slot_t* sl = &ch->SLOT [i];
				sl->AMS = (sl->AMSon ? ch->AMS : 31);
			}
			break;
		}
	}
	
	return 0;
}


static int YM_SET( struct Ym2612_Impl* impl, int Adr, int data )
{
	struct state_t* YM2612 = &impl->YM2612;
	struct tables_t* g = &impl->g;
	switch ( Adr )
	{
		case 0x22:
			if (data & 8) // LFO enable
			{
				// Cool Spot music 1, LFO modified severals time which
				// distord the sound, have to check that on a real genesis...

				g->LFOinc = g->LFO_INC_TAB [data & 7];
			}
			else
			{
				g->LFOinc = g->LFOcnt = 0;
			}
			break;

		case 0x24:
			YM2612->TimerA = (YM2612->TimerA & 0x003) | (((int) data) << 2);

			if (YM2612->TimerAL != (1024 - YM2612->TimerA) << 12)
			{
				YM2612->TimerAcnt = YM2612->TimerAL = (1024 - YM2612->TimerA) << 12;
			}
			break;

		case 0x25:
			YM2612->TimerA = (YM2612->TimerA & 0x3FC) | (data & 3);

			if (YM2612->TimerAL != (1024 - YM2612->TimerA) << 12)
			{
				YM2612->TimerAcnt = YM2612->TimerAL = (1024 - YM2612->TimerA) << 12;
			}
			break;

		case 0x26:
			YM2612->TimerB = data;

			if (YM2612->TimerBL != (256 - YM2612->TimerB) << (4 + 12))
			{
				YM2612->TimerBcnt = YM2612->TimerBL = (256 - YM2612->TimerB) << (4 + 12);
			}
			break;

		case 0x27:
			// Parametre divers
			// b7 = CSM MODE
			// b6 = 3 slot mode
			// b5 = reset b
			// b4 = reset a
			// b3 = timer enable b
			// b2 = timer enable a
			// b1 = load b
			// b0 = load a

			if ((data ^ YM2612->Mode) & 0x40)
			{
				// We changed the channel 2 mode, so recalculate phase step
				// This fix the punch sound in Street of Rage 2

				YM2612_Special_Update();

				YM2612->CHANNEL [2].SLOT [0].Finc = -1;      // recalculate phase step
			}

//          if ((data & 2) && (YM2612->Status & 2)) YM2612->TimerBcnt = YM2612->TimerBL;
//          if ((data & 1) && (YM2612->Status & 1)) YM2612->TimerAcnt = YM2612->TimerAL;

//          YM2612->Status &= (~data >> 4);                  // Reset du Status au cas ou c'est demande
			YM2612->Status &= (~data >> 4) & (data >> 2);    // Reset Status

			YM2612->Mode = data;
			break;

		case 0x28: {
			int nch = data & 3;
			if ( nch == 3 )
				return 1;
			if ( data & 4 )
				nch += 3;
			struct channel_* ch = &YM2612->CHANNEL [nch];

			YM2612_Special_Update();

			if (data & 0x10) KEY_ON(ch, g, S0);    // On appuie sur la touche pour le slot 1
			else KEY_OFF(ch, g, S0);               // On rel'che la touche pour le slot 1
			if (data & 0x20) KEY_ON(ch, g, S1);    // On appuie sur la touche pour le slot 3
			else KEY_OFF(ch, g, S1);               // On rel'che la touche pour le slot 3
			if (data & 0x40) KEY_ON(ch, g, S2);    // On appuie sur la touche pour le slot 2
			else KEY_OFF(ch, g, S2);               // On rel'che la touche pour le slot 2
			if (data & 0x80) KEY_ON(ch, g, S3);    // On appuie sur la touche pour le slot 4
			else KEY_OFF(ch, g, S3);               // On rel'che la touche pour le slot 4
			break;
		}
		
		case 0x2B:
			if (YM2612->DAC ^ (data & 0x80)) YM2612_Special_Update();

			YM2612->DAC = data & 0x80;   // activation/desactivation du DAC
			break;
	}
	
	return 0;
}

void impl_reset( struct Ym2612_Impl* impl );
static void impl_set_rate( struct Ym2612_Impl* impl, int sample_rate, int clock_rate )
{
	assert( sample_rate );
	assert( !clock_rate || clock_rate > sample_rate );

	int i;

	// 144 = 12 * (prescale * 2) = 12 * 6 * 2
	// prescale set to 6 by default

	int Frequency = (clock_rate ? (int)((FP_ONE_CLOCK * clock_rate) / sample_rate / 144) : (int)FP_ONE_CLOCK);
	if ( llabs( Frequency - FP_ONE_CLOCK ) < 1 )
		Frequency = FP_ONE_CLOCK;
	impl->YM2612.TimerBase = Frequency;

    /* double Frequence = (double)Frequency / FP_ONE_CLOCK; */

	// Tableau TL :
	// [0     -  4095] = +output  [4095  - ...] = +output overflow (fill with 0)
	// [12288 - 16383] = -output  [16384 - ...] = -output overflow (fill with 0)

#ifdef YM2612_USE_TL_TAB
	for ( i = 0; i < TL_LENGHT; i++ )
	{
		if (i >= PG_CUT_OFF)    // YM2612 cut off sound after 78 dB (14 bits output ?)
		{
			impl->g.TL_TAB [TL_LENGHT + i] = impl->g.TL_TAB [i] = 0;
		}
		else
		{
			// Decibel -> Voltage
		#ifdef YM2612_CALCUL_TABLES
			impl->g.TL_TAB [i] = (int) (MAX_OUT / pow( 10.0, ENV_STEP / 20.0f * i ));
		#else
			impl->g.TL_TAB [i] = tl_coeff [i];
		#endif
			impl->g.TL_TAB [TL_LENGHT + i] = -impl->g.TL_TAB [i];
		}
	}
#endif
	
	// Tableau SIN :
	// impl->g.SIN_TAB [x] [y] = sin(x) * y; 
	// x = phase and y = volume

	impl->g.SIN_TAB [0] = impl->g.SIN_TAB [SIN_LENGHT / 2] = PG_CUT_OFF;

	for ( i = 1; i <= SIN_LENGHT / 4; i++ )
	{
		// Sinus in dB
	#ifdef YM2612_CALCUL_TABLES
		double x = 20 * log10( 1 / sin( 2.0 * PI * i / SIN_LENGHT ) ); // convert to dB

		int j = (int) (x / ENV_STEP);                       // Get TL range

		if (j > PG_CUT_OFF) j = (int) PG_CUT_OFF;
	#else
		int j = sindb_coeff [i-1];
	#endif

		impl->g.SIN_TAB [i] = impl->g.SIN_TAB [(SIN_LENGHT / 2) - i] = j;
		impl->g.SIN_TAB [(SIN_LENGHT / 2) + i] = impl->g.SIN_TAB [SIN_LENGHT - i] = TL_LENGHT + j;
	}

    #ifdef YM2612_CALCUL_TABLES
	// Tableau LFO (LFO wav) :
	for ( i = 0; i < LFO_LENGHT; i++ )
	{
		double x = 1 + sin( 2.0 * PI * i * (1.0 / LFO_LENGHT) );    // Sinus
		x *= 11.8 / ENV_STEP / 2;       // ajusted to MAX enveloppe modulation

		impl->g.LFO_ENV_TAB [i] = (int) x;

		x = sin( 2.0 * PI * i * (1.0 / LFO_LENGHT) );   // Sinus
		x *= (1 << (LFO_HBITS - 1)) - 1;

		impl->g.LFO_FREQ_TAB [i] = (int) x;
	}
    #endif

	// Tableau Enveloppe :
	// impl->g.ENV_TAB [0] -> impl->g.ENV_TAB [ENV_LENGHT - 1]              = attack curve
	// impl->g.ENV_TAB [ENV_LENGHT] -> impl->g.ENV_TAB [2 * ENV_LENGHT - 1] = decay curve

	for ( i = 0; i < ENV_LENGHT; i++ )
	{
		// Attack curve (x^8 - music level 2 Vectorman 2)
	#if defined(ROCKBOX)
        int k;
        int prescale = (31 - 2*ENV_HBITS); /* used to gain higher precision */
        int x = ENV_LENGHT * (1 << prescale);
        for ( k = 0; k < 8; ++k)
        {
            x = ( x * ((ENV_LENGHT - 1) - i) ) / ENV_LENGHT;
        }
        x >>= prescale;
	#else
		double x = pow( ((ENV_LENGHT - 1) - i) / (double) ENV_LENGHT, 8.0 );
        x *= ENV_LENGHT;
	#endif

		impl->g.ENV_TAB [i] = (int) x;

		// Decay curve (just linear)
		impl->g.ENV_TAB [ENV_LENGHT + i] = i;
	}
	for ( i = 0; i < 8; i++ )
		impl->g.ENV_TAB [i + ENV_LENGHT * 2] = 0;
	
	impl->g.ENV_TAB [ENV_END >> ENV_LBITS] = ENV_LENGHT - 1;      // for the stopped state

	// Tableau pour la conversion Attack -> Decay and Decay -> Attack
	
	int j = ENV_LENGHT - 1;
	for ( i = 0; i < ENV_LENGHT; i++ )
	{
		while ( j && impl->g.ENV_TAB [j] < i )
			j--;

		impl->g.DECAY_TO_ATTACK [i] = j << ENV_LBITS;
	}

	// Tableau pour le Substain Level

	for ( i = 0; i < 15; i++ )
	{
		int x = i * 3 * (int)( (1 << ENV_LBITS) / ENV_STEP); // 3 and not 6 (Mickey Mania first music for test)

		impl->g.SL_TAB [i] = x + ENV_DECAY;
	}

	impl->g.SL_TAB [15] = ((ENV_LENGHT - 1) << ENV_LBITS) + ENV_DECAY; // special case : volume off

	// Tableau Frequency Step

	{
		// * 1 / 2 because MUL = value * 2
	#if SIN_LBITS + SIN_HBITS - (21 - 7) < 0
		/* double const factor = Frequence / 2.0 / (1 << ((21 - 7) - SIN_LBITS - SIN_HBITS)); */
        int const factor = (int)(Frequency / 2 / (1 << ((21 - 7) - SIN_LBITS - SIN_HBITS)) / FP_ONE_CLOCK);
	#else
		/* double const factor = Frequence / 2.0 * (1 << (SIN_LBITS + SIN_HBITS - (21 - 7))); */
        int const factor = (int)(Frequency / 2 * (1 << (SIN_LBITS + SIN_HBITS - (21 - 7))) / FP_ONE_CLOCK);
	#endif
		for ( i = 0; i < 2048; i++ )
        {
			impl->g.FINC_TAB [i] = i * factor;
        }
	}

	// Tableaux Attack & Decay Rate

	for ( i = 0; i < 4; i++ )
	{
		impl->g.AR_TAB [i] = 0;
		impl->g.DR_TAB [i] = 0;
	}

	for ( i = 0; i < 60; i++ )
	{        
        long long x =
				(4LL + ((i & 3))) *             // bits 0-1 : 4*(x1.00, x1.25, x1.50, x1.75)
				(ENV_LENGHT << ENV_LBITS) *     // on ajuste pour le tableau impl->g.ENV_TAB
				Frequency  *
				(1 << (i >> 2)) /               // bits 2-5 : shift bits (x2^0 - x2^15)
                FP_ONE_CLOCK / 4;
                
        long long x_AR = x / AR_RATE;
        long long x_DR = x / DR_RATE;
		
		impl->g.AR_TAB [i + 4] = (unsigned int) ( x_AR > ((1LL<<32) - 1) ? ((1LL<<32) - 1) : x_AR );
		impl->g.DR_TAB [i + 4] = (unsigned int) ( x_DR > ((1LL<<32) - 1) ? ((1LL<<32) - 1) : x_DR );
    }

	for ( i = 64; i < 96; i++ )
	{
		impl->g.AR_TAB [i] = impl->g.AR_TAB [63];
		impl->g.DR_TAB [i] = impl->g.DR_TAB [63];

		impl->g.NULL_RATE [i - 64] = 0;
	}
	
	for ( i = 96; i < 128; i++ )
		impl->g.AR_TAB [i] = 0;

	// Tableau Detune
	{
	#if SIN_LBITS + SIN_HBITS - 21 < 0
		/* double const factor = 1.0 / (1 << (21 - SIN_LBITS - SIN_HBITS)) * Frequence; */
        int const factor = Frequency / (1 << (21 - SIN_LBITS - SIN_HBITS)) / FP_ONE_CLOCK;
	#else
		/* double const factor = (1 << (SIN_LBITS + SIN_HBITS - 21)) * Frequence; */
        int const factor = Frequency * (1 << (SIN_LBITS + SIN_HBITS - 21)) / FP_ONE_CLOCK;
	#endif
		for ( i = 0; i < 4; i++ )
		{
			int j;
			for ( j = 0; j < 32; j++ )
			{
				/* double y = DT_DEF_TAB [(i << 5) + j] * factor; */
                int y = DT_DEF_TAB [(i << 5) + j] * factor;
				
				impl->g.DT_TAB [i + 0] [j] = (int)  y;
				impl->g.DT_TAB [i + 4] [j] = (int) -y;
			}
		}
	}

	// Tableau LFO
	impl->g.LFO_INC_TAB [0] = (int) (3.98 * (1 << (LFO_HBITS + LFO_LBITS))) / sample_rate;
	impl->g.LFO_INC_TAB [1] = (int) (5.56 * (1 << (LFO_HBITS + LFO_LBITS))) / sample_rate;
	impl->g.LFO_INC_TAB [2] = (int) (6.02 * (1 << (LFO_HBITS + LFO_LBITS))) / sample_rate;
	impl->g.LFO_INC_TAB [3] = (int) (6.37 * (1 << (LFO_HBITS + LFO_LBITS))) / sample_rate;
	impl->g.LFO_INC_TAB [4] = (int) (6.88 * (1 << (LFO_HBITS + LFO_LBITS))) / sample_rate;
	impl->g.LFO_INC_TAB [5] = (int) (9.63 * (1 << (LFO_HBITS + LFO_LBITS))) / sample_rate;
	impl->g.LFO_INC_TAB [6] = (int) (48.1 * (1 << (LFO_HBITS + LFO_LBITS))) / sample_rate;
	impl->g.LFO_INC_TAB [7] = (int) (72.2 * (1 << (LFO_HBITS + LFO_LBITS))) / sample_rate;
	
	impl_reset( impl );
}

const char* Ym2612_set_rate( struct Ym2612_Emu* this, int sample_rate, int clock_rate )
{
// Only set rates if necessary
#if defined(ROCKBOX)
	static int last_sample_rate = 0, last_clock_rate = 0;
	if (last_sample_rate == sample_rate && last_clock_rate == clock_rate) return 0;
#endif
	memset( &this->impl.YM2612, 0, sizeof this->impl.YM2612 );
	impl_set_rate( &this->impl, sample_rate, clock_rate );
	
	return 0;
}

static inline void write0( struct Ym2612_Impl* impl, int opn_addr, int data )
{
	assert( (unsigned) data <= 0xFF );
	
	if ( opn_addr < 0x30 )
	{
		impl->YM2612.REG [0] [opn_addr] = data;
		YM_SET( impl, opn_addr, data );
	}
	else if ( impl->YM2612.REG [0] [opn_addr] != data )
	{
		impl->YM2612.REG [0] [opn_addr] = data;
		
		if ( opn_addr < 0xA0 )
			SLOT_SET( impl, opn_addr, data );
		else
			CHANNEL_SET( &impl->YM2612, opn_addr, data );
	}
}

static inline void write1( struct Ym2612_Impl* impl, int opn_addr, int data )
{
	assert( (unsigned) data <= 0xFF );
	
	if ( opn_addr >= 0x30 && impl->YM2612.REG [1] [opn_addr] != data )
	{
		impl->YM2612.REG [1] [opn_addr] = data;

		if ( opn_addr < 0xA0 )
			SLOT_SET( impl, opn_addr + 0x100, data );
		else
			CHANNEL_SET( &impl->YM2612, opn_addr + 0x100, data );
	}
}

void impl_reset( struct Ym2612_Impl* impl )
{
	impl->g.LFOcnt = 0;
	impl->YM2612.TimerA = 0;
	impl->YM2612.TimerAL = 0;
	impl->YM2612.TimerAcnt = 0;
	impl->YM2612.TimerB = 0;
	impl->YM2612.TimerBL = 0;
	impl->YM2612.TimerBcnt = 0;
	impl->YM2612.DAC = 0;

	impl->YM2612.Status = 0;

	int i;
	for ( i = 0; i < ym2612_channel_count; i++ )
	{
		struct channel_* ch = &impl->YM2612.CHANNEL [i];
		
		ch->LEFT = ~0;
		ch->RIGHT = ~0;
		ch->ALGO = 0;
		ch->FB = 31;
		ch->FMS = 0;
		ch->AMS = 0;

		int j;
		for ( j = 0 ;j < 4 ; j++ )
		{
			ch->S0_OUT [j] = 0;
			ch->FNUM [j] = 0;
			ch->FOCT [j] = 0;
			ch->KC [j] = 0;

			ch->SLOT [j].Fcnt = 0;
			ch->SLOT [j].Finc = 0;
			ch->SLOT [j].Ecnt = ENV_END;     // Put it at the end of Decay phase...
			ch->SLOT [j].Einc = 0;
			ch->SLOT [j].Ecmp = 0;
			ch->SLOT [j].Ecurp = RELEASE;

			ch->SLOT [j].ChgEnM = 0;
		}
	}

	for ( i = 0; i < 0x100; i++ )
	{
		impl->YM2612.REG [0] [i] = -1;
		impl->YM2612.REG [1] [i] = -1;
	}

	for ( i = 0xB6; i >= 0xB4; i-- )
	{
		write0( impl, i, 0xC0 );
		write1( impl, i, 0xC0 );
	}

	for ( i = 0xB2; i >= 0x22; i-- )
	{
		write0( impl, i, 0 );
		write1( impl, i, 0 );
	}
	
	write0( impl, 0x2A, 0x80 );
}

void Ym2612_reset( struct Ym2612_Emu* this )
{
	impl_reset( &this->impl );
}

void Ym2612_write0( struct Ym2612_Emu* this, int addr, int data )
{
	write0( &this->impl, addr, data );
}

void Ym2612_write1( struct Ym2612_Emu* this, int addr, int data )
{
	write1( &this->impl, addr, data );
}

void Ym2612_mute_voices( struct Ym2612_Emu* this, int mask ) { this->impl.mute_mask = mask; }

static void update_envelope_( struct slot_t* sl )
{
	switch ( sl->Ecurp )
	{
	case 0:
		// Env_Attack_Next
		
		// Verified with Gynoug even in HQ (explode SFX)
		sl->Ecnt = ENV_DECAY;

		sl->Einc = sl->EincD;
		sl->Ecmp = sl->SLL;
		sl->Ecurp = DECAY;
		break;
	
	case 1:
		// Env_Decay_Next
		
		// Verified with Gynoug even in HQ (explode SFX)
		sl->Ecnt = sl->SLL;

		sl->Einc = sl->EincS;
		sl->Ecmp = ENV_END;
		sl->Ecurp = SUBSTAIN;
		break;
	
	case 2:
		// Env_Substain_Next(slot_t *SL)
		if (sl->SEG & 8)    // SSG envelope type
		{
			int release = sl->SEG & 1;
			
			if ( !release )
			{
				// re KEY ON

				// sl->Fcnt = 0;
				// sl->ChgEnM = ~0;

				sl->Ecnt = 0;
				sl->Einc = sl->EincA;
				sl->Ecmp = ENV_DECAY;
				sl->Ecurp = ATTACK;
			}

			set_seg( sl, (sl->SEG << 1) & 4 );
			
			if ( !release )
				break;
		}
		// fall through
	
	case 3:
		// Env_Release_Next
		sl->Ecnt = ENV_END;
		sl->Einc = 0;
		sl->Ecmp = ENV_END + 1;
		break;
	
	// default: no op
	}
}

static inline void update_envelope( struct slot_t* sl )
{
	int ecmp = sl->Ecmp;
	if ( (sl->Ecnt += sl->Einc) >= ecmp )
		update_envelope_( sl );
}


typedef void (*ym2612_update_chan_t)( struct tables_t*, struct channel_*, short*, int );

#define GET_CURRENT_PHASE \
int in0 = ch->SLOT[S0].Fcnt; \
int in1 = ch->SLOT[S1].Fcnt; \
int in2 = ch->SLOT[S2].Fcnt; \
int in3 = ch->SLOT[S3].Fcnt; \

#define GET_CURRENT_LFO \
int YM2612_LFOinc = g->LFOinc; \
int YM2612_LFOcnt = g->LFOcnt + YM2612_LFOinc; 

#define CALC_EN( x ) \
	int temp##x = ENV_TAB [ch->SLOT [S##x].Ecnt >> ENV_LBITS] + ch->SLOT [S##x].TLL;  \
	int en##x = ((temp##x ^ ch->SLOT [S##x].env_xor) + (env_LFO >> ch->SLOT [S##x].AMS)) & \
 			((temp##x - ch->SLOT [S##x].env_max) >> 31);
				
#define GET_ENV \
int const env_LFO = ENV_TAB_LOOKUP [YM2612_LFOcnt >> LFO_LBITS & LFO_MASK]; \
short const* const ENV_TAB = g->ENV_TAB; \
CALC_EN( 0 ) \
CALC_EN( 1 ) \
CALC_EN( 2 ) \
CALC_EN( 3 )

#ifndef YM2612_USE_TL_TAB
static inline int tl_level( int i )
{
	if (i >= (PG_CUT_OFF + TL_LENGHT)) {
		return 0;
	} else if (i >= TL_LENGHT) {
		return -tl_coeff [i - TL_LENGHT];
	} else if (i >= PG_CUT_OFF) {
		return 0;
	} else
		return tl_coeff [i];
}
#define SINT( i, o ) (tl_level (g->SIN_TAB [(i)] + (o)))
#else
#define SINT( i, o ) (g->TL_TAB [g->SIN_TAB [(i)] + (o)])
#endif

#define DO_FEEDBACK               \
int CH_S0_OUT_0 = ch->S0_OUT [0]; \
{                                 \
	int temp = in0 + ((CH_S0_OUT_0 + CH_S0_OUT_1) >> ch->FB); \
	CH_S0_OUT_1 = CH_S0_OUT_0; \
	CH_S0_OUT_0 = SINT( (temp >> SIN_LBITS) & SIN_MASK, en0 ); \
} \

#define DO_LIMIT \
CH_OUTd >>= MAX_OUT_BITS - output_bits + 2; \

#define UPDATE_PHASE_CYCLE \
unsigned freq_LFO = ((FREQ_TAB_LOOKUP [YM2612_LFOcnt >> LFO_LBITS & LFO_MASK] * \
			ch->FMS) >> (LFO_HBITS - 1 + 1)) + (1 << (LFO_FMS_LBITS - 1)); \
YM2612_LFOcnt += YM2612_LFOinc; \
in0 += (ch->SLOT [S0].Finc * freq_LFO) >> (LFO_FMS_LBITS - 1); \
in1 += (ch->SLOT [S1].Finc * freq_LFO) >> (LFO_FMS_LBITS - 1); \
in2 += (ch->SLOT [S2].Finc * freq_LFO) >> (LFO_FMS_LBITS - 1); \
in3 += (ch->SLOT [S3].Finc * freq_LFO) >> (LFO_FMS_LBITS - 1);

#define UPDATE_ENV \
int t0 = buf [0] + (CH_OUTd & ch->LEFT); \
int t1 = buf [1] + (CH_OUTd & ch->RIGHT); \
update_envelope( &ch->SLOT [0] ); \
update_envelope( &ch->SLOT [1] ); \
update_envelope( &ch->SLOT [2] ); \
update_envelope( &ch->SLOT [3] );

#define DO_OUTPUT_0 \
ch->S0_OUT [0] = CH_S0_OUT_0; \
buf [0] = t0; \
buf [1] = t1; \
buf += 2; \

#define DO_OUTPUT_1 \
ch->S0_OUT [1] = CH_S0_OUT_1;

#define UPDATE_PHASE \
ch->SLOT [S0].Fcnt = in0; \
ch->SLOT [S1].Fcnt = in1; \
ch->SLOT [S2].Fcnt = in2; \
ch->SLOT [S3].Fcnt = in3;

static void ym2612_update_chan0( struct tables_t* g, struct channel_* ch,
		short* buf, int length )
{
	int not_end = ch->SLOT [S3].Ecnt - ENV_END;
	int CH_S0_OUT_1 = ch->S0_OUT [1];
	
	GET_CURRENT_PHASE
	GET_CURRENT_LFO
	
	if ( !not_end )
		return;
	
	do
	{
		GET_ENV
		DO_FEEDBACK
		
		int CH_OUTd;
		int temp = in1 + CH_S0_OUT_1;
		temp = in2 + SINT( (temp >> SIN_LBITS) & SIN_MASK, en1 );
		temp = in3 + SINT( (temp >> SIN_LBITS) & SIN_MASK, en2 );
		CH_OUTd = SINT( (temp >> SIN_LBITS) & SIN_MASK, en3 );
		
		DO_LIMIT
		UPDATE_PHASE_CYCLE
		UPDATE_ENV
		DO_OUTPUT_0
	}
	while ( --length );
	DO_OUTPUT_1
	UPDATE_PHASE
}

static void ym2612_update_chan1( struct tables_t* g, struct channel_* ch,
		short* buf, int length )
{
	int not_end = ch->SLOT [S3].Ecnt - ENV_END;
	int CH_S0_OUT_1 = ch->S0_OUT [1];
	
	GET_CURRENT_PHASE
	GET_CURRENT_LFO
	
	if ( !not_end )
		return;
	
	do
	{
		GET_ENV
		DO_FEEDBACK
		
		int CH_OUTd;
		int temp = in2 + CH_S0_OUT_1 + SINT( (in1 >> SIN_LBITS) & SIN_MASK, en1 );
		temp = in3 + SINT( (temp >> SIN_LBITS) & SIN_MASK, en2 );
		CH_OUTd = SINT( (temp >> SIN_LBITS) & SIN_MASK, en3 );
		
		DO_LIMIT
		UPDATE_PHASE_CYCLE
		UPDATE_ENV
		DO_OUTPUT_0
	}
	while ( --length );
	DO_OUTPUT_1
	UPDATE_PHASE
}

static void ym2612_update_chan2( struct tables_t* g, struct channel_* ch,
		short* buf, int length )
{
	int not_end = ch->SLOT [S3].Ecnt - ENV_END;
	int CH_S0_OUT_1 = ch->S0_OUT [1];
	
	GET_CURRENT_PHASE
	GET_CURRENT_LFO
	
	if ( !not_end )
		return;
	
	do
	{
		GET_ENV
		DO_FEEDBACK
		
		int CH_OUTd;
		int temp = in2 + SINT( (in1 >> SIN_LBITS) & SIN_MASK, en1 );
		temp = in3 + CH_S0_OUT_1 + SINT( (temp >> SIN_LBITS) & SIN_MASK, en2 );
		CH_OUTd = SINT( (temp >> SIN_LBITS) & SIN_MASK, en3 );
		
		DO_LIMIT
		UPDATE_PHASE_CYCLE
		UPDATE_ENV
		DO_OUTPUT_0
	}
	while ( --length );
	DO_OUTPUT_1
	UPDATE_PHASE
}

static void ym2612_update_chan3( struct tables_t* g, struct channel_* ch,
		short* buf, int length )
{
	int not_end = ch->SLOT [S3].Ecnt - ENV_END;
	int CH_S0_OUT_1 = ch->S0_OUT [1];
	
	GET_CURRENT_PHASE
	GET_CURRENT_LFO
	
	if ( !not_end )
		return;
	
	do
	{
		GET_ENV
		DO_FEEDBACK
		
		int CH_OUTd;
		int temp = in1 + CH_S0_OUT_1;
		temp = in3 + SINT( (temp >> SIN_LBITS) & SIN_MASK, en1 ) +
				SINT( (in2 >> SIN_LBITS) & SIN_MASK, en2 );
		CH_OUTd = SINT( (temp >> SIN_LBITS) & SIN_MASK, en3 );
		
		DO_LIMIT
		UPDATE_PHASE_CYCLE
		UPDATE_ENV
		DO_OUTPUT_0
	}
	while ( --length );
	DO_OUTPUT_1
	UPDATE_PHASE
}

static void ym2612_update_chan4( struct tables_t* g, struct channel_* ch,
		short* buf, int length )
{
	int not_end = ch->SLOT [S3].Ecnt - ENV_END;
	not_end |= ch->SLOT [S1].Ecnt - ENV_END;
	
	int CH_S0_OUT_1 = ch->S0_OUT [1];
	
	GET_CURRENT_PHASE
	GET_CURRENT_LFO
	
	if ( !not_end )
		return;
	
	do
	{
		GET_ENV
		DO_FEEDBACK
		
		int CH_OUTd;
		int temp = in3 + SINT( (in2 >> SIN_LBITS) & SIN_MASK, en2 );
		CH_OUTd = SINT( (temp >> SIN_LBITS) & SIN_MASK, en3 ) +
				SINT( ((in1 + CH_S0_OUT_1) >> SIN_LBITS) & SIN_MASK, en1 );
		
		DO_LIMIT
		UPDATE_PHASE_CYCLE
		UPDATE_ENV
		DO_OUTPUT_0
	}
	while ( --length );
	DO_OUTPUT_1
	UPDATE_PHASE
}

static void ym2612_update_chan5( struct tables_t* g, struct channel_* ch,
		short* buf, int length )
{
	int not_end = ch->SLOT [S3].Ecnt - ENV_END;
	not_end |= ch->SLOT [S2].Ecnt - ENV_END;
	not_end |= ch->SLOT [S1].Ecnt - ENV_END;
	
	int CH_S0_OUT_1 = ch->S0_OUT [1];
	
	GET_CURRENT_PHASE
	GET_CURRENT_LFO
	
	if ( !not_end )
		return;
	
	do
	{
		GET_ENV
		DO_FEEDBACK
		
		int CH_OUTd;
		int temp = CH_S0_OUT_1;
		CH_OUTd = SINT( ((in3 + temp) >> SIN_LBITS) & SIN_MASK, en3 ) +
				SINT( ((in1 + temp) >> SIN_LBITS) & SIN_MASK, en1 ) +
				SINT( ((in2 + temp) >> SIN_LBITS) & SIN_MASK, en2 );
		
		DO_LIMIT
		UPDATE_PHASE_CYCLE
		UPDATE_ENV
		DO_OUTPUT_0
	}
	while ( --length );
	DO_OUTPUT_1
	UPDATE_PHASE
}

static void ym2612_update_chan6( struct tables_t* g, struct channel_* ch,
		short* buf, int length )
{
	int not_end = ch->SLOT [S3].Ecnt - ENV_END;
	not_end |= ch->SLOT [S2].Ecnt - ENV_END;
	not_end |= ch->SLOT [S1].Ecnt - ENV_END;
	
	int CH_S0_OUT_1 = ch->S0_OUT [1];
	
	GET_CURRENT_PHASE
	GET_CURRENT_LFO
	
	if ( !not_end )
		return;
	
	do
	{
		GET_ENV
		DO_FEEDBACK
		
		int CH_OUTd;
		CH_OUTd = SINT( (in3 >> SIN_LBITS) & SIN_MASK, en3 ) +
				SINT( ((in1 + CH_S0_OUT_1) >> SIN_LBITS) & SIN_MASK, en1 ) +
				SINT( (in2 >> SIN_LBITS) & SIN_MASK, en2 );
				
		DO_LIMIT
		UPDATE_PHASE_CYCLE
		UPDATE_ENV
		DO_OUTPUT_0
	}
	while ( --length );
	DO_OUTPUT_1
	UPDATE_PHASE
}

static void ym2612_update_chan7( struct tables_t* g, struct channel_* ch,
		short* buf, int length )
{
	int not_end = ch->SLOT [S3].Ecnt - ENV_END;
	not_end |= ch->SLOT [S0].Ecnt - ENV_END;
	not_end |= ch->SLOT [S2].Ecnt - ENV_END;
	not_end |= ch->SLOT [S1].Ecnt - ENV_END;
	
	int CH_S0_OUT_1 = ch->S0_OUT [1];
	
	GET_CURRENT_PHASE
	GET_CURRENT_LFO
	
	if ( !not_end )
		return;
	
	do
	{
		GET_ENV
		DO_FEEDBACK
		
		int CH_OUTd;
		CH_OUTd = SINT( (in3 >> SIN_LBITS) & SIN_MASK, en3 ) +
				SINT( (in1 >> SIN_LBITS) & SIN_MASK, en1 ) +
				SINT( (in2 >> SIN_LBITS) & SIN_MASK, en2 ) + CH_S0_OUT_1;
		
		DO_LIMIT
		UPDATE_PHASE_CYCLE
		UPDATE_ENV
		DO_OUTPUT_0
	}
	while ( --length );
	DO_OUTPUT_1
	UPDATE_PHASE
}

static void (*UPDATE_CHAN[8])(struct tables_t* g, struct channel_* ch,
		short* buf, int length) =
{
	(void *)ym2612_update_chan0,
	(void *)ym2612_update_chan1,
	(void *)ym2612_update_chan2,
	(void *)ym2612_update_chan3,
	(void *)ym2612_update_chan4,
	(void *)ym2612_update_chan5,
	(void *)ym2612_update_chan6,
	(void *)ym2612_update_chan7
};

static void run_timer( struct Ym2612_Impl* impl, int length )
{
	int const step = 6;
	int remain = length;
	do
	{
		int n = step;
		if ( n > remain )
			n = remain;
		remain -= n;
		
		int i = n * impl->YM2612.TimerBase;
		if (impl->YM2612.Mode & 1)                            // Timer A ON ?
		{
	//      if ((impl->YM2612.TimerAcnt -= 14073) <= 0)       // 13879=NTSC (old: 14475=NTSC  14586=PAL)
			if ((impl->YM2612.TimerAcnt -= i) <= 0)
			{
				// timer a overflow
				
				impl->YM2612.Status |= (impl->YM2612.Mode & 0x04) >> 2;
				impl->YM2612.TimerAcnt += impl->YM2612.TimerAL;

				if (impl->YM2612.Mode & 0x80)
				{
					KEY_ON( &impl->YM2612.CHANNEL [2], &impl->g, 0 );
					KEY_ON( &impl->YM2612.CHANNEL [2], &impl->g, 1 );
					KEY_ON( &impl->YM2612.CHANNEL [2], &impl->g, 2 );
					KEY_ON( &impl->YM2612.CHANNEL [2], &impl->g, 3 );
				}
			}
		}

		if (impl->YM2612.Mode & 2)                            // Timer B ON ?
		{
	//      if ((impl->YM2612.TimerBcnt -= 14073) <= 0)       // 13879=NTSC (old: 14475=NTSC  14586=PAL)
			if ((impl->YM2612.TimerBcnt -= i) <= 0)
			{
				// timer b overflow
				impl->YM2612.Status |= (impl->YM2612.Mode & 0x08) >> 2;
				impl->YM2612.TimerBcnt += impl->YM2612.TimerBL;
			}
		}
	}
	while ( remain > 0 );
}

static void impl_run( struct Ym2612_Impl* impl, int pair_count, short out [] )
{
	if ( pair_count <= 0 )
		return;
	
	if ( impl->YM2612.Mode & 3 )
		run_timer( impl, pair_count );
	
	// Mise à jour des pas des compteurs-frequences s'ils ont ete modifies
	
	int chi;
	for ( chi = 0; chi < ym2612_channel_count; chi++ )
	{
		struct channel_* ch = &impl->YM2612.CHANNEL [chi];
		if ( ch->SLOT [0].Finc != -1 )
			continue;
		
		int i2 = 0;
		if ( chi == 2 && (impl->YM2612.Mode & 0x40) )
			i2 = 2;
		
		int i;
		for ( i = 0; i < 4; i++ )
		{
			// static int seq [4] = { 2, 1, 3, 0 };
			// if ( i2 ) i2 = seq [i];
			
			struct slot_t* sl = &ch->SLOT [i];
			int finc = impl->g.FINC_TAB [ch->FNUM [i2]] >> (7 - ch->FOCT [i2]);
			int ksr = ch->KC [i2] >> sl->KSR_S;   // keycode attenuation
			sl->Finc = (finc + sl->DT [ch->KC [i2]]) * sl->MUL;
			if (sl->KSR != ksr)          // si le KSR a change alors
			{                       // les differents taux pour l'enveloppe sont mis à jour
				sl->KSR = ksr;

				sl->EincA = sl->AR [ksr];
				sl->EincD = sl->DR [ksr];
				sl->EincS = sl->SR [ksr];
				sl->EincR = sl->RR [ksr];

				if (sl->Ecurp == ATTACK)
				{
					sl->Einc = sl->EincA;
				}
				else if (sl->Ecurp == DECAY)
				{
					sl->Einc = sl->EincD;
				}
				else if (sl->Ecnt < ENV_END)
				{
					if (sl->Ecurp == SUBSTAIN)
						sl->Einc = sl->EincS;
					else if (sl->Ecurp == RELEASE)
						sl->Einc = sl->EincR;
				}
			}
			
			if ( i2 )
				i2 = (i2 ^ 2) ^ (i2 >> 1);
		}
	}
	
	int i;
	for ( i = 0; i < ym2612_channel_count; i++ )
	{
		if ( !(impl->mute_mask & (1 << i)) && (i != 5 || !impl->YM2612.DAC) )
			UPDATE_CHAN [impl->YM2612.CHANNEL [i].ALGO]( &impl->g, &impl->YM2612.CHANNEL [i], out, pair_count );
	}
	
	impl->g.LFOcnt += impl->g.LFOinc * pair_count;
}

void Ym2612_run( struct Ym2612_Emu* this, int pair_count, short out [] ) { impl_run( &this->impl, pair_count, out ); }
