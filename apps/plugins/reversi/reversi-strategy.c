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

#include "reversi-strategy.h"
#include <stddef.h>


/* Implementation of a rather weak player strategy */
static move_t novice_move_func(const reversi_board_t *game, int color) {
    /* TODO: Implement novice_move_func */
    (void)game;
    (void)color;
    return MOVE_INVALID;
}


/* Implementation of a good player strategy */
static move_t expert_move_func(const reversi_board_t *game, int color) {
    /* TODO: Implement expert_move_func */
    (void)game;
    (void)color;
    return MOVE_INVALID;
}



/* Strategy that requires interaction with the user */
const game_strategy_t strategy_human = {
    false,
    NULL
};

/* Robot that plays not very well (novice) */
const game_strategy_t strategy_novice = {
    true,
    novice_move_func
};

/* Robot that is hard to beat (expert) */
const game_strategy_t strategy_expert = {
    true,
    expert_move_func
};
