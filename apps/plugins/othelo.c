/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2003 Blue Chip
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
/*
   Designed, Written, AI Bots, the lot ...BlueChip =8ªD#

   Thanks espcially to
      Hardeep, DevZer0, LinusN
         for their help with understanding Rockbox & the SDK
*/

#include "plugin.h"
#ifdef HAVE_LCD_BITMAP

/* the following #define had to be taken from Button.c
   'cos it is not defined in the header!! */
/* how long until repeat kicks in */
#define REPEAT_START      6

/* player types */
#define HUMAN     false
#define AIBOT     true

/* for domove() */
#define CHECK     false
#define MAKE      true

/* screen coords - top left x&y */
  /* game over */
#define go_tlx    71
#define go_tly    17
  /* WiNS */
#define win_tlx   63
#define win_tly   1
  /* DRaW */
#define draw_tlx  59
#define draw_tly  1
  /* scores */
#define sc_tlx    65
#define sc_tly    39
  /* logo */
#define logo_tlx  65
#define logo_tly  2

/* board sqaures -
 * there are a number of routines that expect these values asis
 * do not try to play with these, you will likely kill the program
 */
#define PLAYERX   0
#define PLAYERO   1
#define POSS      2
#define CHOICE    3
#define EMPTY     4
#define BORDER    5

/* Who gets first turn */
#define FIRST     PLAYERX
#define DF_PLX    HUMAN
#define DF_AIX    NONE
#define DF_PLO    AIBOT
#define DF_AIO    WEAK

/* Oponent  skill level                   / help level
 * -------- ---------------------------------------------------
 * NONE     no ai                         / no help
 * WEAK     random valid move             / show all possible
 * AVERAGE  most pieces (random)          / all + most pieces
 * SMART    most pieces (weighted/random) / all + weighted
 * EXPERT
 * GURU
 */
#define NONE    0
#define WEAK    1
#define AVERAGE 2
#define SMART   3
#define EXPERT  4
#define GURU    5
#define BEST    3 /* the best ai alogrithm currently available */

/* these are for code clarity, do not change them! */
#define LEFT    0x08
#define RIGHT   0x04
#define UP      0x02
#define DOWN    0x01

/* This represents the maximum number of possible moves
 * I have no idea what the real maximum is, buts tests
 * suggest about 10
 */
#define MAXPOSS 20

struct move
{
    int   x;
    int   y;
    int   taken;
    int   rank;
    bool  player;
};


/*===================================================================
 * Procedure prototypes
 *==================================================================*/
static void changeplayer(bool pl);

static int  othstrlen(char* s);
static void othprint(unsigned char x, unsigned char y, char ch, bool upd);
static void othprints(unsigned char x, unsigned char y, char* s, bool upd);

static void initscreen(void);
static void show_board(void);
static void flashboard(void);
static void show_grid(void);
static void show_score(bool turn);
static void show_players(void);
static void show_f3(bool playing);
static void hilite(struct move* move, bool on);
static void show_endgame(unsigned char scx, unsigned char sco);

static void initboard(void);
static int  getmove(struct move* move, struct move* plist,
                    unsigned char* pcnt, bool turn);
static int  checkmove(unsigned char x, unsigned char y, bool pl,
                      unsigned char dir, bool type);
static void domove(struct move* move, bool type);

static bool calcposs(struct move* plist, unsigned char* pcnt, bool turn);
static int  getplist(struct move* plist, unsigned char pl);
static unsigned char reduceplist(struct move* plist, unsigned char pcnt,
                                 unsigned char ai_help);
static void smartranking(struct move* plist, unsigned char pcnt);
static int  plist_bytaken(const void* m1, const void* m2);
static int  plist_byrank(const void* m1, const void* m2);
static void clearposs(void);


/*===================================================================
 * *static* local global variables
 *==================================================================*/

static struct plugin_api* rb;

/* score */
static  struct
        {
          int x;
          int o;
        }       score;

/* 8x8 with borders */
static  unsigned char     board[10][10];

/* player=HUMAN|AIBOT */
static  bool    player[2] = {DF_PLX, DF_PLO};

/* AI  =     WEAK|AVERAGE|SMART|EXPERT|GURU
   Help=NONE|WEAK|AVERAGE|SMART|EXPERT|GURU */
static  unsigned char     ai_help[2] = {DF_AIX, DF_AIO};

/* is a game under way */
static  bool    playing = false;

/* who's turn is it? */
static  bool    turn = FIRST;

/* Don't reorder this array - you have been warned! */
enum othfontc {
    of_plx,
    of_plo,
    of_poss,
    of_choice,
    of_sp,
    of_h,
    of_c,
    of_0,
    of_1,
    of_2,
    of_3,
    of_4,
    of_5,
    of_6,
    of_7,
    of_8,
    of_9,
    of_colon,
    of_dash,
    of_ptr,
    of_p,
    of_l,
    of_a,
    of_y,
    of_q,
    of_u,
    of_i,
    of_t,
    of_eos
};

