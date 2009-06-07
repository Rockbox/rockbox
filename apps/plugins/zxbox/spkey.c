/*
 * Copyright (C) 1996-1998 Szeredi Miklos
 * Email: mszeredi@inf.bme.hu
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. See the file COPYING. 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* #define DEBUG_KEYS */
#include "zxmisc.h"

#include "spkey.h"
#include "spkey_p.h"

#include "spperif.h"
#include "z80.h"

#include "sptape.h"
#include "snapshot.h"
#include "spsound.h"
#include "spscr.h"

#include "interf.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

int keyboard_type = 0;

/* for kempston support via SK_KP_Up and so on.... */
int cursor_type = 2;


int spkb_allow_ascii = 1;
int spkb_trueshift = -1;
int spkb_funcshift = -1;

static unsigned trueshift;
static unsigned funcshift;

struct spkeydef {
  int type;
  spkeyboard kb;
};

struct spbasekey {
  int index;
  spkeyboard kb;
  spkeyboard misc;
};

struct spnamedkey {
  const char *name;
  spkeyboard kb;
  spkeyboard misc;
};

extern int endofsingle;
extern int sp_nosync;
extern int showframe;
extern int privatemap;

spkeyboard spkey_state;
spkeyboard spmisc_state;

static spkeyboard oldstate = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

struct keystate spkb_kbstate[NR_SPKEYS];
struct onekey   spkb_last;

int spkb_state_changed;

#define SKE {0, 0, 0, 0, 0, 0, 0, 0}

#define SKP(x) BIT_N(x)

#define SKN0(x) {SKP(x), 0, 0, 0, 0, 0, 0, 0} 
#define SKN1(x) {0, SKP(x), 0, 0, 0, 0, 0, 0} 
#define SKN2(x) {0, 0, SKP(x), 0, 0, 0, 0, 0} 
#define SKN3(x) {0, 0, 0, SKP(x), 0, 0, 0, 0} 
#define SKN4(x) {0, 0, 0, 0, SKP(x), 0, 0, 0} 
#define SKN5(x) {0, 0, 0, 0, 0, SKP(x), 0, 0} 
#define SKN6(x) {0, 0, 0, 0, 0, 0, SKP(x), 0} 
#define SKN7(x) {0, 0, 0, 0, 0, 0, 0, SKP(x)} 

#define SKCS0(x) {SKP(0) | SKP(x), 0, 0, 0, 0, 0, 0, 0} 
#define SKCS1(x) {SKP(0), SKP(x), 0, 0, 0, 0, 0, 0} 
#define SKCS2(x) {SKP(0), 0, SKP(x), 0, 0, 0, 0, 0} 
#define SKCS3(x) {SKP(0), 0, 0, SKP(x), 0, 0, 0, 0} 
#define SKCS4(x) {SKP(0), 0, 0, 0, SKP(x), 0, 0, 0} 
#define SKCS5(x) {SKP(0), 0, 0, 0, 0, SKP(x), 0, 0} 
#define SKCS6(x) {SKP(0), 0, 0, 0, 0, 0, SKP(x), 0} 
#define SKCS7(x) {SKP(0), 0, 0, 0, 0, 0, 0, SKP(x)} 


#define SKSS0(x) {SKP(x), 0, 0, 0, 0, 0, 0, SKP(1)} 
#define SKSS1(x) {0, SKP(x), 0, 0, 0, 0, 0, SKP(1)} 
#define SKSS2(x) {0, 0, SKP(x), 0, 0, 0, 0, SKP(1)} 
#define SKSS3(x) {0, 0, 0, SKP(x), 0, 0, 0, SKP(1)} 
#define SKSS4(x) {0, 0, 0, 0, SKP(x), 0, 0, SKP(1)} 
#define SKSS5(x) {0, 0, 0, 0, 0, SKP(x), 0, SKP(1)} 
#define SKSS6(x) {0, 0, 0, 0, 0, 0, SKP(x), SKP(1)} 
#define SKSS7(x) {0, 0, 0, 0, 0, 0, 0, SKP(x) | SKP(1)} 


#define KEMP(x)  {x, 0, 0, 0, 0, 0, 0, 0}

