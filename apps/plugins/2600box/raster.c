/*****************************************************************************

   This file is part of x2600, the Atari 2600 Emulator
   ===================================================

   Copyright 1996 Alex Hornby. For contributions see the file CREDITS.

   This software is distributed under the terms of the GNU General Public
   License. This is free software with ABSOLUTELY NO WARRANTY.

   See the file COPYING for Details.

   $Id: raster.c,v 1.30 1997/11/22 14:27:47 ahornby Exp $
******************************************************************************/

/*
 * most of the code is rewritten; however some parts of the old code could
 * has been left over.
 */

/* Raster graphics procedures */

#include "rbconfig.h"
#include "rb_test.h"

#include "address.h"
#include "vmachine.h"
#include "col_mask.h"
#include "raster.h"
#include "keyboard.h"

#include "palette.h"
#include "tia_video.h"

#include "rb_lcd.h"


// -----------------------------------------------------------------------

// optimization settings

// if defined, this overrides all other optimizations
//#define RASTER_OPT_NONE
//#define RASTER_NO_32BIT_ZEROS
//#define RASTER_32BIT_OPT // obsolet??

// rasterize using 32bit access to BYTE-arrays using precalculated tables
#define RASTER_OPT_MISSILE_TAB32
#define RASTER_OPT_PLAYER_TAB32
#define RASTER_OPT_PLAYFIELD_TAB32

#define RASTER_NOWRAP // this is new


// -----------------------------------------------------------------------

unsigned colour_table[4];

// the following vars contain indices to colour_table for all possible
// combinations of picture elements (Players, Missiles, Ball, Playfield Left
// and Playfield Right)
/* Normal priority */
static int colour_normal[MASK_MAX];
static int colour_scores[MASK_MAX];
/* Alternate priority */
static int colour_priority[MASK_MAX];

static int *colour_lookup;


/* The PlayField structure */
static struct PlayField {
    BYTE pf0,pf1,pf2;
    BYTE ref;
} playfield;


/* for every pixel of the screen: if playfield-bit is set, set the
 * corresponding byte to PF_MASK */
#ifndef RASTER_OPT_PLAYFIELD_TAB32
static BYTE playfield_mask_vector[160];
#endif


// playfield vector w/ 1 byte for 4 pixels
static BYTE playfield_mask_v40[40];

enum {S_PLAYER0=0, S_PLAYER1, S_MISSILE0, S_MISSILE1, S_BALL, NUM_SPRITES};
#define SPRITE_GENERATE_INDEX 160


struct sprite {
# ifndef RASTER_NOWRAP
    BYTE    mask_vector[3*160];
# else
    BYTE    mask_vector[2*160];
# endif // RASTER_NOWRAP
    int     x0_index;   /* this index represents x=0 of visible screen */
    int     grp;        /* player graphics; player only! */
    int     x;
    BYTE    enabled;    /* players doesn't have a tia enable register; enable is */
                        /* set when graphics is ==0 (TODO!)  */
    int     hmm;        /* horizontal movement; hold an actual signed -8 to +7 value! */
    int     width;      /* ?? in binary coding? 0x01=double, ... */
    //int     number;     /* number of player/missile copies */ // TODO: do I need distance as well?
    BYTE    vdel;
    BYTE    vdel_flag;  /* indicates if vertical delay is used (only players and ball!) */
    BYTE    nusize;     /* only for player, and ONLY used to determin if values have changed; */
                        /* actual values for number and width are stored in .width and .number */
    BYTE    reflect;
    BYTE    mask;
    UINT16  mask16;
    UINT32  mask32;
};



struct sprite sprite[NUM_SPRITES];



/* ----------------------------------------------------------------------- */

/*
   There are 7 different objects on the screen. Each takes one bit of the
   collision vector.
   Bit 0: Player 0
   Bit 1: Player 1
   Bit 2: Missile 0
   Bit 3: Missile 1
   Bit 4: Ball
   Bit 5: Playfield left half
   Bit 6: Playfield right half
   Note: playfield is divided to be able to assign the correct colour when
   "score colors" are selected.
 */


#define SPR_MASK(id)        (1<<(id))

#define PL0_MASK        SPR_MASK(S_PLAYER0)
#define PL1_MASK        SPR_MASK(S_PLAYER1)
#define ML0_MASK        SPR_MASK(S_MISSILE0)
#define ML1_MASK        SPR_MASK(S_MISSILE1)
#define BL_MASK         SPR_MASK(S_BALL)

#define PFL_MASK        (1<<5)
#define PFR_MASK        (1<<6)
#define PFL_MASK32      (PFL_MASK | PFL_MASK<<8 | PFL_MASK<<16 | PFL_MASK<<24)
#define PFR_MASK32      (PFR_MASK | PFR_MASK<<8 | PFR_MASK<<16 | PFR_MASK<<24)


/* The collision lookup table */
unsigned short col_table[MASK_MAX];

/* The collision state */
unsigned short collisions;


/*
    For each moveable object (PLAYER0 and 1, MISSILE0 and 1, BALL) an array
    twice the length of the visible scanline is used.
(OBSOLETE:)
    The actual object image is created from index 160 on (one array
    element per pixel), each pixel represented by an individual object
    bitmask.
    The advantage of this approach is that we can simply move the object
    around be re-defining the "x=0"-index of the object vector.
    E.g. setting the x0-index to 150 would result in the object appearing at
    screen position 11, setting x0-index to 10 results in screen position
    150, and setting x0-index to zero completely hides the object.
NEW:
    The object image is created twice, first at index 0 and second at index
    160 (the "main" image). We set the "read"-index at index (160-x); if
    we move the object to the right (the read index decreases), at some
    point the secondary image moves into view. We need this for wraparound
    effects, especially with multiple sprites.
    An read index of "0" indicates a hidden object.
*/


/* ----------------------------------------------------------------------- */

#if 0

/*
 * TODO:
 * I must take endianness into account for this to work!
 */

#define TMPMSK   PL0_MASK
const UINT32 pl0_4bit_lookup[] = {
    0,
                                                (TMPMSK),
                                  (TMPMSK<<8),
                                  (TMPMSK<<8) | (TMPMSK),
                   (TMPMSK<<16),
                   (TMPMSK<<16)               | (TMPMSK),
                   (TMPMSK<<16) | (TMPMSK<<8),
                   (TMPMSK<<16) | (TMPMSK<<8) | (TMPMSK),
    (TMPMSK<<24),
    (TMPMSK<<24)                              | (TMPMSK),
    (TMPMSK<<24)                | (TMPMSK<<8),
    (TMPMSK<<24)                | (TMPMSK<<8) | (TMPMSK),
    (TMPMSK<<24) | (TMPMSK<<16),
    (TMPMSK<<24) | (TMPMSK<<16)               | (TMPMSK),
    (TMPMSK<<24) | (TMPMSK<<16) | (TMPMSK<<8),
    (TMPMSK<<24) | (TMPMSK<<16) | (TMPMSK<<8) | (TMPMSK)
};

const UINT32 pl0_4bit_ref_lookup[] = {
    0,
    (TMPMSK<<24),
                   (TMPMSK<<16),
    (TMPMSK<<24) | (TMPMSK<<16),
                                  (TMPMSK<<8),
    (TMPMSK<<24)                | (TMPMSK<<8),
                   (TMPMSK<<16) | (TMPMSK<<8),
    (TMPMSK<<24) | (TMPMSK<<16) | (TMPMSK<<8),
                                                (TMPMSK),
    (TMPMSK<<24)                              | (TMPMSK),
                   (TMPMSK<<16)               | (TMPMSK),
    (TMPMSK<<24) | (TMPMSK<<16)               | (TMPMSK),
                                  (TMPMSK<<8) | (TMPMSK),
    (TMPMSK<<24)                | (TMPMSK<<8) | (TMPMSK),
                   (TMPMSK<<16) | (TMPMSK<<8) | (TMPMSK),
    (TMPMSK<<24) | (TMPMSK<<16) | (TMPMSK<<8) | (TMPMSK)
};
#undef TMPMSK

#endif // 0


UINT32 pl0_4bit_lookup[16];
UINT32 pl0_4bit_ref_lookup[16];
UINT32 pl1_4bit_lookup[16];
UINT32 pl1_4bit_ref_lookup[16];

UINT32 pfl_4bit_lookup[16];
UINT32 pfl_4bit_ref_lookup[16];
UINT32 pfr_4bit_lookup[16];
UINT32 pfr_4bit_ref_lookup[16];

// [0] and [1] hold length 1, [2] and [3] hold length 2 etc.
#    define TMPMSK   ML0_MASK
const UINT32 m0_len_lookup[8] = {
#  ifdef ROCKBOX_BIG_ENDIAN
    (TMPMSK<<24),                                         0,
    (TMPMSK<<24) | (TMPMSK<<16),                          0,
#  else /* little endian */
                                                (TMPMSK), 0,
                                  (TMPMSK<<8) | (TMPMSK), 0,
#  endif /* endian */
    (TMPMSK<<24) | (TMPMSK<<16) | (TMPMSK<<8) | (TMPMSK), 0,
    (TMPMSK<<24) | (TMPMSK<<16) | (TMPMSK<<8) | (TMPMSK),
                        (TMPMSK<<24) | (TMPMSK<<16) | (TMPMSK<<8) | (TMPMSK) };
