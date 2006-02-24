/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
*
* Copyright (C) 2006 Miguel A. Arévalo 
* Color graphics from eboard
* GNUChess v2 chess engine Copyright (c) 1988  John Stanback
*
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/
 
#include "plugin.h"

#ifdef HAVE_LCD_BITMAP

#include "gnuchess.h"

/* type definitions */
struct cb_command {
	int type;
	char mv_s[5];
	unsigned short mv;
};

/* External bitmaps */
extern const fb_data chessbox_pieces[];


PLUGIN_HEADER

/* button definitions */
#if (CONFIG_KEYPAD == IPOD_3G_PAD) || (CONFIG_KEYPAD == IPOD_4G_PAD)
#define CB_SELECT  BUTTON_SELECT
#define CB_UP      BUTTON_MENU
#define CB_DOWN    BUTTON_PLAY
#define CB_LEFT    BUTTON_LEFT
#define CB_RIGHT   BUTTON_RIGHT
#define CB_PLAY    (BUTTON_SELECT | BUTTON_PLAY)
#define CB_LEVEL   (BUTTON_SELECT | BUTTON_RIGHT)
#define CB_QUIT    (BUTTON_SELECT | BUTTON_MENU)

#elif CONFIG_KEYPAD == IAUDIO_X5_PAD
#define CB_SELECT  BUTTON_MENU
#define CB_UP      BUTTON_UP
#define CB_DOWN    BUTTON_DOWN
#define CB_LEFT    BUTTON_LEFT
#define CB_RIGHT   BUTTON_RIGHT
#define CB_PLAY    BUTTON_PLAY
#define CB_LEVEL   BUTTON_REC
#define CB_QUIT    BUTTON_POWER

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define CB_SELECT  BUTTON_SELECT
#define CB_UP      BUTTON_UP
#define CB_DOWN    BUTTON_DOWN
#define CB_LEFT    BUTTON_LEFT
#define CB_RIGHT   BUTTON_RIGHT
#define CB_PLAY    BUTTON_ON
#define CB_LEVEL   BUTTON_MODE
#define CB_QUIT    BUTTON_OFF

#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
#define CB_SELECT  BUTTON_SELECT
#define CB_UP      BUTTON_UP
#define CB_DOWN    BUTTON_DOWN
#define CB_LEFT    BUTTON_LEFT
#define CB_RIGHT   BUTTON_RIGHT
#define CB_PLAY    BUTTON_PLAY
#define CB_LEVEL   BUTTON_EQ
#define CB_QUIT    BUTTON_MODE

#elif CONFIG_KEYPAD == RECORDER_PAD
#define CB_SELECT  BUTTON_PLAY
#define CB_UP      BUTTON_UP
#define CB_DOWN    BUTTON_DOWN
#define CB_LEFT    BUTTON_LEFT
#define CB_RIGHT   BUTTON_RIGHT
#define CB_PLAY    BUTTON_ON
#define CB_LEVEL   BUTTON_F1
#define CB_QUIT    BUTTON_OFF

#elif CONFIG_KEYPAD == ONDIO_PAD
#define CB_SELECT_PRE BUTTON_MENU
#define CB_SELECT  (BUTTON_MENU|BUTTON_REL)
#define CB_UP      BUTTON_UP
#define CB_DOWN    BUTTON_DOWN
#define CB_LEFT    BUTTON_LEFT
#define CB_RIGHT   BUTTON_RIGHT
#define CB_PLAY_PRE BUTTON_MENU
#define CB_PLAY    (BUTTON_MENU|BUTTON_REPEAT)
#define CB_LEVEL   (BUTTON_MENU|BUTTON_OFF)
#define CB_QUIT    BUTTON_OFF

#else
    #error CHESSBOX: Unsupported keypad
#endif

