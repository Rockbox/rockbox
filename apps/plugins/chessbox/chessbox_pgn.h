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

#include "plugin.h"
#include "gnuchess.h"

/* structure to represent the plies */
struct pgn_ply_node {
    unsigned short player;
    char pgn_text[9];
    unsigned short row_from;
    unsigned short column_from;
    unsigned short row_to;
    unsigned short column_to;
    unsigned short taken_piece;
    unsigned short promotion_piece;
    bool enpassant;
    bool castle;
    bool promotion;
    bool checkmate;
    bool draw;
    struct pgn_ply_node* prev_node;
    struct pgn_ply_node* next_node;
};

/* structure to list the games */
struct pgn_game_node {
    unsigned short game_number;
    char white_player[20];
    char black_player[20];
    char game_date[11];
    char result[8];
    int pgn_line;
    struct pgn_ply_node* first_ply;
    struct pgn_game_node* next_node;
};

/* Return the list of games in a PGN file.
 * Parsing of the games themselves is postponed until
 * the user selects a game, that obviously saves processing
 * and speeds up response when the user selects the file
 */
struct pgn_game_node* pgn_list_games(struct plugin_api* api,
                                     const char* filename);

/* Show the list of games found in a file and allow the user
 * to select a game to be parsed and showed
 */
struct pgn_game_node* pgn_show_game_list(struct plugin_api* api,
                                         struct pgn_game_node* first_game);

/* Parse the pgn string of a game and assign it to the move
 * list in the structure
 */
void pgn_parse_game(struct plugin_api* api, const char* filename,
                    struct pgn_game_node* selected_game);

/* Initialize a new game structure with default values and make
 * it ready to store the history of a newly played match
 */
struct pgn_game_node* pgn_init_game(struct plugin_api* api);

/* Add a new ply to the game structure based on the positions */
void pgn_append_ply(struct plugin_api* api, struct pgn_game_node* game,
                    unsigned short ply_player, char *move_buffer, bool is_mate);

/* Set the result of the game if it was reached during the opponent's ply
 */
void pgn_set_result(struct plugin_api* api, struct pgn_game_node* game,
                    bool is_mate);

/* Store a complete game in the PGN history file
 */
void pgn_store_game(struct plugin_api* api, struct pgn_game_node* game);
