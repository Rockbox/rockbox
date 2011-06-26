// YM2612 FM sound chip emulator

// Game_Music_Emu 0.6-pre
#ifndef YM2612_EMU_H
#define YM2612_EMU_H

#include "blargg_common.h"

#if !defined(ROCKBOX)
	#define YM2612_CALCUL_TABLES
#endif

#if MEMORYSIZE > 2
    #define YM2612_USE_TL_TAB
#endif

enum { ym2612_out_chan_count = 2 }; // stereo
enum { ym2612_channel_count = 6 };
enum { ym2612_disabled_time = -1 };

struct slot_t
{
	const int *DT;  // parametre detune
	int MUL;    // parametre "multiple de frequence"
	int TL;     // Total Level = volume lorsque l'enveloppe est au plus haut
	int TLL;    // Total Level ajusted
	int SLL;    // Sustin Level (ajusted) = volume où l'enveloppe termine sa premiere phase de regression
	int KSR_S;  // Key Scale Rate Shift = facteur de prise en compte du KSL dans la variations de l'enveloppe
	int KSR;    // Key Scale Rate = cette valeur est calculee par rapport à la frequence actuelle, elle va influer
				// sur les differents parametres de l'enveloppe comme l'attaque, le decay ...  comme dans la realite !
	int SEG;    // Type enveloppe SSG
	int env_xor;
	int env_max;

	const int *AR;  // Attack Rate (table pointeur) = Taux d'attaque (AR [KSR])
	const int *DR;  // Decay Rate (table pointeur) = Taux pour la regression (DR [KSR])
	const int *SR;  // Sustin Rate (table pointeur) = Taux pour le maintien (SR [KSR])
	const int *RR;  // Release Rate (table pointeur) = Taux pour le rel'chement (RR [KSR])
	int Fcnt;   // Frequency Count = compteur-frequence pour determiner l'amplitude actuelle (SIN [Finc >> 16])
	int Finc;   // frequency step = pas d'incrementation du compteur-frequence
				// plus le pas est grand, plus la frequence est aïgu (ou haute)
	int Ecurp;  // Envelope current phase = cette variable permet de savoir dans quelle phase
				// de l'enveloppe on se trouve, par exemple phase d'attaque ou phase de maintenue ...
				// en fonction de la valeur de cette variable, on va appeler une fonction permettant
				// de mettre à jour l'enveloppe courante.
	int Ecnt;   // Envelope counter = le compteur-enveloppe permet de savoir où l'on se trouve dans l'enveloppe
	int Einc;   // Envelope step courant
	int Ecmp;   // Envelope counter limite pour la prochaine phase
	int EincA;  // Envelope step for Attack = pas d'incrementation du compteur durant la phase d'attaque
				// cette valeur est egal à AR [KSR]
	int EincD;  // Envelope step for Decay = pas d'incrementation du compteur durant la phase de regression
				// cette valeur est egal à DR [KSR]
	int EincS;  // Envelope step for Sustain = pas d'incrementation du compteur durant la phase de maintenue
				// cette valeur est egal à SR [KSR]
	int EincR;  // Envelope step for Release = pas d'incrementation du compteur durant la phase de rel'chement
				// cette valeur est egal à RR [KSR]
	int *OUTp;  // pointeur of SLOT output = pointeur permettant de connecter la sortie de ce slot à l'entree
				// d'un autre ou carrement à la sortie de la voie
	int INd;    // input data of the slot = donnees en entree du slot
	int ChgEnM; // Change envelop mask.
	int AMS;    // AMS depth level of this SLOT = degre de modulation de l'amplitude par le LFO
	int AMSon;  // AMS enable flag = drapeau d'activation de l'AMS
};

