/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 Pengxuan Liu (Isaac)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/*
   00 01    21 22 23    43 44 45    65 66 67    87 88 89    109110111
00 |-----------|-----------|-----------|-----------|-----------|
01 |           |           |           |           |           |
   |***********|***********|***********|***********|***********|
   |***********|***********|***********|***********|***********|
11 |           |           |           |           |           |
12 |-----------|-----------|-----------|-----------|-----------|
13 |-----------|-----------|-----------|-----------|-----------| y1
14 |           |           |           |           |           |
   |           |           |           |           |           |
22 |           |           |           |           |           |
23 |-----------|-----------|-----------|-----------|-----------| y2
24 |           |           |           |           |           |
   |           |           |           |           |           |
32 |           |           |           |           |           |
33 |-----------|-----------|-----------|-----------|-----------| y3
34 |           |           |           |           |           |
   |           |           |           |           |           |
42 |           |           |           |           |           |
43 |-----------|-----------|-----------|-----------|-----------| y4
44 |           |           |           |           |           |
   |           |           |           |           |           |
52 |           |           |           |           |           |
53 |-----------|-----------|-----------|-----------|-----------| y5
54 |           |           |           |           |           |
   |           |           |           |           |           |
62 |           |           |           |           |           |
63 |-----------|-----------|-----------|-----------|-----------| y6
   x0          x1          x2          x3          x4          x5
*/

/*---------------------------------------------------------------------------
Features:
- Scientific number format core code.  Support range 10^-999 ~ 10^999
- Number of significant figures up to 10

Limitations:
- Right now, only accept "num, operator (+,-,*,/), num, =" input sequence.
  Input "3, +, 5, -, 2, =", the calculator will only do 5-2 and result = 3
  You have to input "3, +, 5, =, -, 2, =" to get 3+5-2 = 6

- "*,/" have no priority.  Actually you can't input 3+5*2 yet.

User Instructions:
use arrow button to move cursor, "play" button to select, "off" button to exit
F1: if typing numbers, it's equal to "Del"; otherwise, equal to "C"
F2: circle input "+, -, *, /"
F3: equal to "="

"MR"  :  load temp memory
"M+"  :  add currently display to temp memory
"C"   :  reset calculator
---------------------------------------------------------------------------*/

#include "plugin.h"
#ifdef HAVE_LCD_BITMAP
#include "math.h"

PLUGIN_HEADER

#define BUTTON_ROWS 5
#define BUTTON_COLS 5

#define REC_HEIGHT (int)(LCD_HEIGHT / (BUTTON_ROWS + 1))
#define REC_WIDTH (int)(LCD_WIDTH / BUTTON_COLS)

#define Y_6_POS (LCD_HEIGHT)       /* Leave room for the border */
#define Y_5_POS (Y_6_POS - REC_HEIGHT) /* y5 = 53 */
#define Y_4_POS (Y_5_POS - REC_HEIGHT) /* y4 = 43 */
#define Y_3_POS (Y_4_POS - REC_HEIGHT) /* y3 = 33 */
#define Y_2_POS (Y_3_POS - REC_HEIGHT) /* y2 = 23 */
#define Y_1_POS (Y_2_POS - REC_HEIGHT) /* y1 = 13 */
#define Y_0_POS 0                      /* y0 = 0  */

#define X_0_POS 0                      /* x0 = 0  */
#define X_1_POS (X_0_POS + REC_WIDTH)  /* x1 = 22 */
#define X_2_POS (X_1_POS + REC_WIDTH)  /* x2 = 44 */
#define X_3_POS (X_2_POS + REC_WIDTH)  /* x3 = 66 */
#define X_4_POS (X_3_POS + REC_WIDTH)  /* x4 = 88 */
#define X_5_POS (X_4_POS + REC_WIDTH)  /* x5 = 110, column 111 left blank */

#define TEXT_1_POS (Y_1_POS-10)  /* y1 = 2  */   /* blank height = 12 */
#define TEXT_2_POS (Y_2_POS-8)   /* y2 = 15 */   /* blank height = 9  */
#define TEXT_3_POS (Y_3_POS-8)   /* y3 = 25 */
#define TEXT_4_POS (Y_4_POS-8)   /* y4 = 35 */
#define TEXT_5_POS (Y_5_POS-8)   /* y5 = 45 */
#define TEXT_6_POS (Y_6_POS-8)   /* y6 = 55 */

#define SIGN(x) ((x)<0?-1:1)
#define ABS(x) ((x)<0?-(x):(x))

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define CALCULATOR_LEFT BUTTON_LEFT
#define CALCULATOR_RIGHT BUTTON_RIGHT
#define CALCULATOR_UP   BUTTON_UP
#define CALCULATOR_DOWN BUTTON_DOWN
#define CALCULATOR_QUIT BUTTON_OFF
#define CALCULATOR_INPUT BUTTON_PLAY
#define CALCULATOR_CALC BUTTON_F3
#define CALCULATOR_OPERATORS BUTTON_F2
#define CALCULATOR_CLEAR BUTTON_F1

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
#define CALCULATOR_LEFT BUTTON_LEFT
#define CALCULATOR_RIGHT BUTTON_RIGHT
#define CALCULATOR_UP   BUTTON_UP
#define CALCULATOR_DOWN BUTTON_DOWN
#define CALCULATOR_QUIT BUTTON_OFF
#define CALCULATOR_INPUT BUTTON_SELECT
#define CALCULATOR_CALC BUTTON_F3
#define CALCULATOR_OPERATORS BUTTON_F2
#define CALCULATOR_CLEAR BUTTON_F1

#elif CONFIG_KEYPAD == ONDIO_PAD
#define CALCULATOR_LEFT BUTTON_LEFT
#define CALCULATOR_RIGHT BUTTON_RIGHT
#define CALCULATOR_UP   BUTTON_UP
#define CALCULATOR_DOWN BUTTON_DOWN
#define CALCULATOR_QUIT BUTTON_OFF
#define CALCULATOR_INPUT_CALC_PRE BUTTON_MENU
#define CALCULATOR_INPUT (BUTTON_MENU | BUTTON_REL)
#define CALCULATOR_CALC (BUTTON_MENU | BUTTON_REPEAT)

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define CALCULATOR_LEFT BUTTON_LEFT
#define CALCULATOR_RIGHT BUTTON_RIGHT
#define CALCULATOR_UP   BUTTON_UP
#define CALCULATOR_DOWN BUTTON_DOWN
#define CALCULATOR_QUIT BUTTON_OFF
#define CALCULATOR_INPUT BUTTON_SELECT
#define CALCULATOR_CALC BUTTON_ON
#define CALCULATOR_OPERATORS BUTTON_MODE
#define CALCULATOR_CLEAR BUTTON_REC

#define CALCULATOR_RC_QUIT BUTTON_RC_STOP

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)

#define CALCULATOR_LEFT BUTTON_LEFT
#define CALCULATOR_RIGHT        BUTTON_RIGHT
#define CALCULATOR_UP_W_SHIFT   BUTTON_SCROLL_BACK
#define CALCULATOR_DOWN_W_SHIFT BUTTON_SCROLL_FWD
#define CALCULATOR_QUIT         BUTTON_MENU
#define CALCULATOR_INPUT        BUTTON_SELECT
#define CALCULATOR_CALC         BUTTON_PLAY

#elif (CONFIG_KEYPAD == IAUDIO_X5M5_PAD)

