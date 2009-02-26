/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2006 Miguel A. Arévalo 
* Color graphics from eboard
* GNUChess v2 chess engine Copyright (c) 1988  John Stanback
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
 
#include "plugin.h"

#ifdef HAVE_LCD_BITMAP

#if (MEMORYSIZE <= 8) && !defined(SIMULATOR) /* Lowmem doesn't have playback in chessbox */
#include "lib/playback_control.h"
#endif

#include "gnuchess.h"
#include "opening.h"
#include "chessbox_pgn.h"

/* type definitions */
struct cb_command {
    int type;
    char mv_s[5];
    unsigned short mv;
};

/* External bitmaps */
extern const fb_data chessbox_pieces[];


PLUGIN_HEADER

/* Tile size defined by the assigned bitmap */
#include "pluginbitmaps/chessbox_pieces.h"
#define TILE_WIDTH BMPWIDTH_chessbox_pieces
#define TILE_HEIGHT (BMPHEIGHT_chessbox_pieces/26)

/* Calculate Offsets */
#define XOFS ((LCD_WIDTH-8*TILE_WIDTH)/2)
#define YOFS ((LCD_HEIGHT-8*TILE_HEIGHT)/2)

/* save files */
#define SAVE_FILE  PLUGIN_GAMES_DIR "/chessbox.save"

/* commands enum */
#define COMMAND_NOP        0
#define COMMAND_MOVE       1
#define COMMAND_PLAY       2
#define COMMAND_LEVEL      3
#define COMMAND_RESTART    4
#define COMMAND_QUIT       5
#define COMMAND_MENU       6
#define COMMAND_SAVE       7
#define COMMAND_RESTORE    8
#define COMMAND_RESUME     9
#define COMMAND_SELECT    10
#define COMMAND_NEXT      11
#define COMMAND_PREV      12

short plugin_mode;

/* level+1's string */
const char *level_string[] = { "Level 1: 60 moves / 5 min" ,
                        "Level 2: 60 moves / 15 min" , 
                        "Level 3: 60 moves / 30 min" ,
                        "Level 4: 40 moves / 30 min" ,
                        "Level 5: 40 moves / 60 min" ,
                        "Level 6: 40 moves / 120 min" ,
                        "Level 7: 40 moves / 240 min" ,
                        "Level 8: 1 move / 15 min" ,
                        "Level 9: 1 move / 60 min" ,
                        "Level 10: 1 move / 600 min" };

/* "While thinking" command */
int wt_command = COMMAND_NOP;

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
#if (LCD_WIDTH > TILE_WIDTH*8) && (LCD_HEIGHT > TILE_HEIGHT*8)
    rb->lcd_drawrect(XOFS - 1, YOFS - 1, TILE_WIDTH*8 + 2, TILE_HEIGHT*8 + 2);
#elif LCD_WIDTH > TILE_WIDTH*8
    rb->lcd_vline(XOFS - 1, 0, LCD_HEIGHT - 1);
    rb->lcd_vline(XOFS + 8*TILE_WIDTH, 0, LCD_HEIGHT - 1);
#elif LCD_HEIGHT > TILE_HEIGHT*8
    rb->lcd_hline(0, LCD_WIDTH - 1, YOFS - 1);
    rb->lcd_hline(0, LCD_WIDTH - 1, YOFS + TILE_HEIGHT*8);
#endif

    rb->lcd_update();
}

/* ---- Switch mark on board ---- */
void cb_switch ( short x , short y ) {
    rb->lcd_set_drawmode ( DRMODE_COMPLEMENT );
    rb->lcd_drawrect ( XOFS + x*TILE_WIDTH + 1 ,
                       YOFS + ( 7 - y )*TILE_HEIGHT +1 ,
                       TILE_WIDTH-2 , TILE_HEIGHT-2 );
    rb->lcd_update();
    rb->lcd_set_drawmode ( DRMODE_SOLID );
}

