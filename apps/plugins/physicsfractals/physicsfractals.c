/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 	*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2002 Damien Teney
* modified to use int instead of float math by Andreas Zwirtes
* heavily extended by Jens Arnold
* Physicsfractals.c by Benjamin Brown
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************


The Q-bit

           W   n
          ____/
         |    |
         | V  | H
         |____|
         /
        S


    n >= 0; n++

    **New Four Number Function**

    Width                     W = n^1

    Height                    H = 2^n

    Value of Parts            V = ((n^1)/2) * n

    Strings of this 2D brane  S = (W - 1) + (H - 1) + 1


  Example of 4

  n=4
  W=4
  H=16
  S=19
  V=32

           W   n = 4
          _|__/
       S-|    |
       S-|V   |
       S-| V  |
       S-|  V | +
       S-|   V|
       S-|VV  |
       S-|V V |
       S-|V__V|-H
       S-| VV |
       S-| V V|
       S-|  VV|
       S-|VVV |
       S-|VV V| -
       S-|V VV|
       S-| VVV|
         |VVVV|
        /  |||
       S   SSS


    **Classic Four Quantum Numbers**

    QNprime             = W

    QNazmuthal          = (W)-1

    QNmagnetic          = (W)^2

    QNspin              =  ...+2,+1,0,-1-2...
                          Derived geometricly from the 2D brane
                          XOR half of W with the other half of W
                               "spin 180 degrees through X/2"
                          A col caught in the middle
                          cancels out for obvious reason See n=3


                       XORs are twos

                      180 degrees spin
                            |
                            |
                            |  2
                            | /
                          |0 0|  0  +
                          |1 0|__1____Bisect
                          |0 1|  1
                          |1 1|  0  -


                          A bisecting line of seperation along Y/2
                          assign charge -bottom and +top




The Truth Table
    The row & col placement of V inside S is (n or not n) basicly
    boolean logic is at play OR and NOT etc "Turing's Busy Beaver"
    This can be replaced with blits and bit shifts of the brane
    until H/2 rows are reached, then the finished half can
    be OR(ed) over the last half producing the finished Q-BIT


    n=4
    seg_counter = 0
    shift_counter = 0

        {{0, 0, 0, 0}}    //rule1 first row always all false seg_counter++ ;end rule1

    seg_counter=1
        {{1, 0, 0, 0}}    //rule1 blit first seg_counter bits of row if < n
        {{0, 1, 0, 0}}    //rule2 shift shift_counter++ until n = shiftcounter then seg_counter++
        {{0, 0, 1, 0}}
        {{0, 0, 0, 1}}    //shift_counter = n;end rule1;end rule2

    seg_counter=2
        {{1, 1, 0, 0}}    //rule1 start at seg_counter bit of row, blit down n-1 rowcounter++
        {{1, 0, 1, 0}}    //rule2 blit seg 1 over rowcounter cols down
        {{1, 0, 0, 1}}
        {{0, 0, 0, 0}}
        {{0, 0, 0, 0}}
        {{0, 0, 0, 0}}
        {{0, 0, 0, 0}}
        {{0, 0, 0, 0}}
        {{0, 0, 0, 0}}
        {{0, 0, 0, 0}}



Memory Requirements

    So instead of...

    static const struct number brane[n=0] =
    {
        {{0}},
        {{0}}    //should be a single {{0}} but an extra zero avoids use of null later
    };

    static const struct number brane[n=1] =
    {
        {{0}},
        {{1}}
    };

    static const struct number brane[n=2] =
    {
        {{0, 0}},
        {{1, 0}},
        {{0, 1}},
        {{1, 1}}
    };

    static const struct number brane[n=3] =
    {
        {{0, 0, 0}},
        {{1, 0, 0}},
        {{0, 1, 0}},
        {{0, 0, 1}},
        {{1, 1, 0}},
        {{1, 0, 1}},
        {{0, 1, 1}},
        {{1, 1, 1}}
    };


Now to itenerate the formulas above maximally in memory using blits bit shifts ORs etc



