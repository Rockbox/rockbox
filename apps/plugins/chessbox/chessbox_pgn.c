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
* All files in this archive are subject to the GNU General Public License.
* See the file COPYING in the source tree root for full license agreement.
*
* This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
* KIND, either express or implied.
*
****************************************************************************/

#include "chessbox_pgn.h"
#include "plugin.h"

/* button definitions */
#if (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD) || \
    (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define CB_SELECT  BUTTON_SELECT
#define CB_UP      BUTTON_MENU
#define CB_DOWN    BUTTON_PLAY
#define CB_LEFT    BUTTON_LEFT
#define CB_RIGHT   BUTTON_RIGHT
#define CB_PLAY    (BUTTON_SELECT | BUTTON_PLAY)
#define CB_LEVEL   (BUTTON_SELECT | BUTTON_RIGHT)
#define CB_RESTART (BUTTON_SELECT | BUTTON_LEFT)
#define CB_MENU    (BUTTON_SELECT | BUTTON_MENU)

#define CB_SCROLL_UP     (BUTTON_SCROLL_FWD|BUTTON_REPEAT)
#define CB_SCROLL_DOWN   (BUTTON_SCROLL_BACK|BUTTON_REPEAT)
#define CB_SCROLL_LEFT   (BUTTON_LEFT|BUTTON_REPEAT)
#define CB_SCROLL_RIGHT  (BUTTON_RIGHT|BUTTON_REPEAT)

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#define CB_SELECT  BUTTON_SELECT
#define CB_UP      BUTTON_UP
#define CB_DOWN    BUTTON_DOWN
#define CB_LEFT    BUTTON_LEFT
#define CB_RIGHT   BUTTON_RIGHT
#define CB_PLAY    BUTTON_PLAY
#define CB_LEVEL   BUTTON_REC
#define CB_RESTART (BUTTON_SELECT | BUTTON_PLAY)
#define CB_MENU    BUTTON_POWER

#define CB_SCROLL_UP     (BUTTON_UP|BUTTON_REPEAT)
#define CB_SCROLL_DOWN   (BUTTON_DOWN|BUTTON_REPEAT)
#define CB_SCROLL_LEFT   (BUTTON_LEFT|BUTTON_REPEAT)
#define CB_SCROLL_RIGHT  (BUTTON_RIGHT|BUTTON_REPEAT)

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define CB_SELECT  BUTTON_SELECT
#define CB_UP      BUTTON_UP
#define CB_DOWN    BUTTON_DOWN
#define CB_LEFT    BUTTON_LEFT
#define CB_RIGHT   BUTTON_RIGHT
#define CB_PLAY    BUTTON_ON
#define CB_LEVEL   BUTTON_MODE
#define CB_RESTART BUTTON_REC
#define CB_MENU    BUTTON_OFF

#define CB_RC_QUIT BUTTON_RC_STOP

#define CB_SCROLL_UP     (BUTTON_UP|BUTTON_REPEAT)
#define CB_SCROLL_DOWN   (BUTTON_DOWN|BUTTON_REPEAT)
#define CB_SCROLL_LEFT   (BUTTON_LEFT|BUTTON_REPEAT)
#define CB_SCROLL_RIGHT  (BUTTON_RIGHT|BUTTON_REPEAT)

#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD
#define CB_SELECT  BUTTON_SELECT
#define CB_UP      BUTTON_UP
#define CB_DOWN    BUTTON_DOWN
#define CB_LEFT    BUTTON_LEFT
#define CB_RIGHT   BUTTON_RIGHT
#define CB_PLAY    BUTTON_PLAY
#define CB_LEVEL   BUTTON_EQ
#define CB_MENU    BUTTON_MODE

#define CB_SCROLL_UP     (BUTTON_UP|BUTTON_REPEAT)
#define CB_SCROLL_DOWN   (BUTTON_DOWN|BUTTON_REPEAT)
#define CB_SCROLL_LEFT   (BUTTON_LEFT|BUTTON_REPEAT)
#define CB_SCROLL_RIGHT  (BUTTON_RIGHT|BUTTON_REPEAT)

#elif CONFIG_KEYPAD == RECORDER_PAD
#define CB_SELECT  BUTTON_PLAY
#define CB_UP      BUTTON_UP
#define CB_DOWN    BUTTON_DOWN
#define CB_LEFT    BUTTON_LEFT
#define CB_RIGHT   BUTTON_RIGHT
#define CB_PLAY    BUTTON_ON
#define CB_LEVEL   BUTTON_F1
#define CB_RESTART BUTTON_F3
#define CB_MENU    BUTTON_OFF

#define CB_SCROLL_UP     (BUTTON_UP|BUTTON_REPEAT)
#define CB_SCROLL_DOWN   (BUTTON_DOWN|BUTTON_REPEAT)
#define CB_SCROLL_LEFT   (BUTTON_LEFT|BUTTON_REPEAT)
#define CB_SCROLL_RIGHT  (BUTTON_RIGHT|BUTTON_REPEAT)

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
#define CB_SELECT  BUTTON_SELECT
#define CB_UP      BUTTON_UP
#define CB_DOWN    BUTTON_DOWN
#define CB_LEFT    BUTTON_LEFT
#define CB_RIGHT   BUTTON_RIGHT
#define CB_PLAY    BUTTON_ON
#define CB_LEVEL   BUTTON_F1
#define CB_RESTART BUTTON_F3
#define CB_MENU    BUTTON_OFF

#define CB_SCROLL_UP     (BUTTON_UP|BUTTON_REPEAT)
#define CB_SCROLL_DOWN   (BUTTON_DOWN|BUTTON_REPEAT)
#define CB_SCROLL_LEFT   (BUTTON_LEFT|BUTTON_REPEAT)
#define CB_SCROLL_RIGHT  (BUTTON_RIGHT|BUTTON_REPEAT)

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
#define CB_RESTART (BUTTON_MENU|BUTTON_LEFT)
#define CB_MENU    BUTTON_OFF

#define CB_SCROLL_UP     (BUTTON_UP|BUTTON_REPEAT)
#define CB_SCROLL_DOWN   (BUTTON_DOWN|BUTTON_REPEAT)
#define CB_SCROLL_LEFT   (BUTTON_LEFT|BUTTON_REPEAT)
#define CB_SCROLL_RIGHT  (BUTTON_RIGHT|BUTTON_REPEAT)

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define CB_SELECT  BUTTON_SELECT
#define CB_UP      BUTTON_UP
#define CB_DOWN    BUTTON_DOWN
#define CB_LEFT    BUTTON_LEFT
#define CB_RIGHT   BUTTON_RIGHT
#define CB_PLAY    BUTTON_POWER
#define CB_LEVEL   BUTTON_MENU
#define CB_MENU    BUTTON_A

#define CB_SCROLL_UP     (BUTTON_UP|BUTTON_REPEAT)
#define CB_SCROLL_DOWN   (BUTTON_DOWN|BUTTON_REPEAT)
#define CB_SCROLL_LEFT   (BUTTON_LEFT|BUTTON_REPEAT)
#define CB_SCROLL_RIGHT  (BUTTON_RIGHT|BUTTON_REPEAT)

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define CB_SELECT  BUTTON_REW
#define CB_UP      BUTTON_SCROLL_UP
#define CB_DOWN    BUTTON_SCROLL_DOWN
#define CB_LEFT    BUTTON_LEFT
#define CB_RIGHT   BUTTON_RIGHT
#define CB_PLAY    BUTTON_PLAY
#define CB_LEVEL   BUTTON_FF
#define CB_RESTART (BUTTON_REW | BUTTON_PLAY)
#define CB_MENU    BUTTON_POWER

#define CB_SCROLL_UP     (BUTTON_SCROLL_UP|BUTTON_REPEAT)
#define CB_SCROLL_DOWN   (BUTTON_SCROLL_DOWN|BUTTON_REPEAT)
#define CB_SCROLL_LEFT   (BUTTON_LEFT|BUTTON_REPEAT)
#define CB_SCROLL_RIGHT  (BUTTON_RIGHT|BUTTON_REPEAT)

#elif CONFIG_KEYPAD == SANSA_E200_PAD
#define CB_SELECT  BUTTON_SELECT
#define CB_UP      BUTTON_UP
#define CB_DOWN    BUTTON_DOWN
#define CB_LEFT    BUTTON_LEFT
#define CB_RIGHT   BUTTON_RIGHT
#define CB_PLAY    (BUTTON_SELECT | BUTTON_RIGHT)
#define CB_LEVEL   BUTTON_REC
#define CB_RESTART (BUTTON_SELECT | BUTTON_REPEAT)
#define CB_MENU    BUTTON_POWER

#define CB_SCROLL_UP     (BUTTON_SCROLL_UP|BUTTON_REPEAT)
#define CB_SCROLL_DOWN   (BUTTON_SCROLL_DOWN|BUTTON_REPEAT)
#define CB_SCROLL_LEFT   (BUTTON_LEFT|BUTTON_REPEAT)
#define CB_SCROLL_RIGHT  (BUTTON_RIGHT|BUTTON_REPEAT)

#else
    #error CHESSBOX: Unsupported keypad
#endif

#define LOG_FILE  PLUGIN_DIR "/chessbox.log"
int loghandler;

struct plugin_api* rb;

short kn_offs[8][2] = {{2,1},{2,-1},{-2,1},{-2,-1},{1,2},{1,-2},{-1,2},{-1,-2}};
short rk_offs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
short bp_offs[4][2] = {{1,1},{-1,1},{1,-1},{-1,-1}};

/* global vars for pl_malloc() */
void *bufptr = NULL;
ssize_t bufleft;

/* simple function to "allocate" memory in pluginbuffer.
 * (borrowed from dict.c)
 */
void *pl_malloc(ssize_t size)
{
    void *ptr;
    ptr = bufptr;

    if (bufleft < size)
    {
        return NULL;
    }
    else
    {
        bufptr += size;
        return ptr;
    }
}

/* init function for pl_malloc() */
void pl_malloc_init(void)
{
    bufptr = rb->plugin_get_buffer((size_t *)&bufleft);
}

void process_tag(struct pgn_game_node* game, char* buffer){
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

unsigned short get_next_token(const char* line_buffer, unsigned short initial_pos,
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

unsigned short piece_from_pgn(char pgn_piece){
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

char pgn_from_piece(unsigned short piece, unsigned short color){
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

void pgn_to_coords(struct pgn_ply_node* ply){
    unsigned short str_length = rb->strlen(ply->pgn_text);
    char str[10];
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

    /* update the board */
    board[locn[ply->row_to][ply->column_to]] = board[locn[ply->row_from][ply->column_from]];
    color[locn[ply->row_to][ply->column_to]] = color[locn[ply->row_from][ply->column_from]];
    board[locn[ply->row_from][ply->column_from]] = no_piece;
    color[locn[ply->row_from][ply->column_from]] = neutral;
}

char * get_game_text(int selected_item, void *data, char *buffer){
    int i;
    struct pgn_game_node *temp_node = (struct pgn_game_node *)data;
    char text_buffer[50];

    for (i=0;i<selected_item && temp_node != NULL;i++){
        temp_node = temp_node->next_node;
    }
    if (temp_node == NULL){
        return NULL;
    }
    rb->snprintf(text_buffer, 50,"%s vs. %s (%s)", temp_node->white_player,
                         temp_node->black_player, temp_node->game_date);

    rb->strcpy(buffer, text_buffer);
    return buffer;
}

/* ---- api functions ---- */
struct pgn_game_node* pgn_list_games(struct plugin_api* api,const char* filename){
    int fhandler;
    char line_buffer[128];
    struct pgn_game_node size_node, *first_game = NULL, *curr_node = NULL, *temp_node;
    unsigned short game_count = 1;
    int line_count = 0;
    bool header_start = true, game_start = false;
    rb = api;

    if ( (fhandler = rb->open(filename, O_RDONLY)) == 0 ) return NULL;

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
            } else if (line_buffer[0] == '\r' || line_buffer[0] == '\n' || line_buffer[0] == '\0'){
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

struct pgn_game_node* pgn_show_game_list(struct plugin_api* api, struct pgn_game_node* first_game){
    int curr_selection = 0;
    int button;
    struct gui_synclist games_list;
    int i;
    struct pgn_game_node *temp_node = first_game;

    rb=api;

    for (i=0;temp_node != NULL;i++){
        temp_node = temp_node->next_node;
    }


    rb->gui_synclist_init(&games_list, &get_game_text, first_game, false, 1);
    rb->gui_synclist_set_title(&games_list, "Games", NOICON);
    rb->gui_synclist_set_icon_callback(&games_list, NULL);
    rb->gui_synclist_set_nb_items(&games_list, i);
    rb->gui_synclist_limit_scroll(&games_list, true);
    rb->gui_synclist_select_item(&games_list, 0);

    while (true) {
        rb->gui_syncstatusbar_draw(rb->statusbars, true);
        rb->gui_synclist_draw(&games_list);
        curr_selection = rb->gui_synclist_get_sel_pos(&games_list);
        button = rb->get_action(CONTEXT_LIST,TIMEOUT_BLOCK);
        if (rb->gui_synclist_do_button(&games_list,button,LIST_WRAP_OFF)){
            continue;
        }
        switch (button) {
            case ACTION_STD_OK:
                temp_node = first_game;
                for (i=0;i<curr_selection && temp_node != NULL;i++){
                    temp_node = temp_node->next_node;
                }
                return temp_node;
            break;
            case ACTION_STD_CANCEL:
                return NULL;
            break;
        }
    }
}

void pgn_parse_game(struct plugin_api* api, const char* filename, struct pgn_game_node* selected_game){
    struct pgn_ply_node size_ply, *first_ply = NULL, *temp_ply = NULL, *curr_node = NULL;
    int fhandler, i;
    char line_buffer[128];
    char token_buffer[10];
    unsigned short pos;
    unsigned short curr_player = white;
    rb = api;

    fhandler = rb->open(filename, O_RDONLY);

    /* seek the line where the pgn of the selected game starts */
    for (i=1;i<selected_game->pgn_line;i++){
        rb->read_line(fhandler, line_buffer, sizeof line_buffer);
    }

    loghandler = rb->open(LOG_FILE, O_WRONLY | O_CREAT);

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
                temp_ply->player = curr_player;
                curr_player = (curr_player==white)?black:white;
                rb->strcpy(temp_ply->pgn_text, token_buffer);
                pgn_to_coords(temp_ply);
                temp_ply->prev_node = NULL;
                temp_ply->next_node = NULL;
                if (first_ply == NULL) {
                    first_ply = curr_node = temp_ply;
                } else {
                    curr_node->next_node = temp_ply;
                    temp_ply->prev_node = curr_node;
                    curr_node = temp_ply;
                }
                rb->fdprintf(loghandler,"player: %u; pgn: %s; from: %u,%u; to: %u,%u; taken: %u.\n",
                             temp_ply->player, temp_ply->pgn_text, temp_ply->row_from, temp_ply->column_from,
                             temp_ply->row_to, temp_ply->column_to, temp_ply->taken_piece);
            }
        }
    }

    rb->close(loghandler);

    /* additional dummy ply to represent end of file without loosing the previous node's pointer */
    if (first_ply != NULL){
        temp_ply = (struct pgn_ply_node *)pl_malloc(sizeof size_ply);
        temp_ply->player = neutral;
        temp_ply->prev_node = curr_node;
        curr_node->next_node = temp_ply;
    }
    selected_game->first_ply = first_ply;
    rb->close(fhandler);
}