static unsigned char othfont[of_eos][6] = {
    /*  +------+
     *  |  ##  |
     *  | #### |
     *  |######|
     *  |######|
     *  | #### |
     *  |  ##  |
     *  +------+
     */
    {0x0C, 0x1E, 0x3F, 0x3F, 0x1E, 0x0C},
    /*  +------+
     *  |  ##  |
     *  | #### |
     *  |##  ##|
     *  |##  ##|
     *  | #### |
     *  |  ##  |
     *  +------+
     */
    {0x0C, 0x1E, 0x33, 0x33, 0x1E, 0x0C},
    /*  +------+
     *  |      |
     *  |      |
     *  |  ##  |
     *  |  ##  |
     *  |      |
     *  |      |
     *  +------+
     */
    {0x00, 0x00, 0x0C, 0x0C, 0x00, 0x00},
    /*  +------+
     *  |      |
     *  | #  # |
     *  |  ##  |
     *  |  ##  |
     *  | #  # |
     *  |      |
     *  +------+
     */
    {0x00, 0x12, 0x0C, 0x0C, 0x12, 0x00},
    /*  +------+
     *  |      |
     *  |      |
     *  |      |
     *  |      |
     *  |      |
     *  |      |
     *  +------+
     */
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    /*  +------+
     *  | #  # | 0001 0010  12
     *  | #  # | 0001 0010  12
     *  | #### | 0001 1110  1E
     *  | #  # | 0001 0010  12
     *  | #  # | 0001 0010  12
     *  | #  # | 0001 0010  12
     *  +------+
     */
    {0x12,0x12,0x1E,0x12,0x12,0x12},
    /*  +------+
     *  |  ##  | 0000 1100  0C
     *  | #  # | 0001 0010  12
     *  |#     | 0010 0000  20
     *  |#     | 0010 0000  20
     *  | #  # | 0001 0010  12
     *  |  ##  | 0000 1100  0C
     *  +------+
     */
    {0x0C,0x12,0x20,0x20,0x12,0x0C},
    /*  +------+
     *  |  ##  | 0000 1100  0C
     *  | #  # | 0001 0010  12
     *  | # ## | 0001 0110  16
     *  | ## # | 0001 1010  1A
     *  | #  # | 0001 0010  12
     *  |  ##  | 0000 1100  0C
     *  +------+
     */
    {0x0C,0x12,0x16,0x1A,0x12,0x0C},
    /*  +------+
     *  |   #  | 0000 0100  04
     *  |  ##  | 0000 1100  0C
     *  |   #  | 0000 0100  04
     *  |   #  | 0000 0100  04
     *  |   #  | 0000 0100  04
     *  |  ### | 0000 1110  0E
     *  +------+
     */
    {0x04,0x0C,0x04,0x04,0x04,0x0E},
    /*  +------+
     *  |  ##  | 0000 1100  0C
     *  | #  # | 0001 0010  12
     *  |    # | 0000 0010  02
     *  |  ##  | 0000 1100  0C
     *  | #    | 0001 0000  10
     *  | #### | 0001 1110  1E
     *  +------+
     */
    {0x0C,0x12,0x02,0x0C,0x10,0x1E},
    /*  +------+
     *  | ###  | 0001 1100  1C
     *  |    # | 0000 0010  02
     *  |  ##  | 0000 1100  0C
     *  |    # | 0000 0010  02
     *  |    # | 0000 0010  02
     *  | ###  | 0001 1100  1C
     *  +------+
     */
    {0x1C,0x02,0x0C,0x02,0x02,0x1C},
    /*  +------+
     *  | #    | 0001 0000  10
     *  | #    | 0001 0000  10
     *  | # #  | 0001 0100  14
     *  | # #  | 0001 0100  14
     *  | #### | 0001 1110  1E
     *  |   #  | 0000 0100  04
     *  +------+
     */
    {0x10,0x10,0x14,0x14,0x1E,0x04},
    /*  +------+
     *  | #### | 0001 1110  1E
     *  | #    | 0001 0000  10
     *  | ###  | 0001 1100  1C
     *  |    # | 0000 0010  02
     *  | #  # | 0001 0010  12
     *  |  ##  | 0000 1100  0C
     *  +------+
     */
    {0x1E,0x10,0x1C,0x02,0x12,0x0C},
    /*  +------+
     *  |  ### | 0000 1110  0E
     *  | #    | 0001 0000  10
     *  | ###  | 0001 1100  1C
     *  | #  # | 0001 0010  12
     *  | #  # | 0001 0010  12
     *  |  ##  | 0000 1100  0C
     *  +------+
     */
    {0x0E,0x10,0x1C,0x12,0x12,0x0C},
    /*  +------+
     *  | #### | 0001 1110  1E
     *  |    # | 0000 0010  02
     *  |   #  | 0000 0100  04
     *  |   #  | 0000 0100  04
     *  |  #   | 0000 1000  08
     *  |  #   | 0000 1000  08
     *  +------+
     */
    {0x1E,0x02,0x04,0x04,0x08,0x08},
    /*  +------+
     *  |  ##  | 0000 1100  0C
     *  | #  # | 0001 0010  12
     *  |  ##  | 0000 1100  0C
     *  | #  # | 0001 0010  12
     *  | #  # | 0001 0010  12
     *  |  ##  | 0000 1100  0C
     *  +------+
     */
    {0x0C,0x12,0x0C,0x12,0x12,0x0C},
    /*  +------+
     *  |  ##  | 0000 1100  0C
     *  | #  # | 0001 0010  12
     *  | #  # | 0001 0010  12
     *  |  ### | 0000 1110  0E
     *  |    # | 0000 0010  02
     *  |  ##  | 0000 1100  0C
     *  +------+
     */
    {0x0C,0x12,0x12,0x0E,0x02,0x0C},
    /*  +------+
     *  |      | 0000 0000  00
     *  |  ##  | 0000 1100  0C
     *  |  ##  | 0000 1100  0C
     *  |      | 0000 0000  00
     *  |  ##  | 0000 1100  0C
     *  |  ##  | 0000 1100  0C
     *  +------+
     */
    {0x00,0x0C,0x0C,0x00,0x0C,0x0C},
    /*  +------+
     *  |      | 0000 0000  00
     *  |      | 0000 0000  00
     *  | #### | 0001 1110  1E
     *  | #### | 0001 1110  1E
     *  |      | 0000 0000  00
     *  |      | 0000 0000  00
     *  +------+
     */
    {0x00,0x00,0x1E,0x1E,0x00,0x00},
    /*  +------+
     *  |      | 0000 0000  00
     *  |   #  | 0000 0100  04
     *  |   ## | 0000 0110  06
     *  |######| 0011 1111  3F
     *  |   ## | 0000 0110  06
     *  |   #  | 0000 0100  04
     *  +------+
     */
    {0x00,0x04,0x06,0x3F,0x06,0x04},
    /*
     *  ÚÄÄÄÄÄÄ¿ ÚÄÄÄÄÄÄ¿ ÚÄÄÄÄÄÄ¿ ÚÄÄÄÄÄÄ¿
     *  ³.###..³.³#.....³.³.###..³.³#...#.³ xx01 1100 | xx10 0000 | xx01 1100 | xx10 0010 | 1C 20 1C 22
     *  ³#...#.³.³#.....³.³#...#.³.³#...#.³ xx10 0010 | xx10 0000 | xx10 0010 | xx10 0010 | 22 20 22 22
     *  ³#...#.³.³#.....³.³#...#.³.³.###..³ xx10 0010 | xx10 0000 | xx10 0010 | xx01 1100 | 22 20 22 1C
     *  ³####..³.³#.....³.³#####.³.³..#...³ xx11 1100 | xx10 0000 | xx11 1110 | xx00 1000 | 3C 20 3E 08
     *  ³#.....³.³#...#.³.³#...#.³.³..#...³ xx10 0000 | xx10 0010 | xx10 0010 | xx00 1000 | 20 22 22 08
     *  ³#.....³.³.###..³.³#...#.³.³..#...³ xx10 0000 | xx01 1100 | xx10 0010 | xx00 1000 | 20 1C 22 08
     *  ÀÄÄÄÄÄÄÙ ÀÄÄÄÄÄÄÙ ÀÄÄÄÄÄÄÙ ÀÄÄÄÄÄÄÙ
     */
    {0x1C,0x22,0x22,0x3C,0x20,0x20},
    {0x20,0x20,0x20,0x20,0x22,0x1C},
    {0x1C,0x22,0x22,0x3E,0x22,0x22},
    {0x22,0x22,0x1C,0x08,0x08,0x08},
    /*  ÚÄÄÄÄÄÄ¿ ÚÄÄÄÄÄÄ¿ ÚÄÄÄÄÄÄ¿ ÚÄÄÄÄÄÄ¿
     *  ³.###..³.³#...#.³.³#####.³.³#####.³ xx01 1100 | xx10 0010 | xx11 1110 | xx11 1110 | 1C 22 3E 3E
     *  ³#...#.³.³#...#.³.³..#...³.³..#...³ xx10 0010 | xx10 0010 | xx00 1000 | xx00 1000 | 22 22 08 08
     *  ³#...#.³.³#...#.³.³..#...³.³..#...³ xx10 0010 | xx10 0010 | xx00 1000 | xx00 1000 | 22 22 08 08
     *  ³#...#.³.³#...#.³.³..#...³.³..#...³ xx10 0010 | xx10 0010 | xx00 1000 | xx00 1000 | 22 22 08 08
     *  ³#..##.³.³#...#.³.³..#...³.³..#...³ xx10 0110 | xx10 0010 | xx00 1000 | xx00 1000 | 22 22 08 08
     *  ³.#####³.³.###..³.³#####.³.³..#...³ xx01 1111 | xx01 1100 | xx11 1110 | xx00 1000 | 1F 1C 3E 08
     *  ÀÄÄÄÄÄÄÙ ÀÄÄÄÄÄÄÙ ÀÄÄÄÄÄÄÙ ÀÄÄÄÄÄÄÙ
     */
    {0x1C,0x22,0x22,0x22,0x22,0x1F},
    {0x22,0x22,0x22,0x22,0x22,0x1C},
    {0x3E,0x08,0x08,0x08,0x08,0x3E},
    {0x3E,0x08,0x08,0x08,0x08,0x08}

};