#undef TMPMSK

#    define TMPMSK   ML1_MASK
const UINT32 m1_len_lookup[8] = {
#  ifdef ROCKBOX_BIG_ENDIAN
    (TMPMSK<<24),                                         0,
    (TMPMSK<<24) | (TMPMSK<<16),                          0,
#  else /* little endian */
                                                (TMPMSK), 0,
                                  (TMPMSK<<8) | (TMPMSK), 0,
#  endif /* endian */
    (TMPMSK<<24) | (TMPMSK<<16) | (TMPMSK<<8) | (TMPMSK), 0,
    (TMPMSK<<24) | (TMPMSK<<16) | (TMPMSK<<8) | (TMPMSK),
                        (TMPMSK<<24) | (TMPMSK<<16) | (TMPMSK<<8) | (TMPMSK) };
#undef TMPMSK

#    define TMPMSK   BL_MASK
const UINT32 bl_len_lookup[8] = {
#  ifdef ROCKBOX_BIG_ENDIAN
    (TMPMSK<<24),                                         0,
    (TMPMSK<<24) | (TMPMSK<<16),                          0,
#  else /* little endian */
                                                (TMPMSK), 0,
                                  (TMPMSK<<8) | (TMPMSK), 0,
#  endif /* endian */
    (TMPMSK<<24) | (TMPMSK<<16) | (TMPMSK<<8) | (TMPMSK), 0,
    (TMPMSK<<24) | (TMPMSK<<16) | (TMPMSK<<8) | (TMPMSK),
                        (TMPMSK<<24) | (TMPMSK<<16) | (TMPMSK<<8) | (TMPMSK) };
#undef TMPMSK


// ----------------------------------------------


/*
 * helper function for init_collision()
 */
/* Does collision testing on the pixel b */
/* b: byte to test for collisions */
/* Used to build up the collision table */
static int set_collisions (BYTE b) COLD_ATTR;
static int set_collisions (BYTE b)
{
    int res=0;

    if ((b & ML0_MASK) && (b & PL1_MASK))
        res |= M0P1_MASK;
    if ((b & ML0_MASK) && (b & PL0_MASK))
        res |= M0P0_MASK;
    if ((b & ML1_MASK) && (b & PL0_MASK))
        res |=  M1P0_MASK;
    if ((b & ML1_MASK) && (b & PL1_MASK))
        res |=  M1P1_MASK;

    if ((b & PL0_MASK) && ((b & PFL_MASK) || (b & PFR_MASK)))
        res |= P0PF_MASK;
    if ((b & PL0_MASK) && (b & BL_MASK))
        res |= P0BL_MASK;
    if ((b & PL1_MASK) && ((b & PFL_MASK) || (b & PFR_MASK)))
        res |= P1PF_MASK ;
    if ((b & PL1_MASK) && (b & BL_MASK))
        res |= P1BL_MASK;

    if ((b & ML0_MASK) && ((b & PFL_MASK) || (b & PFR_MASK)))
        res |=  M0PF_MASK;
    if ((b & ML0_MASK) && (b & BL_MASK))
        res |= M0BL_MASK;
    if ((b & ML1_MASK) && ((b & PFL_MASK) || (b & PFR_MASK)))
        res |=  M1PF_MASK;
    if ((b & ML1_MASK) && (b & BL_MASK))
        res |=  M1BL_MASK;

    if ((b & BL_MASK) && ((b & PFL_MASK) || (b & PFR_MASK)))
        res |=BLPF_MASK;
    if ((b & PL0_MASK) && (b & PL1_MASK))
        res |=P0P1_MASK ;
    if ((b & ML0_MASK) && (b & ML1_MASK))
        res |=M0M1_MASK ;

    return res;
}

/*
 * init the collision lookup table
 * must be called once on startup
 */
static void init_collision_lookup(void) COLD_ATTR;
static void init_collision_lookup(void)
{
    /* Set up the collision lookup table */
    for (unsigned i = 0; i < ARRAY_LEN(col_table); i++)
        col_table[i] = set_collisions(i);

    collisions = 0;
}


/* -------------------------------------- */


static void init_color_lookup(void) COLD_ATTR;
static void init_color_lookup(void)
{
    unsigned i;
    unsigned val;

    /* Normal Priority: Players and Missiles are drawn on top of Playfield */
    for (i=0; i<ARRAY_LEN(colour_normal); i++) {
        if (i & (PL0_MASK | ML0_MASK))
            val = P0M0_COLOUR;
        else if (i & (PL1_MASK | ML1_MASK))
            val = P1M1_COLOUR;
        else if (i & (BL_MASK | PFL_MASK | PFR_MASK))
            val = PFBL_COLOUR;
        else
            val = BK_COLOUR;
        colour_normal[i]=val;
    }

    /* Playfield Priority: Playfield is drawn on top of Players and Missiles */
    for (i=0; i<ARRAY_LEN(colour_priority); i++) {
        if (i & (BL_MASK | PFL_MASK | PFR_MASK))
            val = PFBL_COLOUR;
        else if (i & (PL0_MASK | ML0_MASK))
            val = P0M0_COLOUR;
        else if (i & (PL1_MASK | ML1_MASK))
            val = P1M1_COLOUR;
        else
            val = BK_COLOUR;
        colour_priority[i]=val;
    }

    /* Score Colorscheme: Playfield gets Player colours */
    for (i=0; i<ARRAY_LEN(colour_scores); i++) {
        if (i & (PL0_MASK | ML0_MASK))
            val = P0M0_COLOUR;
        else if (i & (PL1_MASK | ML1_MASK))
            val = P1M1_COLOUR;
        else if (i & (BL_MASK | PFL_MASK))
            val = P0M0_COLOUR;            /* Left Playfield gets P0 colour */
        else if (i & (BL_MASK | PFR_MASK))
            val = P1M1_COLOUR;            /* Right Playfield gets P1 colour */
        else
            val = BK_COLOUR;
        colour_scores[i]=val;
    }

    /* Note: if both the priority bit and the scores bit are set,
       priority colours are applied */

}

void init_sprite_tables(void);

void init_raster(void) COLD_ATTR;
void init_raster(void)
{
    init_collision_lookup();
    init_color_lookup();

    // TODO: delete raster and playfield

    colour_lookup=colour_normal;

    sprite[S_PLAYER0].mask = PL0_MASK;
    sprite[S_PLAYER1].mask = PL1_MASK;
    sprite[S_MISSILE0].mask = ML0_MASK;
    sprite[S_MISSILE1].mask = ML1_MASK;
    sprite[S_BALL].mask = BL_MASK;

    for (unsigned i = 0; i < NUM_SPRITES; i++) {
        sprite[i].hmm = 0x0;
        sprite[i].x = 0x0;
        sprite[i].nusize = 0;
        sprite[i].grp = 0;
        sprite[i].vdel = 0;
        sprite[i].vdel_flag = 0;
        sprite[i].mask16 = (sprite[i].mask << 8) | sprite[i].mask;
        sprite[i].mask32 = (sprite[i].mask16 << 16) | sprite[i].mask16;
        sprite[i].x0_index = 0;
        sprite[i].enabled = 0;
        sprite[i].width = 0;
    }

    /* init missiles/ball: set graphic to 1 pixel */
    sprite[S_MISSILE0].mask_vector[SPRITE_GENERATE_INDEX] = ML0_MASK;
    sprite[S_MISSILE1].mask_vector[SPRITE_GENERATE_INDEX] = ML1_MASK;
    sprite[S_BALL].mask_vector[SPRITE_GENERATE_INDEX] = BL_MASK;

    init_sprite_tables();
}



/* ----------------------------------------------------------------------- */

static BYTE collision_vector[160];