#define KEMPR 0x01
#define KEMPL 0x02
#define KEMPD 0x04
#define KEMPU 0x08
#define KEMPF 0x10

#define KEMP_PORT 0x1F


#define T_MAIN 0
#define T_CMPX 1
#define T_EXTR 2

#define SPK_SPACE SKN7(0)

#define SPK_0 SKN4(0)
#define SPK_1 SKN3(0)
#define SPK_2 SKN3(1)
#define SPK_3 SKN3(2)
#define SPK_4 SKN3(3)
#define SPK_5 SKN3(4)
#define SPK_6 SKN4(4)
#define SPK_7 SKN4(3)
#define SPK_8 SKN4(2)
#define SPK_9 SKN4(1)

#define SPK_A SKN1(0)
#define SPK_B SKN7(4)
#define SPK_C SKN0(3)
#define SPK_D SKN1(2)
#define SPK_E SKN2(2)
#define SPK_F SKN1(3)
#define SPK_G SKN1(4)
#define SPK_H SKN6(4)
#define SPK_I SKN5(2)
#define SPK_J SKN6(3)
#define SPK_K SKN6(2)
#define SPK_L SKN6(1)
#define SPK_M SKN7(2)
#define SPK_N SKN7(3)
#define SPK_O SKN5(1)
#define SPK_P SKN5(0)
#define SPK_Q SKN2(0)
#define SPK_R SKN2(3)
#define SPK_S SKN1(1)
#define SPK_T SKN2(4)
#define SPK_U SKN5(3)
#define SPK_V SKN0(4)
#define SPK_W SKN2(1)
#define SPK_X SKN0(2)
#define SPK_Y SKN5(4)
#define SPK_Z SKN0(1)

#define SPK_ENTER       SKN6(0)
#define SPK_CAPSSHIFT   SKN0(0)
#define SPK_SYMBOLSHIFT SKN7(1)

#define SPK_BS          SKCS4(0)
#define SPK_UP          SKCS4(3)
#define SPK_DOWN        SKCS4(4)
#define SPK_LEFT        SKCS3(4)
#define SPK_RIGHT       SKCS4(2)
#define SPK_CAPSLOCK    SKCS3(1)
#define SPK_EXTRA       SKCS7(1)
#define SPK_EDIT        SKCS3(0)

static spkeyboard spk_extra = SPK_EXTRA;

static struct spkeydef spkey_ascii[] = {
  {T_MAIN, SPK_SPACE},  /* space */
  {T_CMPX, SKSS3(0)}, /* ! */
  {T_CMPX, SKSS5(0)}, /* " */
  {T_CMPX, SKSS3(2)}, /* # */
  {T_CMPX, SKSS3(3)}, /* $ */
  {T_CMPX, SKSS3(4)}, /* % */
  {T_CMPX, SKSS4(4)}, /* & */
  {T_CMPX, SKSS4(3)}, /* ' */
  {T_CMPX, SKSS4(2)}, /* ( */
  {T_CMPX, SKSS4(1)}, /* ) */
  {T_CMPX, SKSS7(4)}, /* * */
  {T_CMPX, SKSS6(2)}, /* + */
  {T_CMPX, SKSS7(3)}, /* , */
  {T_CMPX, SKSS6(3)}, /* - */
  {T_CMPX, SKSS7(2)}, /* . */
  {T_CMPX, SKSS0(4)}, /* / */

  {T_MAIN, SPK_0},  /* 0 */
  {T_MAIN, SPK_1},  /* 1 */
  {T_MAIN, SPK_2},  /* 2 */
  {T_MAIN, SPK_3},  /* 3 */
  {T_MAIN, SPK_4},  /* 4 */
  {T_MAIN, SPK_5},  /* 5 */
  {T_MAIN, SPK_6},  /* 6 */
  {T_MAIN, SPK_7},  /* 7 */
  {T_MAIN, SPK_8},  /* 8 */
  {T_MAIN, SPK_9},  /* 9 */

