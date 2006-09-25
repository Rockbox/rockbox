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

#ifndef SPKEY_P_H
#define SPKEY_P_H

#include "z80_type.h"

struct keystate { 
  dbyte press;
  byte state;
  byte base;
  qbyte frame;
};

struct onekey {
  int index;
  unsigned keysym;
  unsigned shifted;
  unsigned modif;
};

typedef byte spkeyboard[8];

#define SPIP(spk, h) (((qbyte *) (spk))[h])

#define SP_COMBINE(spk1, spk2) \
  SPIP(spk1, 0) |= SPIP(spk2, 0), \
  SPIP(spk1, 1) |= SPIP(spk2, 1) 


#define SP_SUBSTRACT(spk1, spk2) \
  SPIP(spk1, 0) &= ~SPIP(spk2, 0), \
  SPIP(spk1, 1) &= ~SPIP(spk2, 1) 


#define SP_NONEMPTY(spk) (SPIP(spk, 0) || SPIP(spk, 1))

#define SP_CONTAINS(spk1, spk2) \
  ((SPIP(spk1, 0) & SPIP(spk2, 0)) || (SPIP(spk1, 1) & SPIP(spk2, 1))) 

#define SP_SETEMPTY(spk) \
  SPIP(spk, 0) = 0, \
  SPIP(spk, 1) = 0 

#define SP_COPY(spk1, spk2) \
  SPIP(spk1, 0) = SPIP(spk2, 0), \
  SPIP(spk1, 1) = SPIP(spk2, 1) 


#define TRKS(ks) ((ks) - 0xFF00 + 0x100)
#define KS_TO_KEY(ks) \
        (((ks) >= 0x0000 && (ks) <= 0x00FF) ? (int) (ks) : \
         (((ks) >= 0xFF00 && (ks) <= 0xFFFF) ? (int) TRKS(ks) : -1))



/* These are _accidently_ the same as the XK_ counterparts */

#define SK_BackSpace        0xFF08    /* back space, back char */
#define SK_Tab            0xFF09
#define SK_Linefeed        0xFF0A    /* Linefeed, LF */
#define SK_Clear        0xFF0B
#define SK_Return        0xFF0D    /* Return, enter */
#define SK_Pause        0xFF13    /* Pause, hold */
#define SK_Scroll_Lock        0xFF14
#define SK_Sys_Req        0xFF15
#define SK_Escape        0xFF1B
#define SK_Delete        0xFFFF    /* Delete, rubout */

#define SK_Home            0xFF50
#define SK_Left            0xFF51    /* Move left, left arrow */
#define SK_Up            0xFF52    /* Move up, up arrow */
#define SK_Right        0xFF53    /* Move right, right arrow */
#define SK_Down            0xFF54    /* Move down, down arrow */
#define SK_Page_Up        0xFF55  /* Prior, previous */
#define SK_Page_Down        0xFF56  /* Next */
#define SK_End            0xFF57    /* EOL */
#define SK_Begin        0xFF58    /* BOL */

#define SK_Select        0xFF60    /* Select, mark */
#define SK_Print        0xFF61
#define SK_Execute        0xFF62    /* Execute, run, do */
#define SK_Insert        0xFF63    /* Insert, insert here */
#define SK_Undo            0xFF65    /* Undo, oops */
#define SK_Redo            0xFF66    /* redo, again */
#define SK_Menu            0xFF67
#define SK_Find            0xFF68    /* Find, search */
#define SK_Cancel        0xFF69    /* Cancel, stop, abort, exit */
#define SK_Help            0xFF6A    /* Help */
#define SK_Break        0xFF6B
#define SK_Mode_switch        0xFF7E    /* Character set switch */
#define SK_Num_Lock        0xFF7F