#define CALCULATOR_LEFT  BUTTON_LEFT
#define CALCULATOR_RIGHT BUTTON_RIGHT
#define CALCULATOR_UP    BUTTON_UP
#define CALCULATOR_DOWN  BUTTON_DOWN
#define CALCULATOR_QUIT  BUTTON_POWER
#define CALCULATOR_INPUT BUTTON_SELECT
#define CALCULATOR_CALC  BUTTON_PLAY
#define CALCULATOR_CLEAR BUTTON_REC

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)

#define CALCULATOR_LEFT BUTTON_LEFT
#define CALCULATOR_RIGHT BUTTON_RIGHT
#define CALCULATOR_UP   BUTTON_UP
#define CALCULATOR_DOWN BUTTON_DOWN
#define CALCULATOR_QUIT BUTTON_POWER
#define CALCULATOR_INPUT BUTTON_SELECT
#define CALCULATOR_CALC BUTTON_MENU
#define CALCULATOR_CLEAR BUTTON_A

#elif (CONFIG_KEYPAD == SANSA_E200_PAD) || \
(CONFIG_KEYPAD == SANSA_C200_PAD)
#define CALCULATOR_LEFT      BUTTON_LEFT
#define CALCULATOR_RIGHT     BUTTON_RIGHT
#define CALCULATOR_UP        BUTTON_UP
#define CALCULATOR_DOWN      BUTTON_DOWN
#if CONFIG_KEYPAD == SANSA_E200_PAD
/* c200 does not have a scroll wheel */
#define CALCULATOR_UP_W_SHIFT   BUTTON_SCROLL_BACK
#define CALCULATOR_DOWN_W_SHIFT BUTTON_SCROLL_FWD
#endif
#define CALCULATOR_QUIT      BUTTON_POWER
#define CALCULATOR_INPUT_CALC_PRE BUTTON_SELECT
#define CALCULATOR_INPUT     (BUTTON_SELECT|BUTTON_REL)
#define CALCULATOR_CALC      (BUTTON_SELECT|BUTTON_REPEAT)
#define CALCULATOR_CLEAR     BUTTON_REC

#elif (CONFIG_KEYPAD == SANSA_CLIP_PAD)
#define CALCULATOR_LEFT      BUTTON_LEFT
#define CALCULATOR_RIGHT     BUTTON_RIGHT
#define CALCULATOR_UP        BUTTON_UP
#define CALCULATOR_DOWN      BUTTON_DOWN
#define CALCULATOR_QUIT      BUTTON_POWER
#define CALCULATOR_INPUT_CALC_PRE BUTTON_SELECT
#define CALCULATOR_INPUT     (BUTTON_SELECT|BUTTON_REL)
#define CALCULATOR_CALC      (BUTTON_SELECT|BUTTON_REPEAT)
#define CALCULATOR_CLEAR     BUTTON_HOME

#elif (CONFIG_KEYPAD == SANSA_M200_PAD)
#define CALCULATOR_LEFT      BUTTON_LEFT
#define CALCULATOR_RIGHT     BUTTON_RIGHT
#define CALCULATOR_UP        BUTTON_UP
#define CALCULATOR_DOWN      BUTTON_DOWN
#define CALCULATOR_QUIT      BUTTON_POWER
#define CALCULATOR_INPUT_CALC_PRE BUTTON_SELECT
#define CALCULATOR_INPUT     (BUTTON_SELECT|BUTTON_REL)
#define CALCULATOR_CALC      (BUTTON_SELECT|BUTTON_REPEAT)
#define CALCULATOR_CLEAR     (BUTTON_SELECT|BUTTON_UP)

#elif (CONFIG_KEYPAD == IRIVER_H10_PAD)

#define CALCULATOR_LEFT BUTTON_LEFT
#define CALCULATOR_RIGHT          BUTTON_RIGHT
#define CALCULATOR_UP             BUTTON_SCROLL_UP
#define CALCULATOR_DOWN           BUTTON_SCROLL_DOWN
#define CALCULATOR_QUIT           BUTTON_POWER
#define CALCULATOR_INPUT_CALC_PRE BUTTON_PLAY
#define CALCULATOR_INPUT          (BUTTON_PLAY | BUTTON_REL)
#define CALCULATOR_CALC           (BUTTON_PLAY | BUTTON_REPEAT)
#define CALCULATOR_CLEAR          BUTTON_REW

#elif (CONFIG_KEYPAD == GIGABEAT_S_PAD)

#define CALCULATOR_LEFT  BUTTON_LEFT
#define CALCULATOR_RIGHT BUTTON_RIGHT
#define CALCULATOR_UP    BUTTON_UP
#define CALCULATOR_DOWN  BUTTON_DOWN
#define CALCULATOR_QUIT  BUTTON_BACK
#define CALCULATOR_INPUT BUTTON_SELECT
#define CALCULATOR_CALC  BUTTON_MENU
#define CALCULATOR_CLEAR BUTTON_PLAY

#elif (CONFIG_KEYPAD == MROBE100_PAD)

#define CALCULATOR_LEFT BUTTON_LEFT
#define CALCULATOR_RIGHT BUTTON_RIGHT
#define CALCULATOR_UP   BUTTON_UP
#define CALCULATOR_DOWN BUTTON_DOWN
#define CALCULATOR_QUIT BUTTON_POWER
#define CALCULATOR_INPUT BUTTON_SELECT
#define CALCULATOR_CALC BUTTON_MENU
#define CALCULATOR_CLEAR BUTTON_DISPLAY

#elif CONFIG_KEYPAD == IAUDIO_M3_PAD

#define CALCULATOR_LEFT BUTTON_RC_REW
#define CALCULATOR_RIGHT BUTTON_RC_FF
#define CALCULATOR_UP   BUTTON_RC_VOL_UP
#define CALCULATOR_DOWN BUTTON_RC_VOL_DOWN
#define CALCULATOR_QUIT BUTTON_RC_REC
#define CALCULATOR_INPUT BUTTON_RC_PLAY
#define CALCULATOR_CALC BUTTON_RC_MODE
#define CALCULATOR_CLEAR BUTTON_RC_MENU

#define CALCULATOR_RC_QUIT BUTTON_REC

#elif (CONFIG_KEYPAD == COWOND2_PAD)

#define CALCULATOR_QUIT           BUTTON_POWER
#define CALCULATOR_CLEAR          BUTTON_MENU

#elif CONFIG_KEYPAD == IAUDIO67_PAD

#define CALCULATOR_LEFT BUTTON_LEFT
#define CALCULATOR_RIGHT BUTTON_RIGHT
#define CALCULATOR_UP   BUTTON_VOLUP
#define CALCULATOR_DOWN BUTTON_VOLDOWN
#define CALCULATOR_QUIT BUTTON_POWER
#define CALCULATOR_INPUT BUTTON_PLAY
#define CALCULATOR_CALC BUTTON_MENU
#define CALCULATOR_CLEAR BUTTON_STOP

#define CALCULATOR_RC_QUIT (BUTTON_MENU|BUTTON_PLAY)

#elif (CONFIG_KEYPAD == CREATIVEZVM_PAD)

#define CALCULATOR_LEFT  BUTTON_LEFT
#define CALCULATOR_RIGHT BUTTON_RIGHT
#define CALCULATOR_UP    BUTTON_UP
#define CALCULATOR_DOWN  BUTTON_DOWN
#define CALCULATOR_QUIT  BUTTON_BACK
#define CALCULATOR_INPUT BUTTON_SELECT
#define CALCULATOR_CALC  BUTTON_MENU
#define CALCULATOR_CLEAR BUTTON_PLAY