  {T_CMPX, SKSS0(1)}, /* : */
  {T_CMPX, SKSS5(1)}, /* ; */
  {T_CMPX, SKSS2(3)}, /* < */
  {T_CMPX, SKSS6(1)}, /* = */
  {T_CMPX, SKSS2(4)}, /* > */
  {T_CMPX, SKSS0(3)}, /* ? */
  {T_CMPX, SKSS3(1)}, /* @ */

  {T_CMPX, SKCS1(0)}, /* A */
  {T_CMPX, SKCS7(4)}, /* B */
  {T_CMPX, SKCS0(3)}, /* C */
  {T_CMPX, SKCS1(2)}, /* D */
  {T_CMPX, SKCS2(2)}, /* E */
  {T_CMPX, SKCS1(3)}, /* F */
  {T_CMPX, SKCS1(4)}, /* G */
  {T_CMPX, SKCS6(4)}, /* H */
  {T_CMPX, SKCS5(2)}, /* I */
  {T_CMPX, SKCS6(3)}, /* J */
  {T_CMPX, SKCS6(2)}, /* K */
  {T_CMPX, SKCS6(1)}, /* L */
  {T_CMPX, SKCS7(2)}, /* M */
  {T_CMPX, SKCS7(3)}, /* N */
  {T_CMPX, SKCS5(1)}, /* O */
  {T_CMPX, SKCS5(0)}, /* P */
  {T_CMPX, SKCS2(0)}, /* Q */
  {T_CMPX, SKCS2(3)}, /* R */
  {T_CMPX, SKCS1(1)}, /* S */
  {T_CMPX, SKCS2(4)}, /* T */
  {T_CMPX, SKCS5(3)}, /* U */
  {T_CMPX, SKCS0(4)}, /* V */
  {T_CMPX, SKCS2(1)}, /* W */
  {T_CMPX, SKCS0(2)}, /* X */
  {T_CMPX, SKCS5(4)}, /* Y */
  {T_CMPX, SKCS0(1)}, /* Z */

  {T_EXTR, SKCS5(4)}, /* [ */
  {T_EXTR, SKCS1(2)}, /* backslash */
  {T_EXTR, SKCS5(3)}, /* ] */
  {T_CMPX, SKSS6(4)}, /* ^ */
  {T_CMPX, SKSS4(0)}, /* _ */
  {T_CMPX, SKSS0(2)}, /* ` */

  {T_MAIN, SPK_A},  /* a */
  {T_MAIN, SPK_B},  /* b */
  {T_MAIN, SPK_C},  /* c */
  {T_MAIN, SPK_D},  /* d */
  {T_MAIN, SPK_E},  /* e */
  {T_MAIN, SPK_F},  /* f */
  {T_MAIN, SPK_G},  /* g */
  {T_MAIN, SPK_H},  /* h */
  {T_MAIN, SPK_I},  /* i */
  {T_MAIN, SPK_J},  /* j */
  {T_MAIN, SPK_K},  /* k */
  {T_MAIN, SPK_L},  /* l */
  {T_MAIN, SPK_M},  /* m */
  {T_MAIN, SPK_N},  /* n */
  {T_MAIN, SPK_O},  /* o */
  {T_MAIN, SPK_P},  /* p */
  {T_MAIN, SPK_Q},  /* q */
  {T_MAIN, SPK_R},  /* r */
  {T_MAIN, SPK_S},  /* s */
  {T_MAIN, SPK_T},  /* t */
  {T_MAIN, SPK_U},  /* u */
  {T_MAIN, SPK_V},  /* v */
  {T_MAIN, SPK_W},  /* w */
  {T_MAIN, SPK_X},  /* x */
  {T_MAIN, SPK_Y},  /* y */
  {T_MAIN, SPK_Z},  /* z */

  {T_EXTR, SKCS1(3)}, /* { */
  {T_EXTR, SKCS1(1)}, /* | */
  {T_EXTR, SKCS1(4)}, /* } */
  {T_EXTR, SKCS1(0)}, /* ~ */
};

static struct spnamedkey spkey_misc[] = {
  {"none",            SKE,              SKE},