/* use 30x30 tiles */
#if (LCD_HEIGHT >= 240) && (LCD_WIDTH >= 240)
#define TILE_WIDTH  30
#define TILE_HEIGHT 30
/* use 22x22 tiles */
#elif (LCD_HEIGHT >= 176) && (LCD_WIDTH >= 176)
#define TILE_WIDTH  22
#define TILE_HEIGHT 22
/* use 16x16 tiles */
#elif (LCD_HEIGHT >= 128) && (LCD_WIDTH >= 128)
#define TILE_WIDTH  16
#define TILE_HEIGHT 16
/* use 8x8 tiles */
#elif (LCD_HEIGHT >= 64) && (LCD_WIDTH >= 64)
#define TILE_WIDTH  8
#define TILE_HEIGHT 8
#else
  #error BEJEWELED: Unsupported LCD
#endif

/* Calculate Offsets */
#define XOFS ((LCD_WIDTH-8*TILE_WIDTH)/2)
#define YOFS ((LCD_HEIGHT-8*TILE_HEIGHT)/2)

/* commands enum */
#define COMMAND_MOVE	1
#define COMMAND_PLAY	2
#define COMMAND_LEVEL	3
/*#define COMMAND_RESTART	4*/
#define COMMAND_QUIT	5

/* GCC wants this to be present for some targets */
void* memcpy(void* dst, const void* src, size_t size)
{
    return rb->memcpy(dst, src, size);
}

/* ---- Get the board column and row (e2 f.e.) for a physical x y ---- */
void xy2cr ( short x, short y, short *c, short *r ) {
	if (computer == black ) {
		*c = x ;
		*r = y ;
	} else {
		*c = 7 - x ;
		*r = 7 - y ;
	}
}

/* ---- get physical x y for a board column and row (e2 f.e.) ---- */
void cr2xy ( short c, short r, short *x, short *y ) {
	if ( computer == black ) {
		*x = c ;
		*y = r ;
	} else {
		*x = 7 - c ;
		*y = 7 - r ;
	}
}

/* ---- Draw a complete board ---- */
static void cb_drawboard (void) {
    short r , c , x , y ;
	short l , piece , p_color ;
	int b_color=1;
	
	rb->lcd_clear_display();

	for (r = 0; r < 8; r++) {
        for (c = 0; c < 8; c++) {
            l = locn[r][c];
            piece   = board[l] ;
			p_color = color[l] ;
			cr2xy ( c , r , &x , &y );
			if ( piece == no_piece ) {
				rb->lcd_bitmap_part ( chessbox_pieces , 0 ,
									  TILE_HEIGHT * b_color ,
									  TILE_WIDTH ,
									  XOFS + x*TILE_WIDTH ,
				                      YOFS + ( 7 - y )*TILE_HEIGHT ,
				                      TILE_WIDTH ,
				                      TILE_HEIGHT );
			} else {
			    rb->lcd_bitmap_part ( chessbox_pieces ,
									  0 ,
					                  2 * TILE_HEIGHT +
				                          4 * TILE_HEIGHT * ( piece - 1 ) +
				                          2 * TILE_HEIGHT * p_color +
				                          TILE_HEIGHT * b_color ,
									  TILE_WIDTH ,
									  XOFS + x*TILE_WIDTH ,
				                      YOFS + (7 - y)*TILE_HEIGHT ,
				                      TILE_WIDTH ,
				                      TILE_HEIGHT );
			}
			b_color = (b_color == 1) ? 0 : 1 ;
        }
		b_color = (b_color == 1) ? 0 : 1 ;
    }
	
	/* draw board limits */
	if ( LCD_WIDTH > TILE_WIDTH*8 ) {
		rb->lcd_set_drawmode ( DRMODE_FG );
		rb->lcd_drawline ( XOFS - 1 , YOFS ,
		                   XOFS - 1 , YOFS + TILE_HEIGHT*8 );
		rb->lcd_drawline ( XOFS + 8*TILE_WIDTH , YOFS ,
		                   XOFS + 8*TILE_WIDTH , YOFS + TILE_HEIGHT*8 );
	}
	if ( LCD_HEIGHT > TILE_HEIGHT*8 ) {
		rb->lcd_set_drawmode ( DRMODE_FG );
		rb->lcd_drawline ( XOFS , YOFS - 1 ,
		                   XOFS + TILE_WIDTH*8 , YOFS - 1 );
		rb->lcd_drawline ( XOFS , YOFS + TILE_HEIGHT*8  ,
		                   XOFS + 8*TILE_WIDTH , YOFS + TILE_HEIGHT*8 );
	}
	rb->lcd_update();
}

