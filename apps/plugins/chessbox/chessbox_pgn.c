/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2007 Mauricio Peccorini
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
#include "chessbox_pgn.h"

#define LOG_FILE  PLUGIN_GAMES_DATA_DIR  "/chessbox.log"
int loghandler;
const char *pgn_file = PLUGIN_GAMES_DATA_DIR  "/chessbox.pgn";

short kn_offs[8][2] = {{2,1},{2,-1},{-2,1},{-2,-1},{1,2},{1,-2},{-1,2},{-1,-2}};
short rk_offs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
short bp_offs[4][2] = {{1,1},{-1,1},{1,-1},{-1,-1}};

/* global vars for pl_malloc() */
void *bufptr = NULL;
size_t bufleft;

/* simple function to "allocate" memory in pluginbuffer.
 * (borrowed from dict.c)
 */
static void *pl_malloc(size_t size)
{
    void *ptr;
    ptr = bufptr;

    if (bufleft < size)
    {
        return NULL;
    }
    else
    {
        bufptr = (char*)(bufptr) + size;
        bufleft -= size;
        return ptr;
    }
}

/* init function for pl_malloc() */
static void pl_malloc_init(void)
{
    bufptr = rb->plugin_get_buffer(&bufleft);
}

static void process_tag(struct pgn_game_node* game, char* buffer){
    char tag_type[20];
    char tag_value[255];
    short pos=0, pos2=0;
    while (buffer[pos+1] != ' '){
        tag_type[pos] = buffer[pos+1];
        pos++;
    }
    tag_type[pos] = '\0';
    pos+=3;
    while (buffer[pos] != '"'){
        tag_value[pos2] = buffer[pos];
        pos++; pos2++;
    }

    /* truncate tag values that are too large */
    if (pos2 > 19){
        pos2 = 19;
    }
    tag_value[pos2] = '\0';

    if (rb->strcmp(tag_type,"White") == 0){
        rb->strcpy(game->white_player, tag_value);
    }
    if (rb->strcmp(tag_type,"Black") == 0){
        rb->strcpy(game->black_player, tag_value);
    }
    if (rb->strcmp(tag_type,"Result") == 0){
        rb->strcpy(game->result, tag_value);
    }
    if (rb->strcmp(tag_type,"Date") == 0){
        rb->strcpy(game->game_date, tag_value);
    }
}

static unsigned short get_next_token(const char* line_buffer, unsigned short initial_pos,
                              char* token_buffer){
    unsigned short pos, token_pos=0;
    for (pos = initial_pos;line_buffer[pos] == ' ' || line_buffer[pos] == '.';pos++);
    do {
        token_buffer[token_pos] = line_buffer[pos];
        pos++; token_pos++;
    } while (line_buffer[pos] != ' ' && line_buffer[pos] != '.'
             && line_buffer[pos] != '\0');
    /* ignore annotations */
    while (token_buffer[token_pos-1] == '!' || token_buffer[token_pos-1] == '?'){
        token_pos--;
    }
    token_buffer[token_pos] = '\0';
    return pos;
}

static unsigned short piece_from_pgn(char pgn_piece){
    switch (pgn_piece){
        case 'R':
            return rook;
        case 'N':
            return knight;
        case 'B':
            return bishop;
        case 'Q':
            return queen;
        case 'K':
            return king;
    }
    return no_piece;
}

static char pgn_from_piece(unsigned short piece, unsigned short color){
    char pgn_piece = ' ';
    switch (piece){
        case pawn:
            pgn_piece = 'P';
            break;
        case rook:
            pgn_piece = 'R';
            break;
        case knight:
            pgn_piece = 'N';
            break;
        case bishop:
            pgn_piece = 'B';
            break;
        case queen:
            pgn_piece = 'Q';
            break;
        case king:
            pgn_piece = 'K';
            break;
        case no_piece:
            pgn_piece = ' ';
            break;
    }
    if (color == black && pgn_piece != ' '){
        pgn_piece += 32;
    }
    return pgn_piece;
}