#define SK_KP_Space        0xFF80    /* space */
#define SK_KP_Tab        0xFF89
#define SK_KP_Enter        0xFF8D    /* enter */
#define SK_KP_F1        0xFF91    /* PF1, KP_A, ... */
#define SK_KP_F2        0xFF92
#define SK_KP_F3        0xFF93
#define SK_KP_F4        0xFF94
#define SK_KP_Home        0xFF95
#define SK_KP_Left        0xFF96
#define SK_KP_Up        0xFF97
#define SK_KP_Right        0xFF98
#define SK_KP_Down        0xFF99
#define SK_KP_Page_Up        0xFF9A
#define SK_KP_Page_Down        0xFF9B
#define SK_KP_End        0xFF9C
#define SK_KP_Begin        0xFF9D
#define SK_KP_Insert        0xFF9E
#define SK_KP_Delete        0xFF9F
#define SK_KP_Equal        0xFFBD    /* equals */
#define SK_KP_Multiply        0xFFAA
#define SK_KP_Add        0xFFAB
#define SK_KP_Separator        0xFFAC    /* separator, often comma */
#define SK_KP_Subtract        0xFFAD
#define SK_KP_Decimal        0xFFAE
#define SK_KP_Divide        0xFFAF

#define SK_KP_0            0xFFB0
#define SK_KP_1            0xFFB1
#define SK_KP_2            0xFFB2
#define SK_KP_3            0xFFB3
#define SK_KP_4            0xFFB4
#define SK_KP_5            0xFFB5
#define SK_KP_6            0xFFB6
#define SK_KP_7            0xFFB7
#define SK_KP_8            0xFFB8
#define SK_KP_9            0xFFB9

#define SK_F1            0xFFBE
#define SK_F2            0xFFBF
#define SK_F3            0xFFC0
#define SK_F4            0xFFC1
#define SK_F5            0xFFC2
#define SK_F6            0xFFC3
#define SK_F7            0xFFC4
#define SK_F8            0xFFC5
#define SK_F9            0xFFC6
#define SK_F10            0xFFC7
#define SK_F11            0xFFC8
#define SK_F12            0xFFC9

#define SK_Shift_L        0xFFE1    /* Left shift */
#define SK_Shift_R        0xFFE2    /* Right shift */
#define SK_Control_L        0xFFE3    /* Left control */
#define SK_Control_R        0xFFE4    /* Right control */
#define SK_Caps_Lock        0xFFE5    /* Caps lock */
#define SK_Shift_Lock        0xFFE6    /* Shift lock */

#define SK_Meta_L        0xFFE7    /* Left meta */
#define SK_Meta_R        0xFFE8    /* Right meta */
#define SK_Alt_L        0xFFE9    /* Left alt */
#define SK_Alt_R        0xFFEA    /* Right alt */
#define SK_Super_L        0xFFEB    /* Left super */
#define SK_Super_R        0xFFEC    /* Right super */
#define SK_Hyper_L        0xFFED    /* Left hyper */
#define SK_Hyper_R        0xFFEE    /* Right hyper */

/* Modifier masks */

#define SKShiftMask        (1<<0)
#define SKLockMask        (1<<1)
#define SKControlMask        (1<<2)
#define SKMod1Mask        (1<<3)
#define SKMod2Mask        (1<<4)
#define SKMod3Mask        (1<<5)
#define SKMod4Mask        (1<<6)
#define SKMod5Mask        (1<<7)


#define NR_SPKEYS 512

#define ISFKEY(ks) ((ks) >= SK_F1 && (ks) <= SK_F12)

extern volatile int accept_keys;

extern qbyte sp_int_ctr;
extern struct keystate spkb_kbstate[];
extern struct onekey   spkb_last;

extern int spkb_state_changed;

extern spkeyboard spkey_state;
extern spkeyboard kb_mkey;

extern void spkey_textmode(void);
extern void spkey_screenmode(void);

extern const int need_switch_mode;
    
extern void spkb_refresh(void);
extern void clear_keystates(void);
extern int display_keyboard(void);
extern void process_keys(void);
extern void init_basekeys(void);

#endif /* SPKEY_P_H */