void do_raster(int xmin, int xmax)
{
#if TST_TIME_MEASURE
    if (tst_time_measure == 4)
        return;
#endif

    TST_PROFILE(PRF_MASK_RENDER, "do_raster()");

    if (xmax < 1) // should never happen
        return;
    if (xmax <= xmin)
        return;
    if (xmin < 0)
        xmin = 0;
    if (xmax > 160)
        xmax = 160;

    int width = xmax - xmin;
    if (width <= 0)
        return;

/* use VCS 8bit colours until rendering to RB frame buffer */
    BYTE *tv_ptr = &tv_screen[ebeamy][xmin];

    int i;
    BYTE *ptr, *p_dest;

#ifndef RASTER_OPT_PLAYFIELD_TAB32
    /* Playfield */
    p_dest = &collision_vector[0];
    ptr = &playfield_mask_vector[xmin];
    i = width;
    do {
        *p_dest++ = *ptr++;
    } while (--i);
#else
    p_dest = &collision_vector[0];
    for (int x = xmin; x < xmax; ++x) {
        *p_dest++ = playfield_mask_v40[x>>2];
    }
#endif /* RASTER_OPT_PLAYFIELD_TAB32 */

    /* Player0 */
    if (sprite[S_PLAYER0].x0_index) {
        p_dest = &collision_vector[0];
        ptr = &sprite[S_PLAYER0].mask_vector[sprite[S_PLAYER0].x0_index + xmin];
        i = width;
        do {
# ifndef RASTER_NOWRAP
            *p_dest++ |= *ptr | *(ptr+160);
# else
            *p_dest++ |= *ptr;
# endif
            ++ptr;
        } while (--i);
    }
    /* Player1 */
    if (sprite[S_PLAYER1].x0_index) {
        p_dest = &collision_vector[0];
        ptr = &sprite[S_PLAYER1].mask_vector[sprite[S_PLAYER1].x0_index + xmin];
        i = width;
        do {
# ifndef RASTER_NOWRAP
            *p_dest++ |= *ptr | *(ptr+160);
# else
            *p_dest++ |= *ptr;
# endif
            ++ptr;
        } while (--i);
    }
    /* Missile0 */
    if (sprite[S_MISSILE0].x0_index) {
        p_dest = &collision_vector[0];
        ptr = &sprite[S_MISSILE0].mask_vector[sprite[S_MISSILE0].x0_index + xmin];
        i = width;
        do {
# ifndef RASTER_NOWRAP
            *p_dest++ |= *ptr | *(ptr+160);
# else
            *p_dest++ |= *ptr;
# endif
            ++ptr;
        } while (--i);
    }
    /* Missile1 */
    if (sprite[S_MISSILE1].x0_index) {
        p_dest = &collision_vector[0];
        ptr = &sprite[S_MISSILE1].mask_vector[sprite[S_MISSILE1].x0_index + xmin];
        i = width;
        do {
# ifndef RASTER_NOWRAP
            *p_dest++ |= *ptr | *(ptr+160);
# else
            *p_dest++ |= *ptr;
# endif
            ++ptr;
        } while (--i);
    }
    /* Ball */
    if (sprite[S_BALL].x0_index) {
        p_dest = &collision_vector[0];
        ptr = &sprite[S_BALL].mask_vector[sprite[S_BALL].x0_index + xmin];
        i = width;
        do {
            *p_dest++ |= *ptr++;
        } while (--i);
    }

    /* render line */
    ptr = &collision_vector[0];
    i = width;
    do {
        unsigned object_mask = *ptr++;

        /* use VCS 8bit colours until rendering to RB frame buffer */
        int colind = colour_lookup[object_mask];
        *tv_ptr++ = colour_table[colind];

        /* Collision detection */
        collisions |= col_table[object_mask];
    } while (--i);
}


/* ----------------------------------------------------------------- */


/* "draws" portion of playfield in playfield_mask_vector.
 * the portion to be drawn is determined out of the tia register address */
/* should be called whenever a playfield modifying tia register is written */
//void update_playfield_pf0(void)
static INLINE void
update_playfield_pf0(void)
{
    TST_PROFILE(PRF_UPD_PF, "playfield_update()");
#if TST_TIME_MEASURE
    if (tst_time_measure == 2)
        return;
#endif
    int value = playfield.pf0;
#ifndef RASTER_OPT_PLAYFIELD_TAB32
    int pf_pos_r_direction; // +1 or -1 to inc/dec pointer
    UINT32 *pf_pos_l;
    UINT32 *pf_pos_r;
    pf_pos_l = (UINT32*) &playfield_mask_vector[0];
    if (playfield.ref) {
        pf_pos_r = (UINT32*) &playfield_mask_vector[160-4];
        pf_pos_r_direction = -1;
    }
    else {
        pf_pos_r = (UINT32*) &playfield_mask_vector[80];
        pf_pos_r_direction = 1;
    }
    for (int pfm = 0x10; pfm < 0x100; pfm <<= 1) { /* playfield mask */
        if (value & pfm) {
            *pf_pos_l = PFL_MASK32;
            *pf_pos_r = PFR_MASK32;
        }
        else {
            *pf_pos_l = 0;
            *pf_pos_r = 0;
        }
        pf_pos_l++;
        pf_pos_r += pf_pos_r_direction;
    }
#else /* RASTER_OPT_PLAYFIELD_TAB32 */
    UINT32 *pf_pos;
    UINT32 mask;
    mask = pfl_4bit_ref_lookup[value >> 4];
    pf_pos = (UINT32*) &playfield_mask_v40[0];
    *pf_pos = mask;
    if (playfield.ref) {
        mask = pfr_4bit_lookup[value >> 4];
        pf_pos = (UINT32*) &playfield_mask_v40[36];
    }
    else {
        mask = pfr_4bit_ref_lookup[value >> 4];
        pf_pos = (UINT32*) &playfield_mask_v40[20];
    }
    *pf_pos = mask;
#endif /* RASTER_OPT_PLAYFIELD_TAB32 */
}

/* "draws" portion of playfield in playfield_mask_vector.
 * the portion to be drawn is determined out of the tia register address */
/* should be called whenever a playfield modifying tia register is written */
//void update_playfield_pf1(void)
static INLINE void
update_playfield_pf1(void)
{
    TST_PROFILE(PRF_UPD_PF, "playfield_update()");
#if TST_TIME_MEASURE
    if (tst_time_measure == 2) return;
#endif
    int value = playfield.pf1;
#ifndef RASTER_OPT_PLAYFIELD_TAB32
    int pf_pos_r_direction; // +1 or -1 to inc/dec pointer
    UINT32 *pf_pos_l;
    UINT32 *pf_pos_r;
    pf_pos_l = (UINT32*) &playfield_mask_vector[16];
    if (playfield.ref) {
        pf_pos_r = (UINT32*) &playfield_mask_vector[160-4-16];
        pf_pos_r_direction = -1;
    }
    else {
        pf_pos_r = (UINT32*) &playfield_mask_vector[96];
        pf_pos_r_direction = 1;
    }
    for (int pfm = 0x80; pfm > 0; pfm >>= 1) { /* playfield mask */
        if (value & pfm) {
            *pf_pos_l = PFL_MASK32;
            *pf_pos_r = PFR_MASK32;
        }
        else {
            *pf_pos_l = 0;
            *pf_pos_r = 0;
        }
        pf_pos_l++;
        pf_pos_r += pf_pos_r_direction;
    }
#else /* RASTER_OPT_PLAYFIELD_TAB32 */
    UINT32 *pf_pos_l;
    UINT32 mask;
    mask = pfl_4bit_lookup[value >> 4];
    pf_pos_l = (UINT32*) &playfield_mask_v40[4];
    *pf_pos_l++ = mask;
    mask = pfl_4bit_lookup[value & 0x0f];
    *pf_pos_l = mask;
    if (playfield.ref) {
        pf_pos_l = (UINT32*) &playfield_mask_v40[28];
        mask = pfr_4bit_ref_lookup[value & 0x0f];
        *pf_pos_l++ = mask;
        mask = pfr_4bit_ref_lookup[value >> 4];
        *pf_pos_l = mask;
    }
    else {
        pf_pos_l = (UINT32*) &playfield_mask_v40[24];
        mask = pfr_4bit_lookup[value >> 4];
        *pf_pos_l++ = mask;
        mask = pfr_4bit_lookup[value & 0x0f];
        *pf_pos_l = mask;
    }
    *pf_pos_l = mask;
#endif /* RASTER_OPT_PLAYFIELD_TAB32 */
}

/* "draws" portion of playfield in playfield_mask_vector.
 * the portion to be drawn is determined out of the tia register address */
/* should be called whenever a playfield modifying tia register is written */
//void update_playfield_pf2(void)
static INLINE void
update_playfield_pf2(void)
{
    TST_PROFILE(PRF_UPD_PF, "playfield_update()");
#if TST_TIME_MEASURE
    if (tst_time_measure == 2) return;
#endif
    int value = playfield.pf2;
#ifndef RASTER_OPT_PLAYFIELD_TAB32
    int pf_pos_r_direction; // +1 or -1 to inc/dec pointer
    UINT32 *pf_pos_l;
    UINT32 *pf_pos_r;
    pf_pos_l = (UINT32*) &playfield_mask_vector[48];
    if (playfield.ref) {
        pf_pos_r = (UINT32*) &playfield_mask_vector[160-4-48];
        pf_pos_r_direction = -1;
    }
    else {
        pf_pos_r = (UINT32*) &playfield_mask_vector[128];
        pf_pos_r_direction = 1;
    }
    for (int pfm = 0x01; pfm < 0x100; pfm <<= 1) { /* playfield mask */
        if (value & pfm) {
            *pf_pos_l = PFL_MASK32;
            *pf_pos_r = PFR_MASK32;
        }
        else {
            *pf_pos_l = 0;
            *pf_pos_r = 0;
        }
        pf_pos_l++;
        pf_pos_r += pf_pos_r_direction;
    }
#else /* RASTER_OPT_PLAYFIELD_TAB32 */
    UINT32 *pf_pos_l;
    UINT32 mask;
    pf_pos_l = (UINT32*) &playfield_mask_v40[12];
    mask = pfl_4bit_ref_lookup[value & 0x0f];
    *pf_pos_l++ = mask;
    mask = pfl_4bit_ref_lookup[value >> 4];
    *pf_pos_l = mask;
    if (playfield.ref) {
        pf_pos_l = (UINT32*) &playfield_mask_v40[20];
        mask = pfr_4bit_lookup[value >> 4];
        *pf_pos_l++ = mask;
        mask = pfr_4bit_lookup[value & 0x0f];
        *pf_pos_l = mask;
    }
    else {
        pf_pos_l = (UINT32*) &playfield_mask_v40[32];
        mask = pfr_4bit_ref_lookup[value & 0x0f];
        *pf_pos_l++ = mask;
        mask = pfr_4bit_ref_lookup[value >> 4];
        *pf_pos_l = mask;
    }
    *pf_pos_l = mask;
#endif /* RASTER_OPT_PLAYFIELD_TAB32 */
}