static bool pgn_to_coords(struct pgn_ply_node* ply){
    bool success = true;
    unsigned short str_length = rb->strlen(ply->pgn_text);
    char str[9];
    rb->strcpy(str,ply->pgn_text);
    ply->column_from = 0xFF;
    ply->row_from = 0xFF;
    ply->column_to = 0xFF;
    ply->row_to = 0xFF;
    ply->taken_piece = no_piece;
    ply->promotion_piece = no_piece;
    ply->enpassant = false;
    ply->castle = false;
    ply->promotion = false;

    unsigned short i, j, piece;
    bool found = false;

    if (str_length >= 3 && (str[0] == 'O' || str[0] == '0') && str[1] == '-'
        && (str[2] == 'O' || str[2] == '0')) {
        /* castling */
        ply->castle = true;
        if (str_length >= 5 && str[3] == '-' && (str[4] == 'O' || str[4] == '0')){
            /* castle queenside */
            if (ply->player == white){
                ply->row_from = 0; ply->column_from = 4;
                ply->row_to = 0; ply->column_to = 2;
                /* update the rook's position, the king's position will be updated later */
                board[locn[0][3]] = rook; color[locn[0][3]] = white;
                board[locn[0][0]] = no_piece; color[locn[0][0]] = neutral;
            } else {
                ply->row_from = 7; ply->column_from = 4;
                ply->row_to = 7; ply->column_to = 2;
                board[locn[7][3]] = rook; color[locn[7][3]] = black;
                board[locn[7][0]] = no_piece; color[locn[7][0]] = neutral;
            }
        } else {
            /* castle kingside */
            if (ply->player == white){
                ply->row_from = 0; ply->column_from = 4;
                ply->row_to = 0; ply->column_to = 6;
                board[locn[0][5]] = rook; color[locn[0][5]] = white;
                board[locn[0][7]] = no_piece; color[locn[0][7]] = neutral;
            } else {
                ply->row_from = 7; ply->column_from = 4;
                ply->row_to = 7; ply->column_to = 6;
                board[locn[7][5]] = rook; color[locn[7][5]] = black;
                board[locn[7][7]] = no_piece; color[locn[7][7]] = neutral;
            }
        }
    } else if (str[0] >= 'a' && str[0] <= 'h'){
        /* pawns */
        ply->column_from = str[0] - 'a';
        if (str[1] == 'x'){
            ply->row_from = str[3] - '1' + ((ply->player==white)?-1:1);
            ply->row_to = str[3] - '1';
            ply->column_to = str[2] - 'a';
            if (board[locn[ply->row_to][ply->column_to]] == no_piece){
                /* en-passant, remove the pawn */
                ply->enpassant = true;
                board[locn[ply->row_from][ply->column_to]] = no_piece;
                color[locn[ply->row_from][ply->column_to]] = neutral;
                ply->taken_piece = pawn;
            } else {
                ply->taken_piece = board[locn[ply->row_to][ply->column_to]];
            }
        } else {
            ply->column_to = ply->column_from;
            ply->row_from = str[1] - '1' + ((ply->player==white)?-1:1);
            ply->row_to = str[1] - '1';
        }
        if (board[locn[ply->row_from][ply->column_from]] == no_piece){
            /* the pawn moved two squares */
            ply->row_from += ((ply->player==white)?-1:1);
        }
        if (ply->row_to == 7 || ply->row_to == 0){
            /* promotion */
            if (str[2] == '='){
                ply->promotion_piece = piece_from_pgn(str[3]);
            } else {
                ply->promotion_piece = piece_from_pgn(str[5]);
            }
            /* change the piece in the original position and wait
             * for the code at the end to move it
             */
            board[locn[ply->row_from][ply->column_from]] = ply->promotion_piece;
        }
    } else {
        /* the other pieces */
        piece = piece_from_pgn(str[0]);
        if (str[2] == 'x'){
            /* taken a piece and move was ambiguous */
            ply->column_to = str[3] - 'a';
            ply->row_to = str[4] - '1';
            ply->taken_piece = board[locn[ply->row_to][ply->column_to]];
            if (str[1] >= 'a' && str[1] <= 'h') {
                ply->column_from = str[1] - 'a';
            } else {
                ply->row_from = str[1] - '1';
            }
        } else if (str[1] == 'x') {
            /* taken a piece */
            ply->column_to = str[2] - 'a';
            ply->row_to = str[3] - '1';
            ply->taken_piece = board[locn[ply->row_to][ply->column_to]];
        } else if (str_length >= 4 && str[3] >= '0' && str[3] <= '9'){
            /* no piece taken and move was ambiguous */
                ply->column_to = str[2] - 'a';
            ply->row_to = str[3] - '1';
            if (str[1] >= 'a' && str[1] <= 'h') {
                ply->column_from = str[1] - 'a';
            } else {
                ply->row_from = str[1] - '1';
            }
        } else {
            /* regular move */
            ply->column_to = str[1] - 'a';
            ply->row_to = str[2] - '1';
        }
        if (piece == knight) {
            for (i=0;i<8;i++){
                if (ply->row_to + kn_offs[i][0] >= 0 && ply->row_to + kn_offs[i][0] <= 7
                    && ply->column_to + kn_offs[i][1] >= 0 && ply->column_to + kn_offs[i][1] <= 7
                    && board[locn[ply->row_to + kn_offs[i][0]][ply->column_to + kn_offs[i][1]]] == knight
                    && color[locn[ply->row_to + kn_offs[i][0]][ply->column_to + kn_offs[i][1]]] == ply->player
                    && (ply->row_from == 0xFF || ply->row_from == ply->row_to + kn_offs[i][0])
                    && (ply->column_from == 0xFF || ply->column_from == ply->column_to + kn_offs[i][1])) {
                    ply->row_from = ply->row_to + kn_offs[i][0];
                    ply->column_from = ply->column_to + kn_offs[i][1];
                }
            }
        }
        if (piece == rook || piece == queen || piece == king){
            for (i=0;i<4;i++){
                j = 1;
                found = false;
                while (ply->row_to+(j*rk_offs[i][0]) >= 0 && ply->row_to+(j*rk_offs[i][0]) <= 7 &&
                       ply->column_to+(j*rk_offs[i][1]) >= 0 && ply->column_to+(j*rk_offs[i][1]) <= 7){
                    if (board[locn[ply->row_to+(j*rk_offs[i][0])][ply->column_to+(j*rk_offs[i][1])]] != no_piece) {
                        if (board[locn[ply->row_to+(j*rk_offs[i][0])][ply->column_to+(j*rk_offs[i][1])]] == piece &&
                            color[locn[ply->row_to+(j*rk_offs[i][0])][ply->column_to+(j*rk_offs[i][1])]] == ply->player &&
                            (ply->row_from == 0xFF || ply->row_from == ply->row_to+(j*rk_offs[i][0])) &&
                            (ply->column_from == 0xFF || ply->column_from == ply->column_to+(j*rk_offs[i][1]))) {
                            ply->row_from = ply->row_to+(j*rk_offs[i][0]);
                            ply->column_from = ply->column_to+(j*rk_offs[i][1]);
                            found = true;
                        }
                        break;
                    }
                    j++;
                }
                if (found) {
                    break;
                }
            }
        }
        if (piece == bishop || ((piece == queen || piece == king) && !found)){
            for (i=0;i<4;i++){
                j = 1;
                found = false;
                while (ply->row_to+(j*bp_offs[i][0]) >= 0 && ply->row_to+(j*bp_offs[i][0]) <= 7 &&
                       ply->column_to+(j*bp_offs[i][1]) >= 0 && ply->column_to+(j*bp_offs[i][1]) <= 7){
                    if (board[locn[ply->row_to+(j*bp_offs[i][0])][ply->column_to+(j*bp_offs[i][1])]] != no_piece) {
                        if (board[locn[ply->row_to+(j*bp_offs[i][0])][ply->column_to+(j*bp_offs[i][1])]] == piece &&
                            color[locn[ply->row_to+(j*bp_offs[i][0])][ply->column_to+(j*bp_offs[i][1])]] == ply->player &&
                            (ply->row_from == 0xFF || ply->row_from == ply->row_to+(j*bp_offs[i][0])) &&
                            (ply->column_from == 0xFF || ply->column_from == ply->column_to+(j*bp_offs[i][1]))) {
                            ply->row_from = ply->row_to+(j*bp_offs[i][0]);
                            ply->column_from = ply->column_to+(j*bp_offs[i][1]);
                            found = true;
                        }
                        break;
                    }
                    j++;
                }
                if (found) {
                    break;
                }
            }
        }
    }

    /* leave a very complete log of the parsing of the game while it gets stable */
    for (i=0;i<8;i++){
        for (j=0;j<8;j++) {

            rb->fdprintf(loghandler,"%c",pgn_from_piece(board[locn[7-i][j]],color[locn[7-i][j]]));
        }
        rb->fdprintf(loghandler,"\n");
    }
    /* check bounds of row and columns should be 0-7 bad .pgn returns 0xFF */
    if ((ply->row_to | ply->column_to | ply->row_from | ply->column_from) < 8)
    {
    /* update the board */
        board[locn[ply->row_to][ply->column_to]] =
                                   board[locn[ply->row_from][ply->column_from]];
        color[locn[ply->row_to][ply->column_to]] =
                                    color[locn[ply->row_from][ply->column_from]];
        board[locn[ply->row_from][ply->column_from]] = no_piece;
        color[locn[ply->row_from][ply->column_from]] = neutral;
    }
    else
        success = false; /*ERROR*/
    return success;
}

