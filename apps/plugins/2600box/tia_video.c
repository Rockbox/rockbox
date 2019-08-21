
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
    tia_register = a;
    tia_value = b;
}