/*

######### #   # ###   ##   ##   ###
####    # #   # #  # #  # #  # #
##      #  ###  ###  #  # #  # #
##          #   #  # #  # ###  #  ##
##          #   #  # #  # # #  #   #
#           #   ###   ##  #  #  ###
#
#     ### #   #  ### #####  ###  # #   ###
#    #    #   # #      #   #    # # # #
#     ##   ###   ##    #   ###  # # #  ##
#       #   #      #   #   #    # # #    #
#  #    #   #      #   #   #    #   #    #
#### ###    #   ###    #    ### #   # ###



  #####################################
  #####################################


      ##   ##### #  # #### #     ##
     ####    #   #  # #    #    ####
    ######   #   #### ###  #   ##  ##
    ######   #   #  # #    #   ##  ##
     ####    #   #  # #    #    ####
      ##     #   #  # #### ####  ##


  #####################################
  #####################################

                                                        X=42, Y=30
####|####|# # |  # |### |  ##|   #|#   |### |    |    | FF A2 E3 18 E0 00
####|    |# # |  # |#  #| #  |# # | # #|    |    |    | F0 A2 94 A5 00 00
##  |    |#  #|##  |### | #  |# # | # #|    |    |    | C0 9C E4 A5 00 00
##  |    |    |#   |#  #| #  |# ##|#  #|  ##|    |    | C0 08 94 B9 30 00
##  |    |    |#   |#  #| #  |# # |#  #|   #|    |    | C0 08 94 A9 10 00
#   |    |    |#   |### |  ##|  # | #  |### |    |    | 80 08 E3 24 E0 00
#   |    |    |    |    |    |    |    |    |    |    | 80 00 00 00 00 00
#   |  ##|# # |  # | ###| ###|##  |### | # #|   #|##  | 83 A2 77 CE 51 C0
#   | #  |  # |  # |#   |   #|   #|    |# # |# # |    | 84 22 81 10 AA 00
#   |  ##|   #|##  | ## |   #|   #|##  |# # |#  #|#   | 83 1C 61 1C A9 80
#   |    |#   |#   |   #|   #|   #|    |# # |#   | #  | 80 88 11 10 A8 40
#  #|    |#   |#   |   #|   #|   #|    |#   |#   | #  | 90 88 11 10 88 40
####| ###|    |#   |### |   #|    |### |#   |# ##|#   | F7 08 E1 0E 8B 80
    |    |    |    |    |    |    |    |    |    |    | 00 00 00 00 00 00
    |    |    |    |    |    |    |    |    |    |    | 00 00 00 00 00 00
    |    |    |    |    |    |    |    |    |    |    | 00 00 00 00 00 00
  ##|####|####|####|####|####|####|####|####|### |    | 3F FF FF FF FE 00
  ##|####|####|####|####|####|####|####|####|### |    | 3F FF FF FF FE 00
    |    |    |    |    |    |    |    |    |    |    | 00 00 00 00 00 00
    |    |    |    |    |    |    |    |    |    |    | 00 00 00 00 00 00
    |  ##|   #|####| #  |# ##|## #|    | ## |    |    | 03 1F 4B D0 60 00
    | ###|#   | #  | #  |# # |   #|    |####|    |    | 07 84 4A 10 F0 00
    |####|##  | #  | ###|# ##|#  #|   #|#  #|#   |    | 0F C4 7B 91 98 00
    |####|##  | #  | #  |# # |   #|   #|#  #|#   |    | 0F C4 4A 11 98 00
    | ###|#   | #  | #  |# # |   #|    |####|    |    | 07 84 4A 10 F0 00
    |  ##|    | #  | #  |# ##|## #|### | ## |    |    | 03 04 4B DE 60 00
    |    |    |    |    |    |    |    |    |    |    | 00 00 00 00 00 00
    |    |    |    |    |    |    |    |    |    |    | 00 00 00 00 00 00
  ##|####|####|####|####|####|####|####|####|### |    | 3F FF FF FF FE 00
  ##|####|####|####|####|####|####|####|####|### |    | 3F FF FF FF FE 00
*/