static void coords_to_pgn(struct pgn_ply_node* ply){
    int pos = 0,i,j;
    unsigned short moving_piece = board[locn[ply->row_from][ply->column_from]];
    char unambiguous_position;
    bool found = false;
    char alg_move[5];
    char move_buffer[10];
    short move;
    if (moving_piece == king){
        /* check castling */
        if (ply->column_from == 4 && ply->column_to == 6){
            /* castling kingside */
            rb->strcpy(ply->pgn_text,"O-O");
            ply->castle = true;
        } else if (ply->column_from == 4 && ply->column_to == 2){
            /* castling queenside */
            rb->strcpy(ply->pgn_text,"O-O-O");
        } else {
            if (board[locn[ply->row_to][ply->column_to]] != no_piece){
                rb->snprintf(ply->pgn_text,10,"Kx%c%c",'a'+ply->column_to,
                                   '1'+ply->row_to);
            } else {
                rb->snprintf(ply->pgn_text,10,"K%c%c",'a'+ply->column_to,
                                   '1'+ply->row_to);
            }
        }
    } else if (moving_piece == pawn){
        if (ply->column_from != ply->column_to){
            /* check enpassant */
            if (board[locn[ply->row_to][ply->column_to]] == no_piece){
                ply->enpassant = true;
            }
            /* check promotions when taking a piece */
            if (ply->row_to == 0 || ply->row_to == 7) {
                ply->promotion = true;
                ply->promotion_piece = queen;
                rb->snprintf(ply->pgn_text,10,"%cx%c%c=D", 'a'+ply->column_from,
                                   'a'+ply->column_to,'1'+ply->row_to);
            } else {
                rb->snprintf(ply->pgn_text,10,"%cx%c%c", 'a'+ply->column_from,
                                   'a'+ply->column_to,'1'+ply->row_to);
            }
        } else {
            /* check promotions when not taking a piece */
            if (ply->row_to == 0 || ply->row_to == 7) {
                ply->promotion = true;
                ply->promotion_piece = queen;
                rb->snprintf(ply->pgn_text,10,"%c%c=D", 'a'+ply->column_to,
                                   '1'+ply->row_to);
            } else {
                rb->snprintf(ply->pgn_text,10,"%c%c", 'a'+ply->column_to,
                                   '1'+ply->row_to);
            }
        }
    } else {
        /* verify ambiguous moves for the different kinds of pieces */
        unambiguous_position = '\0';
        if (moving_piece == knight){
            for (i=0;i<8;i++){
                if (ply->row_to + kn_offs[i][0] >= 0 && ply->row_to + kn_offs[i][0] <= 7
                    && ply->column_to + kn_offs[i][1] >= 0 && ply->column_to + kn_offs[i][1] <= 7
                    && board[locn[ply->row_to + kn_offs[i][0]][ply->column_to + kn_offs[i][1]]] == knight
                    && color[locn[ply->row_to + kn_offs[i][0]][ply->column_to + kn_offs[i][1]]] == ply->player
                    && (ply->row_to + kn_offs[i][0] != ply->row_from
                        || ply->column_to + kn_offs[i][1] != ply->column_from)){
                    if (ply->row_to + kn_offs[i][0] != ply->row_from){
                        unambiguous_position = '1' + ply->row_from;
                    } else {
                        unambiguous_position = 'a' + ply->column_from;
                    }
                    break;
                }
            }
        }
        if (moving_piece == rook || moving_piece == queen){
            found = false;
            for (i=0;i<4;i++){
                j=1;
                while (ply->row_to+(j*rk_offs[i][0]) >= 0 && ply->row_to+(j*rk_offs[i][0]) <= 7 &&
                       ply->column_to+(j*rk_offs[i][1]) >= 0 && ply->column_to+(j*rk_offs[i][1]) <= 7){
                    if (board[locn[ply->row_to+(j*rk_offs[i][0])][ply->column_to+(j*rk_offs[i][1])]] != no_piece) {
                        if (board[locn[ply->row_to+(j*rk_offs[i][0])][ply->column_to+(j*rk_offs[i][1])]] == moving_piece &&
                            color[locn[ply->row_to+(j*rk_offs[i][0])][ply->column_to+(j*rk_offs[i][1])]] == ply->player &&
                            (ply->row_to+(j*rk_offs[i][0]) != ply->row_from
                             || ply->column_to+(j*rk_offs[i][1]) != ply->column_from)) {
                            if (ply->row_to+(j*rk_offs[i][0]) != ply->row_from){
                                unambiguous_position = '1' + ply->row_from;
                            } else {
                                unambiguous_position = 'a' + ply->column_from;
                            }
                            found = true;
                        }
                        break;
                    }
                    j++;
                }
                if (found) {
                    break;
                }
            }
        }
        if (moving_piece == bishop || (moving_piece == queen && !found)){
            for (i=0;i<4;i++){
                j=1;
                while (ply->row_to+(j*bp_offs[i][0]) >= 0 && ply->row_to+(j*bp_offs[i][0]) <= 7 &&
                       ply->column_to+(j*bp_offs[i][1]) >= 0 && ply->column_to+(j*bp_offs[i][1]) <= 7){
                    if (board[locn[ply->row_to+(j*bp_offs[i][0])][ply->column_to+(j*bp_offs[i][1])]] != no_piece) {
                        if (board[locn[ply->row_to+(j*bp_offs[i][0])][ply->column_to+(j*bp_offs[i][1])]] == moving_piece &&
                            color[locn[ply->row_to+(j*bp_offs[i][0])][ply->column_to+(j*bp_offs[i][1])]] == ply->player &&
                            (ply->row_to+(j*bp_offs[i][0]) != ply->row_from
                             || ply->column_to+(j*bp_offs[i][1]) != ply->column_from)) {
                            if (ply->row_to+(j*bp_offs[i][0]) != ply->row_from){
                                unambiguous_position = '1' + ply->row_from;
                            } else {
                                unambiguous_position = 'a' + ply->column_from;
                            }
                            found = true;
                        }
                        break;
                    }
                    j++;
                }
                if (found) {
                    break;
                }
            }
        }
        /* generate the first portion of the PGN text
         * always as white so all uppercase, black/white considerations
         * will be useful for FEN notation but not in this case
         */
        if (unambiguous_position == '\0'){
            if (board[locn[ply->row_to][ply->column_to]] != no_piece){
                rb->snprintf(ply->pgn_text,10,"%cx%c%c",
                                   pgn_from_piece(moving_piece,white) ,
                                   'a'+ply->column_to, '1'+ply->row_to);
            } else {
                rb->snprintf(ply->pgn_text,10,"%c%c%c",
                                   pgn_from_piece(moving_piece,white) ,
                                   'a'+ply->column_to, '1'+ply->row_to);
            }
        } else {
            if (board[locn[ply->row_to][ply->column_to]] != no_piece){
                rb->snprintf(ply->pgn_text,10,"%c%cx%c%c",
                                   pgn_from_piece(moving_piece,white) ,
                                   unambiguous_position, 'a'+ply->column_to,
                                   '1'+ply->row_to);
            } else {
                rb->snprintf(ply->pgn_text,10,"%c%c%c%c",
                                   pgn_from_piece(moving_piece,white) ,
                                   unambiguous_position, 'a'+ply->column_to,
                                   '1'+ply->row_to);
            }
        }
    }
    /* update the board */
    rb->snprintf(alg_move,5,"%c%c%c%c",'a' + ply->column_from, '1' + ply->row_from,
                'a' + ply->column_to, '1' + ply->row_to);
    /* The function returns false if the move is invalid, but since we're
     * replaying the game, that should not be posible
     */
    VerifyMove (ply->player, alg_move , 0 , &move, move_buffer );

    /* add check/mate indicators */
    for (pos=0;ply->pgn_text[pos] != '\0';pos++);
    if (ply->checkmate) {
        ply->pgn_text[pos] = '#'; pos++;
        ply->pgn_text[pos] = '\0'; pos++;
    } else if (move_buffer[4] == '+'){
        ply->pgn_text[pos] = '+'; pos++;
        ply->pgn_text[pos] = '\0'; pos++;
    }
}

