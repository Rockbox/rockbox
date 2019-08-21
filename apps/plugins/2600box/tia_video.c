
/*
 * partly based on x2600, the Atari Emulator, copyright 1996 by Alex Hornby
 */



#include "rbconfig.h"
#include "rb_test.h"

#include "vmachine.h"
#include "raster.h"
#include "col_mask.h"
#include "keyboard.h"
#include "options.h"
#include "rb_lcd.h"
#include "palette.h"

#include "tia_video.h"

#include "address.h"    // TIA hardware addresses


BYTE tiaRead[0x10];
BYTE tiaWrite[0x40];

unsigned tia_delay;

#define USE_DIV_3_TABLE /* Note: the non-table version leads to unstable hsync
    with some ROMs */

#ifdef USE_DIV_3_TABLE
/* must be more than 228 because a cpu instruction can go up to 6 cycles beyond */
int  div3tab[250]; /* lookup table to convert pixel clock to cpu clock. Index 0 is pixel clock -68 */
int  div3pixel[250]; /* number of pixels I have to add to ebeamx */
#endif

// ---------------------------------------------------------------


/* Initialise the television interface adaptor (TIA) */
void init_tia (void) COLD_ATTR;
void init_tia (void)
{
    for(unsigned i=0; i< ARRAY_LEN(tiaWrite);i++)
        tiaWrite[i]=0;
    for(unsigned i=0; i<ARRAY_LEN(tiaRead); i++)
        tiaRead[i]=0;

    tiaWrite[CTRLPF] = 0x00;

    collisions = 0;

    tiaWrite[VBLANK] = 0;
    tiaRead[INPT4] = 0x80;
    tiaRead[INPT5] = 0x80;

    /* Set up the colour table */
    colour_table[P0M0_COLOUR] = 0;
    colour_table[P1M1_COLOUR] = 0;
    colour_table[PFBL_COLOUR] = 0;
    colour_table[BK_COLOUR] = 0;

    // TODO: reset playfield, sprites... ??

#ifdef USE_DIV_3_TABLE
    for (int i=0; i<(int)ARRAYLEN(div3tab); ++i) {
        div3tab[i] = (228-i) / 3;
        if (div3tab[i] < 0) div3tab[i] += (228/3);
        div3pixel[i] = (228-i);
        if (div3pixel[i] < 0) div3pixel[i] += 228;
    }
#endif
}


// ---------------------------------------------------------------


unsigned read_tia(unsigned a)
{
    a &= 0x0f;

    switch (a) {
    case CXM0P:
        return (collisions & CXM0P_MASK) << 6;
        break;
    case CXM1P:
        return (collisions & CXM1P_MASK) << 4;
        break;
    case CXP0FB:
        return (collisions & CXP0FB_MASK) << 2;
        break;
    case CXP1FB:
        return (collisions & CXP1FB_MASK);
        break;
    case CXM0FB:
        return (collisions & CXM0FB_MASK) >> 2;
        break;
    case CXM1FB:
        return (collisions & CXM1FB_MASK) >> 4;
        break;
    case CXBLPF:
        return (collisions & CXBLPF_MASK) >> 5;
        break;
    case CXPPMM:
        return (collisions & CXPPMM_MASK) >> 7;
        break;
    case INPT0:
#if 0
        if (base_opts.lcon == PADDLE) {
            tiaRead[INPT0] = do_paddle (0);
        }
        else if (base_opts.lcon == KEYPAD) {
            do_keypad (0, 0);
        }
#endif
        return tiaRead[INPT0];
        break;
    case INPT1:
#if 0
        if (base_opts.lcon == PADDLE) {
            tiaRead[INPT1] = do_paddle (1);
        }
        if (base_opts.lcon == KEYPAD) {
            tiaRead[INPT1]=do_keypad (0, 1);
        }
#endif
        return tiaRead[INPT1];
        break;
    case INPT2:
#if 0
        if (base_opts.rcon == KEYPAD)
            do_keypad (1, 0);
#endif
        return tiaRead[INPT2];
        break;
    case INPT3:
#if 0
        if (base_opts.rcon == KEYPAD)
            tiaRead[INPT3]=do_keypad ( 1, 1);
#endif
        return tiaRead[INPT3];
        break;
    case INPT4:
        switch (base_opts.lcon) {
        case KEYPAD:
#if 0
            tiaRead[INPT4]=do_keypad ( 0, 2);
#endif
            break;
        case STICK:
            read_trigger ();
            break;
        }
        return tiaRead[INPT4];
        break;
    case INPT5:
        switch (base_opts.rcon) {
        case KEYPAD:
#if 0
            tiaRead[INPT5]=do_keypad (1, 2);
#endif
            break;
        case STICK:
            read_trigger ();
            break;
        }
        return tiaRead[INPT5];
        break;
    default:
        return 0x00;
    }
}