// This function is called if the reflect flag is changed;
// only the right half of the playfield gets updated.
// NOTE: I'm not sure if I really need that... I don't think it will give great savings.
static INLINE void
update_playfield_right_half(void)
{
    TST_PROFILE(PRF_UPD_PF, "playfield_update()");
#if TST_TIME_MEASURE
    if (tst_time_measure == 2) return;
#endif

    /* PF0 */
    unsigned value = playfield.pf0;
#ifndef RASTER_OPT_PLAYFIELD_TAB32
    int pf_pos_r_direction; // +1 or -1 to inc/dec pointer
    UINT32 *pf_pos_r;
    pf_pos_l = (UINT32*) &playfield_mask_vector[0];
    if (playfield.ref) {
        pf_pos_r = (UINT32*) &playfield_mask_vector[160-4];
        pf_pos_r_direction = -1;
    }
    else {
        pf_pos_r = (UINT32*) &playfield_mask_vector[80];
        pf_pos_r_direction = 1;
    }
    for (int pfm = 0x10; pfm < 0x100; pfm <<= 1) { /* playfield mask */
        if (value & pfm) {
            *pf_pos_r = PFR_MASK32;
        }
        else {
            *pf_pos_r = 0;
        }
        pf_pos_r += pf_pos_r_direction;
    }
#else /* RASTER_OPT_PLAYFIELD_TAB32 */
    UINT32 *pf_pos;
    UINT32 mask;
    if (playfield.ref) {
        mask = pfr_4bit_lookup[value >> 4];
        pf_pos = (UINT32*) &playfield_mask_v40[36];
    }
    else {
        mask = pfr_4bit_ref_lookup[value >> 4];
        pf_pos = (UINT32*) &playfield_mask_v40[20];
    }
    *pf_pos = mask;
#endif /* RASTER_OPT_PLAYFIELD_TAB32 */

    /* PF1 */
    TST_PROFILE(PRF_UPD_PF, "playfield_update()");
    value = playfield.pf1;
#ifndef RASTER_OPT_PLAYFIELD_TAB32
    pf_pos_direction; // +1 or -1 to inc/dec pointer
    if (playfield.ref) {
        pf_pos = (UINT32*) &playfield_mask_vector[160-4-16];
        pf_pos_direction = -1;
    }
    else {
        pf_pos = (UINT32*) &playfield_mask_vector[96];
        pf_pos_direction = 1;
    }
    for (int pfm = 0x80; pfm > 0; pfm >>= 1) { /* playfield mask */
        if (value & pfm) {
            *pf_pos = PFR_MASK32;
        }
        else {
            *pf_pos = 0;
        }
        pf_pos += pf_pos_direction;
    }
#else /* RASTER_OPT_PLAYFIELD_TAB32 */
    if (playfield.ref) {
        pf_pos = (UINT32*) &playfield_mask_v40[28];
        mask = pfr_4bit_ref_lookup[value & 0x0f];
        *pf_pos++ = mask;
        mask = pfr_4bit_ref_lookup[value >> 4];
        *pf_pos = mask;
    }
    else {
        pf_pos = (UINT32*) &playfield_mask_v40[24];
        mask = pfr_4bit_lookup[value >> 4];
        *pf_pos++ = mask;
        mask = pfr_4bit_lookup[value & 0x0f];
        *pf_pos = mask;
    }
#endif /* RASTER_OPT_PLAYFIELD_TAB32 */

    /* PF2 */
    value = playfield.pf2;
#ifndef RASTER_OPT_PLAYFIELD_TAB32
    int pf_pos_direction; // +1 or -1 to inc/dec pointer
    if (playfield.ref) {
        pf_pos = (UINT32*) &playfield_mask_vector[160-4-48];
        pf_pos_direction = -1;
    }
    else {
        pf_pos = (UINT32*) &playfield_mask_vector[128];
        pf_pos_direction = 1;
    }
    for (int pfm = 0x01; pfm < 0x100; pfm <<= 1) { /* playfield mask */
        if (value & pfm) {
            *pf_pos = PFR_MASK32;
        }
        else {
            *pf_pos = 0;
        }
        pf_pos += pf_pos_direction;
    }
#else /* RASTER_OPT_PLAYFIELD_TAB32 */
    if (playfield.ref) {
        pf_pos = (UINT32*) &playfield_mask_v40[20];
        mask = pfr_4bit_lookup[value >> 4];
        *pf_pos++ = mask;
        mask = pfr_4bit_lookup[value & 0x0f];
        *pf_pos = mask;
    }
    else {
        pf_pos = (UINT32*) &playfield_mask_v40[32];
        mask = pfr_4bit_ref_lookup[value & 0x0f];
        *pf_pos++ = mask;
        mask = pfr_4bit_ref_lookup[value >> 4];
        *pf_pos = mask;
    }
#endif /* RASTER_OPT_PLAYFIELD_TAB32 */
}

/* redraw the ball in ball_mask_raster[]. The ball is always generated
 * at the same spot, the position is determined by "moving" the address for
 * the first pixel on screen.
 */
/* should be called whenever a ball modifying tia register is written */
static INLINE void
update_ball(void)
{
    TST_PROFILE(PRF_UPD_BL, "update_ball()");

#if !defined(RASTER_NO_32BIT_ZEROS) && !defined(RASTER_OPT_NONE) && !defined(RASTER_OPT_MISSILE_TAB32)
    /* delete any leftovers of old ball size */
    UINT32 *tmpptr = (UINT32*) &sprite[S_BALL].mask_vector[SPRITE_GENERATE_INDEX];
    *tmpptr++ = 0;
    *tmpptr = 0;
#  ifdef RASTER_NOWRAP
    tmpptr = (UINT32*) &sprite[S_BALL].mask_vector[0];
    *tmpptr++ = 0;
    *tmpptr = 0;
#  endif
#endif

#if defined(RASTER_OPT_NONE) || !(defined(RASTER_32BIT_OPT) || defined(RASTER_OPT_MISSILE_TAB32))

    /* don't use 32bit addressing on BYTE arrays for optimization */
    unsigned ballwidth = 1 << sprite[S_BALL].width;
    BYTE *ptr = &sprite[S_BALL].mask_vector[SPRITE_GENERATE_INDEX];
    for (unsigned i = ballwidth; i; --i)
        *ptr++ = BL_MASK;
#  if defined(RASTER_NO_32BIT_ZEROS)
    for (unsigned i = 9 - ballwidth; i; --i)
        *ptr++ = 0;
#  endif
# ifdef RASTER_NOWRAP
    ptr = &sprite[S_BALL].mask_vector[SPRITE_GENERATE_INDEX];
    for (unsigned i = ballwidth; i; --i)
        *ptr++ = BL_MASK;
#  if defined(RASTER_NO_32BIT_ZEROS)
    for (unsigned i = 9 - ballwidth; i; --i)
        *ptr++ = 0;
#  endif
# endif
#else /* !RASTER_OPT_NONE */

# ifdef RASTER_OPT_MISSILE_TAB32
    const UINT32 *srcptr;
    srcptr = &bl_len_lookup[sprite[S_BALL].width << 1];
    UINT32 *tmpptr = (UINT32*)&sprite[S_BALL].mask_vector[SPRITE_GENERATE_INDEX];
    *tmpptr++ = *srcptr;
    *tmpptr = *(srcptr+1);
# else /* !RASTER_OPT_MISSILE_TAB32 */
    //#if defined(RASTER_32BIT_OPT)
    unsigned width = sprite[S_BALL].width;
    UINT32 *ptr = (UINT32*)&sprite[S_BALL].mask_vector[SPRITE_GENERATE_INDEX];
    UINT32 mask0, mask1;
#  ifdef ROCKBOX_BIG_ENDIAN
    mask0 = sprite[S_BALL].mask32;
    if (UNLIKELY(width==3)) {
        mask1 = mask0;
    }
    else {
        mask1 = 0;
        if (width==0)
            mask0 &= 0xff000000;
        else if (width==1)
            mask0 &= 0xffff0000;
    }
    *ptr++ = mask0;
    *ptr = mask1;
#  else /* little endian */
    mask0 = sprite[S_BALL].mask32;
    if (UNLIKELY(width==3)) {
        mask1 = mask0;
    }
    else {
        mask1 = 0;
        if (width==0)
            mask0 &= 0x000000ff;
        else if (width==1)
            mask0 &= 0x0000ffff;
    }
    *ptr++ = mask0;
    *ptr = mask1;
#  endif /* endian */
# ifdef RASTER_NOWRAP
    ptr = (UINT32*)&sprite[S_BALL].mask_vector[0];
    *ptr++ = mask0;
    *ptr = mask1;
# endif
# endif /* RASTER_OPT_MISSILE_TAB32 */
#endif /* optimization */
}


