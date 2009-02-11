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

#include "util.h"
#include "game.h"


void metadata_summary (void)
{
    char buffer[256] = "";

    if (rb->strlen (header.black) ||
        rb->strlen (header.white) ||
        rb->strlen (header.black_rank) ||
        rb->strlen (header.white_rank))
    rb->snprintf (buffer, sizeof(buffer),
                  "%s [%s] v. %s [%s] ",
                  header.black, header.black_rank,
                  header.white, header.white_rank);

    if (header.handicap > 1)
    {
        rb->snprintf (buffer + rb->strlen(buffer),
                      sizeof (buffer) - rb->strlen (buffer),
                      "%d stones ", header.handicap);
    }

    if (header.komi != 0 && !(header.komi == 1 && header.handicap > 1))
    {
        snprint_fixed (buffer + rb->strlen(buffer),
                        sizeof (buffer) - rb->strlen (buffer),
                        header.komi);
        rb->snprintf (buffer + rb->strlen(buffer),
                      sizeof (buffer) - rb->strlen (buffer),
                      " komi ");
    }

    if (rb->strlen(header.result))
    {
        rb->snprintf (buffer + rb->strlen(buffer),
                      sizeof (buffer) - rb->strlen (buffer),
                      "(%s)", header.result);
    }

    /* waiting for user input messes up the testing code, so ifdef it*/
#if !defined(GBN_TEST)
    if (rb->strlen(buffer))
    {
        rb->splash(0, buffer);
        rb->action_userabort(TIMEOUT_BLOCK);
    }
#endif
}

void *
align_buffer (void *buffer, size_t * buffer_size)
{
    unsigned int wasted = (-(long) buffer) & 3;

    if (!buffer || !buffer_size)
    {
        return NULL;
    }

    if (*buffer_size <= wasted)
    {
        *buffer_size = 0;
        return NULL;
    }

    *buffer_size -= wasted;

    return (void *) (((char *) buffer) + wasted);
}



bool
setup_stack (struct stack_t *stack, void *buffer, size_t buffer_size)
{
    if (!stack || !buffer || !buffer_size)
    {
        DEBUGF ("INVALID STACK SETUP!!\n");
        return false;
    }

    buffer = align_buffer (buffer, &buffer_size);

    if (!buffer || !buffer_size)
    {
        DEBUGF ("Buffer disappeared after alignment!\n");
        return false;
    }

    stack->buffer = buffer;
    stack->size = buffer_size;
    stack->sp = 0;

    return true;
}

bool
push_stack (struct stack_t * stack, void *buffer, size_t buffer_size)
{
    if (stack->sp + buffer_size > stack->size)
    {
        DEBUGF ("stack full!!\n");
        return false;
    }

    rb->memcpy (&stack->buffer[stack->sp], buffer, buffer_size);

    stack->sp += buffer_size;

    return true;
}

bool
pop_stack (struct stack_t * stack, void *buffer, size_t buffer_size)
{
    if (!peek_stack (stack, buffer, buffer_size))
    {
        return false;
    }

    stack->sp -= buffer_size;

    return true;
}

bool
peek_stack (struct stack_t * stack, void *buffer, size_t buffer_size)
{
    if (stack->sp < buffer_size)
    {
        return false;
    }

    rb->memcpy (buffer, &stack->buffer[stack->sp - buffer_size], buffer_size);

    return true;
}

void
empty_stack (struct stack_t *stack)
{
    stack->sp = 0;
}

bool
push_pos_stack (struct stack_t *stack, unsigned short pos)
{
    return push_stack (stack, &pos, sizeof (pos));
}

bool
push_int_stack (struct stack_t *stack, int num)
{
    return push_stack (stack, &num, sizeof (num));
}

bool
push_char_stack (struct stack_t *stack, char num)
{
    return push_stack (stack, &num, sizeof (num));
}



/* IMPORTANT: keep in sync with the enum prop_type_t enum in types.h */
char *prop_names[] = {
    /* look up the SGF specification for the meaning of these */
    "B", "W",
    "AB", "AW", "AE",

    "PL", "C",

    "DM", "GB", "GW", "HO", "UC", "V",

    "BM", "DO", "IT", "TE",

    "CR", "SQ", "TR", "DD", "MA", "SL", "LB", "N",

    "AP", "CA", "FF", "GM", "ST", "SZ",

    "AN", "PB", "PW", "HA", "KM", "TB", "TW", "BR", "WR",
    "BT", "WT", "CP", "DT", "EV", "RO", "GN", "GC", "ON",
    "OT", "PC", "RE", "RU", "SO", "TM", "US",

    "BL", "WL", "OB", "OW", "FG", "PM", "VW"
};

