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

#include "sgf.h"
#include "sgf_storage.h"
#include "types.h"
#include "goban.h"
#include "board.h"
#include "display.h"
#include "game.h"
#include "util.h"

int sgf_fd = -1;
int unhandled_fd = -1;

int tree_head = -1;
int current_node = -1;
int start_node = -1;

bool header_marked = false;

static int add_child_variation (int *variation_number);

static int get_move_from_node (int handle);

static bool is_important_node (int handle);
static bool goto_next_important_node (bool forward);
static bool retreat_node (void);
static bool advance_node (void);

static bool do_add_stones (void);
static void setup_handicap_helper (unsigned short pos);

static int undo_node_helper (void);

static void set_one_mark (unsigned short pos, enum prop_type_t type);
static void set_label_mark (unsigned short pos, char to_set);

bool
play_move_sgf (unsigned short pos, unsigned char color)
{
    int handle;
    int prop_handle;
    int temp;
    int temp2;
    union prop_data_t temp_data;
    int saved = current_node;

    if ((color != BLACK && color != WHITE) ||
        (!on_board (pos) && pos != PASS_POS))
    {
        return false;
    }


    /* go to the node before the next important node (move/add
       stone/variation) this is the right place to look for children, add
       variations, whatever. (if there is no next, we're already at the
       right place) */

    if (get_node (current_node)->next >= 0)
    {
        current_node = get_node (current_node)->next;

        /* true means forward */
        if (goto_next_important_node (true))
        {
            current_node = get_node (current_node)->prev;
        }
    }


    if ((temp = get_matching_child_sgf (pos, color)) >= 0)
    {
        /* don't have to do anything to set up temp as the right variation
           number */

    }
    else
    {
        /* now either there were no children, or none matched the one we
           want so we have to add a new one */

        /* first test if it's legal.  we don't do this above because SGF
           files are allowed to have illegal moves in them, and it seems
           to make sense to allow traversing those variations without
           making the user change to a different play_mode */

        bool suicide_allowed = false;

        if (rb->strcmp (header.ruleset, "NZ") == 0 ||
            rb->strcmp (header.ruleset, "GOE") == 0)
        {
            suicide_allowed = true;
        }

        if (play_mode != MODE_FORCE_PLAY &&
            !legal_move_board (pos, color, suicide_allowed))
        {
            return false;
        }

        handle = add_child_sgf (NULL);

        if (handle < 0)
        {
            current_node = saved;
            return false;
        }

        union prop_data_t temp_prop_data;
        temp_prop_data.position = pos;

        prop_handle = add_prop_sgf (handle,
                                    color == BLACK ? PROP_BLACK_MOVE :
                                    PROP_WHITE_MOVE, temp_prop_data);

        if (prop_handle < 0)
        {
            /* TODO: add code to completely remove the child which we
               added, and then uncomment the following line.  probably
               doens't matter much since we're out of memory, but
               whatever
               free_storage_sgf(handle); */
            rb->splash (2 * HZ,
                        "Out of memory led to invalid state.  Please exit.");
            current_node = saved;
            return false;
        }

        set_game_modified();

        temp = get_matching_child_sgf (pos, color);
    }

    /* now, one way or another temp has been set to the child variation
       number that we should follow, so all we need to do is "choose" it
       and redo_node_sgf */

    current_node = get_node (current_node)->next;
    temp_data.number = temp;

    temp2 = add_or_set_prop_sgf (current_node,
                                 PROP_VARIATION_CHOICE, temp_data);
    /* free up a superfluous prop */
    if (temp == 0)
    {
        delete_prop_handle_sgf (current_node, temp2);
    }

    current_node = saved;
    return redo_node_sgf ();
}

bool
add_mark_sgf (unsigned short pos, enum prop_type_t type)
{
    union prop_data_t temp_data;
    int temp_handle;
    enum prop_type_t original_type;

    if (!on_board (pos) || current_node < 0)
    {
        return false;
    }

    if (type == PROP_CIRCLE ||
        type == PROP_SQUARE ||
        type == PROP_TRIANGLE ||
        type == PROP_MARK || type == PROP_DIM || type == PROP_SELECTED)
    {
        temp_data.position = pos;

        if ((temp_handle = get_prop_pos_sgf (type, temp_data)) >= 0)
        {
            original_type = get_prop (temp_handle)->type;
            delete_prop_handle_sgf (current_node, temp_handle);

            if (type == original_type)
            {
                set_one_mark (pos, PROP_INVALID);
                return true;
            }
        }

        add_prop_sgf (current_node, type, temp_data);
        set_one_mark (pos, type);

        return true;
    }
    else if (type == PROP_LABEL)
    {
#define MIN_LABEL 'a'
#define MAX_LABEL 'f'
        int temp_prop_handle = get_node (current_node)->props;

        while (temp_prop_handle >= 0)
        {
            struct prop_t *temp_prop = get_prop (temp_prop_handle);

            if (temp_prop->type == PROP_LABEL &&
                temp_prop->data.position == pos)
            {
                char to_set = temp_prop->data.label_extra;
                ++to_set;
                if (to_set > MAX_LABEL)
                {
                    delete_prop_handle_sgf (current_node, temp_prop_handle);
                    set_one_mark (pos, PROP_INVALID);
                    return true;
                }

                temp_prop->data.label_extra = to_set;
                set_label_mark (pos, to_set);
                return true;
            }

            temp_prop_handle = temp_prop->next;
        }

        temp_data.label_extra = MIN_LABEL;
        temp_data.position = pos;

        add_prop_sgf (current_node, type, temp_data);
        set_label_mark (pos, MIN_LABEL);
        return true;
    }
    else
    {
        return false;
    }
}