/* lookup table for player and missile (only num and dist) sprites:
 * get number of sprites, distance (in pixel) and size (as multiplyer)
 */
static int player_nusize_num[] =  { 1,  2,  2,  3,  2,  1,  3,  1};
static int player_nusize_dist[] = { 0, 16, 32, 16, 64,  0, 32,  0};
static int player_nusize_size[] = { 1,  1,  1,  1,  1,  2,  1,  4};


/* delete all possible missile leftovers; mind multiple missiles! */
static INLINE void
delete_missiles(unsigned id)
{
    TST_PROFILE(PRF_DEL_MI, "delete_missile()");

#if !defined(RASTER_NO_32BIT_ZEROS)
    UINT32 *ptr;

# ifdef RASTER_NOWRAP
    ptr = (UINT32 *) &sprite[id].mask_vector[0];
# else
    ptr = (UINT32 *) &sprite[id].mask_vector[SPRITE_GENERATE_INDEX];
# endif
    *ptr++ = 0;
    *ptr = 0;
    ptr += 3;
    *ptr++ = 0;
    *ptr = 0;
    ptr += 3;
    *ptr++ = 0;
    *ptr = 0;
    ptr += 7;
    *ptr++ = 0;
    *ptr = 0;
# ifdef RASTER_NOWRAP
    ptr += 23;
    *ptr++ = 0;
    *ptr = 0;
    ptr += 3;
    *ptr++ = 0;
    *ptr = 0;
    ptr += 3;
    *ptr++ = 0;
    *ptr = 0;
    ptr += 7;
    *ptr++ = 0;
    *ptr = 0;
# endif // RASTER_NOWRAP
#else // defined(RASTER_NO_32BIT_ZEROS)
    for (int i=0; i<40; ++i) {
# ifdef RASTER_NOWRAP
        sprite[id].mask_vector[i] = 0;
# endif
        sprite[id].mask_vector[SPRITE_GENERATE_INDEX+i] = 0;
    }
    for (int i=64; i<72; ++i) {
# ifdef RASTER_NOWRAP
        sprite[id].mask_vector[i] = 0;
# endif
        sprite[id].mask_vector[SPRITE_GENERATE_INDEX+i] = 0;
    }
#endif

}


/*
 * id: missile sprite index (S_MISSILE0 or S_MISSILE1)
 */
static INLINE void
update_missile(unsigned id)
{
    unsigned num;    // number of objects (missiles)
    unsigned dist;   //distance of objects
    unsigned width;

    TST_PROFILE(PRF_UPD_MI, "update_missile()");

    num = player_nusize_num[sprite[id].nusize];
    dist = player_nusize_dist[sprite[id].nusize];
#ifdef RASTER_OPT_NONE
    BYTE *misptr;
    width = 1 << sprite[id].width;
    dist -= 8;
    misptr = &sprite[id].mask_vector[SPRITE_GENERATE_INDEX];
    do {
        for (unsigned i = width; i; i--)
            *misptr++ = sprite[id].mask;
        for (unsigned i = 8-width; i; --i)
            *misptr++ = 0;
        misptr += dist;
        --num;
    } while (num > 0);
#  ifdef RASTER_NOWRAP
    misptr = &sprite[id].mask_vector[0];
    num = player_nusize_num[sprite[id].nusize];
    do {
        for (unsigned i = width; i; i--)
            *misptr++ = sprite[id].mask;
        for (unsigned i = 8-width; i; --i)
            *misptr++ = 0;
        misptr += dist;
        --num;
    } while (num > 0);
#  endif
#else /* optimize */
# ifdef RASTER_OPT_MISSILE_TAB32
    dist = (dist >> 2) - 1;
    width = sprite[id].width;
    const UINT32 *srcptr;
    if (id == S_MISSILE0)
        srcptr = &m0_len_lookup[width << 1];
    else
        srcptr = &m1_len_lookup[width << 1];
    UINT32 *tmpptr = (UINT32*)&sprite[id].mask_vector[SPRITE_GENERATE_INDEX];
    do {
        *tmpptr++ = *srcptr;
        *tmpptr = *(srcptr+1);
        tmpptr += dist;
        --num;
    } while (num > 0);
#  ifdef RASTER_NOWRAP
    tmpptr = (UINT32*)&sprite[id].mask_vector[0];
    num = player_nusize_num[sprite[id].nusize];
    do {
        *tmpptr++ = *srcptr;
        *tmpptr = *(srcptr+1);
        tmpptr += dist;
        --num;
    } while (num > 0);
#  endif
# else /* !RASTER_OPT_MISSILE_TAB32 */
    dist = (dist>>2)-1;
    width = sprite[id].width;
    UINT32 *tmpptr = (UINT32*)&sprite[id].mask_vector[SPRITE_GENERATE_INDEX];
    UINT32 mask0, mask1;
#  ifdef ROCKBOX_BIG_ENDIAN
    mask0 = sprite[id].mask32;
    if (UNLIKELY(width==3)) {
        mask1 = mask0;
    }
    else {
        mask1 = 0;
        if (width==0)
            mask0 &= 0xff000000;
        else if (width==1)
            mask0 &= 0xffff0000;
    }
    do {
        *tmpptr++ = mask0;
        *tmpptr = mask1;
        tmpptr += dist;
        --num;
    } while (num > 0);
#  else /* little endian */
    mask0 = sprite[id].mask32;
    if (UNLIKELY(width==3)) {
        mask1 = mask0;
    }
    else {
        mask1 = 0;
        if (width==0)
            mask0 &= 0x000000ff;
        else if (width==1)
            mask0 &= 0x0000ffff;
    }
    do {
        *tmpptr++ = mask0;
        *tmpptr = mask1;
        tmpptr += dist;
        --num;
    } while (num > 0);
#  endif /* endian */
#  ifdef RASTER_NOWRAP
    num = player_nusize_num[sprite[id].nusize];
    tmpptr = (UINT32*)&sprite[id].mask_vector[0];
    do {
        *tmpptr++ = mask0;
        *tmpptr = mask1;
        tmpptr += dist;
        --num;
    } while (num > 0);
#  endif
# endif /* RASTER_OPT_MISSILE_TAB32 */
#endif /* optimization */
}

/*
 * remove player in mask vector; must be called when size or number changes
 */
static INLINE void
delete_player(unsigned id)
{
#if TST_TIME_MEASURE
    if (tst_time_measure == 2) return;
#endif

    UINT32 *ptr;

    TST_PROFILE(PRF_DEL_PL, "delete_player()");
#ifdef RASTER_NO_32BIT_ZEROS
#  ifdef RASTER_NOWRAP
    for (int i=0; i<40; ++i)
        sprite[id].mask_vector[i] = 0;
    for (int i=64; i<72; ++i)
        sprite[id].mask_vector[i] = 0;
#  endif
    for (int i=0; i<40; ++i)
        sprite[id].mask_vector[SPRITE_GENERATE_INDEX+i] = 0;
    for (int i=64; i<72; ++i)
        sprite[id].mask_vector[SPRITE_GENERATE_INDEX+i] = 0;
#else
#  ifdef RASTER_NOWRAP
    ptr = (UINT32 *) &sprite[id].mask_vector[0];
#  else
    ptr = (UINT32 *) &sprite[id].mask_vector[SPRITE_GENERATE_INDEX];
#  endif
    for (unsigned i=10; i; --i) {
        *ptr++ = 0;
    }
    ptr += 6;
    *ptr++ = 0;
    *ptr = 0;
#  ifdef RASTER_NOWRAP
    ptr += 23;
    for (unsigned i=10; i; --i) {
        *ptr++ = 0;
    }
    ptr += 6;
    *ptr++ = 0;
    *ptr = 0;
#  endif
#endif
}


/*
 * id: player sprite index (S_PLAYER0 or S_PLAYER1)
 * update player graphic. Note: number and distance must be maintain unchanged
 */