#else
#error No keymap defined!
#endif

#ifdef HAVE_TOUCHSCREEN
#ifndef CALCULATOR_LEFT
#define CALCULATOR_LEFT           BUTTON_MIDLEFT
#endif
#ifndef CALCULATOR_RIGHT
#define CALCULATOR_RIGHT          BUTTON_MIDRIGHT
#endif
#ifndef CALCULATOR_UP
#define CALCULATOR_UP             BUTTON_TOPMIDDLE
#endif
#ifndef CALCULATOR_DOWN
#define CALCULATOR_DOWN           BUTTON_BOTTOMMIDDLE
#endif
#ifndef CALCULATOR_CALC
#define CALCULATOR_CALC           BUTTON_BOTTOMRIGHT
#endif
#ifndef CALCULATOR_INPUT
#define CALCULATOR_INPUT          BUTTON_CENTER
#endif
#ifndef CALCULATOR_CLEAR
#define CALCULATOR_CLEAR          BUTTON_TOPRIGHT
#endif

#include "lib/touchscreen.h"
static struct ts_raster calc_raster = { X_0_POS, Y_1_POS, BUTTON_COLS*REC_WIDTH, BUTTON_ROWS*REC_HEIGHT, REC_WIDTH, REC_HEIGHT };
#endif

static const struct plugin_api* rb;
MEM_FUNCTION_WRAPPERS(rb);

enum {
    basicButtons,
    sciButtons
} buttonGroup;

unsigned char* buttonChar[2][5][5] = {
    { { "MR" , "M+" , "2nd" , "CE"   , "C"   },
      { "7"  , "8"  , "9"   , "/"    , "sqr" },
      { "4"  , "5"  , "6"   , "*"    , "x^2" },
      { "1"  , "2"  , "3"   , "-"    , "1/x" },
      { "0"  , "+/-", "."   , "+"    , "="   }  },
    
    { { "n!" , "PI" , "1st" , "sin"  , "asi" },
      { "7"  , "8"  , "9"   , "cos"  , "aco" },
      { "4"  , "5"  , "6"   , "tan"  , "ata" },
      { "1"  , "2"  , "3"   , "ln"   , "e^x" },
      { "0"  , "+/-", "."   , "log"  , "x^y" }  }
};

enum { btn_MR , btn_M    , btn_bas , btn_CE    , btn_C      ,
       btn_7  , btn_8    , btn_9   , btn_div   , btn_sqr    ,
       btn_4  , btn_5    , btn_6   , btn_time  , btn_square ,
       btn_1  , btn_2    , btn_3   , btn_minus , btn_rec    ,
       btn_0  , btn_sign , btn_dot , btn_add   , btn_equal
     };

enum { sci_fac, sci_pi   , sci_sci , sci_sin   , sci_asin   ,
       sci_7  , sci_8    , sci_9   , sci_cos   , sci_acos    ,
       sci_4  , sci_5    , sci_6   , sci_tan   , sci_atan    ,
       sci_1  , sci_2    , sci_3   , sci_ln    , sci_exp    ,
       sci_0  , sci_sign , sci_dot , sci_log   , sci_xy
     };

#define MINIMUM 0.000000000001   /* e-12 */
              /*  ^   ^    ^    ^       */
              /*  123456789abcdef       */

#define DIGITLEN 10  /* must <= 10 */
#define SCIENTIFIC_FORMAT ( power < -(DIGITLEN-3) || power > (DIGITLEN))
           /*          0.000 00000 0001         */
           /*          ^   ^ ^   ^ ^   ^        */
           /* DIGITLEN 12345 6789a bcdef        */
           /* power       12 34567 89abc def    */
           /* 10^-       123 45678 9abcd ef     */

unsigned char buf[19];/* 18 bytes of output line,
                         buf[0] is operator
                         buf[1] = 'M' if memTemp is not 0
                         buf[2] = ' '

                         if SCIENTIFIC_FORMAT
                             buf[2]-buf[12] or buf[3]-buf[13] = result;
                             format X.XXXXXXXX
                             buf[13] or buf[14] -buf[17] = power;
                             format eXXX or e-XXX
                         else
                             buf[3]-buf[6] = ' ';
                             buf[7]-buf[17] = result;

                         buf[18] = '\0'                    */

unsigned char typingbuf[DIGITLEN+2];/* byte 0 is sign or ' ',
                                       byte 1~DIGITLEN are num and '.'
                                       byte (DIGITLEN+1) is '\0' */
unsigned char* typingbufPointer = typingbuf;

double result = 0;          /*  main operand, format 0.xxxxx     */
int power = 0;              /*  10^power                         */
double modifier = 0.1;      /*  position of next input           */
double operand = 0;         /*  second operand, format 0.xxxxx   */
int operandPower = 0;       /*  10^power of second operand       */
char oper = ' ';            /*  operators: + - * /               */
bool operInputted = false;  /*  false: do calculation first and
                                       replace current oper
                                true:  just replace current oper */

double memTemp = 0;         /* temp memory                       */
int memTempPower = 0;       /* 10^^power of memTemp              */

int btn_row, btn_col;       /* current position index for button */
int prev_btn_row, prev_btn_col; /* previous cursor position      */
#define CAL_BUTTON (btn_row*5+btn_col)

int btn = BUTTON_NONE;
int lastbtn = BUTTON_NONE;

/* Status of calculator */
enum {cal_normal,  /* 0, normal status, display result */
      cal_typing,  /* 1, currently typing, dot hasn't been typed */
      cal_dotted,  /* 2, currently typing, dot already has been typed. */
      cal_error,
      cal_exit,
      cal_toDo
} calStatus;

