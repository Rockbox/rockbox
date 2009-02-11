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

#ifndef GOBAN_TYPES_H
#define GOBAN_TYPES_H

#include "plugin.h"

/* A generic stack sp is the stack pointer (0 for an empty stack) */
struct stack_t
{
    size_t size;
    size_t sp;
    char *buffer;
};

/* All of the types of SGF properties that we understand. Not all of these
   are handled, even if we understand them, unhandled properties are
   handled differently (basically they are copied to a secondary location
   and copied back if we output the tree) The ones at the end aren't real
   SGF properties, they are only to help us in parsing/outputting/keeping
   track of variations/etc. IMPORTANT: if you edit this, you must be
   careful to keep it in line with prop_names, PROP_NAMES_SIZE, and
   PROP_NAME_LEN otherwise really bad things will happen. There is too much
   information on each of these to list here, please see
   http://www.red-bean.com/sgf/ */
enum prop_type_t
{
    PROP_BLACK_MOVE,
    PROP_WHITE_MOVE,

    PROP_ADD_BLACK,
    PROP_ADD_WHITE,
    PROP_ADD_EMPTY,

    PROP_PLAYER_TO_PLAY,
    PROP_COMMENT,

    /* information about the position reached by the current node */
    PROP_EVEN,
    PROP_BLACK_GOOD,
    PROP_WHITE_GOOD,
    PROP_HOTSPOT,
    PROP_UNCLEAR,
    PROP_VALUE,

    /* information about the current move */
    PROP_BAD,
    PROP_DOUBTFUL,
    PROP_INTERESTING,
    PROP_TESUJI,

    /* marks on the board */
    PROP_CIRCLE,
    PROP_SQUARE,
    PROP_TRIANGLE,
    PROP_DIM,
    PROP_MARK,
    PROP_SELECTED,

    /* labels go on points, names name the node */
    PROP_LABEL,
    PROP_NODE_NAME,

    /* root props */
    PROP_APPLICATION,
    PROP_CHARSET,
    PROP_FILE_FORMAT,
    PROP_GAME,
    PROP_VARIATION_TYPE,
    PROP_SIZE,

    /* game info props */
    PROP_ANNOTATOR,
    PROP_BLACK_NAME,
    PROP_WHITE_NAME,
    PROP_HANDICAP,
    PROP_KOMI,
    PROP_BLACK_TERRITORY,
    PROP_WHITE_TERRITORY,
    PROP_BLACK_RANK,
    PROP_WHITE_RANK,
    PROP_BLACK_TEAM,
    PROP_WHITE_TEAM,
    PROP_COPYRIGHT,
    PROP_DATE,
    PROP_EVENT,
    PROP_ROUND,
    PROP_GAME_NAME,
    PROP_GAME_COMMENT,
    PROP_OPENING_NAME,
    PROP_OVERTIME,
    PROP_PLACE,
    PROP_RESULT,
    PROP_RULESET,
    PROP_SOURCE,
    PROP_TIME_LIMIT,
    PROP_USER,

    /* these are all the <whatever> left /after/ the current move */
    PROP_BLACK_TIME_LEFT,
    PROP_WHITE_TIME_LEFT,
    PROP_BLACK_STONES_LEFT,     /* the number of stones left in a canadian
                                   style overtime period */
    PROP_WHITE_STONES_LEFT,     /* same for white */


    /* these are mostly used for printing, we don't handle these */
    PROP_FIGURE,
    PROP_PRINT_MOVE_MODE,

    /* view only part of the board. probably don't handle this */
    PROP_VIEW,



    /* psuedo PROP types, used for variations and parsing and such */