bool
add_stone_sgf (unsigned short pos, unsigned char color)
{
    int handle;
    int prop_handle;
    int saved = current_node;
    union prop_data_t temp_data;
    enum prop_type_t temp_type;
    int var_number;
    int temp;

    if (!on_board (pos))
    {
        return false;
    }

    if (color == get_point_board (pos))
    {
        return false;
    }

    if ((!is_important_node (current_node) ||
         (current_node == start_node && get_move_sgf () < 0)) ||
        (get_prop_sgf (current_node, PROP_ADD_BLACK, NULL) >= 0 ||
         get_prop_sgf (current_node, PROP_ADD_WHITE, NULL) >= 0 ||
         get_prop_sgf (current_node, PROP_ADD_EMPTY, NULL) >= 0) ||
        get_node (current_node)->props < 0)
    {

        if (color == BLACK)
        {
            temp_type = PROP_ADD_BLACK;
        }
        else if (color == WHITE)
        {
            temp_type = PROP_ADD_WHITE;
        }
        else
        {
            temp_type = PROP_ADD_EMPTY;
        }

        temp_data.position = pos;

        handle = get_prop_pos_sgf (temp_type, temp_data);

        /* we have to always delete the old one and conditionally create a
           new one (instead of trying to reuse the old one by changing
           the type of it) because if we don't, our invariant with
           respect to like-properties being grouped together in the
           property list can easily be violated */
        if (handle >= 0)
        {
            temp_data.stone_extra = get_prop (handle)->data.stone_extra;
            delete_prop_handle_sgf (current_node, handle);
        }
        else
        {
            temp_data.stone_extra = 0;
            if (get_point_board (pos) == EMPTY)
            {
                temp_data.stone_extra |= FLAG_ORIG_EMPTY;
            }
            else if (get_point_board (pos) == BLACK)
            {
                temp_data.stone_extra |= FLAG_ORIG_BLACK;
            }
            /* else do nothing */
        }

        /* now we've saved the information about what the board was
           originally like, we can do the actual set */

        set_point_board (pos, color);

        /* test if what we currently did just returned the board back to
           its original for this position.  if so, we DON'T create a new
           PROP_ADD_*, because it's not needed (we already deleted the old
           one, so in that case we just return) */
        if (((temp_data.stone_extra & FLAG_ORIG_EMPTY) && color == EMPTY) ||
            (!(temp_data.stone_extra & FLAG_ORIG_EMPTY) &&
             (((temp_data.stone_extra & FLAG_ORIG_BLACK) && color == BLACK) ||
              (!(temp_data.stone_extra & FLAG_ORIG_BLACK) && color == WHITE))))
        {
            /* do nothing, set back to original */
        }
        else
        {
            /* we're not set back to original, so add a prop for it */
            add_prop_sgf (current_node, temp_type, temp_data);
        }

        set_game_modified();

        return true;
    }
    else
    {
        /* we have to make a child variation and add stones in it */

        /* go to the node before the next important node (move/add
           stone/variation) this is the right place to look for children,
           add variations, whatever. (if there is no next, we're already
           at the right place) */

        if (get_node (current_node)->next >= 0)
        {
            current_node = get_node (current_node)->next;

            /* true means forward */
            if (goto_next_important_node (true))
            {
                current_node = get_node (current_node)->prev;
            }
        }

        handle = add_child_sgf (&var_number);

        if (handle < 0)
        {
            rb->splash (2 * HZ, "Out of memory!");
            return false;
        }

        temp_data.position = pos;

        if (color == BLACK)
        {
            temp_type = PROP_ADD_BLACK;
        }
        else if (color == WHITE)
        {
            temp_type = PROP_ADD_WHITE;
        }
        else
        {
            temp_type = PROP_ADD_EMPTY;
        }

        prop_handle = add_prop_sgf (handle, temp_type, temp_data);

        if (prop_handle < 0)
        {
            /* TODO: add code to completely remove the child which we
               added, and then uncomment the following line.  probably
               doens't matter much since we're out of memory, but
               whatever
               free_storage_sgf(handle); */
            rb->splash (2 * HZ, "Out of memory!");
            return false;
        }

        set_game_modified();

        /* now, "choose" the variation that we just added */

        current_node = get_node (current_node)->next;
        temp_data.number = var_number;

        temp = add_or_set_prop_sgf (current_node,
                                    PROP_VARIATION_CHOICE, temp_data);

        /* free up a superfluous prop */
        if (var_number == 0)
        {
            delete_prop_handle_sgf (current_node, temp);
        }

        current_node = saved;

        /* and follow to our choice, returning since we already did the
           work */
        return redo_node_sgf ();
    }

    return false;
}

bool
undo_node_sgf (void)
{
    int result = undo_node_helper ();

    /* if we undid a ko threat, we need to figure out what the ko_pos is
       there's no simple way to do this except to undo one /more/ move,
       and then redo back to this location. (we could store it, but this
       isn't that bad) Note: this doesn't need to recurse because we don't
       care what previous move's ko positions were (since the tree is
       already set in stone basically, it wouldn't change anything). */
    if (result == 1)
    {
        int backward_move_num = move_num - 1;
        int saved_current = current_node;

        while (move_num > backward_move_num)
        {
            result = undo_node_helper ();

            if (result < 0)
            {
                DEBUGF
                    ("couldn't undo to previous move in ko threat handling!\n");
                return false;
            }
        }

        /* now we're backed up to the previous move before our destination
           so, let's go forward again until we get to the node we were at
         */

        while (current_node != saved_current)
        {
            if (!redo_node_sgf ())
            {
                DEBUGF
                    ("redoing to correct node failed on ko threat handling!\n");
                return false;
            }
        }
    }
    else if (result < 0)
    {
        DEBUGF ("initial undo failed!\n");
        return false;
    }

    set_all_marks_sgf ();

    /* if there is a move in this node, move the screen so that it is
       visible */
    int handle = get_move_sgf ();
    if (handle >= 0)
    {
        move_display (get_prop (handle)->data.position);
    }

    return true;
}