/*
 * bpl = BYTES per line
 * ppl = PIXELS per line
 * l   = total lines
 */
#define   logo_bpl  6
#define   logo_ppl  42
#define   logo_l    32

static unsigned char logo[] = {
  0xFF,0xA2,0xE3,0x18,0xE0,0x00,
  0xF0,0xA2,0x94,0xA5,0x00,0x00,
  0xC0,0x9C,0xE4,0xA5,0x00,0x00,
  0xC0,0x08,0x94,0xB9,0x30,0x00,
  0xC0,0x08,0x94,0xA9,0x10,0x00,
  0x80,0x08,0xE3,0x24,0xE0,0x00,
  0x80,0x00,0x00,0x00,0x00,0x00,
  0x83,0xA2,0x77,0xCE,0x51,0xC0,
  0x84,0x22,0x81,0x10,0xAA,0x00,
  0x83,0x1C,0x61,0x1C,0xA9,0x80,
  0x80,0x88,0x11,0x10,0xA8,0x40,
  0x90,0x88,0x11,0x10,0x88,0x40,
  0xF7,0x08,0xE1,0x0E,0x8B,0x80,
  0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,
  0x3F,0xFF,0xFF,0xFF,0xFE,0x00,
  0x3F,0xFF,0xFF,0xFF,0xFE,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,
  0x03,0x1F,0x4B,0xD0,0x60,0x00,
  0x07,0x84,0x4A,0x10,0xF0,0x00,
  0x0F,0xC4,0x7B,0x91,0x98,0x00,
  0x0F,0xC4,0x4A,0x11,0x98,0x00,
  0x07,0x84,0x4A,0x10,0xF0,0x00,
  0x03,0x04,0x4B,0xDE,0x60,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,
  0x00,0x00,0x00,0x00,0x00,0x00,
  0x3F,0xFF,0xFF,0xFF,0xFE,0x00,
  0x3F,0xFF,0xFF,0xFF,0xFE,0x00
};

static void showlogo(int x, int y, bool on)
{
    int   px,py;  /* pixel x & y */

    if (on)
    {
        for (py=0; py<logo_l; py++)
            for (px=0; px<logo_ppl; px++)
                if ( ((logo[(py*logo_bpl)+(px/8)] >>(7-(px%8))) &1) )
                    rb->lcd_drawpixel(x+px, y+py);
                else
                    rb->lcd_clearpixel(x+px, y+py);
        rb->lcd_update_rect(x,y, logo_ppl, logo_l);
    }
    else
        rb->lcd_clearrect(x,y, logo_ppl, logo_l);

    return;

}


/********************************************************************
 * strlen ofr use with othello print system
 ********************************************************************/
static int othstrlen(char* s)
{

    int i;

    for(i=0; s[i]!=of_eos; i++);

    return(i);

}

/********************************************************************
 * print othello char upd=true will issue update_lcd()
 ********************************************************************/
static void othprint(unsigned char x, unsigned char y, char ch, bool upd)
{
    int px,py;  /* pixel coords 1..6 */

    for (py=0; py<6; py++)
        for (px=0; px<6; px++)
            if ((othfont[(unsigned char)ch][py] >>(5-px)) &1)
                rb->lcd_drawpixel(x+px, y+py);
            else
                rb->lcd_clearpixel(x+px, y+py);

    if (upd)
        rb->lcd_update_rect(x,y, 6,6);

    return;

}


/********************************************************************
 * print othello string upd=true will issue update_lcd()
 ********************************************************************/
static void othprints(unsigned char x, unsigned char y, char* s, bool upd)
{
    int i;
    int l; /* length of string */

    l = othstrlen(s);

    for (i=0; i<l; i++)
        othprint(x+i*6,y, s[i], false);

    if (upd)
        rb->lcd_update_rect(x,y, l*6,6);

    return;

}


/********************************************************************
 * display game over visuals
 ********************************************************************/