/* These seems to be specified by the SGF specification.  You can do free
   form ones as well, but I haven't implemented that (and don't plan to) */
char *ruleset_names[] = { "AGA", "Japanese", "Chinese", "NZ", "GOE" };



int
create_or_open_file (const char *filename)
{
    int fd;

    if (!rb->file_exists (filename))
    {
        fd = rb->creat (filename);
    }
    else
    {
        fd = rb->open (filename, O_RDWR);
    }

    return fd;
}


int
snprint_fixed (char *buffer, int buffer_size, int fixed)
{
    return rb->snprintf (buffer, buffer_size, "%s%d.%d",
                         fixed < 0 ? "-" : "",
                         abs (fixed) >> 1, 5 * (fixed & 1));
}


int
peek_char (int fd)
{
    char peeked_char;

    int result = rb->read (fd, &peeked_char, 1);

    if (result != 1)
    {
        return -1;
    }

    result = rb->lseek (fd, -1, SEEK_CUR);

    if (result < 0)
    {
        return -1;
    }

    return peeked_char;
}


int
read_char (int fd)
{
    char read_char;

    int result = rb->read (fd, &read_char, 1);

    if (result != 1)
    {
        return -1;
    }

    return read_char;
}


bool
write_char (int fd, char to_write)
{
    int result = write_file (fd, &to_write, 1);

    if (result != 1)
    {
        return false;
    }

    return true;
}

ssize_t
write_file (int fd, const void *buf, size_t count)
{
    const char *buffer = buf;
    int result;
    int ret_val = count;

    while (count)
    {
        result = rb->write (fd, buffer, count);

        if (result < 0)
        {
            return -1;
        }

        count -= result;
        buffer += result;
    }

    return ret_val;
}

ssize_t
read_file (int fd, void *buf, size_t count)
{
    char *buffer = buf;
    int result;
    int ret_val = count;

    while (count)
    {
        result = rb->read (fd, buffer, count);

        if (result <= 0)
        {
            return -1;
        }

        count -= result;
        buffer += result;
    }

    return ret_val;
}

int
read_char_no_whitespace (int fd)
{
    int result = peek_char_no_whitespace (fd);

    read_char (fd);

    return result;
}

int
peek_char_no_whitespace (int fd)
{
    int result;

    while (is_whitespace (result = peek_char (fd)))
    {
        read_char (fd);
    }

    return result;
}


void
close_file (int *fd)
{
    if (*fd >= 0)
    {
        rb->close (*fd);
    }

    *fd = -1;
}

bool
is_whitespace (int value)
{
    if (value == ' ' ||
        value == '\t' ||
        value == '\n' || value == '\r' || value == '\f' || value == '\v')
    {
        return true;
    }
    else
    {
        return false;
    }
}

void
sanitize_string (char *string)
{
    bool escaped = false;

    if (!string)
    {
        return;
    }

    while (1)
    {
        switch (*string)
        {
        case '\0':
            return;
        case '\\':
            escaped = !escaped;
            break;
        case ']':
            if (!escaped)
            {
                *string = ']';
            }
            escaped = false;
            break;
        default:
            break;
        };
        ++string;
    }
}


bool
get_header_string_and_size (struct header_t *header,
                            enum prop_type_t type, char **buffer, int *size)
{
    if (buffer == 0 || header == 0)
    {
        return false;
    }