static int
undo_node_helper (void)
{
    bool ko_threat_move = false;

    if (current_node == start_node)
    {
        /* refuse to undo the initial SGF node, which is tree_head if
           handicap == 0 or 1.  If handicap >= 2, start_node is the node
           with the handicap crap and added moves on it.  don't let the
           user undo past that */
        DEBUGF ("not undoing start_node\n");
        return -1;
    }

    struct prop_t *temp_move = get_prop (get_move_sgf ());

    if (temp_move)
    {
        int undone_caps = 0;
        int undone_suicides = 0;
        unsigned short move_pos = temp_move->data.position;
        unsigned char move_color = temp_move->type == PROP_BLACK_MOVE ? BLACK :
            WHITE;

        unsigned short flags = temp_move->data.stone_extra;

        if (move_pos != PASS_POS)
        {

            if (flags & FLAG_N_CAP)
            {
                undone_caps += flood_fill_board (NORTH (move_pos),
                                                 OTHER (move_color));
            }
            if (flags & FLAG_S_CAP)
            {
                undone_caps += flood_fill_board (SOUTH (move_pos),
                                                 OTHER (move_color));
            }
            if (flags & FLAG_E_CAP)
            {
                undone_caps += flood_fill_board (EAST (move_pos),
                                                 OTHER (move_color));
            }
            if (flags & FLAG_W_CAP)
            {
                undone_caps += flood_fill_board (WEST (move_pos),
                                                 OTHER (move_color));
            }

            if (flags & FLAG_SELF_CAP)
            {
                undone_suicides += flood_fill_board (move_pos, move_color);
            }

            if (flags & FLAG_ORIG_EMPTY)
            {
                set_point_board (move_pos, EMPTY);
            }
            else if (flags & FLAG_ORIG_BLACK)
            {
                set_point_board (move_pos, BLACK);
            }
            else
            {
                set_point_board (move_pos, WHITE);
            }
        }

        if (move_color == BLACK)
        {
            black_captures -= undone_caps;
            white_captures -= undone_suicides;
        }
        else
        {
            white_captures -= undone_caps;
            black_captures -= undone_suicides;
        }

        if (flags & FLAG_KO_THREAT)
        {
            ko_threat_move = true;
        }

        --move_num;
        current_player = OTHER (current_player);
    }
    else
    {
        /* test for added stones! */
        struct prop_t *temp_prop;

        temp_prop = get_prop (get_node (current_node)->props);

        while (temp_prop)
        {
            if ((temp_prop->type == PROP_ADD_BLACK ||
                 temp_prop->type == PROP_ADD_WHITE ||
                 temp_prop->type == PROP_ADD_EMPTY) &&
                on_board (temp_prop->data.position))
            {
                if (temp_prop->data.stone_extra & FLAG_ORIG_EMPTY)
                {
                    set_point_board (temp_prop->data.position, EMPTY);
                }
                else if (temp_prop->data.stone_extra & FLAG_ORIG_BLACK)
                {
                    set_point_board (temp_prop->data.position, BLACK);
                }
                else
                {
                    set_point_board (temp_prop->data.position, WHITE);
                }
            }

            temp_prop = get_prop (temp_prop->next);
        }
    }

    if (!retreat_node ())
    {
        return -1;
    }

    if (ko_threat_move)
    {
        return 1;
    }

    return 0;
}

bool
redo_node_sgf (void)
{
    if (!advance_node ())
    {
        return false;
    }

    set_all_marks_sgf ();

    int temp_move = get_move_sgf ();
    if (temp_move >= 0)
    {
        struct prop_t *move_prop = get_prop (temp_move);
        unsigned short pos = move_prop->data.position;
        unsigned char color =
            move_prop->type == PROP_BLACK_MOVE ? BLACK : WHITE;

        if (color != current_player)
        {
            DEBUGF ("redo_node_sgf: wrong color!\n");
        }

        /* zero out the undo information and set the ko threat flag to the
           correct value */

        move_prop->data.stone_extra = 0;

        if (ko_pos != INVALID_POS)
        {
            move_prop->data.stone_extra |= FLAG_KO_THREAT;
        }

        ko_pos = INVALID_POS;

        if (pos == PASS_POS)
        {
            rb->splashf (HZ / 2, "%s Passes",
                         color == BLACK ? "Black" : "White");
        }
        else
        {
            int n_cap, s_cap, e_cap, w_cap, self_cap;

            n_cap = s_cap = e_cap = w_cap = self_cap = 0;

            if (get_point_board (pos) == EMPTY)
            {
                move_prop->data.stone_extra |= FLAG_ORIG_EMPTY;
            }
            else if (get_point_board (pos) == BLACK)
            {
                move_prop->data.stone_extra |= FLAG_ORIG_BLACK;
            }
            /* else do nothing */

            set_point_board (pos, color);

            /* do captures on the 4 cardinal directions, if the opponent
               stones are breathless */
            if (get_point_board (NORTH (pos)) == OTHER (color) &&
                get_liberties_board (NORTH (pos)) == 0)
            {
                n_cap = flood_fill_board (NORTH (pos), EMPTY);
                move_prop->data.stone_extra |= FLAG_N_CAP;
            }
            if (get_point_board (SOUTH (pos)) == OTHER (color) &&
                get_liberties_board (SOUTH (pos)) == 0)
            {
                s_cap = flood_fill_board (SOUTH (pos), EMPTY);
                move_prop->data.stone_extra |= FLAG_S_CAP;
            }
            if (get_point_board (EAST (pos)) == OTHER (color) &&
                get_liberties_board (EAST (pos)) == 0)
            {
                e_cap = flood_fill_board (EAST (pos), EMPTY);
                move_prop->data.stone_extra |= FLAG_E_CAP;
            }
            if (get_point_board (WEST (pos)) == OTHER (color) &&
                get_liberties_board (WEST (pos)) == 0)
            {
                w_cap = flood_fill_board (WEST (pos), EMPTY);
                move_prop->data.stone_extra |= FLAG_W_CAP;
            }

            /* then check for suicide */
            if (get_liberties_board (pos) == 0)
            {
                self_cap = flood_fill_board (pos, EMPTY);
                move_prop->data.stone_extra |= FLAG_SELF_CAP;
            }


            /* now check for a ko, with the following requirements: 1) we
               captured one opponent stone 2) we placed one stone (not
               connected to a larger group) 3) we have one liberty */

            if (!self_cap &&
                (n_cap + s_cap + e_cap + w_cap == 1) &&
                get_liberties_board (pos) == 1 &&
                get_point_board (NORTH (pos)) != color &&
                get_point_board (SOUTH (pos)) != color &&
                get_point_board (EAST (pos)) != color &&
                get_point_board (WEST (pos)) != color)
            {
                /* We passed all tests, so there is a ko to set. The
                   ko_pos is our single liberty location */

                if (get_point_board (NORTH (pos)) == EMPTY)
                {
                    ko_pos = NORTH (pos);
                }
                else if (get_point_board (SOUTH (pos)) == EMPTY)
                {
                    ko_pos = SOUTH (pos);
                }
                else if (get_point_board (EAST (pos)) == EMPTY)
                {
                    ko_pos = EAST (pos);
                }
                else
                {
                    ko_pos = WEST (pos);
                }
            }

            if (color == BLACK)
            {
                black_captures += n_cap + s_cap + e_cap + w_cap;
                white_captures += self_cap;
            }
            else
            {
                white_captures += n_cap + s_cap + e_cap + w_cap;
                black_captures += self_cap;
            }

            /* this will move the cursor near this move if it was off the
               screen */
            move_display (pos);
        }

        ++move_num;
        current_player = OTHER (color);

        goto redo_node_sgf_succeeded;
    }
    else if (do_add_stones ())
    {
        goto redo_node_sgf_succeeded;
    }

    return false;
    char comment_buffer[512];

redo_node_sgf_succeeded:
#if !defined(GBN_TEST)
    if (auto_show_comments &&
        read_comment_sgf (comment_buffer, sizeof (comment_buffer)))
    {
        unsigned int i;
        for (i = 0; i < sizeof (comment_buffer); ++i)
        {
            /* newlines display badly in rb->splash, so replace them
             * with spaces
             */
            if (comment_buffer[i] == '\n' ||
                comment_buffer[i] == '\r')
            {
                comment_buffer[i] = ' ';
            }
            else if (comment_buffer[i] == '\0')
            {
                break;
            }
        }
        draw_screen_display();
        rb->splash(HZ / 3, comment_buffer);
        rb->button_clear_queue();
        rb->action_userabort(TIMEOUT_BLOCK);
    }
#else
    (void) comment_buffer;
#endif

    return true;
}