/* constant table for CORDIC algorithm */
double cordicTable[51][2]= {
 /* pow(2,0) - pow(2,-50)             atan(pow(2,0) - atan(pow(2,-50) */
    {1e+00,                                    7.853981633974483e-01},
    {5e-01,                                    4.636476090008061e-01},
    {2.5e-01,                                  2.449786631268641e-01},
    {1.25e-01,                                 1.243549945467614e-01},
    {6.25e-02,                                 6.241880999595735e-02},
    {3.125e-02,                                3.123983343026828e-02},
    {1.5625e-02,                               1.562372862047683e-02},
    {7.8125e-03,                               7.812341060101111e-03},
    {3.90625e-03,                              3.906230131966972e-03},
    {1.953125e-03,                             1.953122516478819e-03},
    {9.765625e-04,                             9.765621895593195e-04},
    {4.8828125e-04,                            4.882812111948983e-04},
    {2.44140625e-04,                           2.441406201493618e-04},
    {1.220703125e-04,                          1.220703118936702e-04},
    {6.103515625e-05,                          6.103515617420877e-05},
    {3.0517578125e-05,                         3.051757811552610e-05},
    {1.52587890625e-05,                        1.525878906131576e-05},
    {7.62939453125e-06,                        7.629394531101970e-06},
    {3.814697265625e-06,                       3.814697265606496e-06},
    {1.9073486328125e-06,                      1.907348632810187e-06},
    {9.5367431640625e-07,                      9.536743164059608e-07},
    {4.76837158203125e-07,                     4.768371582030888e-07},
    {2.384185791015625e-07,                    2.384185791015580e-07},
    {1.1920928955078125e-07,                   1.192092895507807e-07},
    {5.9604644775390625e-08,                   5.960464477539055e-08},
    {2.98023223876953125e-08,                  2.980232238769530e-08},
    {1.490116119384765625e-08,                 1.490116119384765e-08},
    {7.450580596923828125e-09,                 7.450580596923828e-09},
    {3.7252902984619140625e-09,                3.725290298461914e-09},
    {1.86264514923095703125e-09,               1.862645149230957e-09},
    {9.31322574615478515625e-10,               9.313225746154785e-10},
    {4.656612873077392578125e-10,              4.656612873077393e-10},
    {2.3283064365386962890625e-10,             2.328306436538696e-10},
    {1.16415321826934814453125e-10,            1.164153218269348e-10},
    {5.82076609134674072265625e-11,            5.820766091346741e-11},
    {2.910383045673370361328125e-11,           2.910383045673370e-11},
    {1.4551915228366851806640625e-11,          1.455191522836685e-11},
    {7.2759576141834259033203125e-12,          7.275957614183426e-12},
    {3.63797880709171295166015625e-12,         3.637978807091713e-12},
    {1.818989403545856475830078125e-12,        1.818989403545856e-12},
    {9.094947017729282379150390625e-13,        9.094947017729282e-13},
    {4.5474735088646411895751953125e-13,       4.547473508864641e-13},
    {2.27373675443232059478759765625e-13,      2.273736754432321e-13},
    {1.136868377216160297393798828125e-13,     1.136868377216160e-13},
    {5.684341886080801486968994140625e-14,     5.684341886080801e-14},
    {2.8421709430404007434844970703125e-14,    2.842170943040401e-14},
    {1.42108547152020037174224853515625e-14,   1.421085471520200e-14},
    {7.10542735760100185871124267578125e-15,   7.105427357601002e-15},
    {3.552713678800500929355621337890625e-15,  3.552713678800501e-15},
    {1.7763568394002504646778106689453125e-15, 1.776356839400250e-15},
    {8.8817841970012523233890533447265625e-16, 8.881784197001252e-16}
};

void doMultiple(double* operandOne, int* powerOne,
                double  operandTwo, int  powerTwo);
void doAdd (double* operandOne, int* powerOne,
            double  operandTwo, int  powerTwo);
void printResult(void);
void formatResult(void);
void oneOperand(void);

void drawLines(void);
void drawButtons(int group);

/* -----------------------------------------------------------------------
Handy funtions
----------------------------------------------------------------------- */
void cleartypingbuf(void)
{
    int k;
    for( k=1; k<=(DIGITLEN+1); k++)
        typingbuf[k] = 0;
    typingbuf[0] = ' ';
    typingbufPointer = typingbuf+1;
}
void clearbuf(void)
{
    int k;
    for(k=0;k<18;k++)
        buf[k]=' ';
    buf[18] = 0;
}
void clearResult(void)
{
    result = 0;
    power = 0;
    modifier = 0.1;
}

void clearInput(void)
{
    calStatus = cal_normal;
    clearResult();
    cleartypingbuf();
    rb->lcd_clear_display();
    drawButtons(buttonGroup);
    drawLines();
}

void clearOperand(void)
{
    operand = 0;
    operandPower = 0;
}

void clearMemTemp(void)
{
    memTemp = 0;
    memTempPower = 0;
}

void clearOper(void)
{
    oper = ' ';
    operInputted = false;
}

void clearMem(void)
{
    clearInput();
    clearMemTemp();
    clearOperand();
    clearOper();
    btn = BUTTON_NONE;
}

void switchOperands(void)
{
    double tempr = operand;
    int tempp = operandPower;
    operand = result;
    operandPower = power;
    result = tempr;
    power = tempp;
}

void drawLines(void)
{
    int i;
    rb->lcd_hline(0, LCD_WIDTH, Y_1_POS-1);
    for (i = 0; i < 5 ; i++)
        rb->lcd_hline(0, LCD_WIDTH, Y_1_POS+i*REC_HEIGHT);
    for (i = 0; i < 4 ; i++)
        rb->lcd_vline(X_1_POS+i*REC_WIDTH, Y_1_POS, LCD_HEIGHT);
}

void drawButtons(int group)
{
    int i, j, w, h;
    for (i = 0; i <= 4; i++){
        for (j = 0; j <= 4; j++){
            rb->lcd_getstringsize( buttonChar[group][i][j],&w,&h);
            if (i == btn_row && j == btn_col) /* selected item */
                rb->lcd_set_drawmode(DRMODE_SOLID);
            else
                rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
            rb->lcd_fillrect( X_0_POS + j*REC_WIDTH,
                              Y_1_POS + i*REC_HEIGHT,
                              REC_WIDTH, REC_HEIGHT+1);
            if (i == btn_row && j == btn_col) /* selected item */
                rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
            else
                rb->lcd_set_drawmode(DRMODE_SOLID);
            rb->lcd_putsxy( X_0_POS + j*REC_WIDTH + (REC_WIDTH - w)/2,
                            Y_1_POS + i*REC_HEIGHT + (REC_HEIGHT - h)/2 + 1,
                            buttonChar[group][i][j] );
        }
    }
    rb->lcd_set_drawmode(DRMODE_SOLID);
}

/* -----------------------------------------------------------------------
Initiate calculator
----------------------------------------------------------------------- */
void cal_initial (void)
{
    int w,h;

    rb->lcd_getstringsize("A",&w,&h);
    if (h >= REC_HEIGHT)
        rb->lcd_setfont(FONT_SYSFIXED);

    rb->lcd_clear_display();

#ifdef CALCULATOR_OPERATORS
    /* basic operators are available through separate button */
    buttonGroup = sciButtons;
#else
    buttonGroup = basicButtons;
#endif

    /* initially, invert button "5" */
    btn_row = 2;
    btn_col = 1;
    prev_btn_row = btn_row;
    prev_btn_col = btn_col;
    drawButtons(buttonGroup);
    drawLines();
    rb->lcd_update();

    /* initial mem and output display*/
    clearMem();
    printResult();

    /* clear button queue */
    rb->button_clear_queue();
}

/* -----------------------------------------------------------------------
   mySqrt uses Heron's algorithm, which is the Newtone-Raphson algorhitm
   in it's private case for sqrt.
   Thanks BlueChip for his intro text and Dave Straayer for the actual name.
   ----------------------------------------------------------------------- */