/* ---- Switch mark on board ---- */
void cb_switch ( short x , short y ) {
	rb->lcd_set_drawmode ( DRMODE_COMPLEMENT );
	rb->lcd_drawrect ( XOFS + x*TILE_WIDTH + 1 ,
	                   YOFS + ( 7 - y )*TILE_HEIGHT +1 ,
	                   TILE_WIDTH-2 , TILE_HEIGHT-2 );
	rb->lcd_update();
}

/* ---- increase playing level ---- */
void cb_levelup ( void ) {
	Level ++;
	if ( Level == 8 ) Level = 1;
	switch (Level) {
    	case 1 :
			TCmoves = 60;
			TCminutes = 5;
			rb->splash ( 50 , true , "Level 1: 60 moves / 5 min" );
			break;
    	case 2 :
			TCmoves = 60;
			TCminutes = 15;
			rb->splash ( 50 , true , "Level 2: 60 moves / 15 min" );
			break;
    	case 3 :
			TCmoves = 60;
			TCminutes = 30;
			rb->splash ( 50 , true , "Level 3: 60 moves / 30 min" );
			break;
    	case 4 :
			TCmoves = 40;
			TCminutes = 30;
			rb->splash ( 50 , true , "Level 4: 40 moves / 30 min" );
			break;
    	case 5 :
			TCmoves = 40;
			TCminutes = 60;
			rb->splash ( 50 , true , "Level 5: 40 moves / 60 min" );
			break;
    	case 6 :
			TCmoves = 40;
			TCminutes = 120;
			rb->splash ( 50 , true , "Level 6: 40 moves / 120 min" );
			break;
    	case 7 :
			TCmoves = 40;
			TCminutes = 240;
			rb->splash ( 50 , true , "Level 7: 40 moves / 240 min" );
			break;
    	case 8 :
			TCmoves = 1;
			TCminutes = 15;
			rb->splash ( 50 , true , "Level 8: 1 move / 15 min" );
			break;
      	case 9 :
			TCmoves = 1;
			TCminutes = 60;
			rb->splash ( 50 , true , "Level 9: 1 move / 60 min" );
			break;
      	case 10 :
			TCmoves = 1;
			TCminutes = 600;
			rb->splash ( 50 , true , "Level 10: 1 move / 600 min" );
			break;
	}
    TCflag = (TCmoves > 1);
  	SetTimeControl();
};

/* ---- main user loop ---- */
struct cb_command cb_getcommand (void) {
	static short x = 4 , y = 4 ;
	short c , r , l;
	int button, lastbutton = BUTTON_NONE;
	int marked = false , from_marked = false ;
	short marked_x = 0 , marked_y = 0 ;
	struct cb_command result = { 0, {0,0,0,0,0}, 0 };
	