QUANTUM SPIN

    XOR at half of W and SUM



  n=0

    {{0}}  XOR  0 SUM 0
    {{0}}  XOR  0 SUM 0  //avoids use of null

    L->R XOR at W/2
    1/2 cut middle parts cancel

    This set {-0}  //if n=0 assign negative; avoids null


    Expected result {null}



  n=1

    {{0}}  XOR  +0 SUM 0
    {{1}}  XOR  -0 SUM 0

    top half positive, lower half negitive

    This set {+0}  //if n=1 assign positive; avoids null


    Expected result {0}




  n=2

    {{0, 0}}  XOR  0
    {{1, 0}}  XOR  +1 SUM +1

    {{0, 1}}  XOR  -1 SUM -1
    {{1, 1}}  XOR  0


    This set  {-1, +1}   and the
    last set    {+0}     are equal
    to      {-1, +0, +1}


    Expected result {-1, 0, +1}



  n=3
                             +2
                             SUM UP
    {{0, 0, 0}}  XOR   0 SUM  0
    {{1, 0, 0}}  XOR  +1 SUM +1
    {{0, 1, 0}}  XOR   0 SUM  0
    {{0, 0, 1}}  XOR  +1 SUM +1

    {{1, 1, 0}}  XOR  -1 SUM -1
    {{1, 0, 1}}  XOR   0 SUM  0
    {{0, 1, 1}}  XOR  -1 SUM -1
    {{1, 1, 1}}  XOR   0 SUM  0
         |                   SUM DOWN
         |                   -2
    1/2 cut middle cols cancel



    This set        {-2, +2}
    and last set  {-1, +0, +1}
    equals    {-2, -1, +0, +1, +2}


    Expected result {-2, -1, 0, +1, +2}




  n=4

    {{0, 0, 0, 0}}  XOR  0,0
    {{1, 0, 0, 0}}  XOR  1,0  SUM +1
    {{0, 1, 0, 0}}  XOR  0,1  SUM +1
    {{0, 0, 1, 0}}  XOR  0,1  SUM +1
    {{0, 0, 0, 1}}  XOR  1,0  SUM +1
    {{1, 1, 0, 0}}  XOR  1,1  SUM +2
    {{1, 0, 1, 0}}  XOR  0,0
    {{1, 0, 0, 1}}  XOR  0,0

    {{0, 1, 1, 0}}  XOR  0,0
    {{0, 1, 0, 1}}  XOR  0,0
    {{0, 0, 1, 1}}  XOR  1,1  SUM -2
    {{1, 1, 1, 0}}  XOR  1,0  SUM -1
    {{1, 1, 0, 1}}  XOR  0,1  SUM -1
    {{1, 0, 1, 1}}  XOR  0,1  SUM -1
    {{0, 1, 1, 1}}  XOR  1,0  SUM -1
    {{0, 0, 0, 0}}  XOR  0,0


    XOR at half of W and SUM (over SUM)?


    This set       {-1, -1, -1, -1, -2, +2, +1, +1, +1, +1}
                               and last set
                           {-2, -1, 0, +1, +2}

 Right here there is an inner pair outer pair rule I'm working out,
 or a SUM over SUM, or another rule. But I need to iternerate these
 larger sequences with error correction (this plugin's job)
 to check my work

    Expected result   {-4,-3,-2,-1,0,+1,+2,+3,+4}




So is redundancy built into the number system defined?
We only need half the pattern to get the last half with OR

    Let's look at three

    {{0, 0, 0}}
    {{1, 0, 0}}
    {{0, 1, 0}}
    {{0, 0, 1}}
    {{1, 1, 0}}
    {{1, 0, 1}}
    {{0, 1, 1}}
    {{1, 1, 1}}


    Notice how I can toss out half the information


    {{0, 0, 0}}  T
    {{1, 0, 0}}  O
    {{0, 1, 0}}  S
    {{0, 0, 1}}  S



   ____CUT______
    {{1, 1, 0}}
    {{1, 0, 1}}
    {{0, 1, 1}}
    {{1, 1, 1}}


    And just OR it back from whats left then rotate

    {{1, 1, 0}}      {{0, 0, 1}}            {{0, 0, 0}}
    {{1, 0, 1}}  OR  {{0, 1, 0}}__ ROTATE __{{1, 0, 0}}
    {{0, 1, 1}}      {{1, 0, 0}}            {{0, 1, 0}}
    {{1, 1, 1}}      {{0, 0, 0}}            {{0, 0, 1}}
                                             ~G~L~U~E~
                                            {{1, 1, 0}}
                                            {{1, 0, 1}}
                                            {{0, 1, 1}}
                                            {{1, 1, 1}}


  Banach-Tarski!!!

  Raid 0 striped ARRAY!!!  just kidding I don't _really_ believe that,
  but it's neat to see the universe keeps a back up =] in the number system

  To me this say's Time is forever, no take backs no time machines allowed
  Entrophy always increases

*********************************************************************/

#include "plugin.h"
#include "fixedpoint.h"
#include "lib/playergfx.h"
#include "lib/pluginlib_exit.h"

#if LCD_DEPTH > 1
#include "lib/mylcd.h" /* MYLCD_CFG_RB_XLCD or MYLCD_CFG_PGFX */
#include "lib/grey.h"
#else
#include "lib/grey.h"
#include "lib/mylcd.h" /* MYLCD_CFG_GREYLIB or MYLCD_CFG_PGFX */
#endif
#include "lib/xlcd.h"

/* Loops that the values are displayed */
#define DISP_TIME 30

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define SHAPE_QUIT          BUTTON_OFF
#define SHAPE_NEXT          BUTTON_RIGHT
#define SHAPE_PREV          BUTTON_LEFT
#define SHAPE_INC           BUTTON_UP
#define SHAPE_DEC           BUTTON_DOWN
#define SHAPE_MODE          BUTTON_F1
#define SHAPE_PAUSE         BUTTON_PLAY
#define SHAPE_HIGHSPEED     BUTTON_ON

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
#define SHAPE_QUIT          BUTTON_OFF
#define SHAPE_NEXT          BUTTON_RIGHT
#define SHAPE_PREV          BUTTON_LEFT
#define SHAPE_INC           BUTTON_UP
#define SHAPE_DEC           BUTTON_DOWN
#define SHAPE_MODE          BUTTON_F1
#define SHAPE_PAUSE         BUTTON_SELECT
#define SHAPE_HIGHSPEED     BUTTON_ON

#elif CONFIG_KEYPAD == PLAYER_PAD
#define SHAPE_QUIT          BUTTON_STOP
#define SHAPE_INC           BUTTON_RIGHT
#define SHAPE_DEC           BUTTON_LEFT
#define SHAPE_NEXT          (BUTTON_ON | BUTTON_RIGHT)
#define SHAPE_PREV          (BUTTON_ON | BUTTON_LEFT)
#define SHAPE_MODE          BUTTON_MENU
#define SHAPE_PAUSE         BUTTON_PLAY
#define SHAPE_HIGHSPEED_PRE BUTTON_ON
#define SHAPE_HIGHSPEED     (BUTTON_ON | BUTTON_REL)

#elif CONFIG_KEYPAD == ONDIO_PAD
#define SHAPE_QUIT          BUTTON_OFF
#define SHAPE_NEXT          BUTTON_RIGHT
#define SHAPE_PREV          BUTTON_LEFT
#define SHAPE_INC           BUTTON_UP
#define SHAPE_DEC           BUTTON_DOWN
#define SHAPE_MODE_PRE      BUTTON_MENU
#define SHAPE_MODE          (BUTTON_MENU | BUTTON_REL)
#define SHAPE_PAUSE         (BUTTON_MENU | BUTTON_LEFT)
#define SHAPE_HIGHSPEED     (BUTTON_MENU | BUTTON_RIGHT)

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define SHAPE_QUIT          BUTTON_OFF
#define SHAPE_NEXT          BUTTON_RIGHT
#define SHAPE_PREV          BUTTON_LEFT
#define SHAPE_INC           BUTTON_UP
#define SHAPE_DEC           BUTTON_DOWN
#define SHAPE_MODE          BUTTON_MODE
#define SHAPE_PAUSE         BUTTON_ON
#define SHAPE_HIGHSPEED     BUTTON_SELECT

#define SHAPE_RC_QUIT       BUTTON_RC_STOP

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define SHAPE_QUIT          (BUTTON_SELECT | BUTTON_MENU)
#define SHAPE_NEXT          BUTTON_RIGHT
#define SHAPE_PREV          BUTTON_LEFT
#define SHAPE_INC           BUTTON_SCROLL_FWD
#define SHAPE_DEC           BUTTON_SCROLL_BACK
#define SHAPE_MODE          BUTTON_MENU
#define SHAPE_PAUSE         BUTTON_PLAY
#define SHAPE_HIGHSPEED_PRE BUTTON_SELECT
#define SHAPE_HIGHSPEED     (BUTTON_SELECT | BUTTON_REL)

#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
#define SHAPE_QUIT          BUTTON_PLAY
#define SHAPE_NEXT          BUTTON_RIGHT
#define SHAPE_PREV          BUTTON_LEFT
#define SHAPE_INC           BUTTON_UP
#define SHAPE_DEC           BUTTON_DOWN
#define SHAPE_MODE          BUTTON_MODE
#define SHAPE_PAUSE         BUTTON_SELECT
#define SHAPE_HIGHSPEED     BUTTON_EQ

#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)
#define SHAPE_QUIT          BUTTON_POWER
#define SHAPE_NEXT          BUTTON_RIGHT
#define SHAPE_PREV          BUTTON_LEFT
#define SHAPE_INC           BUTTON_UP
#define SHAPE_DEC           BUTTON_DOWN
#define SHAPE_MODE          BUTTON_REC
#define SHAPE_PAUSE         BUTTON_PLAY
#define SHAPE_HIGHSPEED     BUTTON_SELECT

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define SHAPE_QUIT          BUTTON_POWER
#define SHAPE_NEXT          BUTTON_RIGHT
#define SHAPE_PREV          BUTTON_LEFT
#define SHAPE_INC           BUTTON_UP
#define SHAPE_DEC           BUTTON_DOWN
#define SHAPE_MODE          BUTTON_MENU
#define SHAPE_PAUSE         BUTTON_SELECT
#define SHAPE_HIGHSPEED     BUTTON_A

#elif (CONFIG_KEYPAD == SANSA_E200_PAD)
#define SHAPE_QUIT          BUTTON_POWER
#define SHAPE_NEXT          BUTTON_RIGHT
#define SHAPE_PREV          BUTTON_LEFT
#define SHAPE_INC           BUTTON_SCROLL_FWD
#define SHAPE_DEC           BUTTON_SCROLL_BACK
#define SHAPE_MODE          BUTTON_DOWN
#define SHAPE_PAUSE         BUTTON_UP
#define SHAPE_HIGHSPEED     BUTTON_SELECT

#elif (CONFIG_KEYPAD == SANSA_FUZE_PAD)
#define SHAPE_QUIT          (BUTTON_HOME|BUTTON_REPEAT)
#define SHAPE_NEXT          BUTTON_RIGHT
#define SHAPE_PREV          BUTTON_LEFT
#define SHAPE_INC           BUTTON_SCROLL_FWD
#define SHAPE_DEC           BUTTON_SCROLL_BACK
#define SHAPE_MODE          BUTTON_DOWN
#define SHAPE_PAUSE         BUTTON_UP
#define SHAPE_HIGHSPEED     BUTTON_SELECT

#elif (CONFIG_KEYPAD == SANSA_C200_PAD) || \
(CONFIG_KEYPAD == SANSA_CLIP_PAD) || \
(CONFIG_KEYPAD == SANSA_M200_PAD)
#define SHAPE_QUIT          BUTTON_POWER
#define SHAPE_NEXT          BUTTON_RIGHT
#define SHAPE_PREV          BUTTON_LEFT
#define SHAPE_INC           BUTTON_VOL_UP
#define SHAPE_DEC           BUTTON_VOL_DOWN
#define SHAPE_MODE          BUTTON_DOWN
#define SHAPE_PAUSE         BUTTON_UP
#define SHAPE_HIGHSPEED     BUTTON_SELECT


#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)
#define SHAPE_QUIT          BUTTON_POWER
#define SHAPE_NEXT          BUTTON_RIGHT
#define SHAPE_PREV          BUTTON_LEFT
#define SHAPE_INC           BUTTON_SCROLL_UP
#define SHAPE_DEC           BUTTON_SCROLL_DOWN
#define SHAPE_MODE          BUTTON_REW
#define SHAPE_PAUSE         BUTTON_PLAY
#define SHAPE_HIGHSPEED     BUTTON_FF

#elif CONFIG_KEYPAD == MROBE500_PAD
#define SHAPE_QUIT          BUTTON_POWER

#elif CONFIG_KEYPAD == GIGABEAT_S_PAD
#define SHAPE_QUIT          BUTTON_BACK
#define SHAPE_NEXT          BUTTON_RIGHT
#define SHAPE_PREV          BUTTON_LEFT
#define SHAPE_INC           BUTTON_UP
#define SHAPE_DEC           BUTTON_DOWN
#define SHAPE_MODE          BUTTON_MENU
#define SHAPE_PAUSE         BUTTON_PLAY
#define SHAPE_HIGHSPEED     BUTTON_SELECT

#elif (CONFIG_KEYPAD == MROBE100_PAD)
#define SHAPE_QUIT          BUTTON_POWER
#define SHAPE_NEXT          BUTTON_RIGHT
#define SHAPE_PREV          BUTTON_LEFT
#define SHAPE_INC           BUTTON_UP
#define SHAPE_DEC           BUTTON_DOWN
#define SHAPE_MODE          BUTTON_MENU
#define SHAPE_PAUSE         BUTTON_PLAY
#define SHAPE_HIGHSPEED     BUTTON_SELECT

#elif (CONFIG_KEYPAD == IAUDIO_M3_PAD)
#define SHAPE_QUIT          BUTTON_RC_REC
#define SHAPE_NEXT          BUTTON_RC_FF
#define SHAPE_PREV          BUTTON_RC_REW
#define SHAPE_INC           BUTTON_RC_VOL_UP
#define SHAPE_DEC           BUTTON_RC_VOL_DOWN
#define SHAPE_MODE          BUTTON_RC_MODE
#define SHAPE_PAUSE         BUTTON_RC_PLAY
#define SHAPE_HIGHSPEED     BUTTON_RC_MENU

#elif CONFIG_KEYPAD == COWON_D2_PAD
#define SHAPE_QUIT          BUTTON_POWER

#elif (CONFIG_KEYPAD == IAUDIO67_PAD)
#define SHAPE_QUIT          BUTTON_POWER
#define SHAPE_NEXT          BUTTON_RIGHT
#define SHAPE_PREV          BUTTON_LEFT
#define SHAPE_INC           BUTTON_VOLUP
#define SHAPE_DEC           BUTTON_VOLDOWN
#define SHAPE_MODE          BUTTON_MENU
#define SHAPE_PAUSE         BUTTON_PLAY
#define SHAPE_HIGHSPEED     BUTTON_STOP

#elif CONFIG_KEYPAD == CREATIVEZVM_PAD
#define SHAPE_QUIT          BUTTON_BACK
#define SHAPE_NEXT          BUTTON_RIGHT
#define SHAPE_PREV          BUTTON_LEFT
#define SHAPE_INC           BUTTON_UP
#define SHAPE_DEC           BUTTON_DOWN
#define SHAPE_MODE          BUTTON_MENU
#define SHAPE_PAUSE         BUTTON_PLAY
#define SHAPE_HIGHSPEED     BUTTON_SELECT

#elif CONFIG_KEYPAD == CREATIVE_ZENXFI3_PAD
#define SHAPE_QUIT          BUTTON_POWER
#define SHAPE_NEXT          BUTTON_UP
#define SHAPE_PREV          BUTTON_DOWN
#define SHAPE_INC           BUTTON_VOL_UP
#define SHAPE_DEC           BUTTON_VOL_DOWN
#define SHAPE_MODE          BUTTON_MENU
#define SHAPE_PAUSE         (BUTTON_PLAY|BUTTON_REL)
#define SHAPE_HIGHSPEED     BUTTON_BACK

#elif CONFIG_KEYPAD == PHILIPS_HDD1630_PAD
#define SHAPE_QUIT          BUTTON_POWER
#define SHAPE_NEXT          BUTTON_RIGHT
#define SHAPE_PREV          BUTTON_LEFT
#define SHAPE_INC           BUTTON_UP
#define SHAPE_DEC           BUTTON_DOWN
#define SHAPE_MODE          BUTTON_MENU
#define SHAPE_PAUSE         BUTTON_SELECT
#define SHAPE_HIGHSPEED     BUTTON_VIEW

#elif CONFIG_KEYPAD == PHILIPS_HDD6330_PAD
#define SHAPE_QUIT          BUTTON_POWER
#define SHAPE_NEXT          BUTTON_NEXT
#define SHAPE_PREV          BUTTON_PREV
#define SHAPE_INC           BUTTON_UP
#define SHAPE_DEC           BUTTON_DOWN
#define SHAPE_MODE          BUTTON_MENU
#define SHAPE_PAUSE         BUTTON_PLAY
#define SHAPE_HIGHSPEED     BUTTON_RIGHT

#elif CONFIG_KEYPAD == PHILIPS_SA9200_PAD
#define SHAPE_QUIT          BUTTON_POWER
#define SHAPE_NEXT          BUTTON_NEXT
#define SHAPE_PREV          BUTTON_PREV
#define SHAPE_INC           BUTTON_UP
#define SHAPE_DEC           BUTTON_DOWN
#define SHAPE_MODE          BUTTON_MENU
#define SHAPE_PAUSE         BUTTON_PLAY
#define SHAPE_HIGHSPEED     BUTTON_RIGHT

#elif CONFIG_KEYPAD == ONDAVX747_PAD
#define SHAPE_QUIT          BUTTON_POWER
#elif CONFIG_KEYPAD == ONDAVX777_PAD
#define SHAPE_QUIT          BUTTON_POWER

#elif (CONFIG_KEYPAD == SAMSUNG_YH820_PAD) || \
      (CONFIG_KEYPAD == SAMSUNG_YH92X_PAD)
#define SHAPE_QUIT          BUTTON_REW
#define SHAPE_NEXT          BUTTON_RIGHT
#define SHAPE_PREV          BUTTON_LEFT
#define SHAPE_INC           BUTTON_UP
#define SHAPE_DEC           BUTTON_DOWN
#define SHAPE_MODE          (BUTTON_FFWD|BUTTON_REL)
#define SHAPE_MODE_PRE      BUTTON_FFWD
#define SHAPE_PAUSE         BUTTON_PLAY
#define SHAPE_HIGHSPEED     (BUTTON_FFWD|BUTTON_REPEAT)
#define SHAPE_HIGHSPEED_PRE BUTTON_FFWD

#elif CONFIG_KEYPAD == PBELL_VIBE500_PAD
#define SHAPE_QUIT          BUTTON_REC
#define SHAPE_NEXT          BUTTON_NEXT
#define SHAPE_PREV          BUTTON_PREV
#define SHAPE_INC           BUTTON_UP
#define SHAPE_DEC           BUTTON_DOWN
#define SHAPE_MODE          BUTTON_MENU
#define SHAPE_PAUSE         BUTTON_PLAY
#define SHAPE_HIGHSPEED     BUTTON_OK

#elif CONFIG_KEYPAD == MPIO_HD200_PAD
#define SHAPE_QUIT          (BUTTON_REC | BUTTON_PLAY)
#define SHAPE_NEXT          BUTTON_FF
#define SHAPE_PREV          BUTTON_REW
#define SHAPE_INC           BUTTON_VOL_UP
#define SHAPE_DEC           BUTTON_VOL_DOWN
#define SHAPE_MODE          BUTTON_REC
#define SHAPE_PAUSE         BUTTON_PLAY
#define SHAPE_HIGHSPEED     BUTTON_FUNC

#elif CONFIG_KEYPAD == MPIO_HD300_PAD
#define SHAPE_QUIT          (BUTTON_MENU | BUTTON_REPEAT)
#define SHAPE_NEXT          BUTTON_FF
#define SHAPE_PREV          BUTTON_REW
#define SHAPE_INC           BUTTON_UP
#define SHAPE_DEC           BUTTON_DOWN
#define SHAPE_MODE          BUTTON_MENU
#define SHAPE_PAUSE         BUTTON_PLAY
#define SHAPE_HIGHSPEED     BUTTON_ENTER

#elif CONFIG_KEYPAD == SANSA_FUZEPLUS_PAD
#define SHAPE_QUIT          BUTTON_POWER
#define SHAPE_NEXT          BUTTON_RIGHT
#define SHAPE_PREV          BUTTON_LEFT
#define SHAPE_INC           BUTTON_UP
#define SHAPE_DEC           BUTTON_DOWN
#define SHAPE_MODE          BUTTON_SELECT
#define SHAPE_PAUSE         BUTTON_PLAYPAUSE
#define SHAPE_HIGHSPEED     BUTTON_BACK

#elif CONFIG_KEYPAD == SANSA_CONNECT_PAD
#define SHAPE_QUIT          BUTTON_POWER
#define SHAPE_NEXT          BUTTON_NEXT
#define SHAPE_PREV          BUTTON_PREV
#define SHAPE_INC           BUTTON_VOL_UP
#define SHAPE_DEC           BUTTON_VOL_DOWN
#define SHAPE_MODE          BUTTON_SELECT
#define SHAPE_PAUSE         BUTTON_DOWN
#define SHAPE_HIGHSPEED     BUTTON_LEFT

#elif (CONFIG_KEYPAD == SAMSUNG_YPR0_PAD)
#define SHAPE_QUIT          BUTTON_BACK
#define SHAPE_NEXT          BUTTON_RIGHT
#define SHAPE_PREV          BUTTON_LEFT
#define SHAPE_INC           BUTTON_UP
#define SHAPE_DEC           BUTTON_DOWN
#define SHAPE_MODE          BUTTON_MENU
#define SHAPE_PAUSE         BUTTON_USER
#define SHAPE_HIGHSPEED     BUTTON_SELECT

#elif (CONFIG_KEYPAD == HM60X_PAD)
#define SHAPE_QUIT          BUTTON_POWER
#define SHAPE_NEXT          BUTTON_RIGHT
#define SHAPE_PREV          BUTTON_LEFT
#define SHAPE_INC           BUTTON_UP
#define SHAPE_DEC           BUTTON_DOWN
#define SHAPE_MODE          (BUTTON_SELECT|BUTTON_POWER)
#define SHAPE_PAUSE         BUTTON_SELECT
#define SHAPE_HIGHSPEED     (BUTTON_UP|BUTTON_POWER)

#elif (CONFIG_KEYPAD == HM801_PAD)
#define SHAPE_QUIT          BUTTON_POWER
#define SHAPE_NEXT          BUTTON_RIGHT
#define SHAPE_PREV          BUTTON_LEFT
#define SHAPE_INC           BUTTON_UP
#define SHAPE_DEC           BUTTON_DOWN
#define SHAPE_MODE          BUTTON_PREV
#define SHAPE_PAUSE         BUTTON_SELECT
#define SHAPE_HIGHSPEED     BUTTON_NEXT

#elif (CONFIG_KEYPAD == SONY_NWZ_PAD)
#define SHAPE_QUIT        BUTTON_BACK
#define SHAPE_NEXT        BUTTON_RIGHT
#define SHAPE_PREV        BUTTON_LEFT
#define SHAPE_INC         BUTTON_UP
#define SHAPE_DEC         BUTTON_DOWN
#define SHAPE_MODE        (BUTTON_POWER|BUTTON_UP)
#define SHAPE_PAUSE       BUTTON_PLAY
#define SHAPE_HIGHSPEED   (BUTTON_POWER|BUTTON_DOWN)

#elif (CONFIG_KEYPAD == CREATIVE_ZEN_PAD)
#define SHAPE_QUIT        BUTTON_BACK
#define SHAPE_NEXT        BUTTON_RIGHT
#define SHAPE_PREV        BUTTON_LEFT
#define SHAPE_INC         BUTTON_UP
#define SHAPE_DEC         BUTTON_DOWN
#define SHAPE_MODE        BUTTON_MENU
#define SHAPE_PAUSE       BUTTON_PLAYPAUSE
#define SHAPE_HIGHSPEED   BUTTON_SHORTCUT

#elif (CONFIG_KEYPAD == DX50_PAD)
#define SHAPE_QUIT          (BUTTON_POWER|BUTTON_REL)
#define SHAPE_NEXT          BUTTON_RIGHT
#define SHAPE_PREV          BUTTON_LEFT
#define SHAPE_INC           BUTTON_VOL_UP
#define SHAPE_DEC           BUTTON_VOL_DOWN
#define SHAPE_MODE          BUTTON_PLAY

#else
#error No keymap defined!
#endif

#ifdef HAVE_TOUCHSCREEN
#ifndef SHAPE_QUIT
#define SHAPE_QUIT          BUTTON_TOPLEFT
#endif
#ifndef SHAPE_NEXT
#define SHAPE_NEXT          BUTTON_MIDRIGHT
#endif
#ifndef SHAPE_PREV
#define SHAPE_PREV          BUTTON_MIDLEFT
#endif
#ifndef SHAPE_INC
#define SHAPE_INC           BUTTON_TOPMIDDLE
#endif
#ifndef SHAPE_DEC
#define SHAPE_DEC           BUTTON_BOTTOMMIDDLE
#endif
#ifndef SHAPE_MODE
#define SHAPE_MODE          BUTTON_TOPRIGHT
#endif
#ifndef SHAPE_PAUSE
#define SHAPE_PAUSE         BUTTON_CENTER
#endif
#ifndef SHAPE_HIGHSPEED
#define SHAPE_HIGHSPEED     BUTTON_BOTTOMRIGHT
#endif
#endif

#ifdef HAVE_LCD_BITMAP

#define DIST (MIN(LCD_HEIGHT, LCD_WIDTH) / 16)
static int x_off = LCD_WIDTH/2;
static int y_off = LCD_HEIGHT/2;

#if LCD_DEPTH == 1
#define USEGSLIB
GREY_INFO_STRUCT
struct my_lcd {
    void (*update)(void);
    void (*clear_display)(void);
    void (*drawline)(int x1, int y1, int x2, int y2);
    void (*putsxy)(int x, int y, const unsigned char *string);
};

static struct my_lcd greyfuncs = {
    grey_update, grey_clear_display, grey_drawline, grey_putsxy
};
static struct my_lcd lcdfuncs; /* initialised at runtime */
static struct my_lcd *mylcd = &greyfuncs;

#define MYLCD(fn) mylcd->fn

#else
#define MYLCD(fn) rb->lcd_ ## fn
#endif

#if CONFIG_LCD == LCD_SSD1815
#define ASPECT 320 /* = 1.25 (fixed point 24.8) */
#else
#define ASPECT 256 /* = 1.00 */
#endif

#else /* !LCD_BITMAP */

#define MYLCD(fn) pgfx_ ## fn
#define DIST 9
static int x_off = 10;
static int y_off = 7;
#define ASPECT 300 /* = 1.175 */

#endif /* !LCD_BITMAP */


//Begin Geometry
struct point_3D {
    int x, y, z;
};

struct point_2D {
    int x, y;
};

struct line {
    int start, end;
};

struct counter {
    const char *label;
    short speed;
    short angle;
};



// Begin Q-bit
// With adjustments to avoid null n can be represented as 0 and 1
//            projected on a matrix of size n(2^n)
//            to define any brane of n that fits in max memory
// Todo: Find Max Memory

// unsigned char *brane_array [1024] [10]; // 1mb for Neon

long long int c;  //counter1
long long int d; //counter2

void *ptr = NULL;
size_t bytes_left;

void init(void)
{
    ptr = rb->plugin_get_buffer(&bytes_left);
}

void *alloc_bytes(size_t n)
{
    void *ret = NULL;
    if(bytes_left - n <= 0)
    {
        ret = ptr;
        ptr = (char*) ptr + n;
        bytes_left -= n;
    }
    return ret;
}

//static void number_pattern(unsigned int i)
//{
//    return;
//}


//  Begin Shape
#define POINTS 3
#define FACES 2
#define LINES 3

struct face {
    int corner[POINTS];
    int line[LINES];
};


/* initial, unrotated shape corners */
static const struct point_3D sommet[POINTS] =
{
    {-DIST+0, -DIST+200, 0},
    { DIST+0, -DIST+200, 0},
    { DIST+0,  DIST+200, 0}
};


/* The 12 lines forming the edges */
static const struct line lines[LINES] =
{
    {0, 1}, {1, 2}, {2, 0}
};


static bool lines_drawn[LINES];


/* The faces of the shape; points are in clockwise order when viewed
   from the outside */
static const struct face faces[FACES] =
{
    {{0, 1, 2}, {0, 1, 2}},
    {{2, 1, 0}, {2, 1, 0}},
};


static struct counter axes[] = {
    {"x-axis", 3, 0},
    {"y-axis", 1, 0},
    {"z-axis", 6, 0}
};
//End Shape



//Begin Transpose linear matrix definitions

static struct point_3D point3D[3];
static struct point_2D point2D[3];

static long matrice[3][3];
static long z_off = 600;


//Define defait video mode
static int mode = 0;

//Define Max points for loops below
static const int nb_points = POINTS;




//ToDo more easier color assignment
#if LCD_DEPTH > 1 || defined(USEGSLIB)
static const unsigned face_colors[6] =
{
#ifdef HAVE_LCD_COLOR
    LCD_RGBPACK(255, 0, 0), LCD_RGBPACK(255, 0, 0), LCD_RGBPACK(0, 255, 0),
    LCD_RGBPACK(0, 255, 0), LCD_RGBPACK(0, 0, 255), LCD_RGBPACK(0, 0, 255)
#elif defined(USEGSLIB)
#ifdef MROBE_100
    GREY_LIGHTGRAY, GREY_LIGHTGRAY, GREY_DARKGRAY,
    GREY_DARKGRAY,  GREY_WHITE,     GREY_WHITE
#else
    GREY_LIGHTGRAY, GREY_LIGHTGRAY, GREY_DARKGRAY,
    GREY_DARKGRAY,  GREY_BLACK,     GREY_BLACK
#endif
#else
    LCD_LIGHTGRAY, LCD_LIGHTGRAY, LCD_DARKGRAY,
    LCD_DARKGRAY,  LCD_BLACK,     LCD_BLACK
#endif
};
#endif



//ToDO  Get rid of this below
enum {
#if LCD_DEPTH > 1 || defined(USEGSLIB)
    SOLID,
#endif
    HIDDEN_LINES,
    NUM_MODES
};



static void shape_rotate(int xa, int ya, int za)
{
/* ToDo:  FF via quake


  FROM: https://en.wikipedia.org/wiki/Fast_inverse_square_root

  A worked example
  As an example, consider the number x = 0.15625,
  for which we want to calculate 1/√x ≈ 2.52982.
  The first steps of the algorithm are illustrated below:

  0011_1110_0010_0000_0000_0000_0000_0000  Bit pattern of both x and i
  0001_1111_0001_0000_0000_0000_0000_0000  Shift right one position: (i >> 1)
  0101_1111_0011_0111_0101_1001_1101_1111  The magic number 0x5f3759df
  0100_0000_0010_0111_0101_1001_1101_1111  The result of 0x5f3759df - (i >> 1)

  Using IEEE 32-bit representation:
  0_01111100_01000000000000000000000  1.25 * 2^-3
  0_00111110_00100000000000000000000  1.125 * 2^-65
  0_10111110_01101110101100111011111  1.432430... * 2^+63
  0_10000000_01001110101100111011111  1.307430... * 2^+1

  Reinterpreting this last bit pattern as a floating point number
  gives the approximation y = 2.61486, which has an error of about 3.4%.
  After the single iteration of Newton's method, the final result
  is y = 2.52549, in error by only 0.17%.

--->
    float Q_rsqrt( float number )
    {
        long i;
        float x2, y;
        const float threehalfs = 1.5F;

        x2 = number * 0.5F;
        y  = number;
        i  = * ( long * ) &y;                       // evil floating point bit level hacking
        i  = 0x5f3759df - ( i >> 1 );               // what the fuck?
        y  = * ( float * ) &i;
        y  = y * ( threehalfs - ( x2 * y * y ) );   // 1st iteration
//        y  = y * ( threehalfs - ( x2 * y * y ) );   // 2nd iteration, this can be removed

        return y;
    }
<---

*/

    int i;
    /* Just to prevent unnecessary lookups */
    int sxa, cxa, sya, cya, sza, cza;

    sxa = fp14_sin(xa);
    cxa = fp14_cos(xa);
    sya = fp14_sin(ya);
    cya = fp14_cos(ya);
    sza = fp14_sin(za);
    cza = fp14_cos(za);

    /* calculate overall translation matrix */
    matrice[0][0] = (cza * cya) >> 14;
    matrice[1][0] = (sza * cya) >> 14;
    matrice[2][0] = -sya;

    matrice[0][1] = (((cza * sya) >> 14) * sxa - sza * cxa) >> 14;
    matrice[1][1] = (((sza * sya) >> 14) * sxa + cxa * cza) >> 14;
    matrice[2][1] = (sxa * cya) >> 14;

    matrice[0][2] = (((cza * sya) >> 14) * cxa + sza * sxa) >> 14;
    matrice[1][2] = (((sza * sya) >> 14) * cxa - cza * sxa) >> 14;
    matrice[2][2] = (cxa * cya) >> 14;


    /* apply translation matrix to all points */
    for (i = 0; i < nb_points; i++)
    {
        point3D[i].x = matrice[0][0] * sommet[i].x + matrice[1][0] * sommet[i].y
                     + matrice[2][0] * sommet[i].z;

        point3D[i].y = matrice[0][1] * sommet[i].x + matrice[1][1] * sommet[i].y
                     + matrice[2][1] * sommet[i].z;

        point3D[i].z = matrice[0][2] * sommet[i].x + matrice[1][2] * sommet[i].y
                     + matrice[2][2] * sommet[i].z;
    }

}


static void shape_viewport(void)
{
    int i;

    /* Do viewport transformation for all points */
    for (i = 0; i < nb_points; i++)
    {
#if ASPECT != 256
        point2D[i].x = (point3D[i].x * ASPECT) / (point3D[i].z + (z_off << 14))
                     + x_off;
#else
        point2D[i].x = (point3D[i].x << 8) / (point3D[i].z + (z_off << 14))
                     + x_off;
#endif
        point2D[i].y = (point3D[i].y << 8) / (point3D[i].z + (z_off << 14))
                     + y_off;
    }
}




static void shape_draw(void)
{
    int i, j, line;

#if LCD_DEPTH > 1 || defined(USEGSLIB)
    unsigned old_foreground;
#endif

    switch (mode)
    {

#if LCD_DEPTH > 1 || defined(USEGSLIB)
      case SOLID:

        old_foreground = mylcd_get_foreground();
        for (i = 0; i < FACES; i++)
        {
            /* backface culling; if the shape winds counter-clockwise, we are
             * looking at the backface, and the (simplified) cross product
             * is < 0. Do not draw it. */
            if (0 >= (point2D[faces[i].corner[1]].x - point2D[faces[i].corner[0]].x)
                   * (point2D[faces[i].corner[2]].y - point2D[faces[i].corner[1]].y)
                   - (point2D[faces[i].corner[1]].y - point2D[faces[i].corner[0]].y)
                   * (point2D[faces[i].corner[2]].x - point2D[faces[i].corner[1]].x))
                continue;

            mylcd_set_foreground(face_colors[i]);
            mylcd_filltriangle(point2D[faces[i].corner[0]].x,
                               point2D[faces[i].corner[0]].y,
                               point2D[faces[i].corner[1]].x,
                               point2D[faces[i].corner[1]].y,
                               point2D[faces[i].corner[2]].x,
                               point2D[faces[i].corner[2]].y);

            mylcd_filltriangle(point2D[faces[i].corner[2]].x,
                               point2D[faces[i].corner[2]].y,
                               point2D[faces[i].corner[1]].x,
                               point2D[faces[i].corner[1]].y,
                               point2D[faces[i].corner[0]].x,
                               point2D[faces[i].corner[0]].y);

        }
        mylcd_set_foreground(old_foreground);
        break;
#endif /* (LCD_DEPTH > 1) || GSLIB */

      case HIDDEN_LINES:

        rb->memset(lines_drawn, 0, sizeof(lines_drawn));
        for (i = 0; i < FACES; i++)
        {
            /* backface culling; if the shape winds counter-clockwise, we are
             * looking at the backface, and the (simplified) cross product
             * is < 0. Do not draw it. */
            if (0 >= (point2D[faces[i].corner[1]].x - point2D[faces[i].corner[0]].x)
                   * (point2D[faces[i].corner[2]].y - point2D[faces[i].corner[1]].y)
                   - (point2D[faces[i].corner[1]].y - point2D[faces[i].corner[0]].y)
                   * (point2D[faces[i].corner[2]].x - point2D[faces[i].corner[1]].x))
                continue;

            for (j = 0; j < LINES; j++)
            {
                line = faces[i].line[j];
                if (!lines_drawn[line])
                {
                    lines_drawn[line] = true;
                    MYLCD(drawline)(point2D[lines[line].start].x,
                                    point2D[lines[line].start].y,
                                    point2D[lines[line].end].x,
                                    point2D[lines[line].end].y);
                }
            }
        }
        break;
    }
}




static void cleanup(void)
{
#ifdef USEGSLIB
    grey_release();
#elif defined HAVE_LCD_CHARCELLS
    pgfx_release();
#endif
}






enum plugin_status plugin_start(const void* parameter)
{

    int t_disp = 0;



#ifdef USEGSLIB
    unsigned char *gbuf;
    size_t gbuf_size = 0;
    bool mode_switch = true;
#endif

    int button;

#if defined(SHAPE_MODE_PRE) || \
    defined(SHAPE_PAUSE_PRE) || \
    defined(SHAPE_HIGHSPEED_PRE)
    int lastbutton = BUTTON_NONE;
#endif

    int curr = 0;
    bool highspeed = false;
    bool paused = false;
    bool redraw = true;
    bool quit = false;
    (void)(parameter);

#ifdef HAVE_LCD_BITMAP
#if defined(USEGSLIB)
    gbuf = (unsigned char *)rb->plugin_get_buffer(&gbuf_size);
    if (!grey_init(gbuf, gbuf_size, GREY_BUFFERED,
                   LCD_WIDTH, LCD_HEIGHT, NULL))
    {
        rb->splash(HZ, "Couldn't init greyscale display");
        return PLUGIN_ERROR;
    }
    /* init lcd_ function pointers */
    lcdfuncs.update =        rb->lcd_update;
    lcdfuncs.clear_display = rb->lcd_clear_display;
    lcdfuncs.drawline =      rb->lcd_drawline;
    lcdfuncs.putsxy =        rb->lcd_putsxy;

#ifdef MROBE_100
    grey_set_background(GREY_BLACK);
#endif
    grey_setfont(FONT_SYSFIXED);
#endif
    rb->lcd_setfont(FONT_SYSFIXED);

#else /* LCD_CHARCELLS */
    if (!pgfx_init(4, 2))
    {
        rb->splash(HZ*2, "Old LCD :(");
        return PLUGIN_OK;
    }
    pgfx_display(0, 0);
#endif

    atexit(cleanup);
    while(!quit)
    {
        if (redraw)
        {
            MYLCD(clear_display)();
            shape_rotate(axes[0].angle, axes[1].angle, axes[2].angle);
            shape_viewport();
            shape_draw();
            redraw = false;
        }
#ifdef HAVE_LCD_BITMAP
        if (t_disp > 0)
        {
            char buffer[30];
            t_disp--;
            rb->snprintf(buffer, sizeof(buffer), "%s: %d %s",
                         axes[curr].label,
                         paused ? axes[curr].angle : axes[curr].speed,
                         highspeed ? "(hs)" : "");
            MYLCD(putsxy)(0, LCD_HEIGHT-8, buffer);
            if (t_disp == 0)
                redraw = true;
        }
#else
        if (t_disp > 0)
        {
            if (t_disp == DISP_TIME)
            {
                rb->lcd_puts(5, 0, axes[curr].label);
                rb->lcd_putsf(5, 1, "%d %c",
                             paused ? axes[curr].angle : axes[curr].speed,
                             highspeed ? 'H' : ' ');
            }
            t_disp--;
            if (t_disp == 0)
            {
                rb->lcd_clear_display();
                pgfx_display(0, 0);
            }
        }
#endif


#ifdef USEGSLIB
        if (mode_switch)
        {
            grey_show(mode == SOLID);
//            mode_switch = false;
        }
#endif

        MYLCD(update)();

        if (!paused)
        {
            int i;

            for (i = 0; i < 3; i++)
            {
                axes[i].angle += axes[i].speed;
                if (axes[i].angle > 359)
                    axes[i].angle -= 360;
                else if (axes[i].angle < 0)
                    axes[i].angle += 360;
            }


            d = (2^c) * (c);

//            rb->lcd_puts(1, 1, alloc_bytes(d));
//            rb->lcd_update();

            alloc_bytes(d);
//            rb->splash (HZ/25, alloc_bytes(d));
            rb->splashf(HZ/25, "%d", d);

            c++;
            redraw = true;
            highspeed = true;

            if (highspeed)
                rb->yield();
            else
                rb->sleep(HZ/25);
            button = rb->button_get(false);
        }
        else
        {
            button = rb->button_get_w_tmo(HZ/25);
        }

        switch (button)
        {
            case SHAPE_INC:
            case SHAPE_INC|BUTTON_REPEAT:
                break;
            case SHAPE_DEC:
            case SHAPE_DEC|BUTTON_REPEAT:
                break;
            case SHAPE_NEXT:
                break;
            case SHAPE_PREV:
                break;
            case SHAPE_MODE:
#ifdef SHAPE_MODE_PRE
                if (lastbutton != SHAPE_MODE_PRE)
                break;
#endif
                break;
            case SHAPE_PAUSE:
#ifdef SHAPE_PAUSE_PRE
                if (lastbutton != SHAPE_PAUSE_PRE)
                break;
#endif
                break;
            case SHAPE_HIGHSPEED:
#ifdef SHAPE_HIGHSPEED_PRE
                if (lastbutton != SHAPE_HIGHSPEED_PRE)
                break;
#endif
                break;
#ifdef SHAPE_RC_QUIT
            case SHAPE_RC_QUIT:

#endif
            case SHAPE_QUIT:
                quit = true;
                break;
            default:
                exit_on_usb(button);
                break;
        }

#if defined(SHAPE_MODE_PRE) || \
    defined(SHAPE_PAUSE_PRE) || \
    defined(SHAPE_HIGHSPEED_PRE)
        if (button != BUTTON_NONE)
            lastbutton = button;
#endif
    }
    return PLUGIN_OK;
}