int
mark_child_variations_sgf (void)
{
    int result;
    int saved = current_node;
    struct node_t *node = get_node (current_node);

    int move_handle;

    if (!node)
    {
        return 0;
    }

    current_node = node->next;
    goto_next_important_node (true);

    result = num_variations_sgf ();

    if (result > 1)
    {
        int i;
        int branch_node = current_node;
        for (i = 0; i < result; ++i)
        {
            go_to_variation_sgf (i);
            goto_next_important_node (true);

            move_handle = get_move_sgf ();

            if (move_handle >= 0)
            {
                set_one_mark (get_prop (move_handle)->data.position,
                              get_prop (move_handle)->type);
            }

            current_node = branch_node;
        }
    }

    current_node = saved;

    return result;
}

void
set_all_marks_sgf (void)
{
    struct prop_t *prop = get_prop (get_node (current_node)->props);

    while (prop)
    {
        if (prop->type == PROP_LABEL)
        {
            set_label_mark (prop->data.position, prop->data.label_extra);
        }
        else if (prop->type == PROP_COMMENT)
        {
            set_comment_display (true);
        }
        else
        {
            set_one_mark (prop->data.position, prop->type);
        }
        prop = get_prop (prop->next);
    }
}

static void
set_one_mark (unsigned short pos, enum prop_type_t type)
{
    switch (type)
    {
    case PROP_CIRCLE:
        set_mark_display (pos, 'c');
        break;
    case PROP_SQUARE:
        set_mark_display (pos, 's');
        break;
    case PROP_TRIANGLE:
        set_mark_display (pos, 't');
        break;
    case PROP_MARK:
        set_mark_display (pos, 'm');
        break;
    case PROP_DIM:
        set_mark_display (pos, 'd');
        break;
    case PROP_SELECTED:
        set_mark_display (pos, 'S');
        break;
    case PROP_BLACK_MOVE:
        set_mark_display (pos, 'b');
        break;
    case PROP_WHITE_MOVE:
        set_mark_display (pos, 'w');
        break;
    case PROP_INVALID:
        set_mark_display (pos, ' ');
    default:
        break;
    }
}

static void
set_label_mark (unsigned short pos, char to_set)
{
    set_mark_display (pos, to_set | (1 << 7));
}

static bool
do_add_stones (void)
{
    bool ret_val = false;
    struct prop_t *temp_prop;
    int temp_handle;

    if (current_node < 0)
    {
        return false;
    }

    temp_handle = get_node (current_node)->props;
    temp_prop = get_prop (temp_handle);

    while (temp_prop)
    {
        if (temp_prop->type == PROP_ADD_BLACK ||
            temp_prop->type == PROP_ADD_WHITE ||
            temp_prop->type == PROP_ADD_EMPTY)
        {

            temp_prop->data.stone_extra = 0;

            /* TODO: we could delete do-nothing PROP_ADD_*s here */

            if (get_point_board (temp_prop->data.position) == EMPTY)
            {
                temp_prop->data.stone_extra |= FLAG_ORIG_EMPTY;
            }
            else if (get_point_board (temp_prop->data.position) == BLACK)
            {
                temp_prop->data.stone_extra |= FLAG_ORIG_BLACK;
            }
            /* else, do nothing */

            if (temp_prop->type == PROP_ADD_BLACK ||
                temp_prop->type == PROP_ADD_WHITE)
            {
                set_point_board (temp_prop->data.position,
                                 temp_prop->type == PROP_ADD_BLACK ? BLACK :
                                 WHITE);
                ret_val = true;
            }
            else
            {
                set_point_board (temp_prop->data.position, EMPTY);

                ret_val = true;
            }
        }

        temp_handle = temp_prop->next;
        temp_prop = get_prop (temp_handle);
    }

    return ret_val;
}

int
add_child_sgf (int *variation_number)
{
    int node_handle;
    struct node_t *node;
    struct node_t *current = get_node (current_node);

    if (current->next < 0)
    {
        node_handle = alloc_storage_sgf ();
        node = get_node (node_handle);

        if (node_handle < 0 || !current)
        {
            return NO_NODE;
        }

        node->prev = current_node;
        node->next = NO_NODE;
        node->props = NO_PROP;

        current->next = node_handle;

        if (variation_number)
        {
            *variation_number = 0;
        }

        return node_handle;
    }
    else
    {
        return add_child_variation (variation_number);
    }
}