static void show_endgame(unsigned char scx, unsigned char sco)
{
    /* end of game messages */
    showlogo(logo_tlx,logo_tly, false);

    /* game over */
    rb->lcd_putsxy(go_tlx+1,go_tly+1, "Game");
    rb->lcd_putsxy(go_tlx+1,go_tly+11, "oveR");
    rb->lcd_invertrect(go_tlx,go_tly, 27,20);

    if (scx==sco)
    {
        /* draw */
        rb->lcd_putsxy(draw_tlx+13,draw_tly+4, "DraW");
        othprint(draw_tlx+5,draw_tly+5, of_plx, true);
        othprint(draw_tlx+40,draw_tly+5, of_plo, true);
        rb->lcd_drawrect(draw_tlx+2,draw_tly+2, 47,11);
        rb->lcd_drawrect(draw_tlx,draw_tly, 51,15);
    }
    else
    {
        /* win */
        rb->lcd_putsxy(win_tlx+14,win_tly+4, "WiNS");
        if (sco>scx)
            othprint(win_tlx+5,win_tly+5, of_plo, true);
        else
            othprint(win_tlx+5,win_tly+5, of_plx, true);
        rb->lcd_drawrect(win_tlx+2,win_tly+2, 39,11);
        rb->lcd_drawrect(win_tlx,win_tly, 43,15);
    }

    rb->lcd_update();

    return;

}


/********************************************************************
 * display othello grid
 * currenly hard coded to the top left corner of the screen
 ********************************************************************/
static void show_grid(void)
{
    int x,y;

    rb->lcd_clearrect(0,0, (8*7)+1,(8*7)+1);
    rb->lcd_drawrect(0,0,  (8*7)+1,(8*7)+1);
    for (x=7; x<((7*7)+1); x+=7)
    {
        rb->lcd_drawline(1,x, 2,x);
        rb->lcd_drawline(x,1, x,2);
        rb->lcd_drawline(x,(8*7)-1, x,(8*7)-2);
        rb->lcd_drawline((8*7)-1,x, (8*7)-2,x);
        for (y=7; y<((7*7)+1); y+=7)
        {
            rb->lcd_drawline(x-2,y, x+2,y);
            rb->lcd_drawline(x,y-2, x,y+2);
        }
    }
    rb->lcd_update_rect(0,0, (8*7)+1,(8*7)+1);

    return;

}


/********************************************************************
 * flash the board - used for invalid move!
 ********************************************************************/
static void flashboard()
{
    rb->lcd_invertrect(0,0, (8*7)+1,(8*7)+1);
    rb->lcd_update_rect(0,0, (8*7)+1,(8*7)+1);
    rb->sleep(HZ/10);
    rb->lcd_invertrect(0,0, (8*7)+1,(8*7)+1);
    rb->lcd_update_rect(0,0, (8*7)+1,(8*7)+1);

    return;

}


/********************************************************************
 * show player skill levels
 ********************************************************************/
static void show_players(void)
{
    static char scs[] = {
        of_plx, of_colon, of_h, of_dash, of_0, of_eos, /* 0 */
        of_plo, of_colon, of_h, of_dash, of_0, of_eos  /* 6 */
    };

    if (player[PLAYERX]==AIBOT)
        scs[2] = of_c;
    scs[4] = ai_help[PLAYERX] +of_0;

    if (player[PLAYERO]==AIBOT)
        scs[8] = of_c;
    scs[10] = ai_help[PLAYERO] +of_0;

    othprints( 2,58, &scs[0], true);
    othprints(40,58, &scs[6], true);

    return;

}


/********************************************************************
 * show f3 function
 ********************************************************************/
static void show_f3(bool playing)
{
    static char scs[10] = {of_p, of_l, of_a, of_y, of_eos,
                           of_q, of_u, of_i, of_t, of_eos };

    if (playing)
        othprints(80,58, &scs[5], true);
    else
        othprints(80,58, &scs[0], true);

    return;

}


/********************************************************************
 * update board tiles
 ********************************************************************/
static void show_board(void)
{
    unsigned char x,y;

    for (y=1; y<=8; y++)
        for (x=1; x<=8; x++)
            othprint(((x-1)*7)+1, ((y-1)*7)+1, board[y][x], false);
    rb->lcd_update_rect(0,0, (8*7)+1,(8*7)+1);

    return;

}


/********************************************************************
 * display scores  player "turn" will get the arrow
 ********************************************************************/
static void show_score(bool turn)
{
    static char  scs[] = {of_ptr, of_eos,            /*  0 */
                          of_sp, of_eos,             /*  2 */
                          of_plx, of_colon, of_eos,  /*  4 */
                          of_plo, of_colon, of_eos,  /*  7 */
                          of_sp, of_sp, of_eos,      /* 10 score.x */
                          of_sp, of_sp, of_eos,      /* 13 score.o */
    };

    rb->snprintf(&scs[10], 3, "%d", score.x);
    scs[10] = scs[10] -'0' +of_0;
    if (scs[11]=='\0')
        scs[11] = of_sp;
    else
        scs[11] = scs[11] -'0' +of_0;
    scs[12] = of_eos;

    rb->snprintf(&scs[13], 3, "%d", score.o);
    scs[13] = scs[13] -'0' +of_0;
    if (scs[14]=='\0')
        scs[14] = of_sp;
    else
        scs[14] = scs[14] -'0' +of_0;
    scs[15] = of_eos;

    /* turn arrow */
    if (turn==PLAYERX)
    {
        othprints(sc_tlx,sc_tly, &scs[0], false);
        othprints(sc_tlx,sc_tly+8, &scs[2], false);
    }
    else
    {
        othprints(sc_tlx,sc_tly, &scs[2], false);
        othprints(sc_tlx,sc_tly+8, &scs[0], false);
    }

    /* names */
    othprints(sc_tlx+10,sc_tly, &scs[4], false);
    othprints(sc_tlx+10,sc_tly+8, &scs[7], false);

    /* scores */
    othprints(sc_tlx+26,sc_tly, &scs[10], false);
    othprints(sc_tlx+26,sc_tly+8, &scs[13], false);

    rb->lcd_update_rect(sc_tlx,sc_tly, 40,14);

    return;
}