/* ---- callback for capturing interaction while thinking ---- */
void cb_wt_callback ( void ) {
    int button = BUTTON_NONE;
    
    wt_command = COMMAND_NOP;
    button = rb->button_get(false);
    switch (button) {
#ifdef CB_RC_QUIT
        case CB_RC_QUIT:
            wt_command = COMMAND_QUIT;
            timeout = true;
            break;
#endif
        case CB_MENU:
            wt_command = COMMAND_MENU;
            timeout = true;
            break;
        case CB_PLAY:
            wt_command = COMMAND_PLAY;
            timeout = true;
            break;
    }
}

/* ---- set playing parameters depending on level ---- */
void cb_setlevel ( int lev ) {
    Level = (lev > 7) ? 7 : ( (lev < 1) ? 1 : lev ) ;
    switch (Level) {
        case 1 :
            TCmoves = 60;
            TCminutes = 5;
            break;
        case 2 :
            TCmoves = 60;
            TCminutes = 15;
            break;
        case 3 :
            TCmoves = 60;
            TCminutes = 30;
            break;
        case 4 :
            TCmoves = 40;
            TCminutes = 30;
            break;
        case 5 :
            TCmoves = 40;
            TCminutes = 60;
            break;
        case 6 :
            TCmoves = 40;
            TCminutes = 120;
            break;
        case 7 :
            TCmoves = 40;
            TCminutes = 240;
            break;
        case 8 :
            TCmoves = 1;
            TCminutes = 15;
            break;
          case 9 :
            TCmoves = 1;
            TCminutes = 60;
            break;
          case 10 :
            TCmoves = 1;
            TCminutes = 600;
            break;
    }
    TCflag = (TCmoves > 1);
    SetTimeControl();
}

/* ---- increase playing level ---- */
void cb_levelup ( void ) {
    if ( Level == 7 )
        cb_setlevel ( 1 );
    else
        cb_setlevel ( Level+1 );
    rb->splash ( HZ/2 , level_string[Level-1] );
};

/* ---- Save current position ---- */
void cb_saveposition ( void ) {
    int fd;
    short sq,i,c;
    unsigned short temp;
    
    rb->splash ( 0 , "Saving position" );

    fd = rb->open(SAVE_FILE, O_WRONLY|O_CREAT);

    computer++; rb->write(fd, &(computer), sizeof(computer)); computer--;
    opponent++; rb->write(fd, &(opponent), sizeof(opponent)); opponent--;
    rb->write(fd, &(Game50), sizeof(Game50));

    rb->write(fd, &(castld[white]), sizeof(castld[white]));
    rb->write(fd, &(castld[black]), sizeof(castld[black]));
    rb->write(fd, &(kingmoved[white]), sizeof(kingmoved[white]));
    rb->write(fd, &(kingmoved[black]), sizeof(kingmoved[black]));

    rb->write(fd, &(withbook), sizeof(withbook));
    rb->write(fd, &(Level), sizeof(Level));
    rb->write(fd, &(TCflag), sizeof(TCflag));
    rb->write(fd, &(OperatorTime), sizeof(OperatorTime));

    rb->write(fd, &(TimeControl.clock[white]),
              sizeof(TimeControl.clock[white]) );
    rb->write(fd, &(TimeControl.clock[black]),
              sizeof(TimeControl.clock[black]) );
    rb->write(fd, &(TimeControl.moves[white]),
              sizeof(TimeControl.moves[white]) );
    rb->write(fd, &(TimeControl.moves[black]),
              sizeof(TimeControl.moves[black]) );
    for (sq = 0; sq < 64; sq++) {
        if (color[sq] == neutral) c = 0; else c = color[sq]+1;
        temp = 256*board[sq] + c ;
        rb->write(fd, &(temp), sizeof(temp));
    }
    for (i = 0; i <= GameCnt; i++) {
        if (GameList[i].color == neutral)
            c = 0;
        else
            c = GameList[i].color + 1;
        rb->write(fd, &(GameList[i].gmove), sizeof(GameList[i].gmove));
        rb->write(fd, &(GameList[i].score), sizeof(GameList[i].score));
        rb->write(fd, &(GameList[i].depth), sizeof(GameList[i].depth));
        rb->write(fd, &(GameList[i].nodes), sizeof(GameList[i].nodes));
        rb->write(fd, &(GameList[i].time), sizeof(GameList[i].time));
        rb->write(fd, &(GameList[i].piece), sizeof(GameList[i].piece));
        rb->write(fd, &(c), sizeof(c));
    }
    rb->close(fd);
}