void write_tia(unsigned a, unsigned b)
{
    a &= 0x3f;
    /* TIA write access is now handled in raster.c */
    TST_PRF_TIA_W(a);
    tia_access = true;
    tia_register = a;
    tia_value = b;

#if 1
    if (tia_register==0x02) { /* TIA WSYNC */
#ifdef USE_DIV_3_TABLE
        clk += div3tab[ebeamx+68]; // needed by timer
        ebeamx += div3pixel[ebeamx+68];
#else
        // Note: jumpy lines with some ROMs
        clk += (160-ebeamx)/3; // needed by timer
        ebeamx = 160;
#endif
        //return;
    }
#endif

    // horizontal delayed tia
    switch (a) {
    case REFP0:
    case REFP1:
        tia_delay = 1;
        break;
    case PF0:
    case PF1:
    case PF2:
        tia_delay = 1;
        break;
    case RESP0:
    case RESP1:
        //tia_delay = 5;    // this is the actual pixel delay
        //tia_delay = 8+5;  // +8 to actually hide the first copy of players (single size only)
        tia_delay = 9; // should be a safe number, because next cpu instr. last at least 3clocks/9pixel
        break;
    case GRP0:
    case GRP1:
        tia_delay = 1;
        break;
    case ENAM0:
    case ENAM1:
    case ENABL:
        tia_delay = 1;
        break;
#  if 0
    // TODO: if these addresses are accessed I don't need to render graphics
    case AUDC0:
    case AUDC1:
    case AUDF0:
    case AUDF1:
    case AUDV0:
    case AUDV1:
    case HMP0:
    case HMP1:
    case HMM0:
    case HMM1:
    case HMBL:
    case HMCLR:
        tia_access = false;
        break;
#  endif
    default:
        tia_delay = 0;
        break;
    }

    if (render_enable) {
#if TST_DUMP_TIA_ACCESS
        if ((frame_counter==1  && ebeamy>-1)||(frame_counter==2 && ebeamy<50) ) {
            if (tia_register==2)
                DEBUGF("Draw Line %d from %d to %d (tiaReg=0x%x / val=0x%x) ---WSYNC!---\n",
                    ebeamy, draw_x_min, ebeamx, tia_register, tia_value);
            else if (tia_register==1)
                DEBUGF("Draw Line %d from %d to %d (tiaReg=0x%x / val=0x%x) ===VBLANK?===\n",
                    ebeamy, draw_x_min, ebeamx, tia_register, tia_value);
            else if (tia_register==0)
                DEBUGF("Draw Line %d from %d to %d (tiaReg=0x%x / val=0x%x) ===VSYNC?===\n",
                    ebeamy, draw_x_min, ebeamx, tia_register, tia_value);
            else
                DEBUGF("Draw Line %d from %d to %d (tiaReg=0x%x / val=0x%x)\n",
                    ebeamy, draw_x_min, ebeamx, tia_register, tia_value);
        }
#endif
        do_raster(draw_x_min, ebeamx + tia_delay);
        draw_x_min = ebeamx + tia_delay;
    }
}