/********************************************************************
 * cls()
 ********************************************************************/
static void initscreen(void)
{
    rb->lcd_setfont(FONT_SYSFIXED);
    rb->lcd_clear_display();
    rb->lcd_update();

    return;
}


/********************************************************************
 * Check is the specified move is valid
 * if type=MOVE - the board will be updated.
 * this is the recursive bit - it is called by domove()
 * checkmove only checks the move in ONE direction
 ********************************************************************/
static int checkmove(unsigned char x, unsigned char y, bool pl,
                     unsigned char dir, bool type)
{
    int i;
    unsigned char t;

    x -= ( ((dir&LEFT )==LEFT ) ?1:0);
    x += ( ((dir&RIGHT)==RIGHT) ?1:0);
    y -= ( ((dir&UP   )==UP   ) ?1:0);
    y += ( ((dir&DOWN )==DOWN ) ?1:0);

    t = board[y][x];

    /* found your piece */
    if ( t == ((pl==PLAYERX)?PLAYERX:PLAYERO) )
        return(1);

    /* found an empty sqaure or board edge */
    if (t>PLAYERO)
        return(0);

    /* must have found opponent piece */
    if ((i = checkmove(x,y, pl, dir, type)))
    {
        if (type==MAKE)
            board[y][x] = pl;
        return(i+1);
    }
    else
        return(0);
}


/********************************************************************
 * this is the control loop for checkmove()
 * checkmove()it is called with all eight possible directoins
 * the move.taken is defined before it returns
 * 0 taken is an invalid move
 ********************************************************************/
static void domove(struct move* move, bool type)
{
    int i;
    unsigned char dir;

    move->taken = 0;
    for (dir=DOWN; dir<=(LEFT|UP); dir++)
    {
        if ( (dir&(UP|DOWN)) ==(UP|DOWN) )
            continue;
        if ((i = checkmove(move->x, move->y, move->player, dir, type)))
            move->taken += i-1;
    }

    return;
}


/********************************************************************
 * initialise a new game board and draw it on the screen
 ********************************************************************/
static void initboard(void)
{
    unsigned char x,y;

    for (y=0; y<10; y++)
        for (x=0; x<10; x++)
            if ( (y%9)==0 || (x%9)==0)
                board[y][x] = BORDER;
            else
                board[y][x] = EMPTY;

    board[4][4] =  PLAYERX;
    board[5][5] =  PLAYERX;
    board[4][5] =  PLAYERO;
    board[5][4] =  PLAYERO;

    score.x = 2;
    score.o = 2;

    show_grid();
    show_board();
    show_score(FIRST);

    return;
}


/********************************************************************
 * remove "possible" markers from the board
 ********************************************************************/
static void clearposs(void)
{
    int x, y;

    for (y=1; y<=8; y++)
        for (x=1; x<=8; x++)
            if (board[y][x]>=POSS)
                board[y][x]=EMPTY;

    return;
}


/********************************************************************
 * build a list of all possible moves
 ********************************************************************/
static int getplist(struct move* plist, unsigned char pl)
{
    int     x, y;
    unsigned char     pcnt = 0;

    /* this significantly reduces the amount of pointer maths */
    struct move    pmove;

    /* clear previous possibilities */
    clearposs();

    for (y=1; y<=8; y++)
        for (x=1; x<=8; x++)
        {
            /* only empty sqaures */
            if (board[y][x]!=EMPTY)
                continue;
            /* try move */
            pmove.x      = x;
            pmove.y      = y;
            pmove.player = pl;
            domove(&pmove, CHECK);
            /* if valid - add to list */
            if (pmove.taken)
                rb->memcpy(&plist[pcnt++], &pmove, sizeof(struct move));
        }

    return(pcnt);

}


/********************************************************************
 * qsort
 ********************************************************************/
static int plist_bytaken(const void* m1, const void* m2)
{
    /* highest is best */
    return( ((struct move*)m2)->taken - ((struct move*)m1)->taken );
}


/********************************************************************
 * qsort
 ********************************************************************/
static int plist_byrank(const void* m1, const void* m2)
{
    /* lowest is best */
    return( ((struct move*)m1)->rank - ((struct move*)m2)->rank );
}


/********************************************************************
 *
CORNERS (1)
x......x       1,1(01)                      1,8(08)
........
........
........
........
........
........
x......x       8,1(08)                      8,8(64)


BLUFF (2)
..x..x..                1,3(03)    1,6(06)
........
x.x..x.x       3,1(03)  3,3(09)    3,6(18)  3,8(24)
........
........
x.x..x.x       6,1(06)  6,3(18)    6,6(36)  6,8(48)
........
..x..x..                8,3(24)    8,6(48)  8,8(64)


EDGE (3)
...xx...                 1,4(00)  1,5(00)
........
........
x......x       4,1(00)                      4,8(00)
x......x       5,1(00)                      5,8(00)
........
........
...xx...                 8,4(00)  8,5(00)


BAD (5) - some of these are edge pieces
.x....x.               1,2(02)      1,7(07)
xx....xx       2,1(02) 2,2(04)      2,7(14) 2,8(16)
........
........
........
........
xx....xx       7,1(07) 7,2(14)      7,7(49) 7,8(56)
.x....x.               8,2(16)      8,7(56)


OTHER (4)

 * this is called my reduceplist, if the "smart" AIBOT is playing
 * board sqaures are weighted as above
 *
 ********************************************************************/