/* ---- Restore saved position ---- */
void cb_restoreposition ( void ) {
    int fd;
    int c;
    short sq;
    unsigned short m;
    
    if ( (fd = rb->open(SAVE_FILE, O_RDONLY)) >= 0 ) {
        rb->splash ( 0 , "Loading position" );
        rb->read(fd, &(computer), sizeof(computer));
        rb->read(fd, &(opponent), sizeof(opponent));
        rb->read(fd, &(Game50), sizeof(Game50));

        rb->read(fd, &(castld[white]), sizeof(castld[white]));
        rb->read(fd, &(castld[black]), sizeof(castld[black]));
        rb->read(fd, &(kingmoved[white]), sizeof(kingmoved[white]));
        rb->read(fd, &(kingmoved[black]), sizeof(kingmoved[black]));

        rb->read(fd, &(withbook), sizeof(withbook));
        rb->read(fd, &(Level), sizeof(Level));
        rb->read(fd, &(TCflag), sizeof(TCflag));
        rb->read(fd, &(OperatorTime), sizeof(OperatorTime));

        rb->read(fd, &(TimeControl.clock[white]),
                 sizeof(TimeControl.clock[white]));
        rb->read(fd, &(TimeControl.clock[black]),
                 sizeof(TimeControl.clock[black]));
        rb->read(fd, &(TimeControl.moves[white]),
                 sizeof(TimeControl.moves[white]));
        rb->read(fd, &(TimeControl.moves[black]),
                 sizeof(TimeControl.moves[black]));
        for (sq = 0; sq < 64; sq++) {
            rb->read(fd, &(m), sizeof(m));
            board[sq] = (m >> 8); color[sq] = (m & 0xFF);
            if (color[sq] == 0)
                color[sq] = neutral;
            else
                --color[sq];
        }
        GameCnt = -1; c = '?';
        while (rb->read(fd, &(GameList[++GameCnt].gmove),
                        sizeof(GameList[GameCnt].gmove)) > 0) {
            rb->read(fd, &(GameList[GameCnt].score),
                     sizeof(GameList[GameCnt].score));
            rb->read(fd, &(GameList[GameCnt].depth),
                     sizeof(GameList[GameCnt].depth));
            rb->read(fd, &(GameList[GameCnt].nodes),
                     sizeof(GameList[GameCnt].nodes));
            rb->read(fd, &(GameList[GameCnt].time),
                     sizeof(GameList[GameCnt].time));
            rb->read(fd, &(GameList[GameCnt].piece),
                     sizeof(GameList[GameCnt].piece));
            rb->read(fd, &(GameList[GameCnt].color),
                         sizeof(GameList[GameCnt].color));
            if (GameList[GameCnt].color == 0)
                GameList[GameCnt].color = neutral;
            else
                --GameList[GameCnt].color;
        }
        GameCnt--;
        if (TimeControl.clock[white] > 0)
            TCflag = true;
        computer--; opponent--;
    }
    rb->close(fd);
    cb_setlevel(Level);
    InitializeStats();
    Sdepth = 0;
}

/* ---- show menu in viewer mode---- */
static int cb_menu_viewer(void)
{
    int selection;
    int result = 0;
    bool menu_quit = false;
    
    MENUITEM_STRINGLIST(menu,"Chessbox Menu",NULL,"Restart Game",
                        "Select Other Game", "Quit");                  
    
    while(!menu_quit)
    {
        switch(rb->do_menu(&menu, &selection, NULL, false))
        {
            case 0:
                menu_quit = true;
                result = COMMAND_RESTART;
                break;
            case 1:
                result = COMMAND_SELECT;
                menu_quit = true;
                break;
            case 2:
                result = COMMAND_QUIT;
                menu_quit = true;
                break;
        }
    }
    return result;
}