struct channel_
{
	int S0_OUT [4];         // anciennes sorties slot 0 (pour le feed back)
	int LEFT;               // LEFT enable flag
	int RIGHT;              // RIGHT enable flag
	int ALGO;               // Algorythm = determine les connections entre les operateurs
	int FB;                 // shift count of self feed back = degre de "Feed-Back" du SLOT 1 (il est son unique entree)
	int FMS;                // Frequency Modulation Sensitivity of channel = degre de modulation de la frequence sur la voie par le LFO
	int AMS;                // Amplitude Modulation Sensitivity of channel = degre de modulation de l'amplitude sur la voie par le LFO
	int FNUM [4];           // hauteur frequence de la voie (+ 3 pour le mode special)
	int FOCT [4];           // octave de la voie (+ 3 pour le mode special)
	int KC [4];             // Key Code = valeur fonction de la frequence (voir KSR pour les slots, KSR = KC >> KSR_S)
	struct slot_t SLOT [4];    // four slot.operators = les 4 slots de la voie
	int FFlag;              // Frequency step recalculation flag
};

struct state_t
{
	int TimerBase;      // TimerBase calculation
	int Status;         // YM2612 Status (timer overflow)
	int TimerA;         // timerA limit = valeur jusqu'à laquelle le timer A doit compter
	int TimerAL;
	int TimerAcnt;      // timerA counter = valeur courante du Timer A
	int TimerB;         // timerB limit = valeur jusqu'à laquelle le timer B doit compter
	int TimerBL;
	int TimerBcnt;      // timerB counter = valeur courante du Timer B
	int Mode;           // Mode actuel des voie 3 et 6 (normal / special)
	int DAC;            // DAC enabled flag
	struct channel_ CHANNEL [ym2612_channel_count];  // Les 6 voies du YM2612
	int REG [2] [0x100];    // Sauvegardes des valeurs de tout les registres, c'est facultatif
						// cela nous rend le debuggage plus facile
};

#undef PI
#define PI 3.14159265358979323846

#define ATTACK    0
#define DECAY     1
#define SUBSTAIN  2
#define RELEASE   3

// SIN_LBITS <= 16
// LFO_HBITS <= 16
// (SIN_LBITS + SIN_HBITS) <= 26
// (ENV_LBITS + ENV_HBITS) <= 28
// (LFO_LBITS + LFO_HBITS) <= 28

#define SIN_HBITS      12                               // Sinus phase counter int part
#define SIN_LBITS      (26 - SIN_HBITS)                 // Sinus phase counter float part (best setting)

#if (SIN_LBITS > 16)
#define SIN_LBITS      16                               // Can't be greater than 16 bits
#endif

#define ENV_HBITS      12                               // Env phase counter int part
#define ENV_LBITS      (28 - ENV_HBITS)                 // Env phase counter float part (best setting)

#define LFO_HBITS      10                               // LFO phase counter int part
#define LFO_LBITS      (28 - LFO_HBITS)                 // LFO phase counter float part (best setting)

#define SIN_LENGHT     (1 << SIN_HBITS)
#define ENV_LENGHT     (1 << ENV_HBITS)
#define LFO_LENGHT     (1 << LFO_HBITS)

#define TL_LENGHT      (ENV_LENGHT * 3)                 // Env + TL scaling + LFO

#define SIN_MASK       (SIN_LENGHT - 1)
#define ENV_MASK       (ENV_LENGHT - 1)
#define LFO_MASK       (LFO_LENGHT - 1)

#define ENV_STEP       (96.0 / ENV_LENGHT)              // ENV_MAX = 96 dB

#define ENV_ATTACK     ((ENV_LENGHT * 0) << ENV_LBITS)
#define ENV_DECAY      ((ENV_LENGHT * 1) << ENV_LBITS)
#define ENV_END        ((ENV_LENGHT * 2) << ENV_LBITS)

#define MAX_OUT_BITS   (SIN_HBITS + SIN_LBITS + 2)      // Modulation = -4 <--> +4
#define MAX_OUT        ((1 << MAX_OUT_BITS) - 1)

#define PG_CUT_OFF     ((int) (78.0 / ENV_STEP))
//#define ENV_CUT_OFF    ((int) (68.0 / ENV_STEP))

#define AR_RATE        399128
#define DR_RATE        5514396

//#define AR_RATE        426136
//#define DR_RATE        (AR_RATE * 12)