    if (type == PROP_BLACK_NAME)
    {
        *buffer = header->black;
        *size = MAX_NAME;
    }
    else if (type == PROP_WHITE_NAME)
    {
        *buffer = header->white;
        *size = MAX_NAME;
    }
    else if (type == PROP_BLACK_RANK)
    {
        *buffer = header->black_rank;
        *size = MAX_RANK;
    }
    else if (type == PROP_WHITE_RANK)
    {
        *buffer = header->white_rank;
        *size = MAX_RANK;
    }
    else if (type == PROP_BLACK_TEAM)
    {
        *buffer = header->black_team;
        *size = MAX_TEAM;
    }
    else if (type == PROP_WHITE_TEAM)
    {
        *buffer = header->white_team;
        *size = MAX_TEAM;
    }
    else if (type == PROP_DATE)
    {
        *buffer = header->date;
        *size = MAX_DATE;
    }
    else if (type == PROP_ROUND)
    {
        *buffer = header->round;
        *size = MAX_ROUND;
    }
    else if (type == PROP_EVENT)
    {
        *buffer = header->event;
        *size = MAX_EVENT;
    }
    else if (type == PROP_PLACE)
    {
        *buffer = header->place;
        *size = MAX_PLACE;
    }
    else if (type == PROP_OVERTIME)
    {
        *buffer = header->overtime;
        *size = MAX_OVERTIME;
    }
    else if (type == PROP_RESULT)
    {
        *buffer = header->result;
        *size = MAX_RESULT;
    }
    else if (type == PROP_RULESET)
    {
        *buffer = header->ruleset;
        *size = MAX_RULESET;
    }
    else
    {
        return false;
    }

    return true;
}


/* TEST CODE BEGINS HERE define GBN_TEST to run this, either in goban.h or
   in the CFLAGS. The tests will be run when the plugin starts, after
   which the plugin will exit. Any error stops testing since many tests
   depend on previous setup. Note: The testing can take a while as there
   are some big loops.  Be patient. */

#ifdef GBN_TEST

#include "goban.h"
#include "types.h"
#include "board.h"
#include "game.h"
#include "sgf.h"
#include "sgf_storage.h"

/* If this isn't on a single line, the line numbers it reports will be wrong.
 *
 * I'm sure there's a way to make it better, but it's not really worth it.
 */
#define gbn_assert(test) if (test) {DEBUGF("%d passed\n", __LINE__);} else {DEBUGF("%d FAILED!\n", __LINE__); rb->splashf(10 * HZ, "Test on line %d of util.c failed!", __LINE__); return;}

