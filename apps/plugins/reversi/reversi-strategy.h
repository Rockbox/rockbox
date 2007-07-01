/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (c) 2006 Alexander Levin
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#ifndef _REVERSI_STRATEGY_H
#define _REVERSI_STRATEGY_H

#include "reversi-game.h"


/* Function for making a move. Is called only for robots.
   Should return the game history entry for the advised move if
   a move has been considered or HISTORY_INVALID_ENTRY if no move
   has been considered. The board should not be modified. */
typedef move_t (*move_func_t)(const reversi_board_t *game, int color);
typedef void   (*init_func_t)(const reversi_board_t *game);

/* A playing party/strategy */
typedef struct _game_strategy_t {
    const bool is_robot;     /* Is the player a robot or does it require user input? */
    init_func_t init_func;   /* Initialise strategy */
    move_func_t move_func;   /* Make a move */
} game_strategy_t;


/* --- Possible playing strategies --- */
extern const game_strategy_t strategy_human;
extern const game_strategy_t strategy_naive;
extern const game_strategy_t strategy_simple;
//extern const game_strategy_t strategy_ab;

#define CHAOSNOISE 0.05 /**< Changerate of heuristic-value */
#define CHAOSPROB  0.1  /**< Probability of change of the heuristic-value */

#endif