static struct pgn_game_node *get_game_info(int selected_item, void *data){
    int i;
    struct pgn_game_node *temp_node = (struct pgn_game_node *)data;

    for (i=0;i<selected_item && temp_node != NULL;i++){
        temp_node = temp_node->next_node;
    }

    return temp_node;
}

static const char* get_game_text(int selected_item, void *data,
                                 char *buffer, size_t buffer_len){
    struct pgn_game_node *temp_node = get_game_info(selected_item, data);

    if (temp_node == NULL){
        return NULL;
    }
    rb->snprintf(buffer, buffer_len,"%s vs. %s (%s)", temp_node->white_player,
                         temp_node->black_player, temp_node->game_date);

    return buffer;
}

static void say_player(const char *name, bool enqueue) {
    if (!rb->strcasecmp(name, "player"))
        rb->talk_id(VOICE_PLAYER, enqueue);
    else if (!rb->strcasecmp(name, "gnuchess"))
        rb->talk_id(VOICE_GNUCHESS, enqueue);
    else
        rb->talk_spell(name, enqueue);
}

static int speak_game_selection(int selected_item, void *data){
    struct pgn_game_node *temp_node = get_game_info(selected_item, data);

    if (temp_node != NULL){
        say_player(temp_node->white_player, false);
        say_player(temp_node->black_player, true);
        if (temp_node->game_date[0] != '?') {
            char buf[12];
            rb->strcpy(buf, temp_node->game_date);
            buf[4] = 0;
            buf[7] = 0;
            rb->talk_id(LANG_MONTH_JANUARY + rb->atoi(&(buf[5])) - 1, true);
            rb->talk_number(rb->atoi(&(buf[8])), true);
            rb->talk_number(rb->atoi(buf), true);
        }
    }

    return 0;
}