int
add_prop_sgf (int node, enum prop_type_t type, union prop_data_t data)
{
    int new_prop;
    int temp_prop_handle;

    if (node < 0)
    {
        return NO_PROP;
    }

    new_prop = alloc_storage_sgf ();

    if (new_prop < 0)
    {
        return NO_PROP;
    }

    get_prop (new_prop)->type = type;
    get_prop (new_prop)->data = data;

    /* check if the new_prop goes at the start */
    if (get_node (node)->props == NO_PROP ||
        (type == PROP_VARIATION &&
         get_prop (get_node (node)->props)->type != PROP_VARIATION))
    {

        if (get_node (node)->props >= 0)
        {
            get_prop (new_prop)->next = get_node (node)->props;
        }
        else
        {
            get_prop (new_prop)->next = NO_PROP;
        }

        get_node (node)->props = new_prop;
        return new_prop;
    }

    temp_prop_handle = get_node (node)->props;

    while (1)
    {
        if (get_prop (temp_prop_handle)->next < 0 ||
            (get_prop (temp_prop_handle)->type == type &&
             get_prop (get_prop (temp_prop_handle)->next)->type != type))
        {
            /* new_prop goes after the current one either because we're at
               the end of the props list, or because we're adding a prop
               after the ones of its same type */
            get_prop (new_prop)->next = get_prop (temp_prop_handle)->next;
            get_prop (temp_prop_handle)->next = new_prop;

            return new_prop;
        }

        temp_prop_handle = get_prop (temp_prop_handle)->next;
    }
}

int
get_prop_pos_sgf (enum prop_type_t type, union prop_data_t data)
{
    int temp_prop_handle;
    struct prop_t *prop;

    if (current_node < 0)
    {
        return -1;
    }

    temp_prop_handle = get_node (current_node)->props;

    while (temp_prop_handle >= 0)
    {
        prop = get_prop (temp_prop_handle);

        if ((type == PROP_ADD_BLACK ||
             type == PROP_ADD_WHITE ||
             type == PROP_ADD_EMPTY)
            &&
            (prop->type == PROP_ADD_BLACK ||
             prop->type == PROP_ADD_WHITE ||
             prop->type == PROP_ADD_EMPTY)
            && (prop->data.position == data.position))
        {
            return temp_prop_handle;
        }

        if ((type == PROP_CIRCLE ||
             type == PROP_SQUARE ||
             type == PROP_TRIANGLE ||
             type == PROP_MARK ||
             type == PROP_DIM ||
             type == PROP_SELECTED) &&
            (prop->type == PROP_CIRCLE ||
             prop->type == PROP_SQUARE ||
             prop->type == PROP_TRIANGLE ||
             prop->type == PROP_MARK ||
             prop->type == PROP_DIM ||
             prop->type == PROP_SELECTED) &&
            (prop->data.position == data.position))
        {
            return temp_prop_handle;
        }

        temp_prop_handle = get_prop (temp_prop_handle)->next;
    }

    return -1;
}

int
add_or_set_prop_sgf (int node, enum prop_type_t type, union prop_data_t data)
{
    int temp_prop_handle;

    if (node < 0)
    {
        return NO_PROP;
    }

    temp_prop_handle = get_prop_sgf (node, type, NULL);

    if (temp_prop_handle >= 0)
    {
        get_prop (temp_prop_handle)->data = data;
        return temp_prop_handle;
    }

    /* there was no prop to set, so we need to add one */
    return add_prop_sgf (node, type, data);
}

int
get_prop_sgf (int node, enum prop_type_t type, int *previous_prop)
{
    int previous_handle = NO_PROP;
    int current_handle;

    if (node < 0)
    {
        return NO_PROP;
    }

    if (get_node (node)->props < 0)
    {
        return NO_PROP;
    }

    current_handle = get_node (node)->props;

    while (current_handle >= 0)
    {
        if (get_prop (current_handle)->type == type || type == PROP_ANY)
        {
            if (previous_prop)
            {
                *previous_prop = previous_handle;
            }

            return current_handle;
        }
        else
        {
            previous_handle = current_handle;
            current_handle = get_prop (current_handle)->next;
        }
    }

    return NO_PROP;
}

bool
delete_prop_sgf (int node, enum prop_type_t type)
{
    if (node < 0)
    {
        return false;
    }

    return delete_prop_handle_sgf (node, get_prop_sgf (node, type, NULL));
}

bool
delete_prop_handle_sgf (int node, int prop)
{
    int previous;

    if (prop < 0 || node < 0 || get_node (node)->props < 0)
    {
        return false;
    }

    if (get_node (node)->props == prop)
    {
        get_node (node)->props = get_prop (get_node (node)->props)->next;
        free_storage_sgf (prop);
        return true;
    }

    previous = get_node (node)->props;

    while (get_prop (previous)->next != prop && get_prop (previous)->next >= 0)
    {
        previous = get_prop (previous)->next;
    }

    if (get_prop (previous)->next < 0)
    {
        return false;
    }
    else
    {
        get_prop (previous)->next = get_prop (get_prop (previous)->next)->next;
        free_storage_sgf (prop);
        return true;
    }
}

int
read_comment_sgf (char *buffer, size_t buffer_size)
{
    size_t bytes_read = 0;

    int prop_handle = get_prop_sgf (current_node, PROP_COMMENT, NULL);

    if (prop_handle < 0)
    {
        return 0;
    }

    if (unhandled_fd < 0)
    {
        DEBUGF ("unhandled file is closed?!\n");
        return 0;
    }

    if (rb->lseek (unhandled_fd, get_prop (prop_handle)->data.number,
                   SEEK_SET) < 0)
    {
        DEBUGF ("couldn't seek in unhandled_fd\n");
        return -1;
    }

    if (!read_char_no_whitespace (unhandled_fd) == 'C' ||
        !read_char_no_whitespace (unhandled_fd) == '[')
    {
        DEBUGF ("comment prop points to incorrect place in unhandled_fd!!\n");
        return -1;
    }

    /* make output a string, the lazy way */
    rb->memset (buffer, 0, buffer_size);
    ++bytes_read;

    bool done = false;
    bool escaped = false;

    while ((buffer_size > bytes_read) && !done)
    {
        int temp = read_char (unhandled_fd);

        switch (temp)
        {
        case ']':
            if (!escaped)
            {
                done = true;
                break;
            }
            *buffer = temp;
            ++buffer;
            ++bytes_read;
            escaped = false;
            break;

        case -1:
            DEBUGF ("encountered end of file before end of comment!\n");
            done = true;
            break;

        case '\\':
            escaped = !escaped;
            if (escaped)
            {
                break;
            }

        default:
            *buffer = temp;
            ++buffer;
            ++bytes_read;
            escaped = false;
            break;
        }
    }

    return bytes_read;
}