double mySqrt(double square)
{
    int k = 0;
    double temp = 0;
    double root= ABS(square+1)/2;

    while( ABS(root - temp) > MINIMUM ){
        temp = root;
        root = (square/temp + temp)/2;
        k++;
        if (k>10000) return 0;
    }

    return root;
}
/* -----------------------------------------------------------------------
   transcendFunc uses CORDIC (COordinate Rotation DIgital Computer) method
   transcendFunc can do sin,cos,log,exp
   input parameter is angle
----------------------------------------------------------------------- */
void transcendFunc(char* func, double* tt, int* ttPower)
{
    double t = (*tt)*M_PI/180; int tPower = *ttPower;
    int sign = 1;
    int n = 50; /* n <=50, tables are all <= 50 */
    int j;
    double x,y,z,xt,yt,zt;

    if (tPower < -998) {
        calStatus = cal_normal;
        return;
    }
    if (tPower > 8) {
        calStatus = cal_error;
        return;
    }
    *ttPower = 0;
    calStatus = cal_normal;

    if( func[0] =='s' || func[0] =='S'|| func[0] =='t' || func[0] =='T')
        sign = SIGN(t);
    else {
        /* if( func[0] =='c' || func[0] =='C') */
        sign = 1;
    }
    t = ABS(t);

    while (tPower > 0){
        t *= 10;
        tPower--;
    }
    while (tPower < 0) {
        t /= 10;
        tPower++;
    }
    j = 0;
    while (t > j*M_TWOPI) {j++;}
    t -= (j-1)*M_TWOPI;
    if (M_PI_2 < t && t < 3*M_PI_2){
        t = M_PI - t;
        if (func[0] =='c' || func[0] =='C')
            sign = -1;
        else if (func[0] =='t' || func[0] =='T')
            t*=-1;
    }
    else if ( 3*M_PI_2 <= t && t <= M_TWOPI)
        t -= M_TWOPI;

    x = 0.60725293500888;  y = 0;  z = t;
    for (j=1;j<n+2;j++){
        xt = x - SIGN(z) * y*cordicTable[j-1][0];
        yt = y + SIGN(z) * x*cordicTable[j-1][0];
        zt = z - SIGN(z) * cordicTable[j-1][1];
        x = xt;
        y=yt;
        z=zt;
    }
    if( func[0] =='s' || func[0] =='S') {
        *tt = sign*y;
        return;
    }
    else if( func[0] =='c' || func[0] =='C') {
        *tt = sign*x;
        return;
    }
    else /*if( func[0] =='t' || func[0] =='T')*/ {
        if(t==M_PI_2||t==-M_PI_2){
            calStatus = cal_error;
            return;
        }
        else{
            *tt = sign*(y/x);
            return;
        }
    }

}
/* -----------------------------------------------------------------------
   add in scientific number format
----------------------------------------------------------------------- */
void doAdd (double* operandOne, int* powerOne,
            double  operandTwo, int  powerTwo)
{
    if ( *powerOne >= powerTwo ){
        if (*powerOne - powerTwo <= DIGITLEN+1){
            while (powerTwo < *powerOne){
                operandTwo /=10;
                powerTwo++;
            }
            *operandOne += operandTwo;
        }
        /*do nothing if operandTwo is too small*/
    }
    else{
        if (powerTwo - *powerOne <= DIGITLEN+1){
            while(powerTwo > *powerOne){
                *operandOne /=10;
                (*powerOne)++;
            }
            (*operandOne) += operandTwo;
        }
        else{/* simply copy operandTwo if operandOne is too small */
            *operandOne = operandTwo;
            *powerOne = powerTwo;
        }
    }
}
/* -----------------------------------------------------------------------
multiple in scientific number format
----------------------------------------------------------------------- */
void doMultiple(double* operandOne, int* powerOne,
                double  operandTwo, int  powerTwo)
{
    (*operandOne) *= operandTwo;
    (*powerOne) += powerTwo;
}

/* -----------------------------------------------------------------------
Handles all one operand calculations
----------------------------------------------------------------------- */
void oneOperand(void)
{
    int k = 0;
    if (buttonGroup == basicButtons){
        switch(CAL_BUTTON){
            case btn_sqr:
                if (result<0)
                    calStatus = cal_error;
                else{
                    if (power%2 == 1){
                        result = (mySqrt(result*10))/10;
                        power = (power+1) / 2;
                    }
                    else{
                        result = mySqrt(result);
                        power = power / 2;
                    }
                    calStatus = cal_normal;
                }
                break;
            case btn_square:
                power *= 2;
                result *= result;
                calStatus = cal_normal;
                break;

            case btn_rec:
                if (result==0)
                    calStatus = cal_error;
                else{
                    power = -power;
                    result = 1/result;
                    calStatus = cal_normal;
                }
                break;
            default:
                calStatus = cal_toDo;
                break; /* just for the safety */
        }
    }
    else{ /* sciButtons */
        switch(CAL_BUTTON){
            case sci_sin:
                transcendFunc("sin", &result, &power);
                break;
            case sci_cos:
                transcendFunc("cos", &result, &power);
                break;
            case sci_tan:
                transcendFunc("tan", &result, &power);
                break;
            case sci_fac:
                if (power<0 || power>8 || result<0 )
                    calStatus = cal_error;
                else if(result == 0) {
                    result = 1;
                    power = 0;
                }
                else{
                    while(power > 0) {
                        result *= 10;
                        power--;
                    }
                    if ( ( result - (int)result) > MINIMUM )
                        calStatus = cal_error;
                    else {
                        k = result; result = 1;
                        while (k > 1){
                            doMultiple(&result, &power, k, 0);
                            formatResult();
                            k--;
                        }
                        calStatus = cal_normal;
                    }
                }
                break;
            default:
                calStatus = cal_toDo;
                break; /* just for the safety */
        }
    }
}


/* -----------------------------------------------------------------------
Handles all two operands calculations
----------------------------------------------------------------------- */
void twoOperands(void)
{
    switch(oper){
        case '-':
            doAdd(&operand, &operandPower, -result, power);
            break;
        case '+':
            doAdd(&operand, &operandPower, result, power);
            break;
        case '*':
            doMultiple(&operand, &operandPower, result, power);
            break;
        case '/':
            if ( ABS(result) > MINIMUM ){
                doMultiple(&operand, &operandPower, 1/result, -power);
            }
            else
                calStatus = cal_error;
            break;
        default: /* ' ' */
            switchOperands(); /* counter switchOperands() below */
            break;
    } /* switch(oper) */
    switchOperands();
    clearOper();
}

/* First, increases *dimen1 by dimen1_delta modulo dimen1_modulo.
   If dimen1 wraps, increases *dimen2 by dimen2_delta modulo dimen2_modulo.
*/
static void move_with_wrap_and_shift(
    int *dimen1, int dimen1_delta, int dimen1_modulo,
    int *dimen2, int dimen2_delta, int dimen2_modulo)
{
    bool wrapped = false;
    
    *dimen1 += dimen1_delta;
    if (*dimen1 < 0)
    {
        *dimen1 = dimen1_modulo - 1;
        wrapped = true;
    }
    else if (*dimen1 >= dimen1_modulo)
    {
        *dimen1 = 0;
        wrapped = true;
    }
    
    if (wrapped)
    {
        /* Make the dividend always positive to be sure about the result.
           Adding dimen2_modulo does not change it since we do it modulo. */
        *dimen2 = (*dimen2 + dimen2_modulo + dimen2_delta) % dimen2_modulo;
    }
}

/* -----------------------------------------------------------------------
Print buttons when switching 1st and 2nd
int group = {basicButtons, sciButtons}
----------------------------------------------------------------------- */
void printButtonGroups(int group)
{
    drawButtons(group);
    drawLines();
    rb->lcd_update();
}
/* -----------------------------------------------------------------------
flash the currently marked button
----------------------------------------------------------------------- */
void flashButton(void)
{
    int k, w, h;
    for (k=2;k>0;k--)
    {
        rb->lcd_getstringsize( buttonChar[buttonGroup][btn_row][btn_col],&w,&h);
        rb->lcd_set_drawmode(DRMODE_SOLID|(k==1) ? 0 : DRMODE_INVERSEVID);
        rb->lcd_fillrect( X_0_POS + btn_col*REC_WIDTH + 1,
                          Y_1_POS + btn_row*REC_HEIGHT + 1,
                          REC_WIDTH - 1, REC_HEIGHT - 1);
        rb->lcd_putsxy( X_0_POS + btn_col*REC_WIDTH + (REC_WIDTH - w)/2,
                        Y_1_POS + btn_row*REC_HEIGHT + (REC_HEIGHT - h)/2 +1,
                        buttonChar[buttonGroup][btn_row][btn_col] );
        rb->lcd_update_rect( X_0_POS + btn_col*REC_WIDTH + 1,
                            Y_1_POS + btn_row*REC_HEIGHT + 1,
                            REC_WIDTH - 1, REC_HEIGHT - 1);

        if (k!= 1)
            rb->sleep(HZ/22);

    }
}

