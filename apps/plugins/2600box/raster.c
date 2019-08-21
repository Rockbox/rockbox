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

// as by https://graphics.stanford.edu/~seander/bithacks.html#BitReverseTable
static const BYTE bit_reverse_table[256] =
{
#   define R2(n)     n,     n + 2*64,     n + 1*64,     n + 3*64
#   define R4(n) R2(n), R2(n + 2*16), R2(n + 1*16), R2(n + 3*16)
#   define R6(n) R4(n), R4(n + 2*4 ), R4(n + 1*4 ), R4(n + 3*4 )
    R6(0), R6(2), R6(1), R6(3)
};
#define REVERSEBITS(n)      bit_reverse_table[n]


/* The PlayField structure */
/*
 * playfield graphic is always stored left-to-right (lsb-msb) for left side,
 * so pf1 bits must be reversed.
 */
static struct PlayField {
    BYTE ref;
    BYTE pf0,pf1,pf2;
    BYTE pf0r,pf1r,pf2r;
} playfield;

enum {S_PLAYER0=0, S_PLAYER1, S_MISSILE0, S_MISSILE1, S_BALL, NUM_SPRITES};

struct sprite {
    BYTE    grp_en;     /* player graphics resp. missile/ball enable flag */
    BYTE    new;        /* new resp. old grp/enable. The relevant value is copied to grp_en */
    BYTE    old;        /* (only used for sprites that have vdel (Players and Ball)) */
    BYTE    vdel_flag;  /* indicates if vertical delay is used (only players and ball!) */
    int     x;
    int     hmm;        /* horizontal movement; hold an actual signed -8 to +7 value! */
    uint8_t     width;  /* ?? in binary coding? 0x01=double, ... */
                        /* for missiles and ball: actual width in pixels */
    //int     number;   /* number of player/missile copies */ // TODO: do I need distance as well?
    BYTE    nusize;     /* only for player, and ONLY used to determin if values have changed; */
                        /* actual values for number and width are stored in .width and .number */
    BYTE    reflect;
    BYTE    mask;
    //UINT16  mask16;
    //UINT32  mask32;
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
#define BL_MASK32       (BL_MASK | BL_MASK<<8 | BL_MASK<<16 | BL_MASK<<24)


/* The collision lookup table */
unsigned short col_table[MASK_MAX];

/* The collision state */
unsigned short collisions;

// exact size to be calculated. Must be >160, because we don't do pixel-exact wraparound!
static BYTE collision_vector[320] ALIGNED_ATTR(4);

/* ----------------------------------------------------------------------- */


/*
 * helper function for init_collision()
 */
/* Does collision testing on the pixel b */
/* b: byte to test for collisions */
/* Used to build up the collision table */
static int set_collisions (const unsigned b) COLD_ATTR;
static int set_collisions (const unsigned b)
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

void init_raster(void) COLD_ATTR;
void init_raster(void)
{
    init_collision_lookup();
    init_color_lookup();
    colour_lookup=colour_normal;

    rb->memset(collision_vector, 0, sizeof(collision_vector));
    rb->memset(&playfield, 0, sizeof(playfield));
    rb->memset(sprite, 0, sizeof(sprite));

    sprite[S_PLAYER0].mask = PL0_MASK;
    sprite[S_PLAYER1].mask = PL1_MASK;
    sprite[S_MISSILE0].mask = ML0_MASK;
    sprite[S_MISSILE1].mask = ML1_MASK;
    sprite[S_BALL].mask = BL_MASK;

#if 0
    for (unsigned i = 0; i < NUM_SPRITES; i++) {
        sprite[i].hmm = 0x0;
        sprite[i].x = 0x0;
        sprite[i].nusize = 0;
        sprite[i].grp_en = 0;
        sprite[i].vdel_flag = 0;
        //sprite[i].mask16 = (sprite[i].mask << 8) | sprite[i].mask;
        //sprite[i].mask32 = (sprite[i].mask16 << 16) | sprite[i].mask16;
        sprite[i].width = 0;
    }
#endif
}

/* ----------------------------------------------------------------------- */

void do_raster(int xmin, int xmax)
{
    TST_PROFILE(PRF_MASK_RENDER, "do_raster()");
#if TST_TIME_USEC
    unsigned long tmp_usec;
    if (tst_time_usec_start > 0) {
        tmp_usec = USEC_TIMER;
    }
#endif

    if (xmax <= 0) // should never happen
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

    int i;
    BYTE *ptr;

    /* render line */
    BYTE *tv_ptr = &tv_screen[ebeamy][xmin];
    ptr = &collision_vector[xmin];
    i = width;
    do {
        unsigned object_mask = *ptr++;

        /* use VCS 8bit colours until rendering to RB frame buffer */
        int colind = colour_lookup[object_mask];
        *tv_ptr++ = colour_table[colind];

        /* Collision detection */
        collisions |= col_table[object_mask];
    } while (--i);
#if TST_TIME_USEC
    if (tst_time_usec_start > 0) {
        tmp_usec = USEC_TIMER - tmp_usec;
        tst_usec[TST_USEC_RASTER] += tmp_usec;
    }
#endif
}


/* ----------------------------------------------------------------- */

/* -------------------------------------------------------------------------
 * Helper functions for tia-write:
 * PLAYFIELD
 */

/**
 * @brief update playfield graphic in collision_vector.
 * @param ptr   pointer to starting point in collision pointer. Note that the
 *              vector is accessed as UINT32 (update 4 pixels at once), so
 *              pointer must be divideable by 4.
 * @param gr    the pixel graphic to be xored to the vector. The caller must
 *              pass gr as "new_graphic^old_graphic". Note that gr is not limited
 *              to 8 bit size!
 * @param mask  must be PFL_MASK32 or PFR_MASK32 (PFL_MASK or PFR_MASK without
 *              32 bit optimization)
 */
static INLINE
#ifdef RASTER_NO_32BIT_OPT
void update_playfield(BYTE *ptr, unsigned gr, const unsigned mask)
#else
void update_playfield(UINT32 *ptr, unsigned gr, const UINT32 mask)
#endif
{
    while (gr) {
# ifdef RASTER_NO_32BIT_OPT
        if (gr & 0x01) {
            *ptr++ ^= mask;
            *ptr++ ^= mask;
            *ptr++ ^= mask;
            *ptr++ ^= mask;
        }
        else {
            ptr += 4;
        }
# else
        if (gr & 0x01) {
            *ptr ^= mask;
        }
        ptr++;
# endif
        gr >>= 1;
    }
}

/* -------------------------------------------------------------------------
 * Helper functions for tia-write:
 * BALL
 */

static INLINE void
delete_ball(const unsigned x)
{
    TST_PROFILE(PRF_UPD_BL, "update_ball()");

    BYTE *ptr = &collision_vector[x];
    for (unsigned i = 8; i; --i)
        *ptr++ &= ~BL_MASK;
}

/* should be called whenever a ball modifying tia register is written */
static INLINE void
update_ball(const unsigned x, const unsigned width)
{
    TST_PROFILE(PRF_UPD_BL, "update_ball()");

    BYTE *ptr = &collision_vector[x];

    for (unsigned i = width; i; --i)
        *ptr++ |= BL_MASK;
    for (unsigned i = 8 - width; i; --i)
        *ptr++ &= ~BL_MASK;
}


/* -------------------------------------------------------------------------
 * Helper functions for tia-write:
 * MISSILES
 */

/* lookup table for player and missile (only num and dist) sprites:
 * get number of sprites, distance (in pixel) and size (as multiplyer)
 */
static int player_nusize_num[] =  { 1,  2,  2,  3,  2,  1,  3,  1};
static int player_nusize_dist[] = { 0, 16, 32, 16, 64,  0, 32,  0};
static int player_nusize_size[] = { 1,  1,  1,  1,  1,  2,  1,  4};


/* delete all possible missile leftovers; mind multiple missiles! */
//static INLINE
void delete_missile(const struct sprite *spr, unsigned mask)
{
    TST_PROFILE(PRF_DEL_MI, "delete_missile()");

    unsigned num;    // number of objects (missiles)
    unsigned dist;   //distance of objects
    //unsigned width;
    num = player_nusize_num[spr->nusize];
    dist = player_nusize_dist[spr->nusize];
    BYTE *misptr;
    //width = spr->width;
    dist -= 8;
    misptr = &collision_vector[spr->x];
    do {
        for (unsigned i = 8; i; --i)
            *misptr++ &= ~mask;
        misptr += dist;
        if (misptr >= &collision_vector[160])
            misptr -= 160;
        --num;
    } while (num > 0);
}


/*
 * id: missile sprite index (S_MISSILE0 or S_MISSILE1)
 */
//static INLINE
void update_missile(const struct sprite *spr)
{
    unsigned num;    // number of objects (missiles)
    unsigned dist;   //distance of objects
    unsigned width;

    TST_PROFILE(PRF_UPD_MI, "update_missile()");

    num = player_nusize_num[spr->nusize];
    dist = player_nusize_dist[spr->nusize];
    BYTE *misptr;
    width = spr->width;
    dist -= 8;
    misptr = &collision_vector[spr->x];
    do {
        for (unsigned i = width; i; i--)
            *misptr++ |= spr->mask;
        for (unsigned i = 8-width; i; --i)
            *misptr++ &= ~spr->mask;
        misptr += dist;
        if (misptr >= &collision_vector[160])
            misptr -= 160;
        --num;
    } while (num > 0);
}

/* -------------------------------------------------------------------------
 * Helper functions for tia-write:
 * PLAYER
 */

//static INLINE 
void
update_player(const struct sprite *spr, unsigned gr, const unsigned mask)
{
    BYTE *plptr;
    int obj_num,
        obj_dist,
        obj_size;

    TST_PROFILE(PRF_UPD_PL, "update_player()");

    BYTE *ptr = &collision_vector[spr->x];
    obj_num = player_nusize_num[spr->nusize];
    obj_dist = player_nusize_dist[spr->nusize];
    obj_size = player_nusize_size[spr->nusize];

    if (LIKELY(obj_size==1)) {
        do {
            plptr = ptr;
            unsigned pl = gr;
            while (pl) {
                if (pl & 0x01)
                    *plptr ^= mask;
                ++plptr;
                pl >>= 1;
            }
            ptr = ptr+obj_dist;
            if (ptr >= &collision_vector[160])
                ptr -= 160;
            --obj_num;
        } while (obj_num > 0);
    }
    else if (obj_size==2) {
        do {
            plptr = ptr;
            unsigned pl = gr;
            while (pl) {
                if (pl & 0x01) {
                    *plptr ^= mask;
                    *(plptr+1) ^= mask;
                }
                plptr+=2;
                pl >>= 1;
            }
            ptr = ptr+obj_dist;
            if (ptr >= &collision_vector[160])
                ptr -= 160;
            --obj_num;
        } while (obj_num > 0);
    }
    else { /* obj_size==4 */
        unsigned pl = gr;
        plptr = ptr;
        while (pl) {
            if (pl & 0x01) {
                *plptr ^= mask;
                *(plptr+1) ^= mask;
                *(plptr+2) ^= mask;
                *(plptr+3) ^= mask;
            }
            plptr+=4;
            if (plptr >= &collision_vector[160])
                plptr -= 160;
            pl >>= 1;
        }
    }
}


//////////////////////////////////////////////////////////////////////////
// Handle tia access in single functions, called via table

/* dummy function: do nothing */
void tia_dummy(unsigned a, unsigned b)
{
    (void) a;   /* silence compiler warnings */
    (void) b;
}

/* unknown tia writes (RSYNC and address >=0x2d) */
#if !TST_DISPLAY_ALERTS
# define tia_unknown    tia_dummy
#else
void tia_unknown(unsigned a, unsigned b)
{
    (void) a;   /* silence compiler warnings */
    (void) b;
    TST_ALERT(0, ALERT_TIA_ADDR);
}
#endif

void tia_vsync(unsigned a, unsigned b)
{
    (void) a;
    if (b & 0x02) {
        /* Start vertical sync */
        if (!(sync_state & SYNC_STATE_VSYNC_RUNNING))
            sync_state |= (SYNC_STATE_VSYNC_WRITTEN | SYNC_STATE_VSYNC_RUNNING);
    }
    else {
        sync_state &= ~(SYNC_STATE_VSYNC_WRITTEN | SYNC_STATE_VSYNC_RUNNING);
    }
}

void tia_vblank(unsigned a, unsigned b)
{
    (void) a;
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
}

void tia_wsync(unsigned a, unsigned b)
{
    (void) a;
    (void) b;
    /* do nothing; this is handled in mainloop. */
}

void tia_nusiz(unsigned a, unsigned b)
{
    a = a & 0x01;
    unsigned tmp;
    /* number and size of players and missiles */
    /* Bit 0..2 : Player/Missile number and Player size */
    /* Bit 4..5 : Missile size 1/2/4/8 pixel (00/01/10/11) */
#if TST_TIME_USEC
    unsigned long tmp_usec;
    if (tst_time_usec_start > 0) {
        tmp_usec = USEC_TIMER;
    }
#endif
    tmp = (b & 0x07); /* number and size of players */
    struct sprite *spr = &sprite[S_PLAYER0+a];
    if (spr->nusize != tmp) {
        if (spr->grp_en)
            update_player(spr, spr->grp_en, PL0_MASK<<a); /* erase player */
        spr->nusize = tmp;
        if (spr->grp_en)
            update_player(spr, spr->grp_en, PL0_MASK<<a);
        if (sprite[S_MISSILE0+a].grp_en)
            delete_missile(&sprite[S_MISSILE0+a], ML0_MASK<<a);
        sprite[S_MISSILE0+a].nusize = tmp;
        sprite[S_MISSILE0+a].width = 1<<((b & 0x30) >> 4);
        if (sprite[S_MISSILE0+a].grp_en) {
            update_missile(&sprite[S_MISSILE0+a]);
        }
#if TST_TIME_USEC
        if (tst_time_usec_start > 0) {
            tmp_usec = USEC_TIMER - tmp_usec;
            tst_usec[TST_USEC_PL] += tmp_usec;
        }
#endif
    }
    else if (1<<((b & 0x30) >> 4) != sprite[S_MISSILE0+a].width) {
        sprite[S_MISSILE0+a].width = 1<<((b & 0x30) >> 4);
        if (sprite[S_MISSILE0+a].grp_en)
            update_missile(&sprite[S_MISSILE0+a]);
#if TST_TIME_USEC
        if (tst_time_usec_start > 0) {
            tmp_usec = USEC_TIMER - tmp_usec;
            tst_usec[TST_USEC_MIBL] += tmp_usec;
        }
#endif
    }
}

void tia_colup0(unsigned a, unsigned b)
{
    (void) a;
    /* P0M0 colour */
#if NUMCOLS==256
    colour_table[P0M0_COLOUR] = b;
#else
    colour_table[P0M0_COLOUR] = b>>1;
#endif
}

void tia_colup1(unsigned a, unsigned b)
{
    (void) a;
    /* P1M1 colour */
#if NUMCOLS==256
    colour_table[P1M1_COLOUR] = b;
#else
    colour_table[P1M1_COLOUR] = b>>1;
#endif
}

void tia_colupf(unsigned a, unsigned b)
{
    (void) a;
    /* PFBL colour */
#if NUMCOLS==256
    colour_table[PFBL_COLOUR] = b;
#else
    colour_table[PFBL_COLOUR] = b>>1;
#endif
}

void tia_colubk(unsigned a, unsigned b)
{
    (void) a;
#if NUMCOLS==256
    colour_table[BK_COLOUR] = b;
#else
    colour_table[BK_COLOUR] = b>>1;
#endif
}

void tia_ctrlpf(unsigned a, unsigned b)
{
    (void) a;
    unsigned tmp;
    /* set ball size */
    tmp = (b & 0x30) >> 4;
    tmp = 1<<tmp;
    struct sprite *spr = &sprite[S_BALL];
    if (spr->width != tmp) {
        spr->width = tmp;
        spr->new = tmp;
        if (spr->grp_en) {
            update_ball(spr->x, spr->width);
        }
    }
#if TST_TIME_USEC
    unsigned long tmp_usec;
    if (tst_time_usec_start > 0) {
        tmp_usec = USEC_TIMER;
    }
#endif
    /* Reflect Playfield */
    tmp = b & 0x01;
    if (playfield.ref != tmp) {
        playfield.ref = tmp;
        update_playfield((UINT32*)&collision_vector[80], playfield.pf0, PFR_MASK32);
        update_playfield((UINT32*)&collision_vector[80], playfield.pf2r, PFR_MASK32);
        update_playfield((UINT32*)&collision_vector[96], playfield.pf1, PFR_MASK32);
        update_playfield((UINT32*)&collision_vector[112], playfield.pf1r, PFR_MASK32);
        update_playfield((UINT32*)&collision_vector[128], playfield.pf2, PFR_MASK32);
        update_playfield((UINT32*)&collision_vector[144], playfield.pf0r, PFR_MASK32);
    }
#if TST_TIME_USEC
    if (tst_time_usec_start > 0) {
        tmp_usec = USEC_TIMER - tmp_usec;
        tst_usec[TST_USEC_PF] += tmp_usec;
    }
#endif
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
}


void tia_refp(unsigned a, unsigned b)
{
    a = (a & 0x01) ^ 0x01;
#if TST_TIME_USEC
    unsigned long tmp_usec;
    if (tst_time_usec_start > 0) {
        tmp_usec = USEC_TIMER;
    }
#endif
    unsigned tmp = (b & 0x08);
    struct sprite *spr = &sprite[S_PLAYER0+a];
    if (spr->reflect != tmp) {
        spr->reflect = tmp;
        tmp = REVERSEBITS(spr->grp_en);
        spr->new = REVERSEBITS(spr->new);
        spr->old = REVERSEBITS(spr->old);
        if (spr->grp_en != tmp) {
            update_player(spr, spr->grp_en ^ tmp, PL0_MASK<<a);
            spr->grp_en = tmp;
        }
    }
#if TST_TIME_USEC
    if (tst_time_usec_start > 0) {
        tmp_usec = USEC_TIMER - tmp_usec;
        tst_usec[TST_USEC_PL] += tmp_usec;
    }
#endif
}


/* playfield raster change */
void tia_pf0(unsigned a, unsigned b)
{
    (void) a;
#if TST_TIME_USEC
    unsigned long tmp_usec;
    if (tst_time_usec_start > 0) {
        tmp_usec = USEC_TIMER;
    }
#endif
    b >>= 4;
    unsigned temp = b ^ playfield.pf0;
    if (temp) {
        playfield.pf0 = b;
# ifdef RASTER_NO_32BIT_OPT
        BYTE mask = PFL_MASK;
        update_playfield(&collision_vector[0], temp, mask);
# else
        UINT32 mask = PFL_MASK32;
        update_playfield((UINT32*)&collision_vector[0], temp, mask);
# endif
        mask <<= 1; /* becomes PFR_MASK32 */
        if (!playfield.ref) {
# ifdef RASTER_NO_32BIT_OPT
            update_playfield(&collision_vector[80], temp, mask);
# else
            update_playfield((UINT32*)&collision_vector[80], temp, mask);
# endif
        }
        b = REVERSEBITS(b) >> 4;
        if (playfield.ref) {
            temp = b ^ playfield.pf0r;
# ifdef RASTER_NO_32BIT_OPT
            update_playfield(&collision_vector[144], temp, mask);
# else
            update_playfield((UINT32*)&collision_vector[144], temp, mask);
# endif
        }
        playfield.pf0r = b;
    }
#if TST_TIME_USEC
    if (tst_time_usec_start > 0) {
        tmp_usec = USEC_TIMER - tmp_usec;
        tst_usec[TST_USEC_PF] += tmp_usec;
    }
#endif
}

void tia_pf1(unsigned a, unsigned b)
{
    (void) a;
#if TST_TIME_USEC
    unsigned long tmp_usec;
    if (tst_time_usec_start > 0) {
        tmp_usec = USEC_TIMER;
    }
#endif
    unsigned temp = b ^ playfield.pf1r;
    if (temp) {
        playfield.pf1r = b;
        UINT32 mask = PFR_MASK32;
        if (playfield.ref) {
            update_playfield((UINT32*)&collision_vector[112], temp, mask);
        }
        b = REVERSEBITS(b);
        temp = b ^ playfield.pf1;
        playfield.pf1 = b;
        if (!playfield.ref) {
            update_playfield((UINT32*)&collision_vector[96], temp, mask);
        }
        mask >>= 1; /* becomes PFL_MASK32 */
        update_playfield((UINT32*)&collision_vector[16], temp, mask);
    }
#if TST_TIME_USEC
    if (tst_time_usec_start > 0) {
        tmp_usec = USEC_TIMER - tmp_usec;
        tst_usec[TST_USEC_PF] += tmp_usec;
    }
#endif
}

void tia_pf2(unsigned a, unsigned b)
{
    (void) a;
#if TST_TIME_USEC
    unsigned long tmp_usec;
    if (tst_time_usec_start > 0) {
        tmp_usec = USEC_TIMER;
    }
#endif
    unsigned temp = b ^ playfield.pf2;
    if (temp) {
        playfield.pf2 = b;
        UINT32 mask = PFL_MASK32;
        update_playfield((UINT32*)&collision_vector[48], temp, mask);
        mask <<= 1; /* becomes PFR_MASK32 */
        if (!playfield.ref) {
            update_playfield((UINT32*)&collision_vector[128], temp, mask);
        }
        b = REVERSEBITS(b);
        if (playfield.ref) {
            temp = b ^ playfield.pf2r;
            update_playfield((UINT32*)&collision_vector[80], temp, mask);
        }
        playfield.pf2r = b;
    }
#if TST_TIME_USEC
    if (tst_time_usec_start > 0) {
        tmp_usec = USEC_TIMER - tmp_usec;
        tst_usec[TST_USEC_PF] += tmp_usec;
    }
#endif
}

/* reset sprite to beam position */
/* leftmost x coordinates according to https://bumbershootsoft.wordpress.com/2018/08/27/atari-2600-placing-static-sprites/ */
void tia_resp(unsigned a, unsigned b)
{
    a = a & 0x01;
    (void) b;
#if TST_TIME_USEC
    unsigned long tmp_usec;
    if (tst_time_usec_start > 0) {
        tmp_usec = USEC_TIMER;
    }
#endif
    struct sprite *spr = &sprite[S_PLAYER0+a];
    if (spr->grp_en)
        update_player(spr, spr->grp_en, PL0_MASK<<a);
    // TODO: Player is delayed by 5 pixel from actual color clock.
    // The first copy of the player shouldn't be displayed
    // until the next scanline.
    if (ebeamx < 0)
        spr->x = 3;
    else
        spr->x = ebeamx + 5;
    if (spr->width)
        spr->x += 1;
    //if (spr->x >= 160)
    //    spr->x -= 160;
    if (spr->grp_en) {
        update_player(spr, spr->grp_en, PL0_MASK<<a);
    }
#if TST_TIME_USEC
    if (tst_time_usec_start > 0) {
        tmp_usec = USEC_TIMER - tmp_usec;
        tst_usec[TST_USEC_PL] += tmp_usec;
    }
#endif
}

void tia_resm(unsigned a, unsigned b)
{
    a = a & 0x01;
    (void) b;
#if TST_TIME_USEC
    unsigned long tmp_usec;
    if (tst_time_usec_start > 0) {
        tmp_usec = USEC_TIMER;
    }
#endif
    struct sprite *spr = &sprite[S_MISSILE0+a];
    delete_missile(spr, ML0_MASK<<a);
    if (ebeamx < 0)
        spr->x = 2;
    else
        spr->x = ebeamx + 4;
    if (spr->grp_en)
        update_missile(spr);
#if TST_TIME_USEC
    if (tst_time_usec_start > 0) {
        tmp_usec = USEC_TIMER - tmp_usec;
        tst_usec[TST_USEC_MIBL] += tmp_usec;
    }
#endif
}

void tia_resbl(unsigned a, unsigned b)
{
    (void) a;
    (void) b;
#if TST_TIME_USEC
    unsigned long tmp_usec;
    if (tst_time_usec_start > 0) {
        tmp_usec = USEC_TIMER;
    }
#endif
    struct sprite *spr = &sprite[S_BALL];
    if (spr->grp_en)
        delete_ball(spr->x);
    if (ebeamx < 0)
        spr->x = 2;
    else
        spr->x = ebeamx + 4;
    if (spr->grp_en)
        update_ball(spr->x, spr->width);
#if TST_TIME_USEC
    if (tst_time_usec_start > 0) {
        tmp_usec = USEC_TIMER - tmp_usec;
        tst_usec[TST_USEC_MIBL] += tmp_usec;
    }
#endif
}


/* change player graphic */
void tia_grp(unsigned a, unsigned b)
{
    /* a=0x1b (grp0) or 0x1c (grp1) */
    a = (a & 0x01) ^ 0x01; /* grp0 = 0, grp1 = 1 */
#if TST_TIME_USEC
    unsigned long tmp_usec;
    if (tst_time_usec_start > 0) {
        tmp_usec = USEC_TIMER;
    }
#endif
    struct sprite *spr;
    spr = &sprite[S_PLAYER0+a];
    if (!spr->reflect)
        b = REVERSEBITS(b);
    spr->new = b;
    if (!spr->vdel_flag) { /* we immediately use the new graphic */
        if (spr->grp_en != b) {
            update_player(spr, spr->grp_en ^ b, PL0_MASK<<a);
            spr->grp_en = b;
        }
    }
    /* handle vertical delay of other player */
    spr = &sprite[S_PLAYER1-a];
    spr->old = spr->new;
    if (spr->vdel_flag) {
        if (spr->grp_en != spr->new) {
            update_player(spr, spr->grp_en ^ spr->new, PL1_MASK>>a);
            spr->grp_en = spr->new;
        }
    }
    /* handle vertical delay of ball */
    if (a != 0) {
        spr = &sprite[S_BALL];
        spr->old = spr->new;
        if (spr->vdel_flag) {
            if (spr->grp_en != spr->new) {
                spr->grp_en = spr->new;
                if (spr->grp_en) {
                    update_ball(spr->x, spr->width);
                }
                else {
                    delete_ball(spr->x);
                }
            }
        }
    }
#if TST_TIME_USEC
    if (tst_time_usec_start > 0) {
        tmp_usec = USEC_TIMER - tmp_usec;
        tst_usec[TST_USEC_PL] += tmp_usec;
    }
#endif
}


/* enable missile/ball */
void tia_enam(unsigned a, unsigned b)
{
    a = (a & 0x01) ^ 0x01;
#if TST_TIME_USEC
    unsigned long tmp_usec;
    if (tst_time_usec_start > 0) {
        tmp_usec = USEC_TIMER;
    }
#endif
    struct sprite *spr = &sprite[S_MISSILE0+a];
    unsigned tmp = b & 0x02;
    if (spr->grp_en != tmp) {
        spr->grp_en = tmp;
        if (tmp) {
            update_missile(spr);
        }
        else {
            delete_missile(spr, ML0_MASK<<a);
        }
    }
#if TST_TIME_USEC
    if (tst_time_usec_start > 0) {
        tmp_usec = USEC_TIMER - tmp_usec;
        tst_usec[TST_USEC_MIBL] += tmp_usec;
    }
#endif
}

void tia_enabl(unsigned a, unsigned b)
{
    (void) a;
#if TST_TIME_USEC
    unsigned long tmp_usec;
    if (tst_time_usec_start > 0) {
        tmp_usec = USEC_TIMER;
    }
#endif
    unsigned tmp = b & 0x02;
    struct sprite *spr = &sprite[S_BALL];
    spr->new = tmp;
    if (!spr->vdel_flag) {
        spr->grp_en = tmp;
        if (tmp) {
            update_ball(spr->x, spr->width);
        }
        else {
            delete_ball(spr->x);
        }
    }
#if TST_TIME_USEC
    if (tst_time_usec_start > 0) {
        tmp_usec = USEC_TIMER - tmp_usec;
        tst_usec[TST_USEC_MIBL] += tmp_usec;
    }
#endif
}


/* horizontal movement */
void tia_hmp0(unsigned a, unsigned b)
{
    (void) a;
    sprite[S_PLAYER0].hmm = ((signed char)b) >> 4;
}

void tia_hmp1(unsigned a, unsigned b)
{
    (void) a;
    sprite[S_PLAYER1].hmm = ((signed char)b) >> 4;
}

void tia_hmm0(unsigned a, unsigned b)
{
    (void) a;
    sprite[S_MISSILE0].hmm = ((signed char)b) >> 4;
}

void tia_hmm1(unsigned a, unsigned b)
{
    (void) a;
    sprite[S_MISSILE1].hmm = ((signed char)b) >> 4;
}

void tia_hmbl(unsigned a, unsigned b)
{
    (void) a;
    sprite[S_BALL].hmm = ((signed char)b) >> 4;
}


/* vertical delay flags */
void tia_vdelp(unsigned a, unsigned b)
{
    a = (a & 0x01) ^ 0x01;
#if TST_TIME_USEC
    unsigned long tmp_usec;
    if (tst_time_usec_start > 0) {
        tmp_usec = USEC_TIMER;
    }
#endif
    unsigned tmp = b & 0x01;
    struct sprite *spr = &sprite[S_PLAYER0+a];
    if (spr->vdel_flag != tmp) {
        spr->vdel_flag = tmp;
        if (!tmp) {                 /* switch from delayed to normal */
            if (spr->grp_en != spr->new) {
                update_player(spr, spr->grp_en ^ spr->new, PL0_MASK<<a);
                spr->grp_en = spr->new;
            }
        }
        else {                      /* switch from normal to delayed */
            if (spr->grp_en != spr->old) {
                update_player(spr, spr->grp_en ^ spr->old, PL0_MASK<<a);
                spr->grp_en = spr->old;
            }
        }
    }
#if TST_TIME_USEC
    if (tst_time_usec_start > 0) {
        tmp_usec = USEC_TIMER - tmp_usec;
        tst_usec[TST_USEC_PL] += tmp_usec;
    }
#endif
}

void tia_vdelbl(unsigned a, unsigned b)
{
    (void) a;
    (void) b;
    unsigned tmp = b & 0x01;
    struct sprite *spr = &sprite[S_BALL];
    if (spr->vdel_flag != tmp) {
        spr->vdel_flag = tmp;
        if (!tmp) {                 /* switch from delayed to normal */
            if (spr->grp_en != spr->new) {
                spr->grp_en = spr->new;
                if (spr->grp_en)
                    update_ball(spr->x, spr->width);
                else
                    delete_ball(spr->x);
            }
        }
        else {                      /* switch from normal to delayed */
            if (spr->grp_en != spr->old) {
                spr->grp_en = spr->old;
                if (spr->grp_en)
                    update_ball(spr->x, spr->width);
                else
                    delete_ball(spr->x);
            }
        }
    }
}


void tia_resmp(unsigned a, unsigned b)
{
    a = a & 0x01;
#if TST_TIME_USEC
    unsigned long tmp_usec;
    if (tst_time_usec_start > 0) {
        tmp_usec = USEC_TIMER;
    }
#endif
    // reset horizontal pos of missile to player
    tiaWrite[RESMP0+a] = b & 0x02;
    struct sprite *spr = &sprite[S_MISSILE0+a];
    if (b & 0x02) {
        // TODO: set sprite[].grp_en to 0?
        if (spr->grp_en) {
            delete_missile(spr, ML0_MASK<<a);
        }
    }
    else {
        if (spr->grp_en) {
            delete_missile(spr, ML0_MASK<<a);
        }
        spr->x = sprite[S_PLAYER0+a].x + 4;
        if (spr->grp_en) {
            update_missile(spr);
        }
    }
#if TST_TIME_USEC
    if (tst_time_usec_start > 0) {
        tmp_usec = USEC_TIMER - tmp_usec;
        tst_usec[TST_USEC_MIBL] += tmp_usec;
    }
#endif
}


/* horizontal move */
void tia_hmove(unsigned a, unsigned b)
{
    (void) a;
    (void) b;
#if TST_TIME_USEC
    unsigned long tmp_usec2;
    if (tst_time_usec_start > 0) {
        tmp_usec2 = USEC_TIMER;
    }
#endif
    TST_ALERT(ebeamx<=-59 || ebeamx>=151, ALERT_TIA_HMOVE);
    int extra_move;
        extra_move = 0;
    if (ebeamx > 94 && ebeamx < 157) {
        /* Note: Brad Mott testet the effect of hmove on all possible cpu cycles:
         * https://www.biglist.com/lists/stella/archives/199804/msg00198.html
         * The case treated here (and that is described as "fairly usefull" by Brad)
         * takes place at cycle 73 or 74, complying with ebeamx=151..154 (no
         * HMOVE blanking!). From ebeamx=157 to -59 a normal HMOVE takes place. */
        //DEBUGF("received HMOVE outside beginning of Line! x=%d, y=%d\n", ebeamx,ebeamy);
        extra_move = 8;
    }
#if TST_TIME_USEC
    unsigned long tmp_usec;
    if (tst_time_usec_start > 0) {
        tmp_usec = USEC_TIMER;
    }
#endif
    struct sprite *spr;
    spr = &sprite[S_PLAYER0];
    if (spr->hmm + extra_move) {
        if (spr->grp_en)
            update_player(spr, spr->grp_en, PL0_MASK);
        spr->x -= spr->hmm + extra_move;
        if (spr->x >= 160)
            spr->x -= 160;
        else if (spr->x < 0)
            spr->x += 160;
        if (spr->grp_en)
            update_player(spr, spr->grp_en, PL0_MASK);
    }
    spr = &sprite[S_PLAYER1];
    if (spr->hmm + extra_move) {
        if (spr->grp_en)
            update_player(spr, spr->grp_en, PL1_MASK);
        spr->x -= spr->hmm + extra_move;
        if (spr->x >= 160)
            spr->x -= 160;
        else if (spr->x < 0)
            spr->x += 160;
        if (spr->grp_en)
            update_player(spr, spr->grp_en, PL1_MASK);
    }
#if TST_TIME_USEC
    if (tst_time_usec_start > 0) {
        tmp_usec = USEC_TIMER - tmp_usec;
        tst_usec[TST_USEC_PL] += tmp_usec;
    }
#endif
#if TST_TIME_USEC
    if (tst_time_usec_start > 0) {
        tmp_usec = USEC_TIMER;
    }
#endif
    spr = &sprite[S_MISSILE0];
    if (spr->hmm + extra_move) {
        if (spr->grp_en)
            delete_missile(spr, ML0_MASK);
        spr->x -= spr->hmm + extra_move;
        if (spr->x >= 160)
            spr->x -= 160;
        else if (spr->x < 0)
            spr->x += 160;
        if (spr->grp_en)
            update_missile(spr);
    }
    spr = &sprite[S_MISSILE1];
    if (spr->hmm + extra_move) {
        if (spr->grp_en)
            delete_missile(spr, ML1_MASK);
        spr->x -= spr->hmm + extra_move;
        if (spr->x >= 160)
            spr->x -= 160;
        else if (spr->x < 0)
            spr->x += 160;
        if (spr->grp_en)
            update_missile(spr);
    }
    spr = &sprite[S_BALL];
    if (spr->hmm + extra_move) {
        if (spr->grp_en)
            delete_ball(spr->x);
        spr->x -= spr->hmm + extra_move;
        if (spr->x >= 160)
            spr->x -= 160;
        else if (spr->x < 0)
            spr->x += 160;
        if (spr->grp_en)
            update_ball(spr->x, spr->width);
    }
#if TST_TIME_USEC
    if (tst_time_usec_start > 0) {
        tmp_usec = USEC_TIMER - tmp_usec;
        tst_usec[TST_USEC_MIBL] += tmp_usec;
    }
#endif
#if TST_TIME_USEC
    if (tst_time_usec_start > 0) {
        tmp_usec2 = USEC_TIMER - tmp_usec2;
        tst_usec[TST_USEC_MOVE] += tmp_usec2;
    }
#endif
}

/* clear horizontal movement registers */
void tia_hmclr(unsigned a, unsigned b)
{
    (void) a;
    (void) b;
    int i = NUM_SPRITES;
    do {
        --i;
        sprite[i].hmm = 0;
    } while (i);
}

/* clear collision latches */
void tia_cxclr(unsigned a, unsigned b)
{
    (void) a;
    (void) b;
    collisions=0;
}


// ------------------------------------------------------------


void (*tia_call_tab[0x40])(unsigned, unsigned) = {
    &tia_vsync,         /* VSYNC   0x00 */
    &tia_vblank,        /* VBLANK  0x01 */
    &tia_wsync,         /* WSYNC   0x02 */
    &tia_unknown,         /* RSYNC   0x03 */
    &tia_nusiz,        /* NUSIZ0  0x04 */
    &tia_nusiz,        /* NUSIZ1  0x05 */
    &tia_colup0,     /* COLUP0  0x06 */
    &tia_colup1,     /* COLUP1  0x07 */
    &tia_colupf,     /* COLUPF  0x08 */
    &tia_colubk,     /* COLUBK  0x09 */
    &tia_ctrlpf,     /* CTRLPF  0x0A */
    &tia_refp,     /* REFP0   0x0B */
    &tia_refp,     /* REFP1   0x0C */
    &tia_pf0,     /* PF0     0x0D */
    &tia_pf1,     /* PF1     0x0E */
    &tia_pf2,     /* PF2     0x0F */
    &tia_resp,     /* RESP0   0x10 */
    &tia_resp,     /* RESP1   0x11 */
    &tia_resm,     /* RESM0   0x12 */
    &tia_resm,     /* RESM1   0x13 */
    &tia_resbl,     /* RESBL   0x14 */
    &tia_dummy,         /* AUDC0   0x15 */
    &tia_dummy,         /* AUDC1   0x16 */
    &tia_dummy,         /* AUDF0   0x17 */
    &tia_dummy,         /* AUDF1   0x18 */
    &tia_dummy,         /* AUDV0   0x19 */
    &tia_dummy,         /* AUDV1   0x1A */
    &tia_grp,     /* GRP0    0x1B */
    &tia_grp,     /* GRP1    0x1C */
    &tia_enam,     /* ENAM0   0x1D */
    &tia_enam,     /* ENAM1   0x1E */
    &tia_enabl,     /* ENABL   0x1F */
    &tia_hmp0,     /* HMP0    0x20 */
    &tia_hmp1,     /* HMP1    0x21 */
    &tia_hmm0,     /* HMM0    0x22 */
    &tia_hmm1,     /* HMM1    0x23 */
    &tia_hmbl,     /* HMBL    0x24 */
    &tia_vdelp,     /* VDELP0  0x25 */
    &tia_vdelp,     /* VDELP1  0x26 */
    &tia_vdelbl,     /* VDELBL  0x27 */
    &tia_resmp,     /* RESMP0  0x28 */
    &tia_resmp,     /* RESMP1  0x29 */
    &tia_hmove,     /* HMOVE   0x2A */
    &tia_hmclr,     /* HMCLR   0x2B */
    &tia_cxclr,     /* CXCLR   0x2C */
    &tia_unknown,         /* 0x2D */
    &tia_unknown,         /* 0x2E */
    &tia_unknown,         /* 0x2F */
    &tia_unknown,         /* 0x30 */
    &tia_unknown,         /* 0x31 */
    &tia_unknown,         /* 0x32 */
    &tia_unknown,         /* 0x33 */
    &tia_unknown,         /* 0x34 */
    &tia_unknown,         /* 0x35 */
    &tia_unknown,         /* 0x36 */
    &tia_unknown,         /* 0x37 */
    &tia_unknown,         /* 0x38 */
    &tia_unknown,         /* 0x39 */
    &tia_unknown,         /* 0x3A */
    &tia_unknown,         /* 0x3B */
    &tia_unknown,         /* 0x3C */
    &tia_unknown,         /* 0x3D */
    &tia_unknown,         /* 0x3E */
    &tia_unknown,         /* 0x3F */
};