int
write_comment_sgf (char *string)
{
    char *orig_string = string;

    int prop_handle = get_prop_sgf (current_node, PROP_COMMENT, NULL);

    int start_of_comment = -1;
    bool overwriting = false;

    int bytes_written = 0;

    set_game_modified();

    if (unhandled_fd < 0)
    {
        DEBUGF ("unhandled file is closed?!\n");
        return 0;
    }

    if (prop_handle >= 0)
    {
        if ((start_of_comment = rb->lseek (unhandled_fd,
                                           get_prop (prop_handle)->data.number,
                                           SEEK_SET)) < 0)
        {
            DEBUGF ("couldn't seek in unhandled_fd\n");
            return -1;
        }
        else
        {
            overwriting = true;
        }
    }
    else
    {
        overwriting = false;
    }

  start_of_write_wcs:

    if (overwriting)
    {
        if (!read_char_no_whitespace (unhandled_fd) == 'C' ||
            !read_char_no_whitespace (unhandled_fd) == '[')
        {
            DEBUGF ("non-comment while overwriting!!\n");
            return -1;
        }
    }
    else
    {
        start_of_comment = rb->lseek (unhandled_fd, 0, SEEK_END);

        if (start_of_comment < 0)
        {
            DEBUGF ("error seeking to end in write_comment_sgf\n");
            return -1;
        }

        write_char (unhandled_fd, 'C');
        write_char (unhandled_fd, '[');
    }


    bool overwrite_escaped = false;

    while (*string)
    {
        if (overwriting)
        {
            int orig_char = peek_char (unhandled_fd);

            switch (orig_char)
            {
            case '\\':
                overwrite_escaped = !overwrite_escaped;
                break;

            case ']':
                if (overwrite_escaped)
                {
                    overwrite_escaped = false;
                    break;
                }
                /* otherwise, fall through */

            case -1:

                /* we reached the end of the part we can put our comment
                   in, but there's more comment to write, so we should
                   start again, this time making a new comment (the old
                   becomes wasted space in unhandled_fd, but it doesn't
                   really hurt anything except extra space on disk */

                overwriting = false;
                string = orig_string;
                bytes_written = 0;
                goto start_of_write_wcs;
                break;

            default:
                overwrite_escaped = false;
                break;
            }
        }

        switch (*string)
        {
        case '\\':
        case ']':
            write_char (unhandled_fd, '\\');

            /* fall through */

        default:
            write_char (unhandled_fd, *string);
            break;
        }

        ++string;
        ++bytes_written;
    }

    /* finish out the record */
    write_char (unhandled_fd, ']');
    write_char (unhandled_fd, ';');

    /* and put the reference into the unhandled_fd into the comment prop */
    union prop_data_t temp_data;
    temp_data.number = start_of_comment;

    add_or_set_prop_sgf (current_node, PROP_COMMENT, temp_data);
    set_comment_display (true);
    return bytes_written;
}

bool
has_more_nodes_sgf (void)
{
    int saved = current_node;
    bool ret_val = false;

    if (current_node < 0)
    {
        return false;
    }

    current_node = get_node (current_node)->next;

    if (current_node >= 0)
    {
        /* returns true if it finds an important node */
        ret_val = goto_next_important_node (true);
    }

    current_node = saved;
    return ret_val;
}

/* logic is different here because the first node in a tree is a valid
   place to go */
bool
has_prev_nodes_sgf (void)
{
    if (current_node < 0)
    {
        return false;
    }

    return current_node != start_node;
}

int
get_move_sgf (void)
{
    int result = -1;
    int saved_current = current_node;

    goto_next_important_node (true);

    result = get_move_from_node (current_node);

    current_node = saved_current;
    return result;
}

static int
get_move_from_node (int handle)
{
    int prop_handle;

    if (handle < 0)
    {
        return -2;
    }

    prop_handle = get_node (handle)->props;


    while (prop_handle >= 0)
    {
        if (get_prop (prop_handle)->type == PROP_BLACK_MOVE ||
            get_prop (prop_handle)->type == PROP_WHITE_MOVE)
        {
            return prop_handle;
        }

        prop_handle = get_prop (prop_handle)->next;
    }

    return -1;
}

static bool
retreat_node (void)
{
    int result = get_node (current_node)->prev;

    if (current_node == start_node)
    {
        return false;
    }

    if (result < 0)
    {
        return false;
    }
    else
    {
        clear_marks_display ();

        current_node = result;

        /* go backwards to the next important node (move/add
           stone/variation/etc.) */
        goto_next_important_node (false);
        return true;
    }
}

static bool
advance_node (void)
{
    int result = get_node (current_node)->next;

    if (result < 0)
    {
        return false;
    }
    else
    {
        clear_marks_display ();

        current_node = result;

        /* go forward to the next move/add stone/variation/etc. node */
        goto_next_important_node (true);
        result = get_prop_sgf (current_node, PROP_VARIATION_CHOICE, NULL);

        if (result >= 0)
        {
            go_to_variation_sgf (get_prop (result)->data.number);
        }

        return true;
    }
}

int
num_variations_sgf (void)
{
    int result = 1;
    struct prop_t *temp_prop;
    struct node_t *temp_node = get_node (current_node);

    if (temp_node == 0)
    {
        return 0;
    }

    if (temp_node->prev >= 0)
    {
        temp_node = get_node (get_node (temp_node->prev)->next);
    }

    temp_prop = get_prop (temp_node->props);

    while (temp_prop)
    {
        if (temp_prop->type == PROP_VARIATION)
        {
            ++result;
        }
        else
        {
            /* variations are at the beginning of the prop list */
            break;
        }

        temp_prop = get_prop (temp_prop->next);
    }

    return result;
}

bool
go_to_variation_sgf (unsigned int num)
{
    int saved = current_node;
    struct node_t *temp_node = get_node (current_node);
    struct prop_t *temp_prop;

    if (!temp_node)
    {
        return false;
    }

    temp_node = get_node (temp_node->prev);

    if (!temp_node)
    {
        return false;
    }

    temp_node = get_node (current_node = temp_node->next);

    if (!temp_node)
    {
        current_node = saved;
        return false;
    }

    temp_prop = get_prop (temp_node->props);

    while (num)
    {
        if (!temp_prop || temp_prop->type != PROP_VARIATION)
        {
            current_node = saved;
            return false;
        }

        if (num == 1)
        {
            current_node = temp_prop->data.node;
            break;
        }

        temp_prop = get_prop (temp_prop->next);
        --num;
    }

    return true;
}

