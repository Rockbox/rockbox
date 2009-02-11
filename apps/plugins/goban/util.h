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

#ifndef GOBAN_UTIL_H
#define GOBAN_UTIL_H

#include "types.h"
#include "goban.h"

/* Call before using a stack, returns false on setup failure */
bool setup_stack (struct stack_t *stack, void *buffer, size_t buffer_size);

/* Push, pop or peek from the stack. Returns false on failure (usually
   stack full or empty, depending on the function) */
bool push_stack (struct stack_t *stack, void *buffer, size_t buffer_size);
bool pop_stack (struct stack_t *stack, void *buffer, size_t buffer_size);
bool peek_stack (struct stack_t *stack, void *buffer, size_t buffer_size);

/* Clear all of the data from the stack and move the stack pointer to the
   beginning */
void empty_stack (struct stack_t *stack);

/* Convenience functions for pushing/poping/peeking standard value types
   to a stack */
#define pop_pos_stack(stack, pos)  pop_stack(stack, pos, sizeof (unsigned short))
#define peek_pos_stack(stack, pos) peek_stack(stack, pos, sizeof (unsigned short))

#define pop_int_stack(stack, num)  pop_stack(stack, num, sizeof (int))
#define peek_int_stack(stack, num) peek_stack(stack, num, sizeof (int))

#define pop_char_stack(stack, num)  pop_stack(stack, num, sizeof (char))
#define peek_char_stack(stack, num) peek_stack(stack, num, sizeof (char))

bool push_pos_stack (struct stack_t *stack, unsigned short pos);
bool push_int_stack (struct stack_t *stack, int num);
bool push_char_stack (struct stack_t *stack, char num);


#define min(x, y) (x < y ? x : y)
#define max(x, y) (x > y ? x : y)

/* Returns the fd of the file */
int create_or_open_file (const char *filename);

/* Returns the number of characters printed */
int snprint_fixed (char *buffer, int buffer_size, int fixed);

/* These should all be obvious, they are simply wrappers on the normal
   rockbox file functions which will loop several times if the rockbox
   file functions don't deal with all of the data at once */
ssize_t read_file (int fd, void *buf, size_t count);
ssize_t write_file (int fd, const void *buf, size_t count);
void close_file (int *fd);

int peek_char (int fd);
int read_char (int fd);
bool write_char (int fd, char to_write);

/* Seek to the next non-whitespace character (doesn't go anywhere if the
   current character is already non-whitespace), and then peek it -1 on
   EOF or error */
int peek_char_no_whitespace (int fd);
/* Same deal, with reading -1 on EOF or error */
int read_char_no_whitespace (int fd);

/* Returns true if a character is whitespace.  Should /NOT/ be called with
   anything but the return value of one of the peeking/reading functions or
   a standard character. */
bool is_whitespace (int value);

/* Gets rid of ']' characdters from the string by overwritting them. This
   is needed in header strings because they would otherwise corrupt the
   SGF file when outputted */
void sanitize_string (char *string);

/* Return an aligned version of the bufer, with the size updated Returns
   NULL if the buffer is too small to align. */
void *align_buffer (void *buffer, size_t * buffer_size);

/* Get the string and buffer size for a SGF property which is stored in
   the header. Returns false on failure, in which case any information set
   in **buffer and *size are not to be trusted or used. */
bool get_header_string_and_size (struct header_t *header,
                                 enum prop_type_t type,
                                 char **buffer, int *size);

/* Output a summary of the game metadata (Game Info)
 */
void metadata_summary (void);

#ifdef GBN_TEST
void run_tests (void);
#endif

#endif