static void write_pgn_token(int fhandler, char *buffer, size_t *line_length){
    if (*line_length + rb->strlen(buffer) + 1 > 80){
        rb->fdprintf(fhandler,"\n");
        *line_length = 0;
    }
    rb->fdprintf(fhandler,"%s ",buffer);
    *line_length += (rb->strlen(buffer) + 1);
}

/* ---- api functions ---- */
struct pgn_game_node* pgn_list_games(const char* filename){
    int fhandler;
    char line_buffer[128];
    struct pgn_game_node size_node, *first_game = NULL;
    struct pgn_game_node *curr_node = NULL, *temp_node;
    unsigned short game_count = 1;
    int line_count = 0;
    bool header_start = true, game_start = false;

    if ( (fhandler = rb->open(filename, O_RDONLY)) < 0 ) return NULL;

    if (bufptr == NULL){
        pl_malloc_init();
    }
    while (rb->read_line(fhandler, line_buffer, sizeof line_buffer) > 0){
        line_count++;
        /* looking for a game header */
        if (header_start) {
            /* a new game header is found */
            if (line_buffer[0] == '['){
                temp_node = (struct pgn_game_node *)pl_malloc(sizeof size_node);
                temp_node->next_node = NULL;
                if (curr_node == NULL) {
                    first_game = curr_node = temp_node;
                } else {
                    curr_node->next_node = temp_node;
                    curr_node = temp_node;
                }
                process_tag(curr_node, line_buffer);
                curr_node->game_number = game_count;
                curr_node->pgn_line = 0;
                game_count++;
                header_start = false;
                game_start = true;
            }
        } else {
            if (line_buffer[0] == '['){
                process_tag(curr_node, line_buffer);
            } else if (line_buffer[0] == '\r'
                       || line_buffer[0] == '\n'
                       || line_buffer[0] == '\0'){
                if (game_start) {
                    game_start = false;
                } else {
                    header_start = true;
                }
            } else {
                if (curr_node->pgn_line == 0) {
                    curr_node->pgn_line = line_count;
                }
            }
        }
    }
    rb->close(fhandler);
    return first_game;
}