  {"space",           SPK_SPACE,        SKE},
  {"enter",           SPK_ENTER,        SKE},
  {"capsshift",       SPK_CAPSSHIFT,    SKE},
  {"symbolshift",     SPK_SYMBOLSHIFT,  SKE},
  
  {"kempston_up",     SKE,              KEMP(KEMPU)},
  {"kempston_down",   SKE,              KEMP(KEMPD)},
  {"kempston_left",   SKE,              KEMP(KEMPL)},
  {"kempston_right",  SKE,              KEMP(KEMPR)},
  {"kempston_fire",   SKE,              KEMP(KEMPF)},

  {NULL, SKE, SKE}
};

#define MAXBASEKEYS 128

static struct spbasekey basekeys[MAXBASEKEYS];
static int numbasekeys;

static struct spbasekey customkeys[MAXBASEKEYS];
static int numcustomkeys = 0;

static struct spbasekey normalkeys[] = {
  {'0',    SPK_0,   SKE},  /* 0 */
  {'1',    SPK_1,   SKE},  /* 1 */
  {'2',    SPK_2,   SKE},  /* 2 */
  {'3',    SPK_3,   SKE},  /* 3 */
  {'4',    SPK_4,   SKE},  /* 4 */
  {'5',    SPK_5,   SKE},  /* 5 */
  {'6',    SPK_6,   SKE},  /* 6 */
  {'7',    SPK_7,   SKE},  /* 7 */
  {'8',    SPK_8,   SKE},  /* 8 */
  {'9',    SPK_9,   SKE},  /* 9 */

  {'a',    SPK_A,   SKE},  /* a */
  {'b',    SPK_B,   SKE},  /* b */
  {'c',    SPK_C,   SKE},  /* c */
  {'d',    SPK_D,   SKE},  /* d */
  {'e',    SPK_E,   SKE},  /* e */
  {'f',    SPK_F,   SKE},  /* f */
  {'g',    SPK_G,   SKE},  /* g */
  {'h',    SPK_H,   SKE},  /* h */
  {'i',    SPK_I,   SKE},  /* i */
  {'j',    SPK_J,   SKE},  /* j */
  {'k',    SPK_K,   SKE},  /* k */
  {'l',    SPK_L,   SKE},  /* l */
  {'m',    SPK_M,   SKE},  /* m */
  {'n',    SPK_N,   SKE},  /* n */
  {'o',    SPK_O,   SKE},  /* o */
  {'p',    SPK_P,   SKE},  /* p */
  {'q',    SPK_Q,   SKE},  /* q */
  {'r',    SPK_R,   SKE},  /* r */
  {'s',    SPK_S,   SKE},  /* s */
  {'t',    SPK_T,   SKE},  /* t */
  {'u',    SPK_U,   SKE},  /* u */
  {'v',    SPK_V,   SKE},  /* v */
  {'w',    SPK_W,   SKE},  /* w */
  {'x',    SPK_X,   SKE},  /* x */
  {'y',    SPK_Y,   SKE},  /* y */
  {'z',    SPK_Z,   SKE},  /* z */
  
  {-1, SKE, SKE}
};

static struct spbasekey extendedkeys[] = {
  {' ',                SPK_SPACE,          SKE}, /* space */
  {TRKS(SK_Return),    SPK_ENTER,          SKE}, /* enter */
  {TRKS(SK_KP_Enter),  SPK_ENTER,          SKE},
  {TRKS(SK_Shift_L),   SPK_CAPSSHIFT,      SKE}, /* caps shift */
  {TRKS(SK_Shift_R),   SPK_SYMBOLSHIFT,    SKE}, /* symbol shift */
  {TRKS(SK_BackSpace), SPK_BS,             SKE}, /* backspace */
  {TRKS(SK_Delete),    SPK_BS,             SKE},
  {TRKS(SK_KP_Delete), SPK_BS,             SKE},
  {TRKS(SK_Escape),    SPK_EDIT,           SKE}, /* caps shift + '1' */

  {-1, SKE, SKE}
};

static struct spbasekey spectrumkeys[] = {
  {',',                SPK_SYMBOLSHIFT,   SKE},
  {'.',                SPK_SPACE,         SKE},
  {';',                SPK_ENTER,         SKE},