static INLINE void
update_player(int id)
{
#if TST_TIME_MEASURE
    if (tst_time_measure == 3) return;
#endif

    BYTE *plptr;
    int obj_num,
        obj_dist,
        obj_size;

    TST_PROFILE(PRF_UPD_PL, "update_player()");

    obj_num = player_nusize_num[sprite[id].nusize];
    obj_dist = player_nusize_dist[sprite[id].nusize];
    obj_size = player_nusize_size[sprite[id].nusize];

    obj_dist -= obj_size*8;

    plptr = &sprite[id].mask_vector[SPRITE_GENERATE_INDEX];
    if (LIKELY(obj_size==1)) {
#ifndef RASTER_OPT_PLAYER_TAB32
        do {
            if (!sprite[id].reflect) {
                for (int pl_mask= 0x80; pl_mask > 0; pl_mask >>= 1) {
                    if (sprite[id].grp & pl_mask)
                        *plptr = sprite[id].mask;
                    else
                        *plptr = 0;
                    ++plptr;
                }
            }
            else {
                for (int pl_mask= 0x01; pl_mask < 0x100; pl_mask <<= 1) {
                    if (sprite[id].grp & pl_mask)
                        *plptr = sprite[id].mask;
                    else
                        *plptr = 0;
                    ++plptr;
                }
            }
            plptr += obj_dist;
            --obj_num;
        } while (obj_num > 0);
#  ifdef RASTER_NOWRAP
        plptr = &sprite[id].mask_vector[0];
        obj_num = player_nusize_num[sprite[id].nusize];
        do {
            if (!sprite[id].reflect) {
                for (int pl_mask= 0x80; pl_mask > 0; pl_mask >>= 1) {
                    if (sprite[id].grp & pl_mask)
                        *plptr = sprite[id].mask;
                    else
                        *plptr = 0;
                    ++plptr;
                }
            }
            else {
                for (int pl_mask= 0x01; pl_mask < 0x100; pl_mask <<= 1) {
                    if (sprite[id].grp & pl_mask)
                        *plptr = sprite[id].mask;
                    else
                        *plptr = 0;
                    ++plptr;
                }
            }
            plptr += obj_dist;
            --obj_num;
        } while (obj_num > 0);
#  endif
#else /* RASTER_OPT_PLAYER_TAB32 */
        UINT32 *dst = (UINT32 *) &sprite[id].mask_vector[SPRITE_GENERATE_INDEX];
        UINT32 src1, src2;
        if (!sprite[id].reflect) {
            if (id==S_PLAYER0) {
                src1 = pl0_4bit_lookup[sprite[id].grp >> 4];
                src2 = pl0_4bit_lookup[sprite[id].grp & 0x0f];
            }
            else {
                src1 = pl1_4bit_lookup[sprite[id].grp >> 4];
                src2 = pl1_4bit_lookup[sprite[id].grp & 0x0f];
            }
        }
        else {
            if (id==S_PLAYER0) {
                src1 = pl0_4bit_ref_lookup[sprite[id].grp & 0x0f];
                src2 = pl0_4bit_ref_lookup[sprite[id].grp >> 4];
            }
            else {
                src1 = pl1_4bit_ref_lookup[sprite[id].grp & 0x0f];
                src2 = pl1_4bit_ref_lookup[sprite[id].grp >> 4];
            }
        }
        obj_dist = (obj_dist >> 2) + 1;
        do {
            *dst++ = src1;
            *dst = src2;
            dst += obj_dist;
            --obj_num;
        } while (obj_num > 0);
#  ifdef RASTER_NOWRAP
        dst = (UINT32 *) &sprite[id].mask_vector[0];
        obj_num = player_nusize_num[sprite[id].nusize];
        do {
            *dst++ = src1;
            *dst = src2;
            dst += obj_dist;
            --obj_num;
        } while (obj_num > 0);
#  endif // RASTER_NOWRAP
#endif /* RASTER_OPT_PLAYER_TAB32 */
    }
    else if (obj_size==2) {
        do {
            if (!sprite[id].reflect) {
                for (int pl_mask= 0x80; pl_mask > 0; pl_mask >>= 1) {
                    if (sprite[id].grp & pl_mask)
                        *(UINT16*)plptr = sprite[id].mask16;
                    else
                        *(UINT16*)plptr = 0;
                    plptr += 2;
                }
            }
            else {
                for (int pl_mask= 0x01; pl_mask < 0x100; pl_mask <<= 1) {
                    if (sprite[id].grp & pl_mask)
                        *(UINT16*)plptr = sprite[id].mask16;
                    else
                        *(UINT16*)plptr = 0;
                    plptr += 2;
                }
            }
            plptr += obj_dist;
            --obj_num;
        } while (obj_num > 0);
#  ifdef RASTER_NOWRAP
        plptr = &sprite[id].mask_vector[0];
        obj_num = player_nusize_num[sprite[id].nusize];
        do {
            if (!sprite[id].reflect) {
                for (int pl_mask= 0x80; pl_mask > 0; pl_mask >>= 1) {
                    if (sprite[id].grp & pl_mask)
                        *(UINT16*)plptr = sprite[id].mask16;
                    else
                        *(UINT16*)plptr = 0;
                    plptr += 2;
                }
            }
            else {
                for (int pl_mask= 0x01; pl_mask < 0x100; pl_mask <<= 1) {
                    if (sprite[id].grp & pl_mask)
                        *(UINT16*)plptr = sprite[id].mask16;
                    else
                        *(UINT16*)plptr = 0;
                    plptr += 2;
                }
            }
            plptr += obj_dist;
            --obj_num;
        } while (obj_num > 0);
#  endif // RASTER_NOWRAP
    }
    else { /* obj_size==4 */
        do {
            if (!sprite[id].reflect) {
                for (int pl_mask= 0x80; pl_mask > 0; pl_mask >>= 1) {
                    if (sprite[id].grp & pl_mask)
                        *(UINT32*)plptr = sprite[id].mask32;
                    else
                        *(UINT32*)plptr = 0;
                    plptr += 4;
                }
            }
            else {
                for (int pl_mask= 0x01; pl_mask < 0x100; pl_mask <<= 1) {
                    if (sprite[id].grp & pl_mask)
                        *(UINT32*)plptr = sprite[id].mask32;
                    else
                        *(UINT32*)plptr = 0;
                    plptr += 4;
                }
            }
            plptr += obj_dist;
            --obj_num;
        } while (obj_num > 0);
#  ifdef RASTER_NOWRAP
        plptr = &sprite[id].mask_vector[0];
        obj_num = player_nusize_num[sprite[id].nusize];
        do {
            if (!sprite[id].reflect) {
                for (int pl_mask= 0x80; pl_mask > 0; pl_mask >>= 1) {
                    if (sprite[id].grp & pl_mask)
                        *(UINT32*)plptr = sprite[id].mask32;
                    else
                        *(UINT32*)plptr = 0;
                    plptr += 4;
                }
            }
            else {
                for (int pl_mask= 0x01; pl_mask < 0x100; pl_mask <<= 1) {
                    if (sprite[id].grp & pl_mask)
                        *(UINT32*)plptr = sprite[id].mask32;
                    else
                        *(UINT32*)plptr = 0;
                    plptr += 4;
                }
            }
            plptr += obj_dist;
            --obj_num;
        } while (obj_num > 0);
#  endif // RASTER_NOWRAP
    }
}


// macro for: set x-position of Sprite
#define SPRITE_MASKV_SETX(id) \
    sprite[id].x0_index = SPRITE_GENERATE_INDEX - sprite[id].x

// macro for: set read-index to "disable"
#define SPRITE_MASKV_DISABLE(id) \
    sprite[id].x0_index = 0


/*
 * tia register is written:
 * execute tia command.
 * a=tia address, b=byte to be written
 * return: 1 if wait for sync register is written, 0 otherwise
 */