struct pgn_game_node* pgn_show_game_list(struct pgn_game_node* first_game){
    int curr_selection = 0;
    int button;
    struct gui_synclist games_list;
    int i;
    struct pgn_game_node *temp_node = first_game;

    for (i=0;temp_node != NULL;i++){
        temp_node = temp_node->next_node;
    }


    rb->gui_synclist_init(&games_list, &get_game_text, first_game, false, 1, NULL);
    rb->gui_synclist_set_title(&games_list, rb->str(LANG_CHESSBOX_GAMES), NOICON);
    rb->gui_synclist_set_icon_callback(&games_list, NULL);
    if (rb->global_settings->talk_menu)
        rb->gui_synclist_set_voice_callback(&games_list, speak_game_selection);
    rb->gui_synclist_set_nb_items(&games_list, i);
    rb->gui_synclist_limit_scroll(&games_list, true);
    rb->gui_synclist_select_item(&games_list, 0);

    rb->gui_synclist_draw(&games_list);
    rb->gui_synclist_speak_item(&games_list);

    while (true) {
        curr_selection = rb->gui_synclist_get_sel_pos(&games_list);
        button = rb->get_action(CONTEXT_LIST,TIMEOUT_BLOCK);
        if (rb->gui_synclist_do_button(&games_list,&button,LIST_WRAP_OFF)){
            continue;
        }
        switch (button) {
            case ACTION_STD_OK:
                return get_game_info(curr_selection, first_game);
            break;
            case ACTION_STD_CANCEL:
                return NULL;
            break;
        }
    }
}