void
run_tests (void)
{
    rb->splash (3 * HZ, "Running tests.  Failures will stop testing.");



    /* allocating and freeing storage units */

    gbn_assert (alloc_storage_sgf ());

    int prevent_infinite = 100000000;

    int count = 1;
    while (alloc_storage_sgf () >= 0 && --prevent_infinite)
    {
        ++count;
    }

    gbn_assert (prevent_infinite);
    gbn_assert (count > 100);

    /* make sure it fails a few times */
    gbn_assert (alloc_storage_sgf () < 0);
    gbn_assert (alloc_storage_sgf () < 0);
    gbn_assert (alloc_storage_sgf () < 0);

    free_storage_sgf (0);

    gbn_assert (alloc_storage_sgf () == 0);

    gbn_assert (alloc_storage_sgf () < 0);

    int i;
    for (i = 0; i <= count; ++i)
    {
        free_storage_sgf (i);
    }

    gbn_assert (alloc_storage_sgf () >= 0);
    --count;

    for (i = 0; i < count; ++i)
    {
        gbn_assert (alloc_storage_sgf () >= 0);
    }

    free_tree_sgf ();



    /* setting up, saving and loading */
    gbn_assert (setup_game (MAX_BOARD_SIZE, MAX_BOARD_SIZE, 0, 15));
    gbn_assert (setup_game (MAX_BOARD_SIZE, MAX_BOARD_SIZE, 0, -30));
    gbn_assert (setup_game (MAX_BOARD_SIZE, MAX_BOARD_SIZE, 4, 1));
    gbn_assert (setup_game (MIN_BOARD_SIZE, MIN_BOARD_SIZE, 1, 1));

    gbn_assert (setup_game (MIN_BOARD_SIZE, MAX_BOARD_SIZE, 1, 1));
    gbn_assert (setup_game (MAX_BOARD_SIZE, MIN_BOARD_SIZE, 1, 1));

    gbn_assert (!setup_game (MAX_BOARD_SIZE + 1, MAX_BOARD_SIZE + 1, 0, 15));
    gbn_assert (!setup_game (MIN_BOARD_SIZE - 1, MIN_BOARD_SIZE - 1, 0, 15));
    gbn_assert (!setup_game (MAX_BOARD_SIZE, MAX_BOARD_SIZE, -1, 15));

    gbn_assert (setup_game (MAX_BOARD_SIZE, MAX_BOARD_SIZE, 1, 1));
    gbn_assert (save_game (DEFAULT_SAVE_DIR "/t1.sgf"));
    gbn_assert (load_game (DEFAULT_SAVE_DIR "/t1.sgf"));
    gbn_assert (save_game (DEFAULT_SAVE_DIR "/t2.sgf"));
    gbn_assert (load_game (DEFAULT_SAVE_DIR "/t2.sgf"));

    gbn_assert (!save_game ("/DIR_DOESNT_EXIST/blah.sgf"));
    gbn_assert (!load_game ("/DIR_DOESNT_EXIST/blah.sgf"));
    gbn_assert (!load_game (DEFAULT_SAVE_DIR "/DOESNT_EXIST.sgf"));



    /* test of a long game, captures, illegal moves */
    gbn_assert (load_game (DEFAULT_SAVE_DIR "/long.sgf"));
    while (move_num < 520)
    {
        gbn_assert (num_variations_sgf () == 1);
        gbn_assert (redo_node_sgf ());
    }

    gbn_assert (play_move_sgf (POS (2, 0), BLACK));
    gbn_assert (play_move_sgf (POS (2, 1), WHITE));

    gbn_assert (move_num == 522);

    gbn_assert (white_captures == 261 && black_captures == 0);

    gbn_assert (play_move_sgf (PASS_POS, BLACK));
    gbn_assert (play_move_sgf (PASS_POS, WHITE));

    gbn_assert (move_num == 524);

    int x, y;
    int b_count, w_count, e_count;
    b_count = w_count = e_count = 0;
    for (x = 0; x < 19; ++x)
    {
        for (y = 0; y < 19; ++y)
        {
            gbn_assert (!legal_move_board (POS (x, y), BLACK, false));
            gbn_assert (!play_move_sgf (POS (x, y), BLACK));
            switch (get_point_board (POS (x, y)))
            {
            case BLACK:
                ++b_count;
                break;
            case WHITE:
                ++w_count;
                break;
            case EMPTY:
                ++e_count;
                break;
            default:
                gbn_assert (false);
            }
        }
    }

    gbn_assert (b_count == 0 && w_count == 261 && e_count == 19 * 19 - 261);

    gbn_assert (undo_node_sgf ());
    gbn_assert (move_num == 523);

    int infinite_prevention = 0;
    while (move_num > 0)
    {
        gbn_assert (undo_node_sgf ());

        ++infinite_prevention;
        gbn_assert (infinite_prevention < 100000);
    }

    gbn_assert (save_game (DEFAULT_SAVE_DIR "/long_out.sgf"));


    /* test of basic moves, legal moves, adding and removing stones */
    gbn_assert (setup_game (MAX_BOARD_SIZE, MAX_BOARD_SIZE, 0, 0));
    gbn_assert (play_move_sgf
                (POS (MAX_BOARD_SIZE / 2, MAX_BOARD_SIZE / 2), BLACK));
    gbn_assert (move_num == 1 && current_player == WHITE);
    gbn_assert (!legal_move_board
                (POS (MAX_BOARD_SIZE / 2, MAX_BOARD_SIZE / 2), WHITE, true));

    int saved_node = current_node;
    gbn_assert (add_stone_sgf (POS (0, 0), BLACK));
    gbn_assert (current_node != saved_node);
    gbn_assert (get_point_board (POS (0, 0)) == BLACK);
    gbn_assert (move_num == 1 && current_player == WHITE);

    saved_node = current_node;
    gbn_assert (add_stone_sgf (POS (0, 1), WHITE));
    gbn_assert (current_node == saved_node);
    gbn_assert (get_point_board (POS (0, 1)) == WHITE);

    gbn_assert (add_stone_sgf (POS (0, 0), EMPTY));
    gbn_assert (add_stone_sgf (POS (0, 1), EMPTY));
    gbn_assert (get_point_board (POS (0, 0)) == EMPTY);
    gbn_assert (get_point_board (POS (0, 1)) == EMPTY);


    /* test captures */
    gbn_assert (load_game (DEFAULT_SAVE_DIR "/cap.sgf"));
    gbn_assert (play_move_sgf (POS (0, 0), BLACK));
    gbn_assert (black_captures == 8);
    gbn_assert (undo_node_sgf ());
    gbn_assert (black_captures == 0);

    gbn_assert (!play_move_sgf (POS (0, 0), WHITE));
    play_mode = MODE_FORCE_PLAY;
    gbn_assert (play_move_sgf (POS (0, 0), WHITE));
    play_mode = MODE_PLAY;

    gbn_assert (black_captures == 9);
    gbn_assert (get_point_board (POS (0, 0)) == EMPTY);
    gbn_assert (undo_node_sgf ());
    gbn_assert (black_captures == 0);

    gbn_assert (play_move_sgf (POS (9, 9), BLACK));
    gbn_assert (black_captures == 44);

    for (x = 0; x < 19; ++x)
    {
        for (y = 0; y < 19; ++y)
        {
            gbn_assert (get_point_board (POS (x, y)) == BLACK ||
                        add_stone_sgf (POS (x, y), BLACK));
        }
    }

    gbn_assert (get_point_board (POS (0, 0)) == BLACK);
    gbn_assert (add_stone_sgf (POS (9, 9), EMPTY));
    gbn_assert (play_move_sgf (POS (9, 9), WHITE));
    gbn_assert (white_captures == 360);

    gbn_assert (undo_node_sgf ());
    gbn_assert (white_captures == 0);

    play_mode = MODE_FORCE_PLAY;
    gbn_assert (play_move_sgf (POS (9, 9), BLACK));
    play_mode = MODE_PLAY;
    gbn_assert (white_captures == 361);

    for (x = 0; x < 19; ++x)
    {
        for (y = 0; y < 19; ++y)
        {
            gbn_assert (get_point_board (POS (x, y)) == EMPTY);
        }
    }


    /* test ko */
    gbn_assert (setup_game (MAX_BOARD_SIZE, MAX_BOARD_SIZE, 0, 15));

    /*
     * Set up the board to look like this:
     * -X------
     * XO------
     * O-------
     * --------
     */
    gbn_assert (add_stone_sgf (POS (0, 1), BLACK));
    gbn_assert (add_stone_sgf (POS (1, 0), BLACK));
    gbn_assert (add_stone_sgf (POS (1, 1), WHITE));
    gbn_assert (add_stone_sgf (POS (0, 2), WHITE));

    /* take the ko and make sure black can't take back */
    gbn_assert (play_move_sgf (POS (0, 0), WHITE));
    gbn_assert (!play_move_sgf (POS (0, 1), BLACK));

    /* make sure white can fill, even with the ko_pos set */
    gbn_assert (play_move_sgf (POS (0, 1), WHITE));
    /* and make sure undo sets the ko again */
    gbn_assert (undo_node_sgf ());
    gbn_assert (!play_move_sgf (POS (0, 1), BLACK));

    /* make sure ko threats clear the ko */
    gbn_assert (play_move_sgf (POS (2, 2), BLACK));     /* ko threat */
    gbn_assert (play_move_sgf (POS (2, 3), WHITE));     /* response */
    gbn_assert (play_move_sgf (POS (0, 1), BLACK));     /* take ko */

    gbn_assert (undo_node_sgf ());
    gbn_assert (undo_node_sgf ());
    gbn_assert (undo_node_sgf ());

    /* make sure a pass is counted as a ko threat */
    gbn_assert (!play_move_sgf (POS (0, 1), BLACK));
    gbn_assert (play_move_sgf (PASS_POS, BLACK));
    gbn_assert (play_move_sgf (PASS_POS, WHITE));
    gbn_assert (play_move_sgf (POS (0, 1), BLACK));

    /* and finally let's make sure that white can't directly retake */
    gbn_assert (!play_move_sgf (POS (0, 0), WHITE));



    /* test some header information saving/loading as well as comment
       saving loading */
    char some_comment[] =
        "blah blah blah i am a stupid comment. here's some annoying characters: 01234567890!@#$%^&*()[[[[\\\\\\]ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    /* that bit near the end is literally this: \\\] which tests escaping
       of ]s */
    char read_buffer[256];

    gbn_assert (setup_game (MAX_BOARD_SIZE, MAX_BOARD_SIZE, 5, -20));

    /* this also tests that ko_pos is reset by setuping up a new game */
    gbn_assert (play_move_sgf (POS (0, 0), WHITE));
    gbn_assert (write_comment_sgf (some_comment) > 0);
    gbn_assert (play_move_sgf (POS (0, 1), BLACK));
    rb->strcpy (header.black, "Jack Black");
    rb->strcpy (header.white, "Jill White");

    gbn_assert (save_game (DEFAULT_SAVE_DIR "/head.sgf"));

    gbn_assert (setup_game (MIN_BOARD_SIZE, MIN_BOARD_SIZE, 1, 1));
    gbn_assert (load_game (DEFAULT_SAVE_DIR "/head.sgf"));

    gbn_assert (header.komi == -20 && header.handicap == 5);
    gbn_assert (board_width == MAX_BOARD_SIZE
                && board_height == MAX_BOARD_SIZE);
    gbn_assert (rb->strcmp (header.black, "Jack Black") == 0);
    gbn_assert (rb->strcmp (header.white, "Jill White") == 0);
    gbn_assert (redo_node_sgf ());
    gbn_assert (read_comment_sgf (read_buffer, sizeof (read_buffer)));
    gbn_assert (rb->strcmp (read_buffer, some_comment) == 0);
    gbn_assert (redo_node_sgf ());
    gbn_assert (get_point_board (POS (0, 0)) == WHITE);
    gbn_assert (get_point_board (POS (0, 1)) == BLACK);



    /* test saving and loading a file with unhandled SGF properties. this
       test requires that the user diff unhnd.sgf with unhnd_out.sgf (any
       substantial difference is a bug and should be reported) the
       following are NOT substantial differences: - reordering of
       properties in a node - whitespace changes outside of a comment
       value or other property value - reordering of property values */
    gbn_assert (load_game (DEFAULT_SAVE_DIR "/unhnd.sgf"));
    gbn_assert (save_game (DEFAULT_SAVE_DIR "/unhnd_out.sgf"));



    /* Test variations a bit */
    gbn_assert (setup_game (MAX_BOARD_SIZE, MAX_BOARD_SIZE, 0, 13));
    /* start at a move, otherwise add_stone won't create a variation */
    gbn_assert (play_move_sgf (POS (5, 5), BLACK));
    /* make sure it doesn't */
    gbn_assert (undo_node_sgf ());
    gbn_assert (add_stone_sgf (POS (4, 5), WHITE));
    gbn_assert (!undo_node_sgf ());
    gbn_assert (num_variations_sgf () == 1);
    gbn_assert (play_move_sgf (POS (5, 5), BLACK));

    gbn_assert (play_move_sgf (POS (0, 0), BLACK));
    gbn_assert (num_variations_sgf () == 1);
    gbn_assert (undo_node_sgf ());
    gbn_assert (play_move_sgf (POS (0, 1), BLACK));
    gbn_assert (num_variations_sgf () == 2);
    gbn_assert (undo_node_sgf ());
    gbn_assert (play_move_sgf (POS (0, 1), BLACK));
    gbn_assert (num_variations_sgf () == 2);
    gbn_assert (undo_node_sgf ());
    gbn_assert (play_move_sgf (POS (0, 2), BLACK));
    gbn_assert (num_variations_sgf () == 3);
    gbn_assert (undo_node_sgf ());
    gbn_assert (play_move_sgf (POS (0, 3), WHITE));
    gbn_assert (num_variations_sgf () == 4);
    gbn_assert (undo_node_sgf ());
    gbn_assert (play_move_sgf (PASS_POS, BLACK));
    gbn_assert (num_variations_sgf () == 5);
    gbn_assert (undo_node_sgf ());
    gbn_assert (add_stone_sgf (POS (1, 1), BLACK));
    gbn_assert (add_stone_sgf (POS (1, 2), BLACK));
    gbn_assert (add_stone_sgf (POS (1, 3), WHITE));
    gbn_assert (num_variations_sgf () == 6);
    gbn_assert (undo_node_sgf ());
    gbn_assert (add_stone_sgf (POS (1, 1), BLACK));
    gbn_assert (add_stone_sgf (POS (1, 2), BLACK));
    gbn_assert (add_stone_sgf (POS (1, 3), WHITE));
    gbn_assert (num_variations_sgf () == 7);
    gbn_assert (next_variation_sgf ());
    gbn_assert (get_point_board (POS (0, 0)) == BLACK);
    gbn_assert (get_point_board (POS (0, 1)) == EMPTY);
    gbn_assert (get_point_board (POS (0, 2)) == EMPTY);
    gbn_assert (get_point_board (POS (1, 1)) == EMPTY);
    gbn_assert (get_point_board (POS (1, 2)) == EMPTY);
    gbn_assert (get_point_board (POS (1, 3)) == EMPTY);

    rb->splash (10 * HZ, "All tests passed.  Exiting");
}
#endif /* GBN_TEST */