int do_tia_write(int a, int b)
//int do_tia_write(unsigned a, unsigned b)
{
    int tmp;

    TST_PROFILE(PRF_TIA_ACC, "do_tia_access()");
#if TST_TIME_MEASURE
    if (tst_time_measure >= 6) {
        /* Time Measurement: without TIA access. Only Syncs must be handled. */
        if (a==WSYNC) {
            return 1;   /* this reports the "WSYNC" to the caller! */
        }
        if (a==VSYNC) {
            if (b & 0x02) {
                /* Start vertical sync */
                if (!(sync_state & SYNC_STATE_VSYNC_RUNNING))
                    sync_state |= (SYNC_STATE_VSYNC_WRITTEN | SYNC_STATE_VSYNC_RUNNING);
            }
            else {
                sync_state &= ~(SYNC_STATE_VSYNC_WRITTEN | SYNC_STATE_VSYNC_RUNNING);
            }
        }
        sync_state |= SYNC_STATE_VBLANK; /* pretend vsync to skip raster and draw function */
        return 0;
    }
#endif

    switch (a) {
    case VSYNC:
        if (b & 0x02) {
            /* Start vertical sync */
            if (!(sync_state & SYNC_STATE_VSYNC_RUNNING))
                sync_state |= (SYNC_STATE_VSYNC_WRITTEN | SYNC_STATE_VSYNC_RUNNING);
        }
        else {
            sync_state &= ~(SYNC_STATE_VSYNC_WRITTEN | SYNC_STATE_VSYNC_RUNNING);
        }
        break;
    case VBLANK:
        if (b & 0x02) {
            sync_state |= SYNC_STATE_VBLANK;
        }
        else {
            sync_state &= ~SYNC_STATE_VBLANK;
        }
        /* Ground paddle pots */
        if (b & 0x80) {
            /* Grounded ports */
            tiaRead[INPT0] = 0x00;
            tiaRead[INPT1] = 0x00;
        }
        else {
            /* Processor now measures time for a logic 1 to appear
               at each paddle port */
            tiaRead[INPT0] = 0x80;
            tiaRead[INPT1] = 0x80;
            paddle[0].val = clk;
            paddle[1].val = clk;
        }
        /* Logic for dumped input ports */
        if (b & 0x40) {
            if (tiaWrite[VBLANK] & 0x40) {
                tiaRead[INPT4] = 0x80;
                tiaRead[INPT5] = 0x80;
            }
            else {
                read_trigger ();
            }
        }
        tiaWrite[VBLANK] = b;
        break;
    case WSYNC:
        /* Skip to HSYNC pulse */
        /* completely handled outside of this function */
        return 1;   /* this reports the "WSYNC" to the caller! */
        break;
    case RSYNC:
        /* used in chip testing */
        break;
    case NUSIZ0:    /* number and size of players and missiles */
                    /* Bit 0..2 : Player/Missile number and Player size */
                    /* Bit 4..5 : Missile size 1/2/4/8 pixel (00/01/10/11) */
#  if TST_TIME_MEASURE
        if (tst_time_measure == 3) {
            if ((b & 0x30) >> 4 != sprite[S_MISSILE0].width) {
                sprite[S_MISSILE0].width = (b & 0x30) >> 4;
                update_missile(S_MISSILE0);
            }
            break;
        }
#  endif
        tmp = (b & 0x07); /* number and size of players */
        if (sprite[S_PLAYER0].nusize != tmp) {
            sprite[S_PLAYER0].nusize = tmp;
            // !!! TODO: update must look if "old" (vdel!) or "new" bitmap is used!
            // TODO: only call if player0 enabled?
            delete_player(S_PLAYER0);
            update_player(S_PLAYER0);
            sprite[S_MISSILE0].nusize = tmp;
            sprite[S_MISSILE0].width = (b & 0x30) >> 4;
            // TODO: only call if missile0 enabled?
            delete_missiles(S_MISSILE0);
            update_missile(S_MISSILE0);
        }
        else if ((b & 0x30) >> 4 != sprite[S_MISSILE0].width) {
            sprite[S_MISSILE0].width = (b & 0x30) >> 4;
            update_missile(S_MISSILE0);
        }
        break;
    case NUSIZ1:
#  if TST_TIME_MEASURE
        if (tst_time_measure == 3) {
            if ((b & 0x30) >> 4 != sprite[S_MISSILE1].width) {
                sprite[S_MISSILE1].width = (b & 0x30) >> 4;
                update_missile(S_MISSILE1);
            }
            break;
        }
#  endif
        tmp = (b & 0x07); /* number and size of players */
        if (sprite[S_PLAYER1].nusize != tmp) {
            sprite[S_PLAYER1].nusize = tmp;
            delete_player(S_PLAYER1);
            update_player(S_PLAYER1);
            sprite[S_MISSILE1].nusize = tmp;
            sprite[S_MISSILE1].width = (b & 0x30) >> 4;
            delete_missiles(S_MISSILE1);
            update_missile(S_MISSILE1);
        }
        else if ((b & 0x30) >> 4 != sprite[S_MISSILE1].width) {
            sprite[S_MISSILE1].width = (b & 0x30) >> 4;
            update_missile(S_MISSILE1);
        }
        break;
    case COLUP0:
        /* P0M0 colour */
        colour_table[P0M0_COLOUR] = b;
        break;
    case COLUP1:
        /* P1M1 colour */
        colour_table[P1M1_COLOUR] = b;
        break;
    case COLUPF:
        /* PFBL colour */
        colour_table[PFBL_COLOUR] = b;
        break;
    case COLUBK:
        colour_table[BK_COLOUR] = b;
        break;
    case CTRLPF:
        /* set ball size */
        tmp = (b & 0x30) >> 4;
        if (sprite[S_BALL].width != tmp) {
            sprite[S_BALL].width = tmp;
            update_ball();
        }
#  if TST_TIME_MEASURE
        if (tst_time_measure == 2) break;
#  endif
        /* Reflect Playfield */
        tmp = b & 0x01;
        if (playfield.ref != tmp) {
            playfield.ref = tmp;
            // TODO: only right half needs to be updated!
#   if 1
            update_playfield_pf0();
            update_playfield_pf1();
            update_playfield_pf2();
#   else    // TODO: stop the time if this is really useful.
            // I'm not sure if I really need this.
            update_playfield_right_half();
#   endif
        }
        /* Playfield priority and Playfield Score Colors */
        switch (b & 0x06) {
            case 0x00:  /* normal priority, no scores */
                // TODO: check if this level of indirection can be removed
                // and colours can be changed directly. stop time!
                colour_lookup = colour_normal;
                break;
            case 0x02:  /* normal priority, score colors */
                colour_lookup = colour_scores;
                break;
            case 0x04:  /* playfield priority, no scores */
                colour_lookup = colour_priority;
                break;
            case 0x06:  /* playfield priority, score colors */
                colour_lookup = colour_priority;
                break;
        }
        break;

    case REFP0:
#  if TST_TIME_MEASURE
        if (tst_time_measure == 3) break;
#  endif
        tmp = (b & 0x08);
        if (sprite[S_PLAYER0].reflect != tmp) {
            sprite[S_PLAYER0].reflect = tmp;
            update_player(S_PLAYER0);
        }
        break;
    case REFP1:
#  if TST_TIME_MEASURE
        if (tst_time_measure == 3) break;
#  endif
        tmp = (b & 0x08);
        if (sprite[S_PLAYER1].reflect != tmp) {
            sprite[S_PLAYER1].reflect = tmp;
            update_player(S_PLAYER1);
        }
        break;

    /* playfield raster change */
    case PF0:
#  if TST_TIME_MEASURE
        if (tst_time_measure == 2) break;
#  endif
        tmp = (b & 0xf0);
        if (playfield.pf0 != tmp) {
            playfield.pf0 = tmp;
            update_playfield_pf0();
        }
        break;
    case PF1:
#  if TST_TIME_MEASURE
        if (tst_time_measure == 2) break;
#  endif
        if (playfield.pf1 != b) {
            playfield.pf1 = b;
            update_playfield_pf1();
        }
        break;
    case PF2:
#  if TST_TIME_MEASURE
        if (tst_time_measure == 2) break;
#  endif
        if (playfield.pf2 != b) {
            playfield.pf2 = b;
            update_playfield_pf2();
        }
        break;

    /* reset sprite to beam position */
    /* leftmost x coordinates according to https://bumbershootsoft.wordpress.com/2018/08/27/atari-2600-placing-static-sprites/ */
    case RESP0:
#  if TST_TIME_MEASURE
        if (tst_time_measure == 3) break;
#  endif
        // TODO: Player is delayed by 5 pixel from actual color clock.
        // The first copy of the player shouldn't be displayed
        // until the next scanline.
        sprite[S_PLAYER0].x = ebeamx + 5;
        if (sprite[S_PLAYER0].x < 0)
            sprite[S_PLAYER0].x = 3;
        if (sprite[S_PLAYER0].width)
            sprite[S_PLAYER0].x += 1;
        //if (sprite[S_PLAYER0].x >= 160)
        //    sprite[S_PLAYER0].x -= 160;
        if (sprite[S_PLAYER0].x0_index)
            SPRITE_MASKV_SETX(S_PLAYER0);
        break;
    case RESP1:
#  if TST_TIME_MEASURE
        if (tst_time_measure == 3) break;
#  endif
        sprite[S_PLAYER1].x = ebeamx + 5;
        if (sprite[S_PLAYER1].x < 0)
            sprite[S_PLAYER1].x = 3;
        if (sprite[S_PLAYER1].width)
            sprite[S_PLAYER1].x += 1;
        //if (sprite[S_PLAYER1].x >= 160)
        //    sprite[S_PLAYER1].x -= 160;
        if (sprite[S_PLAYER1].x0_index)
            SPRITE_MASKV_SETX(S_PLAYER1);
        break;
    case RESM0:
        sprite[S_MISSILE0].x = ebeamx + 4;
        if (sprite[S_MISSILE0].x < 0)
            sprite[S_MISSILE0].x = 2;
        if(sprite[S_MISSILE0].x0_index)
            SPRITE_MASKV_SETX(S_MISSILE0);
        break;
    case RESM1:
        sprite[S_MISSILE1].x = ebeamx + 4;
        if (sprite[S_MISSILE1].x < 0)
            sprite[S_MISSILE1].x = 2;
        if(sprite[S_MISSILE1].x0_index)
            SPRITE_MASKV_SETX(S_MISSILE1);
        break;
    case RESBL:
        sprite[S_BALL].x = ebeamx + 4;
        if (sprite[S_BALL].x < 0)
            sprite[S_BALL].x = 2;
#if 1
        if (sprite[S_BALL].x0_index)
#else
        if((sprite[S_BALL].enabled && !sprite[S_BALL].vdel_flag) ||
                (sprite[S_BALL].vdel_flag && sprite[S_BALL].vdel))
#endif
            SPRITE_MASKV_SETX(S_BALL);
        break;

    /* TIA sound */
#if 0 // TO BE DONE
    case AUDC0:
        sound_waveform (0, b & 0x0f);
        break;
    case AUDC1:
        sound_waveform (1, b & 0x0f);
        break;
    case AUDF0:
        sound_freq (0, b & 0x1f);
        break;
    case AUDF1:
        sound_freq (1, b & 0x1f);
        break;
    case AUDV0:
        sound_volume (0, b & 0x0f);
        break;
    case AUDV1:
        sound_volume (1, b & 0x0f);
        break;
#endif // TO BE DONE

    /* change player graphic */
    case GRP0:
#  if TST_TIME_MEASURE
        if (tst_time_measure == 3) break;
#  endif
        if (sprite[S_PLAYER0].grp != b) {
            sprite[S_PLAYER0].grp = b;
            if (!sprite[S_PLAYER0].vdel_flag) {
                if (b) {
                    update_player(S_PLAYER0);
                    // could have been disabled...
                    SPRITE_MASKV_SETX(S_PLAYER0);
                }
                else
                    SPRITE_MASKV_DISABLE(S_PLAYER0);
            }
        }
        if (sprite[S_PLAYER1].vdel_flag) {
            if (sprite[S_PLAYER1].vdel != sprite[S_PLAYER1].grp) {
                sprite[S_PLAYER1].vdel = sprite[S_PLAYER1].grp;
                if (sprite[S_PLAYER1].vdel) {
                    update_player(S_PLAYER1);
                    // could have been disabled...
                    SPRITE_MASKV_SETX(S_PLAYER1);
                }
                else
                    SPRITE_MASKV_DISABLE(S_PLAYER1);
            }
        }
        break;
    case GRP1:
#  if TST_TIME_MEASURE
        if (tst_time_measure == 3) break;
#  endif
        if (sprite[S_PLAYER1].grp != b) {
            sprite[S_PLAYER1].grp = b;
            if (!sprite[S_PLAYER1].vdel_flag) {
                if (b) {
                    update_player(S_PLAYER1);
                    // could have been disabled...
                    SPRITE_MASKV_SETX(S_PLAYER1);
                }
                else
                    SPRITE_MASKV_DISABLE(S_PLAYER1);
            }
        }
        // vertical delay: update Player 0 and Ball
        if (sprite[S_PLAYER0].vdel_flag) {
            if (sprite[S_PLAYER0].vdel != sprite[S_PLAYER0].grp) {
                sprite[S_PLAYER0].vdel = sprite[S_PLAYER0].grp;
                if (sprite[S_PLAYER0].vdel) {
                    update_player(S_PLAYER0);
                    // could have been disabled...
                    SPRITE_MASKV_SETX(S_PLAYER0);
                }
                else
                    SPRITE_MASKV_DISABLE(S_PLAYER0);
            }
        }
        if (sprite[S_BALL].vdel_flag) {
            if (sprite[S_BALL].vdel != sprite[S_BALL].enabled) {
                sprite[S_BALL].vdel = sprite[S_BALL].enabled;
                if (sprite[S_BALL].vdel)
                    SPRITE_MASKV_SETX(S_BALL);
                else
                    SPRITE_MASKV_DISABLE(S_BALL);
            }
        }
        break;

    /* enable missile/ball */
    case ENAM0:
        tmp = b & 0x02;
        if (sprite[S_MISSILE0].enabled != tmp) {
            sprite[S_MISSILE0].enabled = tmp;
            if (tmp)
                SPRITE_MASKV_SETX(S_MISSILE0);
            else
                SPRITE_MASKV_DISABLE(S_MISSILE0);
        }
        break;
    case ENAM1:
        tmp = b & 0x02;
        if (sprite[S_MISSILE1].enabled != tmp) {
            sprite[S_MISSILE1].enabled = tmp;
            if (tmp)
                SPRITE_MASKV_SETX(S_MISSILE1);
            else
                SPRITE_MASKV_DISABLE(S_MISSILE1);
        }
        break;
    case ENABL:
        tmp = b & 0x02;
        sprite[S_BALL].enabled = tmp;
        if (!sprite[S_BALL].vdel_flag) {
            if (tmp)
                SPRITE_MASKV_SETX(S_BALL);
            else
                SPRITE_MASKV_DISABLE(S_BALL);
        }
        break;

    /* horizontal movement */
    case HMP0:  /* horizontal motion player 0 */
        sprite[S_PLAYER0].hmm = ((signed char)b) >> 4;
        break;
    case HMP1:
        sprite[S_PLAYER1].hmm = ((signed char)b) >> 4;
        break;
    case HMM0:
        sprite[S_MISSILE0].hmm = ((signed char)b) >> 4;
        break;
    case HMM1:
        sprite[S_MISSILE1].hmm = ((signed char)b) >> 4;
        break;
    case HMBL:
        sprite[S_BALL].hmm = ((signed char)b) >> 4;
        break;

    /* vertical delay flags */
    case VDELP0:    /* vertical delay flags */
#  if TST_TIME_MEASURE
        if (tst_time_measure == 3) break;
#  endif
        tmp = b & 0x01;
        if (sprite[S_PLAYER0].vdel_flag != tmp) {
            sprite[S_PLAYER0].vdel_flag = tmp;
            if (!tmp) {                 /* switch from delayed to normal */
                if (LIKELY(!sprite[S_PLAYER0].grp)) {
                    SPRITE_MASKV_DISABLE(S_PLAYER0);
                }
                else {
                    update_player(S_PLAYER0);
                    // could have been disabled...
                    SPRITE_MASKV_SETX(S_PLAYER0);
                }
            }
            else {                      /* switch from normal to delayed */
                /* nothing can be done now, because I don't save delayed GRP */
            }
        }
        break;
    case VDELP1:
#  if TST_TIME_MEASURE
        if (tst_time_measure == 3) break;
#  endif
        tmp = b & 0x01;
        if (sprite[S_PLAYER1].vdel_flag != tmp) {
            sprite[S_PLAYER1].vdel_flag = tmp;
            if (!tmp) {                 /* switch from delayed to normal */
                if (LIKELY(!sprite[S_PLAYER1].grp)) {
                    SPRITE_MASKV_DISABLE(S_PLAYER1);
                }
                else {
                    update_player(S_PLAYER1);
                    // could have been disabled...
                    SPRITE_MASKV_SETX(S_PLAYER1);
                }
            }
            else {                      /* switch from normal to delayed */
                /* nothing can be done now, because I don't save delayed GRP */
            }
        }
        break;
    case VDELBL:
        sprite[S_BALL].vdel_flag = b & 0x01;
        break;

    case RESMP0:
        // reset horizontal pos of missile to player
        tiaWrite[RESMP0] = b & 0x02;
        if (b & 0x02) {
            SPRITE_MASKV_DISABLE(S_MISSILE0);
        }
        else {
            sprite[S_MISSILE0].x = sprite[S_PLAYER0].x + 4;
            if (sprite[S_MISSILE0].enabled) {
                SPRITE_MASKV_SETX(S_MISSILE0);
            }
        }
        break;
    case RESMP1:
        tiaWrite[RESMP1] = b & 0x02;
        if (b & 0x02) {
            SPRITE_MASKV_DISABLE(S_MISSILE1);
        }
        else {
            sprite[S_MISSILE1].x = sprite[S_PLAYER1].x + 4;
            if (sprite[S_MISSILE1].enabled) {
                SPRITE_MASKV_SETX(S_MISSILE1);
            }
        }
        break;

    /* horizontal move */
    case HMOVE:
# if 1 // TODO: find the correct pixel for this effect
        if (ebeamx > -56) { // arbitrary number; no clue what it should be
            //DEBUGF("received HMOVE outside beginning of Line! x=%d, y=%d\n", ebeamx,ebeamy);
            for (int i = NUM_SPRITES-1; i >= 0; --i) {
                sprite[i].x -= 8;
            }
        }
# endif
        for (int i = NUM_SPRITES-1; i >= 0; --i) {
            sprite[i].x -= sprite[i].hmm;
            if (sprite[i].x >= 160)
                sprite[i].x -= 160;
            else if (sprite[i].x < 0)
                sprite[i].x += 160;
            if (sprite[i].x0_index)
                SPRITE_MASKV_SETX(i);
        }
        break;

    /* clear horizontal movement registers */
    case HMCLR:
        for (int i = NUM_SPRITES-1; i >= 0; --i) {
            sprite[i].hmm = 0;
        }
        break;
    case CXCLR:     /* clear collision latches */
        collisions=0;
        break;
    }

    return 0;
}