#define LFO_FMS_LBITS  9    // FIXED (LFO_FMS_BASE gives somethink as 1)
#define LFO_FMS_BASE   ((int) (0.05946309436 * 0.0338 * (double) (1 << LFO_FMS_LBITS)))

#define S0             0    // Stupid typo of the YM2612
#define S1             2
#define S2             1
#define S3             3

struct tables_t
{
	short SIN_TAB [SIN_LENGHT];                 // SINUS TABLE (offset into TL TABLE)
	int LFOcnt;         // LFO counter = compteur-frequence pour le LFO
	int LFOinc;         // LFO step counter = pas d'incrementation du compteur-frequence du LFO
						// plus le pas est grand, plus la frequence est grande
	unsigned int AR_TAB [128];                  // Attack rate table
	unsigned int DR_TAB [96];                   // Decay rate table
	unsigned int DT_TAB [8] [32];               // Detune table
	unsigned int SL_TAB [16];                   // Substain level table
	unsigned int NULL_RATE [32];                // Table for NULL rate
	int LFO_INC_TAB [8];                        // LFO step table
	
	short ENV_TAB [2 * ENV_LENGHT + 8];         // ENV CURVE TABLE (attack & decay)
#ifdef YM2612_CALCUL_TABLES
	short LFO_ENV_TAB [LFO_LENGHT];             // LFO AMS TABLE (adjusted for 11.8 dB)
	short LFO_FREQ_TAB [LFO_LENGHT];            // LFO FMS TABLE
#endif
#ifdef YM2612_USE_TL_TAB
    int TL_TAB [TL_LENGHT * 2];                 // TOTAL LEVEL TABLE (positif and minus)
#endif
	unsigned int DECAY_TO_ATTACK [ENV_LENGHT];  // Conversion from decay to attack phase
	unsigned int FINC_TAB [2048];               // Frequency step table
};

struct Ym2612_Impl
{	
	struct state_t YM2612;
	int mute_mask;
	struct tables_t g;
};

void impl_reset( struct Ym2612_Impl* impl );

struct Ym2612_Emu  {
	struct Ym2612_Impl impl;
		
	// Impl
	int last_time;
	int sample_rate;
	int clock_rate;
	short* out;
};

static inline void Ym2612_init( struct Ym2612_Emu* this_ )
{ 
	this_->last_time = ym2612_disabled_time; this_->out = 0;
	this_->impl.mute_mask = 0;
}
	
// Sets sample rate and chip clock rate, in Hz. Returns non-zero
// if error. If clock_rate=0, uses sample_rate*144
const char* Ym2612_set_rate( struct Ym2612_Emu* this_, int sample_rate, int clock_rate );
	
// Resets to power-up state
void Ym2612_reset( struct Ym2612_Emu* this_ );
	
// Mutes voice n if bit n (1 << n) of mask is set
void Ym2612_mute_voices( struct Ym2612_Emu* this_, int mask );
	
// Writes addr to register 0 then data to register 1
void Ym2612_write0( struct Ym2612_Emu* this_, int addr, int data );
	
// Writes addr to register 2 then data to register 3
void Ym2612_write1( struct Ym2612_Emu* this_, int addr, int data );
	
// Runs and adds pair_count*2 samples into current output buffer contents
void Ym2612_run( struct Ym2612_Emu* this_, int pair_count, short* out );

static inline void Ym2612_enable( struct Ym2612_Emu* this_, bool b ) { this_->last_time = b ? 0 : ym2612_disabled_time; }
static inline bool Ym2612_enabled( struct Ym2612_Emu* this_ ) { return this_->last_time != ym2612_disabled_time; }
static inline void Ym2612_begin_frame( struct Ym2612_Emu* this_, short* buf ) { this_->out = buf; this_->last_time = 0; }
		
static inline int Ym2612_run_until( struct Ym2612_Emu* this_, int time )
{
	int count = time - this_->last_time;
	if ( count > 0 )
	{
		if ( this_->last_time < 0 )
			return false;
		this_->last_time = time;
		short* p = this_->out;
		this_->out += count * ym2612_out_chan_count;
		Ym2612_run( this_, count, p );
	}
	return true;
}
#endif