int
get_matching_child_sgf (unsigned short pos, unsigned char color)
{
    struct node_t *temp_node;
    struct prop_t *temp_prop;
    int variation_count = 0;
    int saved;

    /* set true later in a loop if we want a prop in the first variation
       which means that we shouldn't check that branch for children */
    bool dont_check_first_var_children = false;

    temp_node = get_node (current_node);

    if (!temp_node)
    {
        return -3;
    }

    temp_node = get_node (temp_node->next);

    if (!temp_node)
    {
        return -2;
    }

    temp_prop = get_prop (temp_node->props);

    while (temp_prop)
    {
        if (temp_prop->type == PROP_VARIATION)
        {
            ++variation_count;
            saved = current_node;
            current_node = temp_prop->data.node;

            struct prop_t *temp_move = get_prop (get_move_sgf ());

            current_node = saved;

            if (temp_move &&
                temp_move->data.position == pos &&
                ((color == BLACK && temp_move->type == PROP_BLACK_MOVE) ||
                 (color == WHITE && temp_move->type == PROP_WHITE_MOVE)))
            {
                return variation_count;
            }
        }
        else if ((temp_prop->type == PROP_BLACK_MOVE && color == BLACK) ||
                 (temp_prop->type == PROP_WHITE_MOVE && color == WHITE))
        {
            if (temp_prop->data.position == pos)
            {
                return 0;
            }
            else
            {
                return -4;
            }
        }
        else if (temp_prop->type == PROP_ADD_WHITE ||
                 temp_prop->type == PROP_ADD_BLACK ||
                 temp_prop->type == PROP_ADD_EMPTY)
        {
            dont_check_first_var_children = true;
        }

        temp_prop = get_prop (temp_prop->next);
    }

    if (dont_check_first_var_children)
    {
        return -1;
    }
    else
    {
        saved = current_node;
        current_node = temp_node->next;

        struct prop_t *temp_move = get_prop (get_move_sgf ());
        if (temp_move &&
            pos == temp_move->data.position &&
            color == (temp_move->type == PROP_BLACK_MOVE ? BLACK : WHITE))
        {
            current_node = saved;
            return 0;
        }

        current_node = saved;
        return -1;
    }
}

int
next_variation_sgf (void)
{
    int saved = current_node;
    int saved_start = start_node;
    union prop_data_t temp_data;
    int prop_handle;
    int num_vars = 0;

    if (current_node < 0 || get_node (current_node)->prev < 0)
    {
        return -1;
    }

    start_node = NO_NODE;



    if (num_variations_sgf () < 2)
    {

        current_node = saved;
        start_node = saved_start;
        return -2;
    }

    /* now we're at a branch node which we should go to the next variation
       (we were at the "chosen" one, so go to the next one after that,
       (mod the number of variations)) */

    int chosen = 0;
    int branch_node = get_node (get_node (current_node)->prev)->next;

    prop_handle = get_prop_sgf (branch_node, PROP_VARIATION_CHOICE, NULL);

    if (prop_handle >= 0)
    {
        chosen = get_prop (prop_handle)->data.number;
    }

    ++chosen;

    if (chosen >= (num_vars = num_variations_sgf ()))
    {
        chosen = 0;
    }

    temp_data.number = chosen;
    add_or_set_prop_sgf (branch_node, PROP_VARIATION_CHOICE, temp_data);

    if (!undo_node_sgf ())
    {
        current_node = saved;
        start_node = saved_start;
        return -3;
    }

    if (redo_node_sgf ())
    {
        start_node = saved_start;
        return chosen + 1;
    }
    else
    {
        current_node = saved;
        start_node = saved_start;
        return -4;
    }
}

int
add_child_variation (int *variation_number)
{
    struct node_t *temp_node = get_node (current_node);
    struct prop_t *temp_prop;
    int temp_prop_handle;
    int new_node = alloc_storage_sgf ();
    int new_prop = alloc_storage_sgf ();
    int temp_variation_number;

    if (new_node < 0 || new_prop < 0)
    {
        if (new_node >= 0)
        {
            free_storage_sgf (new_node);
        }
        if (new_prop >= 0)
        {
            free_storage_sgf (new_prop);
        }

        return NO_NODE;
    }

    if (!temp_node)
    {
        free_storage_sgf (new_node);
        free_storage_sgf (new_prop);

        return NO_NODE;
    }

    temp_node = get_node (temp_node->next);

    if (!temp_node)
    {
        free_storage_sgf (new_node);
        free_storage_sgf (new_prop);

        return NO_NODE;
    }

    get_node (new_node)->prev = current_node;
    get_node (new_node)->next = NO_NODE;
    get_node (new_node)->props = NO_PROP;

    get_prop (new_prop)->type = PROP_VARIATION;
    get_prop (new_prop)->next = NO_PROP;
    get_prop (new_prop)->data.node = new_node;

    temp_prop_handle = temp_node->props;

    if (temp_prop_handle < 0)
    {
        temp_node->props = new_prop;

        if (variation_number)
        {
            *variation_number = 1;
        }

        return new_node;
    }

    if (get_prop (temp_prop_handle)->type != PROP_VARIATION)
    {
        get_prop (new_prop)->next = temp_node->props;
        temp_node->props = new_prop;

        if (variation_number)
        {
            *variation_number = 1;
        }

        return new_node;
    }

    /* the lowest it can be, since 1 isn't it */
    temp_variation_number = 2;

    while (1)
    {
        temp_prop = get_prop (temp_prop_handle);
        if (temp_prop->next < 0 ||
            get_prop (temp_prop->next)->type != PROP_VARIATION)
        {
            get_prop (new_prop)->next = temp_prop->next;
            temp_prop->next = new_prop;

            if (variation_number)
            {
                *variation_number = temp_variation_number;
            }

            return new_node;
        }

        ++temp_variation_number;
        temp_prop_handle = temp_prop->next;
    }
}

static bool
is_important_node (int handle)
{
    struct prop_t *temp_prop;

    if (handle < 0)
    {
        return false;
    }

    if (handle == start_node)
    {
        return true;
    }

    if (get_node (handle)->prev < 0)
    {
        return true;
    }

    temp_prop = get_prop (get_node (handle)->props);

    while (temp_prop)
    {
        if (temp_prop->type == PROP_BLACK_MOVE ||
            temp_prop->type == PROP_WHITE_MOVE ||
            temp_prop->type == PROP_ADD_BLACK ||
            temp_prop->type == PROP_ADD_WHITE ||
            temp_prop->type == PROP_ADD_EMPTY ||
            temp_prop->type == PROP_VARIATION)
        {
            return true;
        }

        temp_prop = get_prop (temp_prop->next);
    }

    return false;
}