/* -----------------------------------------------------------------------
pos is the position that needs animation. pos = [1~18]
----------------------------------------------------------------------- */
void deleteAnimation(int pos)
{
    int k;
    if (pos<1 || pos >18)
        return;
    pos--;
    rb->lcd_fillrect(1+pos*6, TEXT_1_POS, 6, 8);
    rb->lcd_update_rect(1+pos*6, TEXT_1_POS, 6, 8);

    for (k=1;k<=4;k++){
        rb->sleep(HZ/32);
        rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        rb->lcd_fillrect(1+pos*6, TEXT_1_POS, 6, 8);
        rb->lcd_set_drawmode(DRMODE_SOLID);
        rb->lcd_fillrect(1+pos*6+1+k, TEXT_1_POS+k,
                         (5-2*k)>0?(5-2*k):1, (7-2*k)>0?(7-2*k):1 );
        rb->lcd_update_rect(1+pos*6, TEXT_1_POS, 6, 8);
    }

}

/* -----------------------------------------------------------------------
result may be one of these formats:
0
xxxx.xxxx
0.xxxx
0.0000xxxx

formatResult() change result to standard format: 0.xxxx
if result is close to 0, let it be 0;
if result is close to 1, let it be 0.1 and power++;
----------------------------------------------------------------------- */
void formatResult(void)
{
    int resultsign = SIGN(result);
    result = ABS(result);
    if(result > MINIMUM ){ /* doesn't check power, might have problem
                              input wouldn't,
                              + - * / of two formatted number wouldn't.
                              only a calculation that makes a formatted
                              number (0.xxxx) less than MINIMUM in only
                              one operation  */

        if (result<1){
            while( (int)(result*10) == 0 ){
                result *= 10;
                power--;
                modifier *= 10;
            }
        }
        else{ /* result >= 1 */
            while( (int)result != 0 ){
                result /= 10;
                power++;
                modifier /= 10;
            }
        } /* if result<1 */

        if (result > (1-MINIMUM)){
            result = 0.1;
            power++;
            modifier /= 10;
        }
        result *= resultsign;
    }
    else {
        result = 0;
        power = 0;
        modifier = 0.1;
    }
}

/* -----------------------------------------------------------------------
result2typingbuf() outputs standard format result to typingbuf.
case SCIENTIFIC_FORMAT, let temppower = 1;
case temppower >  0:  print '.' in the middle
case temppower <= 0:  print '.' in the begining
----------------------------------------------------------------------- */
void result2typingbuf(void)
{
    bool haveDot = false;
    char tempchar = 0;
    int k;
    double tempresult = ABS(result); /* positive num makes things simple */

    int temppower;
    double tempmodifier = 1;
    int count;

    if(SCIENTIFIC_FORMAT)
        temppower = 1; /* output x.xxxx format */
    else
        temppower = power;

    cleartypingbuf();

    if(tempresult < MINIMUM){ /* if 0,faster display and avoid complication*/
        typingbuf[0] = ' ';
        typingbuf[1] = '0';
    }
    else{ /* tempresult > 0 */
        typingbuf[0] = (SIGN(result)<0)?'-':' ';

        typingbufPointer = typingbuf;
        if(temppower > 0){
            for (k = 0; k<DIGITLEN+1 ; k++){
                typingbufPointer++;
                if(temppower || *(typingbufPointer-1) == '.'){
                    count = 0;
                    tempmodifier = tempmodifier/10;
                    while( (tempresult-tempmodifier*count) >
                           (tempmodifier-MINIMUM)){
                        count++;
                    }
                    tempresult -= tempmodifier*count;
                    tempresult = ABS(tempresult);
                    temppower-- ;
                    *typingbufPointer = count + '0';
                }
                else{ /* temppower == 0 */
                    *typingbufPointer = '.';
                    haveDot = true;
                }
            } /* for */
        }
        else{
            haveDot = true;
            typingbufPointer++;  *typingbufPointer = '0';
            typingbufPointer++;  *typingbufPointer = '.';
            for (k = 2; k<DIGITLEN+1 ; k++){
                typingbufPointer++;
                count = 0;
                if ( (-temppower) < (k-1)){
                    tempmodifier = tempmodifier/10;
                    while((tempresult-tempmodifier*count)>(tempmodifier-MINIMUM)){
                        count++;

                    }
                    tempresult -= tempmodifier*count;
                    tempresult = ABS(tempresult);
                    temppower-- ;
                }
                *typingbufPointer = count + '0';
            }
        }
        /* now, typingbufPointer = typingbuf + 16 */
        /* backward strip off 0 and '.' */
        if (haveDot){
            while( (*typingbufPointer == '0') || (*typingbufPointer == '.')){
                tempchar = *typingbufPointer;
                *typingbufPointer = 0;
                typingbufPointer--;
                if (tempchar == '.') break;
            }
        }
        typingbuf[DIGITLEN+1] = 0;
    } /* else tempresult > 0 */
}

/* -----------------------------------------------------------------------
printResult() generates LCD display.
----------------------------------------------------------------------- */
void printResult(void)
{
    int k, w, h;

    char operbuf[3] = {0, 0, 0};

    switch_Status:
    switch(calStatus){
        case cal_exit:
            rb->lcd_clear_display();
            rb->splash(HZ/3, "Bye now!");
            break;
        case cal_error:
            clearbuf();
            rb->snprintf(buf, 19, "%18s","Error");
            break;
        case cal_toDo:
            clearbuf();
            rb->snprintf(buf, 19, "%18s","Coming soon ^_* ");
            break;

        case cal_normal:
            formatResult();

            if( power > 1000 ){  /* power -1 > 999  */
                calStatus = cal_error;
                goto switch_Status;
            }
            if (power < -998 )   /* power -1 < -999 */
                clearResult();   /* too small, let it be 0 */

            result2typingbuf();
            clearbuf();

            operbuf[0] = oper;
            operbuf[1] = ( ABS(memTemp) > MINIMUM )?'M':' ';
            operbuf[2] = '\0';

            if(SCIENTIFIC_FORMAT){
                /* output format: X.XXXX eXXX */
                if(power > -98){ /* power-1 >= -99, eXXX or e-XX */
                    rb->snprintf(buf, 12, "%11s",typingbuf);
                    for(k=11;k<=14;k++) buf[k] = ' ';
                    cleartypingbuf();
                    rb->snprintf(typingbuf, 5, "e%d",power-1);
                    rb->snprintf(buf+11, 5, "%4s",typingbuf);
                }
                else{  /* power-1 <= -100, e-XXX */
                    rb->snprintf(buf, 12, "%11s",typingbuf);
                    rb->snprintf(buf+11, 6, "e%d",power-1);
                }
            }
            else{
                rb->snprintf(buf, 12, "%11s",typingbuf);
            } /* if SCIENTIFIC_FORMAT */
            break;
        case cal_typing:
        case cal_dotted:
            clearbuf();
            operbuf[0] = oper;
            operbuf[1] = ( ABS(memTemp) > MINIMUM )?'M':' ';
            rb->snprintf(buf, 12, "%11s",typingbuf);
            break;

    }

    rb->lcd_getstringsize(buf, &w, &h);
    rb->screen_clear_area(rb->screens[0], 0, 0, LCD_WIDTH, REC_HEIGHT-1);
    rb->lcd_putsxy(4, Y_1_POS - h -1, operbuf);
    rb->lcd_putsxy(LCD_WIDTH - w - 4, Y_1_POS - h -1, buf);
    rb->lcd_update_rect(0, 1, LCD_WIDTH, Y_1_POS);
}