static void smartranking(struct move* plist, unsigned char pcnt)
{

#define corner \
  ( ((y==1)||(y==8)) && ((x==1)||(x==8)) )

#define bluff \
  ( ((y==1)||(y==3)||(y==6)||(y==8)) &&   \
    ((x==1)||(x==3)||(x==6)||(x==8)) )

#define edge \
  ( ( ((y==1)||(y==8)) && ((x==4)||(x==5)) ) ||     \
    ( ((y==4)||(y==5)) && ((x==1)||(x==8)) ) )

    int i;
    register unsigned char mul;
    register unsigned char x, y;

    for (i=0; i<pcnt; i++)
    {
        x   = plist[i].x;
        y   = plist[i].y;
        mul = x *y;

        /* preferred squares */
        if      (corner)  { plist[i].rank = 1;  continue; }
        else if (bluff)   { plist[i].rank = 2;  continue; }
        else if (edge)    { plist[i].rank = 3;  continue; }

        /* uninteresting square */
        plist[i].rank = 4;

        /* avoid "bad" sqaures */
        if ( (mul==02)||(mul==04)||
             (mul==07)||(mul==14)||(mul==16)||
             (mul==49)||(mul==56) )
            plist[i].rank = 5;
    }

    return;

#undef corner
#undef bluff
#undef edge

}


/********************************************************************
 * called by pressing f1 or f2 to change player modes
 ********************************************************************/
static void changeplayer(bool pl)
{
    ai_help[pl]++;
    if (ai_help[pl]>BEST)
    {
        player[pl]  = (player[pl]==HUMAN)?AIBOT:HUMAN;
        if (player[pl]==HUMAN)
            ai_help[pl] = NONE;
        else
            ai_help[pl] = WEAK;
    }
    show_players();

    return;
}


/********************************************************************
 * this proc reduces the list of possible moves to a short list of
 * preferred moves, dependand on the player AI
 ********************************************************************/
static unsigned char reduceplist(struct move* plist, unsigned char pcnt, unsigned char ai_help)
{

    int i;

    switch(ai_help)
    {
        /* ------------------------------------------------- */
        /* weak does not modify the possible's list */
        /*
          case WEAK:
          break;
        */
        /* ------------------------------------------------- */
        case GURU:
            break;
            /* ------------------------------------------------- */
        case EXPERT:
            break;
            /* ------------------------------------------------- */
            /* this player will favour certain known moves */
        case SMART:
            if (pcnt>1)
            {
                smartranking(plist, pcnt);
                rb->qsort(plist, pcnt, sizeof(struct move), plist_byrank);
                for (i=1; i<pcnt; i++)
                    if (plist[i].rank!=plist[i-1].rank)
                        break;
                pcnt = i;
            }
            /* FALL THROUGH */
            /* ------------------------------------------------- */
            /* reduce possibilites to "most pieces taken" */
        case AVERAGE:
            if (pcnt>1)
            {
                rb->qsort(plist, pcnt, sizeof(struct move), plist_bytaken);
                for (i=1; i<pcnt; i++)
                    if (plist[i].taken!=plist[i-1].taken)
                        break;
                pcnt = i;
            }
            break;
            /* ------------------------------------------------- */
        default:
            // you should never get here!
            break;
    }

    return(pcnt);

}


/********************************************************************
 * calc all moves with wieghting and report back to the user/aibot
 ********************************************************************/
static bool calcposs(struct move* plist, unsigned char* pcnt, bool turn)
{
    int i;

    /* only evaluate moves for AIBOTs or HUMAN+HELP */
    if ( (player[turn]==AIBOT) || (ai_help[turn]) )
    {
        /* get list of all possible moves */
        (*pcnt) = getplist(plist, turn);

        /* no moves? trigger Game Over */
        if (!(*pcnt))
            return(true);

        /* mark all possible moves on board */
        for (i=0; i<(*pcnt); i++)
            board[plist[i].y][plist[i].x] = POSS;

        /* use ai to reduce list */
        (*pcnt) = reduceplist(plist, (*pcnt), ai_help[turn]);

        /* higlight preferred moves */
        if (ai_help[turn]>WEAK)
            for (i=0; i<(*pcnt); i++)
                board[plist[i].y][plist[i].x] = CHOICE;
    }
    else /* no ai/help required */
    {
        /* create dummy plist entry for default cursor position */
        plist[0].x = 4;
        plist[0].y = 4;
    }

    return(false); /* do not cause Game Over */
}


/********************************************************************
 * cursor highlight
 ********************************************************************/
static void hilite(struct move* move, bool on)
{
    int x = (move->x-1)*7;
    int y = (move->y-1)*7;

    rb->lcd_invertrect(x+1,y+1, 6,6);
    if (on)
        rb->lcd_drawrect(x,y, 8,8);
    else
    {
        if (x)
            rb->lcd_clearline(x,y+3, x,y+4);
        if (y)
            rb->lcd_clearline(x+3,y, x+4,y);
        if (x!=7*7)
            rb->lcd_clearline(x+7,y+3, x+7,y+4);
        if (y!=7*7)
            rb->lcd_clearline(x+3,y+7, x+4,y+7);
    }
    rb->lcd_update_rect(x,y, 8,8);
}


/********************************************************************
 * main othelo keyboard handler
 * returns the key that it terminated with
 ********************************************************************/
static int getmove(struct move* move, struct move* plist, unsigned char* pcnt, bool turn)
{
    int     key;
    bool    waiting = true;

    /* get next move */
    do
    {
        hilite(move, true);
        key = rb->button_get(true);
        hilite(move, false);

        switch(key)
        {
            case BUTTON_ON:
            case BUTTON_OFF:
            case BUTTON_F3:
                waiting = false;
                break;
            case BUTTON_UP:
            case BUTTON_UP | BUTTON_REPEAT:
                if (move->y>1) move->y--;
                break;
            case BUTTON_DOWN:
            case BUTTON_DOWN | BUTTON_REPEAT:
                if (move->y<8) move->y++;
                break;
            case BUTTON_LEFT:
            case BUTTON_LEFT | BUTTON_REPEAT:
                if (move->x>1) move->x--;
                break;
            case BUTTON_RIGHT:
            case BUTTON_RIGHT | BUTTON_REPEAT:
                if (move->x<8) move->x++;
                break;
            case BUTTON_PLAY:
                if (board[move->y][move->x]>=POSS)
                    waiting = false;
                else
                    flashboard();
                break;
            case BUTTON_F1:
            case BUTTON_F2:
            {
                bool pl;

                pl = (key==BUTTON_F1)?PLAYERX:PLAYERO;

                changeplayer(pl);
                /* update board if *current* player options changed */
                if (move->player==pl)
                {
                    clearposs();
                    calcposs(plist, pcnt, turn);
                    show_board();
                }
                break;
            }
            default:
                break;
        }
    } while (waiting);

    return(key);
}