/* ---- get a command in game mode ---- */
struct cb_command cb_get_viewer_command (void) {
    int button;
    struct cb_command result = { 0, {0,0,0,0,0}, 0 };
    
    /* main loop */
    while ( true ) {
        button = rb->button_get(true);
        switch (button) {
#ifdef CB_RC_QUIT
            case CB_RC_QUIT:
                result.type = COMMAND_QUIT;
                return result;
#endif
            case CB_MENU:
                result.type = cb_menu_viewer();
                return result;
            case CB_LEFT:
                result.type = COMMAND_PREV;
                return result;
            case CB_RIGHT:
                result.type = COMMAND_NEXT;
                return result;
        }
    }

}

/* ---- viewer main loop ---- */
void cb_start_viewer(char* filename){
    struct pgn_game_node *first_game, *selected_game;
    struct pgn_ply_node *curr_ply;
    bool exit_game = false;
    bool exit_viewer = false;
    struct cb_command command;

    first_game = pgn_list_games(filename);
    if (first_game == NULL){
        rb->splash ( HZ*2 , "No games found !" );
        return;
    }

    do {
        selected_game = pgn_show_game_list(first_game);
        if (selected_game == NULL){
            break;
        }

        pgn_parse_game(filename, selected_game);
        if (selected_game->first_ply != NULL) {

            /* init board */
            GNUChess_Initialize();
    
            /* draw the board */
            cb_drawboard();

            curr_ply = selected_game->first_ply;
            exit_game = false;

            do {
                command = cb_get_viewer_command ();
                switch (command.type) {
                    case COMMAND_PREV:
                        /* unapply the previous ply */
                        if (curr_ply->prev_node != NULL){
                            curr_ply = curr_ply->prev_node;
                        } else {
                            rb->splash ( HZ*2 , "At the begining of the game" );
                            break;
                        }
                        board[locn[curr_ply->row_from][curr_ply->column_from]] 
                            = board[locn[curr_ply->row_to][curr_ply->column_to]];
                        color[locn[curr_ply->row_from][curr_ply->column_from]]
                            = color[locn[curr_ply->row_to][curr_ply->column_to]];
                        board[locn[curr_ply->row_to][curr_ply->column_to]] = no_piece;
                        color[locn[curr_ply->row_to][curr_ply->column_to]] = neutral;
                        if (curr_ply->taken_piece != no_piece && !curr_ply->enpassant){
                            board[locn[curr_ply->row_to][curr_ply->column_to]]
                                = curr_ply->taken_piece;
                            color[locn[curr_ply->row_to][curr_ply->column_to]]
                                = ((curr_ply->player==white)?black:white);
                        }
                        if (curr_ply->castle){
                            if (curr_ply->column_to == 6){
                                /* castling kingside */
                                board[locn[curr_ply->row_to][7]] = rook;
                                color[locn[curr_ply->row_to][7]] = curr_ply->player;
                                board[locn[curr_ply->row_to][5]] = no_piece;
                                color[locn[curr_ply->row_to][5]] = neutral;
                            } else {
                                /* castling queenside */
                                board[locn[curr_ply->row_to][0]] = rook;
                                color[locn[curr_ply->row_to][0]] = curr_ply->player;
                                board[locn[curr_ply->row_to][3]] = no_piece;
                                color[locn[curr_ply->row_to][3]] = neutral;
                            }
                        }
                        if (curr_ply->enpassant){
                            board[locn[curr_ply->row_from][curr_ply->column_to]] = pawn;
                            color[locn[curr_ply->row_from][curr_ply->column_to]]
                                = ((curr_ply->player==white)?black:white);
                        }
                        if (curr_ply->promotion){
                            board[locn[curr_ply->row_from][curr_ply->column_from]] = pawn;
                            color[locn[curr_ply->row_from][curr_ply->column_from]]
                                = curr_ply->player;
                        }

                        cb_drawboard();
                        break;
                    case COMMAND_NEXT:
                        /* apply the current move */
                        if (curr_ply->player == neutral){
                            rb->splash ( HZ*2 , "At the end of the game" );
                            break;
                        }
                        board[locn[curr_ply->row_to][curr_ply->column_to]]
                            = board[locn[curr_ply->row_from][curr_ply->column_from]];
                        color[locn[curr_ply->row_to][curr_ply->column_to]]
                            = color[locn[curr_ply->row_from][curr_ply->column_from]];
                        board[locn[curr_ply->row_from][curr_ply->column_from]] = no_piece;
                        color[locn[curr_ply->row_from][curr_ply->column_from]] = neutral;
                        if (curr_ply->castle){
                            if (curr_ply->column_to == 6){
                                /* castling kingside */
                                board[locn[curr_ply->row_to][5]] = rook;
                                color[locn[curr_ply->row_to][5]] = curr_ply->player;
                                board[locn[curr_ply->row_to][7]] = no_piece;
                                color[locn[curr_ply->row_to][7]] = neutral;
                            } else {
                                /* castling queenside */
                                board[locn[curr_ply->row_to][3]] = rook;
                                color[locn[curr_ply->row_to][3]] = curr_ply->player;
                                board[locn[curr_ply->row_to][0]] = no_piece;
                                color[locn[curr_ply->row_to][0]] = neutral;
                            }
                        }
                        if (curr_ply->enpassant){
                            board[locn[curr_ply->row_from][curr_ply->column_to]] = no_piece;
                            color[locn[curr_ply->row_from][curr_ply->column_to]] = neutral;
                        }
                        if (curr_ply->promotion){
                            board[locn[curr_ply->row_to][curr_ply->column_to]]
                                = curr_ply->promotion_piece;
                            color[locn[curr_ply->row_to][curr_ply->column_to]]
                                = curr_ply->player;
                        }
                        if (curr_ply->next_node != NULL){
                            curr_ply = curr_ply->next_node;
                        }
                        cb_drawboard();
                        break;
                    case COMMAND_RESTART:
                        GNUChess_Initialize();
                        cb_drawboard();
                        curr_ply = selected_game->first_ply;
                    case COMMAND_SELECT:
                        exit_game = true;
                        break;
                    case COMMAND_QUIT:
                        exit_viewer = true;
                        break;
                }
            } while (!exit_game && !exit_viewer);
        } else {
            rb->splash ( HZ*2 , "Error parsing game !");
        }
    } while (!exit_viewer);
}