  {-1, SKE, SKE}
};                      
                      
                      
static struct spbasekey compatkeys[] = {  
  {TRKS(SK_Shift_L),   SPK_CAPSSHIFT,     SKE}, /* caps shift */
  {TRKS(SK_Shift_R),   SPK_CAPSSHIFT,     SKE}, 
  {TRKS(SK_Alt_L),     SPK_SYMBOLSHIFT,   SKE}, /* symbol shift */
  {TRKS(SK_Alt_R),     SPK_SYMBOLSHIFT,   SKE}, 
  {TRKS(SK_Meta_L),    SPK_SYMBOLSHIFT,   SKE},
  {TRKS(SK_Meta_R),    SPK_SYMBOLSHIFT,   SKE}, 
#ifdef TRUEKOMPAT                  
  {TRKS(SK_Control_L), SPK_EXTRA,         SKE}, /* caps shift + symbol shift */
  {TRKS(SK_Control_R), SPK_EXTRA,         SKE},
#endif                      

  {-1, SKE, SKE}
};                      
                      
static struct spbasekey shiftedcurs[] = { 
  {TRKS(SK_Up),        SPK_UP,            SKE}, /* up */
  {TRKS(SK_KP_Up),     SPK_UP,            SKE},
  {TRKS(SK_Down),      SPK_DOWN,          SKE}, /* down */
  {TRKS(SK_KP_Down),   SPK_DOWN,          SKE},
  {TRKS(SK_Left),      SPK_LEFT,          SKE}, /* left */
  {TRKS(SK_KP_Left),   SPK_LEFT,          SKE},
  {TRKS(SK_Right),     SPK_RIGHT,         SKE}, /* right */
  {TRKS(SK_KP_Right),  SPK_RIGHT,         SKE},

  {-1, SKE, SKE}
};                      

static struct spbasekey rawcurs[] = {      
  {TRKS(SK_Up),        SPK_7,             SKE}, /* up */
  {TRKS(SK_KP_Up),     SPK_7,             SKE},
  {TRKS(SK_Down),      SPK_6,             SKE}, /* down */
  {TRKS(SK_KP_Down),   SPK_6,             SKE},
  {TRKS(SK_Left),      SPK_5,             SKE}, /* left */
  {TRKS(SK_KP_Left),   SPK_5,             SKE},
  {TRKS(SK_Right),     SPK_8,             SKE}, /* right */
  {TRKS(SK_KP_Right),  SPK_8,             SKE},

  {-1, SKE, SKE}
};

static struct spbasekey joycurs[] = {
  {TRKS(SK_Up),           SKE,   KEMP(KEMPU)}, /* up */
  {TRKS(SK_KP_Up),        SKE,   KEMP(KEMPU)},
  {TRKS(SK_Down),         SKE,   KEMP(KEMPD)}, /* down */
  {TRKS(SK_KP_Down),      SKE,   KEMP(KEMPD)},
  {TRKS(SK_Left),         SKE,   KEMP(KEMPL)}, /* left */
  {TRKS(SK_KP_Left),      SKE,   KEMP(KEMPL)},
  {TRKS(SK_Right),        SKE,   KEMP(KEMPR)}, /* right */
  {TRKS(SK_KP_Right),     SKE,   KEMP(KEMPR)},
  {TRKS(SK_KP_Insert),    SKE,   KEMP(KEMPF)}, /* fire */
  {TRKS(SK_Insert),       SKE,   KEMP(KEMPF)},
  {TRKS(SK_KP_Delete),    SKE,   KEMP(KEMPF)},
  {TRKS(SK_KP_Home),      SKE,   KEMP(KEMPU | KEMPL)}, /* up + left*/
  {TRKS(SK_Home),         SKE,   KEMP(KEMPU | KEMPL)},
  {TRKS(SK_KP_Page_Up),   SKE,   KEMP(KEMPU | KEMPR)}, /* up + right*/
  {TRKS(SK_Page_Up),      SKE,   KEMP(KEMPU | KEMPR)},
  {TRKS(SK_KP_End),       SKE,   KEMP(KEMPD | KEMPL)}, /* down + left*/
  {TRKS(SK_End),          SKE,   KEMP(KEMPD | KEMPL)},
  {TRKS(SK_KP_Page_Down), SKE,   KEMP(KEMPD | KEMPR)}, /* down + right*/
  {TRKS(SK_Page_Down),    SKE,   KEMP(KEMPD | KEMPR)},