void pgn_parse_game(const char* filename,
                    struct pgn_game_node* selected_game){
    struct pgn_ply_node size_ply, *first_ply = NULL;
    struct pgn_ply_node *temp_ply = NULL, *curr_node = NULL;
    bool success = true;
    int fhandler, i;
    char line_buffer[128];
    char token_buffer[9];
    unsigned short pos;
    unsigned short curr_player = white;

    fhandler = rb->open(filename, O_RDONLY);

    /* seek the line where the pgn of the selected game starts */
    for (i=1;i<selected_game->pgn_line;i++){
        rb->read_line(fhandler, line_buffer, sizeof line_buffer);
    }

    loghandler = rb->open(LOG_FILE, O_WRONLY | O_CREAT, 0666);

    GNUChess_Initialize();

    while (rb->read_line(fhandler, line_buffer, sizeof line_buffer) > 0){
        if (line_buffer[0] == '\r' || line_buffer[0] == '\n' || line_buffer[0] == '\0'){
            break;
        }
        pos = 0;
        while (pos < rb->strlen(line_buffer)){
            pos = get_next_token(line_buffer, pos, token_buffer);
            if ((token_buffer[0] >= 'A' && token_buffer[0] <= 'Z')
                || (token_buffer[0] >= 'a' && token_buffer[0] <= 'z')
                || (token_buffer[0] == '0' && token_buffer[2] != '1')){
                temp_ply = (struct pgn_ply_node *)pl_malloc(sizeof size_ply);
                /* Null pointer can be returned from pl_malloc check for this */
                if (temp_ply)
                {
                    temp_ply->player = curr_player;
                    curr_player = (curr_player==white)?black:white;
                    rb->strcpy(temp_ply->pgn_text, token_buffer);
                    success = pgn_to_coords(temp_ply);
                    temp_ply->prev_node = NULL;
                    temp_ply->next_node = NULL;
                    if (first_ply == NULL && success) {
                        first_ply = curr_node = temp_ply;
                    } else if (success){
                        curr_node->next_node = temp_ply;
                        temp_ply->prev_node = curr_node;
                        curr_node = temp_ply;
                    } else{
                       /* bad .pgn break loop and notify user */
                       first_ply = NULL;
                        break;
                    }

                    rb->fdprintf(loghandler,
                        "player: %u; pgn: %s; from: %u,%u; to: %u,%u; taken: %u.\n",
                             temp_ply->player, temp_ply->pgn_text, temp_ply->row_from,
                             temp_ply->column_from, temp_ply->row_to,
                             temp_ply->column_to, temp_ply->taken_piece);
                }
                else
                    first_ply = NULL;
            }
        }
    }

    rb->close(loghandler);

    /* additional dummy ply to represent end of file without
     *loosing the previous node's pointer
     */
    if (first_ply != NULL){
        temp_ply = (struct pgn_ply_node *)pl_malloc(sizeof size_ply);
        temp_ply->player = neutral;
        temp_ply->prev_node = curr_node;
        curr_node->next_node = temp_ply;
    }
    selected_game->first_ply = first_ply;

    rb->close(fhandler);
}

struct pgn_game_node* pgn_init_game(void){
    struct pgn_game_node game_size, *game;
    struct pgn_ply_node ply_size, *ply;
    struct tm *current_time;

    if (bufptr == NULL){
        pl_malloc_init();
    }

    /* create an "end of game" dummy ply and assign defaults */
    ply = (struct pgn_ply_node *)pl_malloc(sizeof ply_size);
    ply->player = neutral;
    ply->pgn_text[0] = '\0';
    ply->prev_node = NULL;
    ply->next_node = NULL;

    /* create the game and assign defaults */
    game = (struct pgn_game_node *)pl_malloc(sizeof game_size);
    game->game_number = 0;
    rb->strcpy(game->white_player,"Player");
    rb->strcpy(game->black_player,"GnuChess");
    current_time = rb->get_time();
    if (current_time->tm_year < 100){
        rb->snprintf(game->game_date,11,"????.??.??");
    } else {
        rb->snprintf(game->game_date,11,"%4u.%2u.%2u",current_time->tm_year + 1900,
                     current_time->tm_mon + 1, current_time->tm_mday);
    }
    rb->strcpy(game->result,"*");
    game->pgn_line = 0;
    game->first_ply = ply;
    game->next_node = NULL;