/* -----------------------------------------------------------------------
Process typing buttons: 1-9, '.', sign
main operand "result" and typingbuf are processed seperately here.
----------------------------------------------------------------------- */
void typingProcess(void){
    switch( CAL_BUTTON ){
        case btn_sign:
            if (calStatus == cal_typing ||
                calStatus == cal_dotted)
                typingbuf[0] = (typingbuf[0]=='-')?' ':'-';
            result = -result;
            break;
        case btn_dot:
            operInputted = false;
            switch(calStatus){
                case cal_normal:
                    clearInput();
                    *typingbufPointer = '0';
                    typingbufPointer++;
                case cal_typing:
                    calStatus = cal_dotted;
                    *typingbufPointer = '.';
                    if (typingbufPointer != typingbuf+DIGITLEN+1)
                        typingbufPointer++;
                    break;
                default:  /* cal_dotted */
                    break;
            }
            break;
        default:  /* 0-9 */
            operInputted = false;
            /* normal,0; normal,1-9; typing,0; typing,1-9 */
            switch(calStatus){
                case cal_normal:
                    if(CAL_BUTTON == btn_0 )
                        break;   /* first input is 0, ignore */
                    clearInput();
                    /*no operator means start a new calculation*/
                    if (oper ==' ')
                        clearOperand();
                    calStatus = cal_typing;
                    /* go on typing, no break */
                case cal_typing:
                case cal_dotted:
                    switch(CAL_BUTTON){
                        case btn_0:
                            *typingbufPointer = '0';
                            break;
                        default:
                            *typingbufPointer=(7+btn_col-3*(btn_row-1))+ '0';
                            break;
                    }
                    if (typingbufPointer!=typingbuf+DIGITLEN+1){
                        typingbufPointer++;

                        {/* result processing */
                         if (calStatus == cal_typing) power++;
                         if (CAL_BUTTON != btn_0)
                             result= result +
                                     SIGN(result)*
                                     (7+btn_col-3*(btn_row-1))*modifier;
                         modifier /= 10;
                        }
                    }
                    else /* last byte always '\0' */
                        *typingbufPointer = 0;
                    break;
                default: /* cal_error, cal_exit */
                    break;
            }
            break; /* default, 0-9 */
    } /* switch( CAL_BUTTON ) */
}

/* -----------------------------------------------------------------------
Handle delete operation
main operand "result" and typingbuf are processed seperately here.
----------------------------------------------------------------------- */
void doDelete(void){
    deleteAnimation(18);
    switch(calStatus){
        case cal_dotted:
            if (*(typingbufPointer-1) == '.'){
                /* if dotted and deleting '.',
                   change status and delete '.' below */
                calStatus = cal_typing;
            }
            else{ /* if dotted and not deleting '.',
                     power stays */
                power++; /* counter "power--;" below */
            }
        case cal_typing:
            typingbufPointer--;

            {/* result processing */   /* 0-9, '.' */
             /* if deleting '.', do nothing */
             if ( *typingbufPointer != '.'){
                power--;
                modifier *= 10;
                result = result - SIGN(result)*
                    ((*typingbufPointer)- '0')*modifier;
             }
            }

            *typingbufPointer = 0;

            /* if (only one digit left and it's 0)
               or  no digit left, change status*/
            if ( typingbufPointer == typingbuf+1 ||
                 ( typingbufPointer == typingbuf+2 &&
                   *(typingbufPointer-1) == '0' ))
                calStatus = cal_normal;
            break;
        default: /* normal, error, exit */
            break;
    }
}
/* -----------------------------------------------------------------------
Handle buttons on basic screen
----------------------------------------------------------------------- */
void basicButtonsProcess(void){
    switch (btn) {
        case CALCULATOR_INPUT:
            if (calStatus == cal_error && (CAL_BUTTON != btn_C) ) break;
            flashButton();
            switch( CAL_BUTTON ){
                case btn_MR:
                    operInputted = false;
                    result = memTemp; power = memTempPower;
                    calStatus = cal_normal;
                    break;
                case btn_M:
                    formatResult();
                    if (memTemp > MINIMUM)
                        doAdd(&memTemp, &memTempPower, result, power);
                    else {
                        /* if result is too small and memTemp = 0,
                           doAdd will not add */
                        memTemp = result;
                        memTempPower = power;
                    }
                    calStatus = cal_normal;
                    break;

                case btn_C:    clearMem();        break;
                case btn_CE:   clearInput();      break;

                case btn_bas:
                    buttonGroup = sciButtons;
                    printButtonGroups(buttonGroup);
                    break;

                /* one operand calculation, may be changed to
                   like sin, cos, log, etc */
                case btn_sqr:
                case btn_square:
                case btn_rec:
                    formatResult(); /* not necessary, just for safty */
                    oneOperand();
                    break;

                case_btn_equal:  /* F3 shortkey entrance */
                case btn_equal:
                    formatResult();
                    calStatus = cal_normal;
                    operInputted = false;
                    if (oper != ' ') twoOperands();
                    break;

                case btn_div:
                case btn_time:
                case btn_minus:
                case btn_add:
                    if(!operInputted) {twoOperands(); operInputted = true;}
                    oper = buttonChar[basicButtons][btn_row][btn_col][0];
#ifdef CALCULATOR_OPERATORS
                    case_cycle_operators:  /* F2 shortkey entrance */
#endif
                    calStatus = cal_normal;
                    formatResult();
                    operand = result;
                    operandPower = power;

                    break;

                case btn_sign:
                case btn_dot:
                default:  /* 0-9 */
                    typingProcess();
                    break;
            } /* switch (CAL_BUTTON) */
            break;

#ifdef CALCULATOR_OPERATORS
        case CALCULATOR_OPERATORS:
            if (calStatus == cal_error) break;
            if (!operInputted) {twoOperands(); operInputted = true;}
            switch (oper){
                case ' ':
                case '/':  oper = '+';  flashButton();    break;
                case '+':  oper = '-';  flashButton();  break;
                case '-':  oper = '*';  flashButton();   break;
                case '*':  oper = '/';  flashButton();    break;
            }
            goto case_cycle_operators;
            break;
#endif

        case CALCULATOR_CALC:
            if (calStatus == cal_error) break;
            flashButton();
            goto case_btn_equal;
            break;
        default: break;
    }
    printResult();
}