static bool
goto_next_important_node (bool forward)
{
    int temp_node = current_node;
    int last_good = temp_node;

    while (temp_node >= 0 && !is_important_node (temp_node))
    {
        last_good = temp_node;

        temp_node = forward ? get_node (temp_node)->next :
            get_node (temp_node)->prev;

    }

    if (temp_node < 0)
    {
        current_node = last_good;
    }
    else
    {
        current_node = temp_node;
    }

    if (temp_node < 0)
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool
is_handled_sgf (enum prop_type_t type)
{
    if (type == PROP_BLACK_MOVE ||
        type == PROP_WHITE_MOVE ||
        type == PROP_ADD_BLACK ||
        type == PROP_ADD_WHITE ||
        type == PROP_ADD_EMPTY ||
        type == PROP_CIRCLE || type == PROP_SQUARE || type == PROP_TRIANGLE ||
#if 0
        /* these marks are stupid and nobody uses them.  if we could find
           a good way to draw them we could do them anyway, but no reason
           to unless it's easy */
        type == PROP_DIM || type == PROP_SELECTED ||
#endif
        type == PROP_COMMENT ||
        type == PROP_MARK ||
        type == PROP_LABEL ||
        type == PROP_GAME ||
        type == PROP_FILE_FORMAT ||
        type == PROP_APPLICATION ||
        type == PROP_CHARSET ||
        type == PROP_SIZE ||
        type == PROP_KOMI ||
        type == PROP_BLACK_NAME ||
        type == PROP_WHITE_NAME ||
        type == PROP_BLACK_RANK ||
        type == PROP_WHITE_RANK ||
        type == PROP_BLACK_TEAM ||
        type == PROP_WHITE_TEAM ||
        type == PROP_DATE ||
        type == PROP_ROUND ||
        type == PROP_EVENT ||
        type == PROP_PLACE ||
        type == PROP_OVERTIME ||
        type == PROP_RESULT ||
        type == PROP_TIME_LIMIT ||
        type == PROP_RULESET ||
        type == PROP_HANDICAP || type == PROP_VARIATION_TYPE)
    {
        return true;
    }

    return false;
}

void
setup_handicap_sgf (void)
{
    union prop_data_t temp_data;

    if (header.handicap <= 1)
    {
        return;
    }

    current_node = start_node;

    temp_data.number = header.handicap;
    add_prop_sgf (current_node, PROP_HANDICAP, temp_data);

    /* now, add the actual stones */

    if ((board_width != 19 && board_width != 13 && board_width != 9) ||
        board_width != board_height || header.handicap > 9)
    {
        rb->splashf (5 * HZ,
                     "Use the 'Add Black' tool to add %d handicap stones!",
                     header.handicap);
        return;
    }

    int handicaps_to_place = header.handicap;

    int low_coord = 0, mid_coord = 0, high_coord = 0;

    if (board_width == 19)
    {
        low_coord = 3;
        mid_coord = 9;
        high_coord = 15;
    }
    else if (board_width == 13)
    {
        low_coord = 3;
        mid_coord = 6;
        high_coord = 9;
    }
    else if (board_width == 9)
    {
        low_coord = 2;
        mid_coord = 4;
        high_coord = 6;
    }

    /* first four go in the corners */
    handicaps_to_place -= 2;
    setup_handicap_helper (POS (high_coord, low_coord));
    setup_handicap_helper (POS (low_coord, high_coord));

    if (!handicaps_to_place)
    {
        goto done_adding_stones;
    }

    --handicaps_to_place;
    setup_handicap_helper (POS (high_coord, high_coord));

    if (!handicaps_to_place)
    {
        goto done_adding_stones;
    }

    --handicaps_to_place;
    setup_handicap_helper (POS (low_coord, low_coord));

    if (!handicaps_to_place)
    {
        goto done_adding_stones;
    }

    /* now done with first four, if only one left it goes in the center */
    if (handicaps_to_place == 1)
    {
        --handicaps_to_place;
        setup_handicap_helper (POS (mid_coord, mid_coord));
    }
    else
    {
        handicaps_to_place -= 2;
        setup_handicap_helper (POS (high_coord, mid_coord));
        setup_handicap_helper (POS (low_coord, mid_coord));
    }

    if (!handicaps_to_place)
    {
        goto done_adding_stones;
    }

    /* done with first 6 */

    if (handicaps_to_place == 1)
    {
        --handicaps_to_place;
        setup_handicap_helper (POS (mid_coord, mid_coord));
    }
    else
    {
        handicaps_to_place -= 2;
        setup_handicap_helper (POS (mid_coord, high_coord));
        setup_handicap_helper (POS (mid_coord, low_coord));
    }

    if (!handicaps_to_place)
    {
        goto done_adding_stones;
    }

    /* done with first eight, there can only be the tengen remaining */

    setup_handicap_helper (POS (mid_coord, mid_coord));

  done_adding_stones:
    goto_handicap_start_sgf ();
    return;
}

static void
setup_handicap_helper (unsigned short pos)
{
    union prop_data_t temp_data;

    temp_data.position = pos;

    add_prop_sgf (current_node, PROP_ADD_BLACK, temp_data);
}

void
goto_handicap_start_sgf (void)
{
    if (start_node != tree_head)
    {
        current_node = get_node (start_node)->prev;
        redo_node_sgf ();
    }
}

bool
post_game_setup_sgf (void)
{
    int temp_handle = alloc_storage_sgf ();
    int saved = current_node;

    if (temp_handle < 0)
    {
        return false;
    }

    union prop_data_t temp_data;
    temp_data.number = 0;       /* meaningless */

    if (!header_marked)
    {
        add_prop_sgf (tree_head, PROP_ROOT_PROPS, temp_data);
        header_marked = true;
    }

    get_node (temp_handle)->next = current_node;
    get_node (temp_handle)->prev = NO_NODE;
    get_node (temp_handle)->props = NO_PROP;

    current_node = temp_handle;

    redo_node_sgf ();

    if (current_node == temp_handle)
    {
        current_node = saved;
    }

    free_storage_sgf (temp_handle);

    return true;
}