    return game;
}

void pgn_append_ply(struct pgn_game_node* game,
                    unsigned short ply_player, char *move_buffer, bool is_mate){
    struct pgn_ply_node ply_size, *ply, *temp;

    ply = (struct pgn_ply_node *)pl_malloc(sizeof ply_size);
    ply->player = ply_player;
    ply->column_from = move_buffer[0] - 'a';
    ply->row_from = move_buffer[1] - '1';
    ply->column_to = move_buffer[2] - 'a';
    ply->row_to = move_buffer[3] - '1';
    ply->castle = false;
    ply->promotion = false;
    ply->enpassant = false;
    ply->promotion_piece = no_piece;
    ply->taken_piece = no_piece;
    ply->draw = false;
    ply->checkmate = is_mate;

    /* move the pointer to the "end of game" marker ply */
    for (temp=game->first_ply;temp->next_node!=NULL;temp=temp->next_node);

    /* arrange the pointers to insert the ply before the marker */
    ply->next_node = temp;
    ply->prev_node = temp->prev_node;
    if (temp->prev_node == NULL){
        game->first_ply = ply;
    } else {
        temp->prev_node->next_node = ply;
    }
    temp->prev_node = ply;
}

void pgn_set_result(struct pgn_game_node* game,
                    bool is_mate){

    struct pgn_ply_node *ply;
    for(ply=game->first_ply;ply->next_node != NULL;ply=ply->next_node);
    if (is_mate){
        ply->prev_node->checkmate = true;
    } else {
        ply->prev_node->draw = true;
    }
}

void pgn_store_game(struct pgn_game_node* game){
    int fhandler;
    struct pgn_ply_node *ply;
    unsigned ply_count;
    size_t line_length=0;
    char buffer[10];

    GNUChess_Initialize();

    ply_count=0;
    ply=game->first_ply;
    while (ply->next_node!=NULL){
        coords_to_pgn(ply);
        if (ply->checkmate){
            if (ply->player == white){
                rb->strcpy(game->result,"1-0");
            } else {
                rb->strcpy(game->result,"0-1");
            }
        }
        if (ply->draw){
            rb->strcpy(game->result,"1/2-1/2");
        }
        ply=ply->next_node;
        ply_count++;
    }

    fhandler = rb->open(pgn_file, O_WRONLY|O_CREAT|O_APPEND, 0666);


    /* the first 7 tags are mandatory according to the PGN specification so we
     * have to include them even if the values don't make much sense
     */
    rb->fdprintf(fhandler,"[Event \"Casual Game\"]\n");
    rb->fdprintf(fhandler,"[Site \"?\"]\n");
    rb->fdprintf(fhandler,"[Date \"%s\"]\n",game->game_date);
    rb->fdprintf(fhandler,"[Round \"?\"]\n");
    rb->fdprintf(fhandler,"[White \"%s\"]\n",game->white_player);
    rb->fdprintf(fhandler,"[Black \"%s\"]\n",game->black_player);
    rb->fdprintf(fhandler,"[Result \"%s\"]\n",game->result);
    rb->fdprintf(fhandler,"[PlyCount \"%u\"]\n",ply_count);

    /* leave a blank line between the tag section and the game section */
    rb->fdprintf(fhandler,"\n");

    /* write the plies in several lines of up to 80 characters */
    for (ply_count=0, ply=game->first_ply;ply->next_node!=NULL;
         ply=ply->next_node,ply_count++){
        /* write the move number */
        if (ply->player == white){
            rb->snprintf(buffer,10,"%u.",(ply_count/2)+1);
            write_pgn_token(fhandler, buffer, &line_length);
        }
        /* write the actual move */
        write_pgn_token(fhandler,ply->pgn_text,&line_length);
        /* write the result of the game at the end */
        if (ply->checkmate){
            if (ply->player == white){
                write_pgn_token(fhandler,"1-0",&line_length);
            } else {
                write_pgn_token(fhandler,"0-1",&line_length);
            }
            break;
        } else if (ply->draw){
            write_pgn_token(fhandler,"1/2-1/2",&line_length);
            break;
        } else if (ply->next_node->player == neutral) {
            /* unknown end of the game */
            write_pgn_token(fhandler,"*",&line_length);
            break;
        }
    }

    /* leave a blank line between the tag section and the game section */
    rb->fdprintf(fhandler,"\n\n");

    rb->close(fhandler);
}