/* -----------------------------------------------------------------------
Handle buttons on scientific screen
----------------------------------------------------------------------- */
void sciButtonsProcess(void){
    switch (btn) {
        case CALCULATOR_INPUT:
            if (calStatus == cal_error && (CAL_BUTTON != sci_sci) ) break;
            flashButton();
            switch( CAL_BUTTON ){

                case sci_pi:
                    result = M_PI;  power = 0;
                    calStatus = cal_normal;
                    break;

                case sci_xy:  break;

                case sci_sci:
                    buttonGroup = basicButtons;
                    printButtonGroups(basicButtons);
                    break;

                case sci_fac:
                case sci_sin:
                case sci_asin:
                case sci_cos:
                case sci_acos:
                case sci_tan:
                case sci_atan:
                case sci_ln:
                case sci_exp:
                case sci_log:
                    formatResult(); /* not necessary, just for safty */
                    oneOperand();
                    break;

                case btn_sign:
                case btn_dot:
                default:  /* 0-9 */
                    typingProcess();
                    break;
            } /* switch (CAL_BUTTON) */
            break;

#ifdef CALCULATOR_OPERATORS
        case CALCULATOR_OPERATORS:
            if (calStatus == cal_error) break;
            if (!operInputted) {twoOperands(); operInputted = true;}
            switch (oper){
                case ' ':  oper = '+'; break;
                case '/':  oper = '+'; deleteAnimation(1);  break;
                case '+':  oper = '-'; deleteAnimation(1);  break;
                case '-':  oper = '*'; deleteAnimation(1);  break;
                case '*':  oper = '/'; deleteAnimation(1);  break;
            }
            calStatus = cal_normal;
            formatResult();
            operand = result;
            operandPower = power;
            break;
#endif

        case CALCULATOR_CALC:
            if (calStatus == cal_error) break;
            formatResult();
            calStatus = cal_normal;
            operInputted = false;
            if (oper != ' ') twoOperands();
            break;
        default: break;
    }
    printResult();
}

/* -----------------------------------------------------------------------
move button index
Invert display new button, invert back previous button
----------------------------------------------------------------------- */
int handleButton(int button){
    switch(button)
    {
        case CALCULATOR_INPUT:
        case CALCULATOR_CALC:
#ifdef CALCULATOR_INPUT_CALC_PRE
            if (lastbtn != CALCULATOR_INPUT_CALC_PRE)
                break;
            /* no unconditional break; here! */
#endif
#ifdef CALCULATOR_OPERATORS
        case CALCULATOR_OPERATORS:
#endif
            switch(buttonGroup){
                case basicButtons:
                    basicButtonsProcess();
                    break;
                case sciButtons:
                    sciButtonsProcess();
                    break;
            }
            break;

#ifdef CALCULATOR_CLEAR
        case CALCULATOR_CLEAR:
            switch(calStatus){
                case cal_typing:
                case cal_dotted:
                    doDelete();
                    break;
                default: /* cal_normal, cal_error, cal_exit */
                    clearMem();
                    break;
            }
            printResult();
            break;
#endif
        case CALCULATOR_LEFT:
        case CALCULATOR_LEFT | BUTTON_REPEAT:
            move_with_wrap_and_shift(
                &btn_col, -1, BUTTON_COLS,
                &btn_row,  0, BUTTON_ROWS);
            break;

        case CALCULATOR_RIGHT:
        case CALCULATOR_RIGHT | BUTTON_REPEAT:
            move_with_wrap_and_shift(
                &btn_col,  1, BUTTON_COLS,
                &btn_row,  0, BUTTON_ROWS);
            break;

#ifdef CALCULATOR_UP
        case CALCULATOR_UP:
        case CALCULATOR_UP | BUTTON_REPEAT:
            move_with_wrap_and_shift(
                &btn_row, -1, BUTTON_ROWS,
                &btn_col,  0, BUTTON_COLS);
            break;
#endif
#ifdef CALCULATOR_DOWN
        case CALCULATOR_DOWN:
        case CALCULATOR_DOWN | BUTTON_REPEAT:
            move_with_wrap_and_shift(
                &btn_row,  1, BUTTON_ROWS,
                &btn_col,  0, BUTTON_COLS);
            break;
#endif

#ifdef CALCULATOR_UP_W_SHIFT
        case CALCULATOR_UP_W_SHIFT:
        case CALCULATOR_UP_W_SHIFT | BUTTON_REPEAT:
            move_with_wrap_and_shift(
                &btn_row, -1, BUTTON_ROWS,
                &btn_col, -1, BUTTON_COLS);
            break;
#endif
#ifdef CALCULATOR_DOWN_W_SHIFT
        case CALCULATOR_DOWN_W_SHIFT:
        case CALCULATOR_DOWN_W_SHIFT | BUTTON_REPEAT:
            move_with_wrap_and_shift(
                &btn_row,  1, BUTTON_ROWS,
                &btn_col,  1, BUTTON_COLS);
            break;
#endif
#ifdef CALCULATOR_LEFT_W_SHIFT
        case CALCULATOR_LEFT_W_SHIFT:
        case CALCULATOR_LEFT_W_SHIFT | BUTTON_REPEAT:
            move_with_wrap_and_shift(
                &btn_col, -1, BUTTON_COLS,
                &btn_row, -1, BUTTON_ROWS);
            break;
#endif
#ifdef CALCULATOR_RIGHT_W_SHIFT
        case CALCULATOR_RIGHT_W_SHIFT:
        case CALCULATOR_RIGHT_W_SHIFT | BUTTON_REPEAT:
            move_with_wrap_and_shift(
                &btn_col,  1, BUTTON_COLS,
                &btn_row,  1, BUTTON_ROWS);
            break;
#endif
#ifdef CALCULATOR_RC_QUIT
        case CALCULATOR_RC_QUIT:
#endif
        case CALCULATOR_QUIT:
            return -1;
    }

    return 0;

    prev_btn_row = btn_row;
    prev_btn_col = btn_col;
}

/* -----------------------------------------------------------------------
Main();
----------------------------------------------------------------------- */
enum plugin_status plugin_start(const struct plugin_api* api, const void* parameter)
{
    (void)parameter;
    rb = api;

    /* now go ahead and have fun! */

    cal_initial();

    while (calStatus != cal_exit ) {
        btn = rb->button_get_w_tmo(HZ/2);
#ifdef HAVE_TOUCHSCREEN
        if(btn & BUTTON_TOUCHSCREEN)
        {
            struct ts_raster_result res;
            if(touchscreen_map_raster(&calc_raster, rb->button_get_data() >> 16,
                                      rb->button_get_data() & 0xffff, &res) == 1)
            {
                btn_row = res.y;
                btn_col = res.x;
                drawButtons(buttonGroup);
                drawLines();

                rb->lcd_update();

                prev_btn_row = btn_row;
                prev_btn_col = btn_col;
                if(btn & BUTTON_REL)
                {
                    btn = CALCULATOR_INPUT;
                    switch(buttonGroup){
                        case basicButtons:
                            basicButtonsProcess();
                            break;
                        case sciButtons:
                            sciButtonsProcess();
                            break;
                    }
                    btn = BUTTON_TOUCHSCREEN;
                }
            }
        }
#endif
        if (handleButton(btn) == -1)
        {
            calStatus = cal_exit;
            printResult();
        }
        else
        {
            drawButtons(buttonGroup);
            drawLines();
        }

        rb->lcd_update();

        if(rb->default_event_handler(btn) == SYS_USB_CONNECTED)
            return PLUGIN_USB_CONNECTED;

        if (btn != BUTTON_NONE)
            lastbtn = btn;
    } /* while (calStatus != cal_exit ) */

    rb->button_clear_queue();
    return PLUGIN_OK;
}

#endif /* #ifdef HAVE_LCD_BITMAP */