  {-1, SKE, SKE}
};

int spkey_new_custom(int key)
{
  if(numcustomkeys >= MAXBASEKEYS) return 0;

  customkeys[numcustomkeys].index = key;
  SP_SETEMPTY(customkeys[numcustomkeys].kb);
  SP_SETEMPTY(customkeys[numcustomkeys].misc);
  numcustomkeys++;

  return 1;
}

int spkey_add_custom(const char *name)
{
  int curr;

  curr = numcustomkeys - 1;

  if(!name[1] && isalnum(name[0])) {
    int ai;
    ai = tolower(name[0])-32;
    SP_COMBINE(customkeys[curr].kb, spkey_ascii[ai].kb);
    return 1;
  }
  else {
    int i;

    for(i = 0; spkey_misc[i].name != NULL; i++) {
      if(mis_strcasecmp(spkey_misc[i].name, name) == 0) {
    SP_COMBINE(customkeys[curr].kb, spkey_misc[i].kb);
    SP_COMBINE(customkeys[curr].misc, spkey_misc[i].misc);
    return 1;
      }
    }
  }
  return 0;
}

static int key_reset(struct keystate *ck)
{
  if(ck->state == 2 && sp_int_ctr >= ck->frame) {
    ck->state = 0;
    return 1;
  }
  else return 0;
}


void process_keys(void)
{
  int i;
  struct keystate *ck;
  int tsh;
  int kalone;
  static int extrai = 0;
  static qbyte extraendframe;
  
  if(extrai && !spkb_kbstate[extrai].state) extrai = 0;

  if(!spkb_state_changed && (!extrai || !extraendframe)) return;

  SP_SETEMPTY(spkey_state);
  SP_SETEMPTY(spmisc_state);

  kalone = 0;
  ck = spkb_kbstate + spkb_last.index;
  tsh = spkb_last.modif & trueshift;
  key_reset(ck);

  
  if(spkb_allow_ascii && ck->state && (!ck->base || tsh)) {
    unsigned ks;
    ks = tsh ? spkb_last.shifted : spkb_last.keysym;
    if(ks >= 32 && ks < 127) {
      if(spkey_ascii[ks-32].type <= T_CMPX) { 
    SP_COMBINE(spkey_state, spkey_ascii[ks-32].kb);
    kalone = 1;
      }
      else if(spkey_ascii[ks-32].type == T_EXTR) {
    if(!extrai || sp_int_ctr < extraendframe) {
      if(!extrai) {
        extrai = spkb_last.index;
        extraendframe = sp_int_ctr + 1;
      }
      SP_COMBINE(spkey_state, spk_extra);
    }
    else {
      SP_COMBINE(spkey_state, spkey_ascii[ks-32].kb);
      extraendframe = 0;
    }
    kalone = 1;
      }
    }
  }
  
  if(!kalone) {
    for(i = 0; i < numbasekeys; i++) {
      ck = spkb_kbstate + basekeys[i].index; 
      key_reset(ck);
      if(ck->state) {
    SP_COMBINE(spkey_state, basekeys[i].kb);
    SP_COMBINE(spmisc_state, basekeys[i].misc);
      }
    }
  }
  SP_COMBINE(spkey_state, kb_mkey);

  spkb_refresh();
  spkb_state_changed = 0;
}

void clear_keystates(void)
{
  int i;
  for(i = 0; i < NR_SPKEYS; i++) spkb_kbstate[i].state = 0;
  spkb_last.index = 0;
  SP_SETEMPTY(spkey_state);
  SP_SETEMPTY(kb_mkey);
  SP_SETEMPTY(spmisc_state);
  spkb_refresh();
}

static void keycpy(struct spbasekey *to, struct spbasekey *from)
{
  to->index = from->index;
  SP_COPY(to->kb, from->kb);
  SP_COPY(to->misc, from->misc);
}