    PROP_VARIATION,             /* used for branches */
    PROP_INVALID,
    PROP_GENERIC_UNHANDLED,     /* used to mark the place where an
                                   unhandled property was copied to
                                   secondary storage (so we can output it
                                   again when we output the game tree) */
    PROP_ANY,                   /* Used as a parameter when any property
                                   type is supposed to match */
    PROP_VARIATION_TO_PROCESS,  /* Used in parsing/outputting */
    PROP_VARIATION_CHOICE,      /* Used to store which variation we should
                                   follow when we get to a branch */
    PROP_ROOT_PROPS             /* Marks the place where we should output
                                   the information from struct header_t
                                   header */
};

extern char *prop_names[];
/* IMPORTANT: keep this array full of all properties that we want to be
   able to parse out of an SGF file.  This next part assumes that they go
   before unparseable (psuedo) props in the above enum, or else we need to
   insert placeholders in the array for unparseables */

#define PROP_NAMES_SIZE (63)

/* The only one character property names are the moves, everything else is
   two characters */
#define PROP_NAME_LEN(type) ((type == PROP_BLACK_MOVE || \
                              type == PROP_WHITE_MOVE) ? 1 : 2)

/* Data for a property.  Can be a number, a node handle (for variations),
   or a position and either a short or a character (the extras are for
   label characters, and stone undo data (moves and added/removed)) */
union prop_data_t
{
    unsigned int number;
    int node;
    struct
    {
        unsigned short position;
        union
        {
            unsigned short stone_extra;
            unsigned char label_extra;
        };
    };
};

/* What should happen when the user "plays" a move */
enum play_mode_t
{
    MODE_PLAY,
    MODE_FORCE_PLAY,
    MODE_ADD_BLACK,
    MODE_ADD_WHITE,
    MODE_REMOVE,
    MODE_MARK,
    MODE_CIRCLE,
    MODE_SQUARE,
    MODE_TRIANGLE,
    MODE_LABEL
};

/* Different types of board marks */
enum mark_t
{
    MARK_VARIATION,
    MARK_SQUARE,
    MARK_CIRCLE,
    MARK_TRIANGLE,
    MARK_LAST_MOVE,
    MARK_LABEL
};

/* An SGF property next is the handle to the next property in the node, or
   less than zero if there isn't another */
struct prop_t
{
    union prop_data_t data;
    enum prop_type_t type;
    int next;
};


/* The names of the rulesets, ex. "AGA", "Japanese", etc. */
extern char *ruleset_names[];

/* IMPORTANT! keep in sync with ruleset_names!!! */
enum ruleset_t
{
    RULESET_AGA = 0,
    RULESET_JAPANESE,
    RULESET_CHINESE,
    RULESET_NEW_ZEALAND,
    RULESET_ING,
    __RULESETS_SIZE             /* make sure i am last! */
};

#define NUM_RULESETS ((int) __RULESETS_SIZE)


/* One SGF node which can contain an indefinite number of SGF properties
   Less than zero for any of these means that there is nothing in that
   direction. */
struct node_t
{
    int props;
    int next;
    int prev;
};


/* convenience union for keeping a mixed array of props and nodes */
union storage_t
{
    struct prop_t prop;
    struct node_t node;
};


/* The game metadata which can be stored in the SGF file */

#define MAX_NAME 59
#define MAX_EVENT 100
#define MAX_RESULT 16
#define MAX_RANK 10
#define MAX_TEAM 32
#define MAX_DATE 32
#define MAX_ROUND 8
#define MAX_PLACE 100
#define MAX_OVERTIME 32
#define MAX_RULESET 32

struct header_t
{
    char white[MAX_NAME];
    char black[MAX_NAME];
    char white_rank[MAX_RANK];
    char black_rank[MAX_RANK];
    char white_team[MAX_TEAM];
    char black_team[MAX_TEAM];
    char date[MAX_DATE];
    char round[MAX_ROUND];
    char event[MAX_EVENT];
    char place[MAX_PLACE];
    char result[MAX_RESULT];
    char overtime[MAX_OVERTIME];
    char ruleset[MAX_RULESET];
    int time_limit;
    int komi;
    uint8_t handicap;
};

#endif