/********************************************************************
 * main control loop
 ********************************************************************/
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
#define STILL_PLAYING (!gameover && !quit)

#define default_players       \
  player[PLAYERX]  = DF_PLX;  \
  ai_help[PLAYERX] = DF_AIX;  \
  player[PLAYERO]  = DF_PLO;  \
  ai_help[PLAYERO] = DF_AIO;

    int     key;

    unsigned char     pcnt;
    struct move    plist[MAXPOSS];

    bool    gameover;
    bool    quit;

    struct move    move;

    TEST_PLUGIN_API(api);
    (void)parameter;
    rb = api;
    
    quit = false;

    do /* while !quit */
    {
        initscreen();
        showlogo(logo_tlx,logo_tly, true);
        show_players();
        show_f3(true);

        if (!playing)
        {
            initboard();
            playing = true;
            turn    = FIRST;
        }
        else
        {
            show_grid();
            show_board();
            show_score(turn);
        }

        gameover = false;

        do /* while !gameover && !quit */
        {
            /* who's move is it? */
            move.player = turn;

            /* perform ai/help routine */
            if ((gameover = calcposs(plist, &pcnt, turn))) continue;

            /* player now gets to take a turn */
            if (player[turn]==AIBOT)
            {
                int   timeout;
                bool  held = false;

                unsigned char t;
                /* select a random move from the possibles list */
                /* this block of code corrupts pcnt */
                if (pcnt>1)
                    t = rb->rand() % pcnt;
                else
                    t = 0;
                move.x = plist[t].x;
                move.y = plist[t].y;
                /* move selected will always be valid! */
                domove(&move, CHECK);

                /* bots run faster when no humans are playing */
                if ((player[PLAYERX]==AIBOT) && (player[PLAYERO]==AIBOT))
                    timeout = *rb->current_tick +((HZ*6)/10);
                else
                    timeout = *rb->current_tick +((HZ*(REPEAT_START+1))/10);
                while (TIME_BEFORE(*rb->current_tick, timeout))
                {
                    key = rb->button_get(false);
                    switch (key)
                    {
                        case SYS_USB_CONNECTED:
                            rb->usb_screen();
                            return PLUGIN_USB_CONNECTED;
                            /* hold play to freeze board */
                        case BUTTON_PLAY:
                        case BUTTON_PLAY|BUTTON_REPEAT:
                            timeout = *rb->current_tick +HZ;
                            held = true;
                            break;
                        case BUTTON_PLAY|BUTTON_REL:
                            if (held)
                                timeout = *rb->current_tick-1;
                            continue;
                        case BUTTON_F3:
                            gameover = true;
                            break;
                        case BUTTON_OFF:
                            default_players;
                            playing = false;
                            /* Fall through to BUTTON_ON */
                        case BUTTON_ON:
                            return PLUGIN_OK;
                        default:
                            break;
                    }
                } /*endwhile*/;
            }
            else /* player is human */
            {
                /* only display poss on screen if help is enabled */
                if (ai_help[turn]) show_board();
                move.x = plist[0].x;
                move.y = plist[0].y;
                while(true)
                {
                    key = getmove(&move, plist, &pcnt, turn);
                    switch(key)
                    {
                        case SYS_USB_CONNECTED:
                            rb->usb_screen();
                            return PLUGIN_USB_CONNECTED;
                        case BUTTON_OFF:
                            playing = false;
                            default_players;
                        case BUTTON_ON:
                            rb->lcd_setfont(FONT_UI);
                            return PLUGIN_OK;
                        case BUTTON_F3:
                            gameover = true;
                        default:
                            break;
                    }
                    if (key==BUTTON_F3)
                        break;

                    /* check move is valid & retrieve "pieces taken" */
                    domove(&move, CHECK);
                    if (move.taken==0)
                        flashboard();
                    else
                        break;
                }
            }

            /* player may have hit restart instead of moving */
            if (STILL_PLAYING)
            {
                /* MAKE MOVE */
                /* add new piece */
                board[move.y][move.x] = move.player;
                /* flip opponent pieces */
                domove(&move, MAKE);
                /* update board */
                clearposs();
                show_board();
                /* update score */
                if (turn==PLAYERX)
                {
                    score.x += move.taken+1;
                    score.o -= move.taken;
                }
                if (turn==PLAYERO)
                {
                    score.o += move.taken+1;
                    score.x -= move.taken;
                }
                /* next player please */
                turn = (turn==PLAYERX)?PLAYERO:PLAYERX;
                show_score(turn);
            }

        } while(STILL_PLAYING);

        clearposs();
        show_board();
        show_f3(false);
        show_endgame(score.x, score.o);
        playing = false;

        do
        {
            if ((key = rb->button_get(true)) ==BUTTON_F3)
                break;
            switch(key)
            {
                case SYS_USB_CONNECTED:
                    rb->usb_screen();
                    return PLUGIN_USB_CONNECTED;
                case BUTTON_OFF:
                    default_players;
                case BUTTON_ON:
                    quit = true;
                    break;
                case BUTTON_F1:
                    changeplayer(PLAYERX);
                    break;
                case BUTTON_F2:
                    changeplayer(PLAYERO);
                    break;
                default:
                    break;
            }
        } while(!quit);

    }while(!quit);

    return PLUGIN_OK;

}
#endif