static void copy_key(struct spbasekey *addk)
{
  int i;
  int nindex;

  nindex = addk->index;

  if(SP_NONEMPTY(addk->kb) || SP_NONEMPTY(addk->misc)) {
    for(i = 0; i < numbasekeys; i++) {
      if(basekeys[i].index == nindex) {                /* Replace */
    keycpy(&basekeys[i], addk); 
    return;
      }
    }
    if(numbasekeys < MAXBASEKEYS - 1) {                /* Add */
      keycpy(&basekeys[numbasekeys], addk);
      spkb_kbstate[nindex].base = 1;
      numbasekeys++;
    }
  }
  else {                                               /* Delete */
    for(i = 0; i < numbasekeys; i++) {
      if(basekeys[i].index == nindex) {
    i++;
    for(; i < numbasekeys; i++) keycpy(&basekeys[i-1], &basekeys[i]);
    spkb_kbstate[nindex].base = 0;
    numbasekeys--;
    break;
      }
    }
  }
}

static void copy_basekeys(struct spbasekey *addk)
{
  int i;

  for(i = 0; addk[i].index >= 0; i++) copy_key(&addk[i]);
}

static unsigned transform_shift(int modif)
{
  if(!modif) return 0;
  else return BIT_N(modif - 1);
}


void init_basekeys(void)
{
  int i;
  numbasekeys = 0;

  for(i = 0; i < NR_SPKEYS; i++) spkb_kbstate[i].base = 0;

  customkeys[numcustomkeys].index = -1;

  copy_basekeys(normalkeys);
  copy_basekeys(extendedkeys);
  copy_basekeys(shiftedcurs);
  
  switch(keyboard_type) {
  case 0:
    break;
    
  case 1:
    copy_basekeys(spectrumkeys);
    break;
    
  case 2:
    if(spkb_trueshift == -1) spkb_trueshift = 0;
#ifdef TRUEKOMPAT
    if(spkb_funcshift == -1) spkb_funcshift = 0;
#endif

    copy_basekeys(compatkeys);
    break;

  case 3:
    copy_basekeys(customkeys);
    break;
  }
  
  switch(cursor_type) {
  case 0:
    break;
    
  case 1:
    copy_basekeys(rawcurs);
    break;
    
  case 2:
    copy_basekeys(joycurs);
    break;
  }

  if(spkb_trueshift == -1) spkb_trueshift = 4; /* mod1 */
  if(spkb_funcshift == -1) spkb_funcshift = 3; /* control */

  trueshift = transform_shift(spkb_trueshift);
  funcshift = transform_shift(spkb_funcshift);
}


void spkb_refresh(void)
{
  int port, pb;
  int i, j, changed;
  byte *km, *kmo;
  byte mm;
  byte pv;
  spkeyboard statemx;

  km = spkey_state;
  kmo = oldstate;
  changed = 0;
  for(i = 8; i; i--) {
    if(*km != *kmo) *kmo = *km, changed = 1;
    km++, kmo++;
  }

  if(changed) {

    /* Matrix behavior: ONLY 1 level, does anybody need more ? */

    for(i = 0; i < 8; i++) {  
      pv = spkey_state[i];
      mm = pv;
      if(pv) {
    km = spkey_state;
    for(j = 8; j; j--) {
      if((*km & pv) & 0x1F) mm |= *km;
      km++;
    }
      }
      statemx[i] = mm;
    }
    
    for(port = 0; port < 256; port++) {
      km = statemx;
      pv = 0;
      pb = port;
      for(i = 8; i; i--) {
    if(!(pb & 1)) pv |= *km;
    pb >>= 1;
    km++;
      }
      sp_fe_inport_high[port] = 
    (sp_fe_inport_high[port] | 0x1F) & ~(pv & 0x1F); 
    }
  }

  pv = spmisc_state[0];

  if((pv & KEMPR) && (pv & KEMPL)) pv &= ~(KEMPR | KEMPL);
  if((pv & KEMPU) && (pv & KEMPD)) pv &= ~(KEMPD | KEMPU);
  z80_inports[KEMP_PORT] = pv;
}

