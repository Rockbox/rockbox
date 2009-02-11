/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007-2009 Joshua Simmons <mud at majidejima dot com>
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

#ifndef GOBAN_SGF_H
#define GOBAN_SGF_H

#include "types.h"

/* Play one move.  If play_mode is not PLAY_MODE_FORCE, this will fail if
   an illegal move is played (plays by the "wrong" color are not counted
   as illegal. pos may be a POS() on the board, or PASS_POS. Returns true
   if the move was successfully played. */
bool play_move_sgf (unsigned short pos, unsigned char color);

/* Add a stone to the board, or remove one if color == EMPTY. returns true
   if the stone is successfully added/removed (will return false if there
   is nothing to add/remove there, eg. when adding a black stone when a
   black stone is already there) */
bool add_stone_sgf (unsigned short pos, unsigned char color);

/* Add a mark to the board at the current node pos must be on the board
   Returns false on failure. */
bool add_mark_sgf (unsigned short pos, enum prop_type_t type);

/* Returns true if there are more nodes before or after (respectively) the
   current node */
bool has_prev_nodes_sgf (void);
bool has_more_nodes_sgf (void);

/* When at the start of a branch, this will follow one of the branches.
   Variations are numbered from 0. Returns false on failure. */
bool go_to_variation_sgf (unsigned int num);

/* Get the number of variations at the current move (will be 1 unless the
   current node is a branch node */
int num_variations_sgf (void);

/* Calls into the display subsystem to mark any child variations of the
   current node. Returns the number of variations marked */
int mark_child_variations_sgf (void);

/* Calls into the display subsystem to pass marks to be drawn on the board
   for the current node. Does not do child variation marks. */
void set_all_marks_sgf (void);

/* Add a child regardless of if there is a child node already or not.
   *variation_number will be set to the variation number of the added
   variation Returns the handle of the new node. */
int add_child_sgf (int *variation_number);

/* Goes to the next variation after the current one if the current node is
   a branch node. Returns the number of the new variation */
int next_variation_sgf (void);

/* ints in these are handles to storage locations, bools mean failure if
   false */
int add_prop_sgf (int node, enum prop_type_t type, union prop_data_t data);
bool delete_prop_sgf (int node, enum prop_type_t type);
bool delete_prop_handle_sgf (int node, int prop);
int get_prop_sgf (int node, enum prop_type_t type, int *previous_prop);

/* If there is already a property of the same type, it will be
   overwritten, otherwise a new one is added Returns the handle of the
   added property. */
int add_or_set_prop_sgf (int node, enum prop_type_t type,
                         union prop_data_t data);

/* Find a property of similar type with the same position in the current
   node. (for example, if type == PROP_ADD_BLACK and pos == POS(0, 0), it
   will find a prop with type == PROP_ADD_WHITE and pos == POS(0, 0), but
   not any of the mark types with pos == POS(0, 0). returns the handle of
   the found property */
int get_prop_pos_sgf (enum prop_type_t type, union prop_data_t data);

/* If there is a move in the current node, return its handle. */
int get_move_sgf (void);

/* If there is a comment in the current node, this will read it out into
   the buffer.  Returns the size of the comment read (including the '\0').
   The buffer can be treated as a string. */
int read_comment_sgf (char *buffer, size_t buffer_size);

/* Write a comment property to the current node.  This will overwrite any
   comment that currently exists. Returns the number of characters
   written. */
int write_comment_sgf (char *string);

/* Move forward or back in the SGF tree, following any chosen variations
   (variations are "chosen" every time you go through one) These will
   update the board showing any moves/added or removed stones/ marks/etc.
   . */
bool undo_node_sgf (void);
bool redo_node_sgf (void);

/* Returns true if the SGF property type is handled in some way.  For
 * real SGF properties (in other words, ones that can actually be read from
 * a file, not psuedo-properties), if they are unhandled that just means that
 * we copy them verbatim from the old file to the new, keeping them with the
 * correct node
 */
bool is_handled_sgf (enum prop_type_t type);

/* Sets up the handicap on the board (based on header.handicap) */
void setup_handicap_sgf (void);

/* Goes to the start of a handicap game. */
void goto_handicap_start_sgf (void);

/* Must be called after setting up a new game, either blank or loaded from
   a file. (Picks a place to put the header properties if none was found,
   and may do other stuff) */
bool post_game_setup_sgf (void);

/* Get the child that matches the given move.
 *
 * Returns the variation number of the matching move, or negative if
 * none is found.
 */
int get_matching_child_sgf (unsigned short pos, unsigned char color);

#define NO_NODE (-1)
#define NO_PROP (-1)

/* These flags are used in undo handling for moves and added/removed
   stones */
#define FLAG_ORIG_EMPTY ((uint8_t) (1 << 7))
#define FLAG_ORIG_BLACK ((uint8_t) (1 << 6))
#define FLAG_KO_THREAT  ((uint8_t) (1 << 5))
#define FLAG_SELF_CAP   ((uint8_t) (1 << 4))
#define FLAG_W_CAP      ((uint8_t) (1 << 3))
#define FLAG_E_CAP      ((uint8_t) (1 << 2))
#define FLAG_S_CAP      ((uint8_t) (1 << 1))
#define FLAG_N_CAP      ((uint8_t) (1))

#define MIN_STORAGE_BUFFER_SIZE 200
#define UNHANDLED_PROP_LIST_FILE (PLUGIN_GAMES_DIR "/gbn_misc.bin")

/* Handle of the current node, the start of the game, and the root
 * of the tree
 */
extern int current_node;
extern int tree_head;
extern int start_node;

extern int sgf_fd;
extern int unhandled_fd;

/* true if the header location has already been marked in the current
   game, false otherwise */
extern bool header_marked;

#endif