// ------------------------------------------------------------


static void make_raster_tab(UINT32 *dest, const unsigned mask, const bool reflected) COLD_ATTR;
static void make_raster_tab(UINT32 *dest, const unsigned mask, const bool reflected)
{
    for (int i = 0; i < 16; ++i) {
        UINT32 m;
#  ifdef ROCKBOX_BIG_ENDIAN
        if (!reflected)
            m = mask;
        else
            m = mask << 24;
#  else /* little endian */
        if (!reflected)
            m = mask << 24;
        else
            m = mask;
#  endif /* endian */
        *dest = 0;
        for (int j = 0; j < 4; ++j) {
            if (i & (1 << j))
                *dest |= m;
#  ifdef ROCKBOX_BIG_ENDIAN
            if (!reflected)
                m <<= 8;
            else
                m >>= 8;
#  else /* little endian */
            if (!reflected)
                m >>= 8;
            else
                m <<= 8;
#  endif /* endian */
        }
        ++dest;
    }
}


void init_sprite_tables(void) COLD_ATTR;
void init_sprite_tables(void)
{
    make_raster_tab(pl0_4bit_lookup, PL0_MASK, false);
    make_raster_tab(pl0_4bit_ref_lookup, PL0_MASK, true);
    make_raster_tab(pl1_4bit_lookup, PL1_MASK, false);
    make_raster_tab(pl1_4bit_ref_lookup, PL1_MASK, true);
    make_raster_tab(pfl_4bit_lookup, PFL_MASK, false);
    make_raster_tab(pfl_4bit_ref_lookup, PFL_MASK, true);
    make_raster_tab(pfr_4bit_lookup, PFR_MASK, false);
    make_raster_tab(pfr_4bit_ref_lookup, PFR_MASK, true);
}