/* ---- show menu ---- */
static int cb_menu(void)
{
    int selection;
    int result = 0;
    bool menu_quit = false;
    
    MENUITEM_STRINGLIST(menu,"Chessbox Menu",NULL,"New Game","Resume Game",
                        "Save Game", "Restore Game",
#if (MEMORYSIZE <= 8) && !defined(SIMULATOR) /* Lowmem doesn't have playback in chessbox */
                        "Playback Control",
#endif
                                            "Quit");                  
    
    while(!menu_quit)
    {
        switch(rb->do_menu(&menu, &selection, NULL, false))
        {
            case 0:
                menu_quit = true;
                result = COMMAND_RESTART;
                break;
            case 1:
                result = COMMAND_RESUME;
                menu_quit = true;
                break;
            case 2:
                result = COMMAND_SAVE;
                menu_quit = true;
                break;
            case 3:
                result = COMMAND_RESTORE;
                menu_quit = true;
                break;
            case 4:
#if (MEMORYSIZE <= 8) && !defined(SIMULATOR) /* Lowmem doesn't have playback in chessbox */
                playback_control(NULL);
                break;
            case 5:
#endif
                result = COMMAND_QUIT;
                menu_quit = true;
                break;
        }
    }
    return result;
}

/* ---- get a command in game mode ---- */
struct cb_command cb_getcommand (void) {
    static short x = 4 , y = 3 ;
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
#ifdef CB_RC_QUIT
            case CB_RC_QUIT:
                result.type = COMMAND_QUIT;
                return result;
#endif
            case CB_MENU:
                result.type = cb_menu();
                return result;
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

/* ---- game main loop ---- */
void cb_play_game(void) {
    struct cb_command command;
    struct pgn_game_node *game;
    char move_buffer[20];

    /* init status */
    bool exit = false;

    /* load opening book, soon */

    /* init board */
    GNUChess_Initialize();

    /* init PGN history data structures */
    game = pgn_init_game();
    
    /* restore saved position, if saved */
    cb_restoreposition();
    /* TODO: save/restore the PGN history of unfinished games */
    
    /* draw the board */
    /* I don't like configscreens, start game inmediatly */
    cb_drawboard();

    while (!exit) {
        if ( mate ) {
            rb->splash ( HZ*3 , "Checkmate!" );
            rb->button_get(true);
            pgn_store_game(game);
            GNUChess_Initialize();
            game = pgn_init_game();
            cb_drawboard();
        }
        command = cb_getcommand ();
        switch (command.type) {
            case COMMAND_MOVE:
                if ( ! VerifyMove (opponent, command.mv_s , 0 , &command.mv, move_buffer ) ) {
                    rb->splash ( HZ/2 , "Illegal move!" );
                    cb_drawboard();
                } else {
                    cb_drawboard();

                    /* Add the ply to the PGN history (in algebraic notation) */
                    pgn_append_ply(game, opponent, move_buffer, mate);

                    rb->splash ( 0 , "Thinking..." );
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
                    rb->cpu_boost ( true );
#endif
                    SelectMove ( computer , 0 , cb_wt_callback, move_buffer);
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
                    rb->cpu_boost ( false );
#endif
                    /* Add the ply to the PGN history (in algebraic notation) and check
                     * for the result of the game which is only calculated in SelectMove
                     */
                    if (move_buffer[0] != '\0'){
                        pgn_append_ply(game, computer, move_buffer, mate);
                    } else {
                        pgn_set_result(game, mate);
                    }

                    if ( wt_command == COMMAND_QUIT ) {
                        exit = true;
                        break;
                    }
                    cb_drawboard();
                }
                break;
#ifdef COMMAND_RESTART
            case COMMAND_RESTART:
                GNUChess_Initialize();
                game = pgn_init_game();
                cb_drawboard();
                break;
#endif
            case COMMAND_RESUME:
                cb_drawboard();
                break;
            case COMMAND_SAVE:
                cb_saveposition();
                cb_drawboard();
                break;
            case COMMAND_RESTORE:
                /* watch out, it will reset the game if no previous game was saved! */

                /* init board */
                GNUChess_Initialize();

                /* init PGN history data structures */
                game = pgn_init_game();

                /* restore saved position, if saved */
                cb_restoreposition();

                cb_drawboard();
                break;
            case COMMAND_PLAY:
                if (opponent == white) {
                    opponent = black;
                    computer = white;
                } else {
                    opponent = white;
                    computer = black;
                }
                rb->splash ( 0 , "Thinking..." );
                ElapsedTime(1);
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
                rb->cpu_boost ( true );
#endif
                SelectMove ( computer , 0 , cb_wt_callback , move_buffer );
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
                rb->cpu_boost ( false );
#endif

                /* Add the ply to the PGN history (in algebraic notation) and check
                 * for the result of the game which is only calculated in SelectMove
                 */
                if (move_buffer[0] != '\0'){
                    pgn_append_ply(game, computer, move_buffer, mate);
                } else {
                    pgn_set_result(game, mate);
                }

                if ( wt_command == COMMAND_QUIT ) {
                    exit = true;
                    break;
                }
                cb_drawboard();
                break;
            case COMMAND_LEVEL:
                cb_levelup ( );
                cb_drawboard();
                break;
            case COMMAND_QUIT:
                exit = true;
                break;
        }
    }
    
    cb_saveposition();
    /* TODO: save/restore the PGN history of unfinished games */
    rb->lcd_setfont(FONT_UI);

}

/*****************************************************************************
* plugin entry point.
******************************************************************************/
enum plugin_status plugin_start(const void* parameter) {
    
    /* plugin init */

#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif

    /* end of plugin init */

    /* if the plugin was invoked as a viewer, parse the file and show the game list 
     * else, start playing a game
     */
    if (parameter != NULL) {
        cb_start_viewer((char *)parameter);
     } else {
        cb_play_game();
    }

    return PLUGIN_OK;
}

#endif /* HAVE_LCD_BITMAP */