	cb_switch ( x , y );
	/* main loop */
	while ( true ) {
		button = rb->button_get(true);
		switch (button) {
			case CB_QUIT:
				result.type = COMMAND_QUIT;
				return result;
/*			case CB_RESTART:
				result.type = COMMAND_RESTART;
				return result;*/
			case CB_LEVEL:
				result.type = COMMAND_LEVEL;
				return result;
			case CB_PLAY:
#ifdef CB_PLAY_PRE
                if (lastbutton != CB_PLAY_PRE)
                    break;
#endif
				result.type = COMMAND_PLAY;
				return result;
			case CB_UP:
				if ( !from_marked ) cb_switch ( x , y );
				y++;
				if ( y == 8 ) {
					y = 0;
					x--;
					if ( x < 0 ) x = 7;
				}
				if ( marked && ( marked_x == x ) && ( marked_y == y ) ) {
					from_marked = true ;
				} else {
					from_marked = false ;
					cb_switch ( x , y );
				}
				break;
			case CB_DOWN:
				if ( !from_marked ) cb_switch ( x , y );
				y--;
				if ( y < 0 ) {
					y = 7;
					x++;
					if ( x == 8 ) x = 0;
				}
				if ( marked && ( marked_x == x ) && ( marked_y == y ) ) {
					from_marked = true ;
				} else {
					from_marked = false ;
					cb_switch ( x , y );
				}
				break;
			case CB_LEFT:
				if ( !from_marked ) cb_switch ( x , y );
				x--;
				if ( x < 0 ) {
					x = 7;
					y++;
					if ( y == 8 ) y = 0;
				}
				if ( marked && ( marked_x == x ) && ( marked_y == y ) ) {
					from_marked = true ;
				} else {
					from_marked = false ;
					cb_switch ( x , y );
				}
				break;
			case CB_RIGHT:
				if ( !from_marked ) cb_switch ( x , y );
				x++;
				if ( x == 8 ) {
					x = 0;
					y--;
					if ( y < 0 ) y = 7;
				}
				if ( marked && ( marked_x == x ) && ( marked_y == y ) ) {
					from_marked = true ;
				} else {
					from_marked = false ;
					cb_switch ( x , y );
				}
				break;
			case CB_SELECT:
#ifdef CB_SELECT_PRE
                if (lastbutton != CB_SELECT_PRE)
                    break;
#endif
				if ( !marked ) {
					xy2cr ( x , y , &c , &r );
                    l = locn[r][c];
                    if ( ( color[l]!=computer ) && ( board[l]!=no_piece ) ) {
					    marked = true;
					    from_marked = true ;
					    marked_x = x;
					    marked_y = y;
					}
				} else {
					if ( ( marked_x == x ) && ( marked_y == y ) ) {
						marked = false;
						from_marked = false;
					} else {
						xy2cr ( marked_x , marked_y , &c , &r );
						result.mv_s[0] = 'a' + c;
						result.mv_s[1] = '1' + r;
						xy2cr ( x , y , &c , &r );
						result.mv_s[2] = 'a' + c;
						result.mv_s[3] = '1' + r;
						result.mv_s[4] = '\00';
						result.type = COMMAND_MOVE;
						return result;
					}
				}
				break;
		}
		if (button != BUTTON_NONE)
            lastbutton = button;
	}

}

/*****************************************************************************
* plugin entry point.
******************************************************************************/
enum plugin_status plugin_start(struct plugin_api* api, void* parameter) {
	struct cb_command command;
	/* init status */
	bool exit = false;
	
    /* plugin init */
    (void)parameter;
    rb = api;
    /* end of plugin init */

	/* load opening book, soon */
	
	/* init board */
	GNUChess_Initialize();
	
	/* draw the board */
	/* I don't like configscreens, start game inmediatly */
	cb_drawboard();
	
	while (!exit) {
		if ( mate ) {
			rb->splash ( 500 , true , "Checkmate!" );
			rb->button_get(true);
			GNUChess_Initialize();
			cb_drawboard();
		}
		command = cb_getcommand ();
		switch (command.type) {
			case COMMAND_MOVE:
				if ( ! VerifyMove ( command.mv_s , 0 , &command.mv ) ) {
					rb->splash ( 50 , true , "Illegal move!" );
					cb_drawboard();
				} else {
					cb_drawboard();
					rb->splash ( 0 , true , "Thinking..." );
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
					rb->cpu_boost ( true );
#endif
					SelectMove ( computer , 0 );
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
					rb->cpu_boost ( false );
#endif
					cb_drawboard();
				}
				break;
/*			case COMMAND_RESTART:
				GNUChess_Initialize();
				cb_drawboard();
				break;*/
			case COMMAND_PLAY:
				opponent = !opponent; computer = !computer;
				rb->splash ( 0 , true , "Thinking..." );
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
				rb->cpu_boost ( true );
#endif
				SelectMove ( computer , 0 );
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
				rb->cpu_boost ( false );
#endif
				cb_drawboard();
				break;
			case COMMAND_LEVEL:
				cb_levelup ( );
				cb_drawboard();
				break;
			case COMMAND_QUIT:
				/*cb_saveposition();*/
				exit = true;
				break;
		}
	}
	
    rb->lcd_setfont(FONT_UI);
    return PLUGIN_OK;
}

#endif /* HAVE_LCD_BITMAP */
