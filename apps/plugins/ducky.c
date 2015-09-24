/*******************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2015 Franklin Wei
 *
 * Kudos to foolsh and hak5darren for the idea and hardware to test on.
 * Expression-parsing code is from:
 * http://en.literateprograms.org/Shunting_yard_algorithm_%28C%29
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; either version 2 of the License, or (at your option) any later
 * version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ******************************************************************************/

#include "plugin.h"

#include "fixedpoint.h"
#include "lib/pluginlib_exit.h"

/*******************************************************************************
 * The scripting language implemented here is an extension of DuckyScript.
 * DuckyScript as it is now is limited to simple tasks, as it lacks good flow
 * control or variable support.
 *
 * These following extensions to DuckyScript are (to be) implemented.
 *
 * Variables: there are 255 available variables, each addressed by an ASCII
 *            character. However, not all of them are addressable. The digits
 *            0-9, whitespace characters, mathematical operators and _ may NOT
 *            be used as variable names.
 *
 * NOTE: to have a command on a line following a command using an expression, a
 *       semicolon (;) is needed to separate them.
 * NOTE 2: the highest resolution timing achievable is bounded by HZ (so 10ms).
 *
 * "STRING_DELAY <EXPR>;" - sets the delay between keys in a string to <EXPR> ms
 *
 * "JUMP <EXPR>;" - jumps to line <EXPR> in the file
 *
 * "IF <CONDITION>;" - if <CONDITION> equals zero, skip the rest of the line
 *
 * "SET X <EXPR>" - loads the value of <EXPR> into variable X. Greedy.
 * "SWP X Y" - X <-> Y
 *
 * "OUT <EXPR>;" - outputs the value of <EXPR> in base 10
 * "OUTHEX <EXPR>;" - outputs the value of <EXPR> in hexadecimal
 * "ASCII <EXPR>;" - outputs the ASCII equivalent of <EXPR> if typeable
 *
 * "LOG ..." - outputs any remaining text to the device's screen. Greedy.
 * "LOGVAR <EXPR>;" - outputs variable <EXPR> in decimal to the device's screen
 ******************************************************************************/

/*** Defines ***/

/* how long a key needs to be held down before forcing execution */
#define FORCE_EXEC_THRES (HZ / 2)

#define DEFAULT_DELAY 0
#define STRING_DELAY 0
#define TOKEN_IS(str) (rb->strcmp(tok, str) == 0)
#define MAX_LINE_LEN 512

#define MAXOPSTACK 64
#define MAXNUMSTACK 64
#define CALL_STACK_SZ 64
#define VARMAP_SIZE 256

#define VARFORMAT "%d"
#define VARNAME_MAX 15

typedef int vartype;

/*** Globals ***/

off_t *line_offset = NULL;
int default_delay = DEFAULT_DELAY, string_delay = STRING_DELAY;

unsigned lines_executed = 0, current_line = 0, num_lines;

unsigned call_stack[CALL_STACK_SZ];
unsigned stack_frame = 0;

int log_fd = -1, file_des = -1;

int keys_sent = 0;

struct varnode_t {
    char name[VARNAME_MAX + 1];
    vartype val;
    struct varnode_t *next;
};

void error(const char *fmt, ...) __attribute__((noreturn,format(printf,1,2)));
void vid_write(const char *str);
void vid_writef(const char *fmt, ...) __attribute__((format(printf,1,2)));

size_t malloc_remaining;

void *my_malloc(size_t sz)
{
    static void *plugin_buffer = NULL;
    if(!plugin_buffer)
        plugin_buffer = rb->plugin_get_buffer(&malloc_remaining);

    void *ret = plugin_buffer;
    plugin_buffer += sz;
    if((signed long) malloc_remaining - (signed long) sz < 0)
    {
        error("OOM");
    }
    malloc_remaining -= sz;
    return ret;
}

/* variables are stored in a chained hash map */

struct varnode_t *var_map[VARMAP_SIZE];

/* simple DJB hash */
uint32_t hash_string(const char *str, int *len)
{
    uint32_t hash = 5381;
    *len = 0;
    char c;
    while((c = *str++))
    {
        hash = ((hash << 5) + hash) ^ c;
        (*len)++;
    }

    return hash;
}

struct varnode_t *lookup_var(const char *name)
{
    int len = 0;
    uint32_t hash = hash_string(name, &len) % VARMAP_SIZE;

    struct varnode_t *iter = var_map[hash], *last = NULL;

    while(iter)
    {
        /* could use strcmp here, but memcmp is faster */
        if(rb->memcmp(name, iter->name, len) == 0)
            return iter;
        last = iter;
        iter = iter->next;
    }
    /* not found in this bucket, so add it to the linked list */
    struct varnode_t *new = my_malloc(sizeof(struct varnode_t));

    rb->strlcpy(new->name, name, sizeof(new->name));
    new->val = 0;
    new->next = NULL;

    if(!last)
        var_map[hash] = new;
    else
        last->next = new;

    return new;
}

vartype getVariable(const char *name)
{
    vid_writef("getvariable %s", name);
    return lookup_var(name)->val;
}

void setVariable(const char *name, vartype val)
{
    lookup_var(name)->val = val;
}

void incVar(const char *name)
{
    ++lookup_var(name)->val;
}

void decVar(const char *name)
{
    --lookup_var(name)->val;
}

/*** Utility functions ***/

void exit_handler(void)
{
    if(file_des >= 0)
        rb->close(file_des);
    if(log_fd >= 0)
        rb->close(log_fd);
}

void vid_write(const char *str)
{
    static int curs_y = 0, curs_x = 0;
    static int num_rows, num_cols;

    if(!num_rows)
    {
        int w, h;
        rb->lcd_getstringsize("X", &w, &h);

        num_rows = LCD_HEIGHT / h;
        num_cols = LCD_WIDTH / w;

        log_fd  = rb->open("/ducky_log.txt", O_WRONLY | O_CREAT | O_TRUNC, 0666);

        if(log_fd < 0)
            exit(PLUGIN_ERROR); /* don't call error() */
    }

    const char newline = '\n';

    rb->write(log_fd, str, rb->strlen(str));
    rb->write(log_fd, &newline, 1);

    while(1)
    {
        char c = *str;
        if(!c)
            break;
        str++;

        if(c == '\n')
        {
            curs_y = (curs_y + 1) % num_rows;
            curs_x = 0;
        }
        else
        {
            if(curs_x + 1 > num_cols)
            {
                curs_y = (curs_y + 1) % num_rows;
                curs_x = 0;
            }
            rb->lcd_putsf(curs_x, curs_y, "%c", c);

            ++curs_x;
        }
    }

    rb->lcd_update();

    curs_y = (curs_y + 1) % num_rows;
    if(!curs_y)
        rb->lcd_clear_display();

    curs_x = 0;
}

void __attribute__((format(printf,1,2))) vid_writef(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char buf[256];
    rb->vsnprintf(buf, sizeof(buf), fmt, ap);
    vid_write(buf);
    va_end(ap);
}

void __attribute__((noreturn,format(printf,1,2))) error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char buf[256];
    rb->vsnprintf(buf, sizeof(buf), fmt, ap);
    vid_writef("Line %d: ERROR: %s", current_line, buf);
    va_end(ap);

    rb->action_userabort(-1);
    exit(PLUGIN_ERROR);
}

void __attribute__((format(printf,1,2))) warning(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char buf[256];
    rb->vsnprintf(buf, sizeof(buf), fmt, ap);
    vid_writef("Line %d: WARNING: %s", current_line, buf);
    va_end(ap);
}

/* index_lines() precalculates the offset of each line for faster jumping */

off_t *index_lines(int fd, unsigned *numlines)
{
    vid_write("Indexing file...");

    /* allocate part of the plugin buffer to store line number info in */
    /* each index points to the offset within the file for that line */

    off_t *data = my_malloc(sizeof(off_t));
    size_t max_lines = malloc_remaining / sizeof(off_t);

    /*  this uses 1-indexed line numbers, so the first indice is wasted */
    unsigned idx = 1;

    /* there's no realloc(), so it's worked around by calling malloc() to
       increment the allocation pointer*/
    my_malloc(sizeof(off_t));
    data[idx++] = 0;

    while(1)
    {
        char buf[MAX_LINE_LEN];
        if(!rb->read_line(fd, buf, sizeof(buf)))
            break;
        my_malloc(sizeof(off_t));
        data[idx++] = rb->lseek(fd, 0, SEEK_CUR);
        if(idx > max_lines)
            error("too many lines in file (max = %d)!", max_lines);
    }
    vid_writef("Lines: %d (max %d)", idx - 1, max_lines);

    rb->lseek(fd, 0, SEEK_SET);

    *numlines = idx - 1;

    return data;
}

inline void send(int status)
{
    rb->usb_hid_send(HID_USAGE_PAGE_KEYBOARD_KEYPAD, status);
    ++keys_sent;
}

inline void jump_line(int fd, unsigned where)
{
    vid_writef("JUMP to line %d", where);
    if(1 <= where && where <= num_lines)
        rb->lseek(fd, line_offset[where], SEEK_SET);
    else
        error("JUMP target out of range (%d)", where);
    current_line = where - 1;
}

inline void sub_call(int fd, unsigned where)
{
    if(stack_frame < ARRAYLEN(call_stack))
    {
        call_stack[stack_frame] = current_line + 1;
        ++stack_frame;
        jump_line(fd, where);
    }
    else
        error("call stack overflow");
}

inline void sub_return(int fd)
{
    if(stack_frame > 0)
    {
        --stack_frame;
        jump_line(fd, call_stack[stack_frame]);
    }
}

/* Rockbox's HID driver supports up to 4 keys simultaneously, 1 in each byte */

inline void add_key(int *keystate, unsigned *nkeys, int newkey)
{
    *keystate = (*keystate << 8) | newkey;
    if(nkeys)
        (*nkeys)++;
}

struct char_mapping {
    char c;
    int key;
};

struct char_mapping shift_tab[] = {
    { '~', HID_KEYBOARD_BACKTICK },
    { '!', HID_KEYBOARD_1 },
    { '@', HID_KEYBOARD_2 },
    { '#', HID_KEYBOARD_3 },
    { '$', HID_KEYBOARD_4 },
    { '%', HID_KEYBOARD_5 },
    { '^', HID_KEYBOARD_6 },
    { '&', HID_KEYBOARD_7 },
    { '*', HID_KEYBOARD_8 },
    { '(', HID_KEYBOARD_9 },
    { ')', HID_KEYBOARD_0 },
    { '_', HID_KEYBOARD_HYPHEN },
    { '+', HID_KEYBOARD_EQUAL_SIGN },
    { '}', HID_KEYBOARD_RIGHT_BRACKET },
    { '{', HID_KEYBOARD_LEFT_BRACKET },
    { '|', HID_KEYBOARD_BACKSLASH },
    { '"', HID_KEYBOARD_QUOTE },
    { ':', HID_KEYBOARD_SEMICOLON },
    { '?', HID_KEYBOARD_SLASH },
    { '>', HID_KEYBOARD_DOT },
    { '<', HID_KEYBOARD_COMMA },
};

struct char_mapping char_tab[] = {
    { ' ', HID_KEYBOARD_SPACEBAR },
    { '`', HID_KEYBOARD_BACKTICK },
    { '-', HID_KEYBOARD_HYPHEN },
    { '=', HID_KEYBOARD_EQUAL_SIGN },
    { '[', HID_KEYBOARD_LEFT_BRACKET },
    { ']', HID_KEYBOARD_RIGHT_BRACKET },
    { '\\', HID_KEYBOARD_BACKSLASH },
    { '\'', HID_KEYBOARD_QUOTE },
    { ';', HID_KEYBOARD_SEMICOLON },
    { '/', HID_KEYBOARD_SLASH },
    { ',', HID_KEYBOARD_COMMA },
    { '.', HID_KEYBOARD_DOT },
    { '\t',HID_KEYBOARD_TAB },
};

void add_char(int *keystate, unsigned *nkeys, char c)
{
    if('a' <= c && c <= 'z')
    {
        add_key(keystate, nkeys, c - 'a' + HID_KEYBOARD_A);
    }
    else if('A' <= c && c <= 'Z')
    {
        add_key(keystate, nkeys, HID_KEYBOARD_LEFT_SHIFT);
        add_key(keystate, nkeys, c - 'A' + HID_KEYBOARD_A);
    }
    else if('0' <= c && c <= '9')
    {
        if(c == '0')
            add_key(keystate, nkeys, HID_KEYBOARD_0);
        else
            add_key(keystate, nkeys, c - '1' + HID_KEYBOARD_1);
    }
    else
    {
        /* search the character table */
        for(unsigned int i = 0; i < ARRAYLEN(char_tab); ++i)
        {
            if(char_tab[i].c == c)
            {
                add_key(keystate, nkeys, char_tab[i].key);
                return;
            }
        }

        /* search the shift-mapping table */
        for(unsigned int i = 0; i < ARRAYLEN(shift_tab); ++i)
        {
            if(shift_tab[i].c == c)
            {
                add_key(keystate, nkeys, HID_KEYBOARD_LEFT_SHIFT);
                add_key(keystate, nkeys, shift_tab[i].key);
                return;
            }
        }

        warning("invalid character '%c'", c);
    }
}

inline bool handle_fn_key(char *tok, int *keystate, unsigned *num_keys)
{
    int len = rb->strlen(tok);
    /* check if it's a function key and handle it */
    if(2 <= len && len <= 3)
    {
        if(tok[0] == 'F')
        {
            ++tok;
            int num = rb->atoi(tok);
            if(1 <= num && num <= 12)
            {
                add_key(keystate, num_keys, HID_KEYBOARD_F1 + rb->atoi(tok) - 1);
                return true;
            }
            else if(13 <= num && num <= 24)
            {
                add_key(keystate, num_keys, HID_KEYBOARD_F13 + rb->atoi(tok) - 13);
                return true;
            }
        }
    }
    return false;
}

/** Expression Parsing **/

/* based on http://en.literateprograms.org/Shunting_yard_algorithm_%28C%29 */

vartype eval_uminus(vartype a1, vartype a2)
{
    (void) a2;
    return -a1;
}
vartype eval_exp(vartype a1, vartype a2)
{
    return a2<0 ? 0 : (a2==0?1:a1*eval_exp(a1, a2-1));
}
vartype eval_mul(vartype a1, vartype a2)
{
    return a1*a2;
}
vartype eval_div(vartype a1, vartype a2)
{
    if(!a2) {
        error("division by zero");
    }
    return a1/a2;
}
vartype eval_mod(vartype a1, vartype a2)
{
    if(!a2) {
        error("division by zero");
    }
    return a1%a2;
}
vartype eval_add(vartype a1, vartype a2)
{
    return a1+a2;
}
vartype eval_sub(vartype a1, vartype a2)
{
    return a1-a2;
}
vartype eval_eq(vartype a1, vartype a2)
{
    return a1 == a2;
}
vartype eval_neq(vartype a1, vartype a2)
{
    return a1 != a2;
}
vartype eval_leq(vartype a1, vartype a2)
{
    return a1 <= a2;
}
vartype eval_geq(vartype a1, vartype a2)
{
    return a1 >= a2;
}
vartype eval_lt(vartype a1, vartype a2)
{
    return a1 < a2;
}
vartype eval_gt(vartype a1, vartype a2)
{
    return a1 > a2;
}
vartype eval_log_neg(vartype a1, vartype a2)
{
    (void) a2;
    return !a1;
}
vartype eval_log_and(vartype a1, vartype a2)
{
    return a1 && a2;
}
vartype eval_log_or(vartype a1, vartype a2)
{
    return a1 || a2;
}
vartype eval_bit_and(vartype a1, vartype a2)
{
    return a1 & a2;
}
vartype eval_bit_xor(vartype a1, vartype a2)
{
    return a1 ^ a2;
}
vartype eval_bit_or(vartype a1, vartype a2)
{
    return a1 | a2;
}
vartype eval_bit_comp(vartype a1, vartype a2)
{
    (void) a2;
    return ~a1;
}
vartype eval_lsh(vartype a1, vartype a2)
{
    return a1 << a2;
}
vartype eval_rsh(vartype a1, vartype a2)
{
    return a1 >> a2;
}
vartype eval_sqrt(vartype a1, vartype a2)
{
    (void) a2;
    return isqrt(a1) + 1;
}

enum {ASSOC_NONE=0, ASSOC_LEFT, ASSOC_RIGHT};

/* order matters in this table, because operators can share prefixes */
/* apart from that, they should be ordered by frequency of use */
/* operator precedence is based on that of C */
/* frequency is based off a crude analysis of the rockbox source tree: */

/* 99639 * */
/* 48282 -  */
/* 46639 +  */
/* 27678 & */
/* 24542 <  */
/* 21862 /  */
/* 20000 | */
/* 19138 == */
/* 12694 %  */
/* 11619 >  */
/* 11087 !  */
/* 8230 << */
/* 7339 && */
/* 7180 != */
/* 6010 >> */
/* 5575 || */
/* 3121 ~ */
/* 1311 ^ */

struct op_s {
    const char *op;
    int prec;
    int assoc;
    int unary;
    vartype (*eval)(vartype a1, vartype a2);
    unsigned int len;
} ops[] = {
    {"+",    20, ASSOC_LEFT,  0, eval_add, -1},
    {"-",    20, ASSOC_LEFT,  0, eval_sub, -1},
    {"**",   40, ASSOC_RIGHT, 0, eval_exp, -1},
    {"*",    30, ASSOC_LEFT,  0, eval_mul, -1},
    {"&&",   8,  ASSOC_LEFT,  0, eval_log_and, -1},
    {"&",    11, ASSOC_LEFT,  0, eval_bit_and, -1},
    {"<<",   15, ASSOC_LEFT,  0, eval_lsh, -1},
    {">>",   15, ASSOC_LEFT,  0, eval_rsh, -1},
    {"<=",   14, ASSOC_LEFT,  0, eval_leq, -1},
    {">=",   14, ASSOC_LEFT,  0, eval_geq, -1},
    {"<",    14, ASSOC_LEFT,  0, eval_lt, -1},
    {">",    14, ASSOC_LEFT,  0, eval_gt, -1},
    {"/",    30, ASSOC_LEFT,  0, eval_div, -1},
    {"||",   7,  ASSOC_LEFT,  0, eval_log_or, -1},
    {"|",    9,  ASSOC_LEFT,  0, eval_bit_or, -1},
    {"==",   12, ASSOC_LEFT,  0, eval_eq, -1},
    {"!=",   12, ASSOC_LEFT,  0, eval_neq, -1},
    {"%",    30, ASSOC_LEFT,  0, eval_mod, -1},
    {"!",    50, ASSOC_LEFT,  1, eval_log_neg, -1},
    {"~",    50, ASSOC_LEFT,  1, eval_bit_comp, -1},
    {"^",    10, ASSOC_LEFT,  0, eval_bit_xor, -1},
    {"(",    0,  ASSOC_NONE,  0, NULL, -1},
    {")",    0,  ASSOC_NONE,  0, NULL, -1},
    {"sqrt", 1,  ASSOC_LEFT,  1, eval_sqrt, -1},
};

/* specially calculated for maximum efficiency, don't change this! */
#define OPMAP_SIZE 58

uint32_t fasthash(const char *str, size_t maxlen)
{
    uint32_t ret = 0;
    char c;
    while((c = *str++) && maxlen-- > 0)
    {
        ret += c;
        ret *= 27;
    }
    return ret % OPMAP_SIZE;
}

/* optimized hash map for fast lookup of operators */
struct op_s op_map[OPMAP_SIZE];
size_t longest_op = 0;

void opmap_insert(struct op_s *op)
{
    uint32_t hash = fasthash(op->op, op->len) % OPMAP_SIZE;
    if(op_map[hash].op)
        error("hash map collision %lu: %s VS %s", hash, op->op, op_map[hash].op);
    rb->memcpy(op_map+hash, op, sizeof(*op));
}

void init_optable(void)
{
    rb->memset(op_map, 0, sizeof(op_map));
    for(unsigned int i = 0; i < ARRAYLEN(ops); ++i)
    {
        ops[i].len = rb->strlen(ops[i].op);
        if(ops[i].len > longest_op)
            longest_op = ops[i].len;
        opmap_insert(ops+i);
    }
}

const struct op_s *getop(const char *ch, int *len)
{
    vid_writef("getop %s", ch);
    unsigned int i = 1;
    do {
        uint32_t hash = fasthash(ch, i);
        vid_writef("hash: %d - : %d", hash, fasthash("-", 1));
        if(op_map[hash].op && rb->strncmp(ch, op_map[hash].op, op_map[hash].len) == 0)
        {
            *len = op_map[hash].len;
            return op_map + hash;
        }
    } while(ch[i++] && i <= longest_op);
    vid_write("not found");
    return NULL;
}

const struct op_s *opstack[MAXOPSTACK];
int nopstack;

vartype numstack[MAXNUMSTACK];
int nnumstack;

void push_opstack(const struct op_s *op)
{
    if(nopstack>MAXOPSTACK - 1) {
        error("operator stack overflow");
    }
    opstack[nopstack++] = op;
}

const struct op_s *pop_opstack(void)
{
    if(!nopstack) {
        error("operator stack empty");
    }
    return opstack[--nopstack];
}

void push_numstack(vartype num)
{
    if(nnumstack>MAXNUMSTACK - 1) {
        error("number stack overflow");
    }
    numstack[nnumstack++] = num;
}

vartype pop_numstack(void)
{
    if(!nnumstack) {
        error("number stack empty");
    }
    return numstack[--nnumstack];
}

inline bool isDigit(char c)
{
    return '0' <= c && c <= '9';
}

inline bool isValidNumber(char *str)
{
    //vid_writef("isValidNumber %s", str);
    if(str && (isDigit(*str) || *str == '-'))
    {
        while(1)
        {
            char c = *str++;
            if(!c)
                break;
            if(!isDigit(c))
                return false;
        }
        return true;
    }
    return false;
}

inline bool isSpace(char c)
{
    //vid_writef("isSpace '%c'", c);
    return (c == ' ') || (c == '\t');
}

inline bool isValidVariable(const char *c)
{
    //vid_writef("isValidVariable %s", c);
    if(!isDigit(*c) && !getop(c, NULL) && !isSpace(*c))
    {
        return true;
    }
    return false;
}

/** BEGIN BSD-LICENSED CODE **/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Convert a string to a long integer.
 *
 * Ignores `locale' stuff.  Assumes that the upper and lower case
 * alphabets and digits are each contiguous.
 */
long
strtol(const char *nptr, char **endptr, int base)
{
        const char *s;
        long acc, cutoff;
        int c;
        int neg, any, cutlim;

        /*
         * Skip white space and pick up leading +/- sign if any.
         * If base is 0, allow 0x for hex and 0 for octal, else
         * assume decimal; if base is already 16, allow 0x.
         */
        s = nptr;
        do {
                c = (unsigned char) *s++;
        } while (c <= ' ' || c >= 0x7f);
        if (c == '-') {
                neg = 1;
                c = *s++;
        } else {
                neg = 0;
                if (c == '+')
                        c = *s++;
        }
        if ((base == 0 || base == 16) &&
            c == '0' && (*s == 'x' || *s == 'X')) {
                c = s[1];
                s += 2;
                base = 16;
        }
        if (base == 0)
                base = c == '0' ? 8 : 10;

        /*
         * Compute the cutoff value between legal numbers and illegal
         * numbers.  That is the largest legal value, divided by the
         * base.  An input number that is greater than this value, if
         * followed by a legal input character, is too big.  One that
         * is equal to this value may be valid or not; the limit
         * between valid and invalid numbers is then based on the last
         * digit.  For instance, if the range for longs is
         * [-2147483648..2147483647] and the input base is 10,
         * cutoff will be set to 214748364 and cutlim to either
         * 7 (neg==0) or 8 (neg==1), meaning that if we have accumulated
         * a value > 214748364, or equal but the next digit is > 7 (or 8),
         * the number is too big, and we will return a range error.
         *
         * Set any if any `digits' consumed; make it negative to indicate
         * overflow.
         */
        cutoff = neg ? LONG_MIN : LONG_MAX;
        cutlim = cutoff % base;
        cutoff /= base;
        if (neg) {
                if (cutlim > 0) {
                        cutlim -= base;
                        cutoff += 1;
                }
                cutlim = -cutlim;
        }
        for (acc = 0, any = 0;; c = (unsigned char) *s++) {
                if (c >= '0' && c <= '9')
                        c -= '0';
                else if (c >= 'A' && c <= 'Z')
                        c -= 'A' - 10;
                else if (c >= 'a' && c <= 'z')
                        c -= 'a' - 10;
                else
                        break;
                if (c >= base)
                        break;
                if (any < 0)
                        continue;
                if (neg) {
                        if (acc < cutoff || (acc == cutoff && c > cutlim)) {
                                any = -1;
                                acc = LONG_MIN;
                        } else {
                                any = 1;
                                acc *= base;
                                acc -= c;
                        }
                } else {
                        if (acc > cutoff || (acc == cutoff && c > cutlim)) {
                                any = -1;
                                acc = LONG_MAX;
                        } else {
                                any = 1;
                                acc *= base;
                                acc += c;
                        }
                }
        }
        if (endptr != 0)
                *endptr = (char *) (any ? s - 1 : nptr);
        return (acc);
}

/** END OF BSD-LICENSED CODE **/

vartype getValue(char *str, char *cur)
{
    //vid_writef("getValue %s", str);
    if(str && isValidVariable(str))
    {
        /* isolate the variable name into a buffer */
        char varname[VARNAME_MAX + 1] = { 0 };
        rb->memcpy(varname, str, cur - str);
        return getVariable(varname);
    }
    return strtol(str, NULL, 0);
}

inline bool isValidNumberOrVariable(const char *c)
{
    //vid_writef("isValidNumberOrVariable %s", c);
    if(isDigit(*c) || isValidVariable(c))
    {
        return true;
    }
    return false;
}

void shunt_op(const struct op_s *op)
{
    const struct op_s *pop;
    vartype n1, n2;
    if(rb->strcmp(op->op, "(") == 0)
    {
        push_opstack(op);
        return;
    }
    else if(rb->strcmp(op->op, ")") == 0)
    {
        while(nopstack > 0 && rb->strcmp(opstack[nopstack-1]->op, "(") != 0)
        {
            pop = pop_opstack();
            n1  = pop_numstack();

            if(pop->unary)
                push_numstack(pop->eval(n1, 0));
            else
            {
                n2 = pop_numstack();
                push_numstack(pop->eval(n2, n1));
            }
        }

        if(!(pop = pop_opstack()) || rb->strcmp(pop->op,"(") != 0)
        {
            error("mismatched parentheses");
        }
        return;
    }

    if(op->assoc == ASSOC_LEFT)
    {
        while(nopstack && op->prec <= opstack[nopstack - 1]->prec)
        {
            pop = pop_opstack();
            n1  = pop_numstack();
            if(pop->unary)
                push_numstack(pop->eval(n1, 0));
            else
            {
                n2 = pop_numstack();
                push_numstack(pop->eval(n2, n1));
            }
        }
    }
    else
    {
        while(nopstack && op->prec<opstack[nopstack - 1]->prec)
        {
            pop = pop_opstack();
            n1  = pop_numstack();
            if(pop->unary)
                push_numstack(pop->eval(n1, 0));
            else
            {
                n2 = pop_numstack();
                push_numstack(pop->eval(n2, n1));
            }
        }
    }

    push_opstack(op);
}

vartype eval_expr(char *str)
{
    //vid_write("**************** EVAL EXPR ***************");

    /* token start */
    char *tstart = NULL;

    /* hard-code some operators that are for internal use only */
    const struct op_s startop = {"startop_", 0, ASSOC_NONE, 0, NULL, rb->strlen("startop_")};
    const struct op_s unaryminus = {"-", 50, ASSOC_RIGHT, 1, eval_uminus, rb->strlen("-")};

    const struct op_s *op = NULL;
    vartype n1, n2;
    const struct op_s *lastop = &startop;

    nopstack = 0;
    nnumstack = 0;

    int len;
    char *expr;
    for(expr = str; *expr; expr += len)
    {
        //vid_write("****** PARSING A CHARACTER ******");
        len = 1;
        if(!tstart)
        {
            if((op = getop(expr, &len)))
            {
                if(lastop && (lastop == &startop || rb->strcmp(lastop->op, ")") != 0))
                {
                    if(rb->strcmp(op->op, "-") == 0)
                    {
                        op = &unaryminus;
                        len = 1;
                    }
                    else if(rb->strcmp(op->op, "(") != 0 && !op->unary)
                    {
                        error("illegal use of binary operator (%s)", op->op);
                    }
                }
                shunt_op(op);
                lastop = op;
            }
            else if(isValidNumberOrVariable(expr))
                tstart = expr;
            else if(!isSpace(*expr))
            {
                error("syntax error");
            }
        }
        else
        {
            if(isSpace(*expr))
            {
                push_numstack(getValue(tstart, expr));
                tstart = NULL;
                lastop = NULL;
            }
            else if((op = getop(expr, &len)))
            {
                push_numstack(getValue(tstart, expr));
                tstart = NULL;
                shunt_op(op);
                lastop = op;
            }
            else if(!isValidNumberOrVariable(expr))
            {
                error("syntax error");
            }
        }
    }

    if(tstart)
        push_numstack(getValue(tstart, expr));

    while(nopstack) {
        op = pop_opstack();
        n1 = pop_numstack();
        if(!op->unary)
        {
            n2 = pop_numstack();
            push_numstack(op->eval(n2, n1));
        }
        else
            push_numstack(op->eval(n1, 0));
    }

    if(nnumstack != 1) {
        error("invalid expression");
    }

    vid_writef("eval_expr %s = %d", str, numstack[0]);
    return numstack[0];
}

void send_string(const char *str)
{
    while(*str)
    {
        int string_state = 0;
        if(!*str)
            break;
        add_char(&string_state, NULL, *str);

        send(string_state);

        ++str;

        /* rb->sleep(0) sleeps till the next tick, which is not what
           we want */
        if(string_delay)
        {
            rb->sleep(string_delay);
            rb->yield();
        }
    }
}

#define KEY_HANDLER(key, name)                                    \
    int name ## _handler(char **save, int *keystate, int *numkeys, int *repeats_left) \
    {                                                             \
        add_key(keystate, numkeys, key);                          \
        (void) save; (void) repeats_left;                         \
        return 0;                                                 \
    }

KEY_HANDLER(HID_KEYBOARD_RETURN, enter);
KEY_HANDLER(HID_KEYBOARD_DELETE, del);
KEY_HANDLER(HID_KEYBOARD_TAB, tab);
KEY_HANDLER(HID_KEYBOARD_ESCAPE, esc);
KEY_HANDLER(HID_KEYBOARD_LEFT_GUI, gui);
KEY_HANDLER(HID_KEYBOARD_RIGHT_GUI, rgui);
KEY_HANDLER(HID_KEYBOARD_LEFT_CONTROL, ctrl);
KEY_HANDLER(HID_KEYBOARD_RIGHT_CONTROL, rctrl);
KEY_HANDLER(HID_KEYBOARD_LEFT_ALT, alt);
KEY_HANDLER(HID_KEYBOARD_RIGHT_ALT, ralt);
KEY_HANDLER(HID_KEYBOARD_LEFT_SHIFT, shift)
KEY_HANDLER(HID_KEYBOARD_RIGHT_SHIFT, rshift);
KEY_HANDLER(HID_KEYBOARD_MENU, menu);
KEY_HANDLER(HID_KEYBOARD_PAUSE, pause);
KEY_HANDLER(HID_KEYBOARD_PRINT_SCREEN, psn);
KEY_HANDLER(HID_KEYPAD_NUM_LOCK_AND_CLEAR, numlck);
KEY_HANDLER(HID_KEYBOARD_CAPS_LOCK, cpslck);
KEY_HANDLER(HID_KEYBOARD_SCROLL_LOCK, scrllck);
KEY_HANDLER(HID_KEYBOARD_BACKSPACE, bksp);
KEY_HANDLER(HID_KEYBOARD_UP_ARROW, up);
KEY_HANDLER(HID_KEYBOARD_DOWN_ARROW, down);
KEY_HANDLER(HID_KEYBOARD_LEFT_ARROW, left);
KEY_HANDLER(HID_KEYBOARD_RIGHT_ARROW, right);

KEY_HANDLER(HID_KEYBOARD_PAGE_UP, pgup);
KEY_HANDLER(HID_KEYBOARD_PAGE_DOWN, pgdn);

KEY_HANDLER(HID_KEYBOARD_INSERT, ins);
KEY_HANDLER(HID_KEYBOARD_HOME, home);

KEY_HANDLER(HID_KEYBOARD_F1, f1);
KEY_HANDLER(HID_KEYBOARD_F2, f2);
KEY_HANDLER(HID_KEYBOARD_F3, f3);
KEY_HANDLER(HID_KEYBOARD_F4, f4);
KEY_HANDLER(HID_KEYBOARD_F5, f5);
KEY_HANDLER(HID_KEYBOARD_F6, f6);
KEY_HANDLER(HID_KEYBOARD_F7, f7);
KEY_HANDLER(HID_KEYBOARD_F8, f8);
KEY_HANDLER(HID_KEYBOARD_F9, f9);
KEY_HANDLER(HID_KEYBOARD_F10, f10);
KEY_HANDLER(HID_KEYBOARD_F11, f11);
KEY_HANDLER(HID_KEYBOARD_F12, f12);

#define OK 0
#define DONE 1
#define NEXT 2
#define BREAK 3

int string_handler(char **save, int *keystate, int *nkeys, int *repeats_left)
{
    char *str = rb->strtok_r(NULL, "", save);
    if(!str)
        return OK;
    send_string(str);
    return OK;
}

int let_handler(char **save, int *keystate, int *nkeys, int *repeats_left)
{
    char *varname = rb->strtok_r(NULL, "= \t", save);

    if(varname && isValidVariable(varname))
    {
        struct varnode_t *node = lookup_var(varname);

        char *tok = rb->strtok_r(NULL, "=;", save);
        vid_writef("VARNAME IS %s", varname);
        if(tok)
            node->val = eval_expr(tok);
        else
            error("exprected valid expression after LET");
    }
    else
    {
        error("invalid variable name for LET");
    }
    return OK;
}

int repeat_handler(char **save, int *keystate, int *nkeys, int *repeats_left)
{
    char *tok = rb->strtok_r(NULL, ";", save);
    if(tok)
    {
        if(current_line == 1)
            error("REPEAT without a previous instruction");
        *repeats_left = eval_expr(tok) - 1;
        jump_line(file_des, current_line - 1);
        return NEXT;
    }
    else
        error("expected valid expression after REPEAT");
}

int goto_handler(char **save, int *keystate, int *nkeys, int *repeats_left)
{
    char *tok = rb->strtok_r(NULL, ";", save);
    if(tok)
    {
        jump_line(file_des, eval_expr(tok));
        return NEXT;
    }
    else
        error("expected valid expression after GOTO");
}

int call_handler(char **save, int *keystate, int *nkeys, int *repeats_left)
{
    char *tok = rb->strtok_r(NULL, "", save);
    if(tok)
    {
        sub_call(file_des, eval_expr(tok));
        return NEXT;
    }
    else
        error("expected destination for CALL");
}

int ret_handler(char **save, int *keystate, int *nkeys, int *repeats_left)
{
    sub_return(file_des);
    return NEXT;
}

int inc_handler(char **save, int *keystate, int *nkeys, int *repeats_left)
{
    char *tok = rb->strtok_r(NULL, " \t", save);
    if(isValidVariable(tok))
        incVar(tok);
    return OK;
}

int dec_handler(char **save, int *keystate, int *nkeys, int *repeats_left)
{
    char *tok = rb->strtok_r(NULL, " \t", save);
    if(isValidVariable(tok))
        decVar(tok);
    return OK;
}

int if_handler(char **save, int *keystate, int *nkeys, int *repeats_left)
{
    char *tok = rb->strtok_r(NULL, ";", save);

    if(!tok)
        error("expected conditional after IF");

    /* break out of the do-while if the condition is false */
    if(!eval_expr(tok))
    {
        return BREAK;
    }
    return OK;
}

int delay_handler(char **save, int *keystate, int *nkeys, int *repeats_left)
{
    /* delay N 1000ths of a sec */
    char *tok = rb->strtok_r(NULL, ";", save);
    if(tok)
        rb->sleep((HZ / 100) * eval_expr(tok) / 10);
    else
        error("expected valid expression after DELAY");

    return OK;
}

int out_handler(char **save, int *keystate, int *nkeys, int *repeats_left)
{
    char *tok = rb->strtok_r(NULL, ";", save);
    if(tok)
    {
        char str[32];
        rb->snprintf(str, sizeof(str), VARFORMAT, eval_expr(tok));
        send_string(str);
        return OK;
    }
    else
        error("expected valid variable name for OUT");
}

int dfldel_handler(char **save, int *keystate, int *nkeys, int *repeats_left)
{
    /* sets time between instructions, 1000ths of a second */
    char *tok = rb->strtok_r(NULL, ";", save);
    if(tok)
    {
        default_delay = eval_expr(tok) / 10 * (HZ / 100);
        return OK;
    }
    else
        error("expected valid expression after DEFAULT_DELAY");
}

int strdel_handler(char **save, int *keystate, int *nkeys, int *repeats_left)
{
    char *tok = rb->strtok_r(NULL, ";", save);
    if(tok)
    {
        string_delay = eval_expr(tok) / 10 * (HZ / 100);
        return OK;
    }
    else
        error("expected valid expression after STRING_DELAY");
}

int log_handler(char **save, int *keystate, int *nkeys, int *repeats_left)
{
    char *tok = rb->strtok_r(NULL, "", save);

    vid_write(tok);
    return OK;
}

int logvar_handler(char **save, int *keystate, int *nkeys, int *repeats_left)
{
    char *tok = rb->strtok_r(NULL, ";", save);
    if(tok)
    {
        vid_writef("%d", eval_expr(tok));
        return OK;
    }
    else
        error("expected expression after LOGVAR");

}

int rem_handler(char **save, int *keystate, int *nkeys, int *repeats_left)
{
    return BREAK;
}

int quit_handler(char **save, int *keystate, int *nkeys, int *repeats_left)
{
    return DONE;
}

struct token_t {
    const char *tok;
    int (*func)(char **save, int *keystate, int *numkeys, int *repeats_left);
    size_t len;
} tokens[] = {
    { "STRING",        string_handler  , -1 },
    { "LET",           let_handler     , -1 },
    { "ENTER",         enter_handler   , -1 },
    { "DEL",           del_handler     , -1 },
    { "DELETE",        del_handler     , -1 },
    { "TAB",           tab_handler     , -1 },
    { "ESC",           esc_handler     , -1 },
    { "ESCAPE",        esc_handler     , -1 },
    { "GUI",           gui_handler     , -1 },
    { "RGUI",          rgui_handler    , -1 },
    { "CTRL",          ctrl_handler    , -1 },
    { "CONTROL",       ctrl_handler    , -1 },
    { "RCTRL",         rctrl_handler   , -1 },
    { "RCONTROL",      rctrl_handler   , -1 },
    { "ALT",           alt_handler     , -1 },
    { "META",          alt_handler     , -1 },
    { "RALT",          ralt_handler    , -1 },
    { "REMTA",         ralt_handler    , -1 },
    { "SHIFT",         shift_handler   , -1 },
    { "RSHIFT",        rshift_handler  , -1 },
    { "MENU",          menu_handler    , -1 },
    { "APP",           menu_handler    , -1 },
    { "PAUSE",         pause_handler   , -1 },
    { "BREAK",         pause_handler   , -1 },
    { "PRINT_SCREEN",  psn_handler     , -1 },
    { "PRINTSCREEN",   psn_handler     , -1 },
    { "SYSRQ",         psn_handler     , -1 },
    { "NUMLOCK",       numlck_handler  , -1 },
    { "NUM_LOCK",      numlck_handler  , -1 },
    { "CAPS",          cpslck_handler  , -1 },
    { "CAPSLOCK",      cpslck_handler  , -1 },
    { "CAPS_LOCK",     cpslck_handler  , -1 },
    { "SCROLL",        scrllck_handler , -1 },
    { "SCROLLLOCK",    scrllck_handler , -1 },
    { "SCROLL_LOCK",   scrllck_handler , -1 },
    { "BKSP",          bksp_handler    , -1 },
    { "BACKSPACE",     bksp_handler    , -1 },
    { "UP",            up_handler      , -1 },
    { "UPARROW",       up_handler      , -1 },
    { "DOWN",          down_handler    , -1 },
    { "DOWNARROW",     down_handler    , -1 },
    { "LEFT",          left_handler    , -1 },
    { "LEFTARROW",     left_handler    , -1 },
    { "RIGHT",         right_handler   , -1 },
    { "RIGHTARROW",    right_handler   , -1 },
    { "PGUP",          pgup_handler    , -1 },
    { "PAGEUP",        pgup_handler    , -1 },
    { "PGDOWN",        pgdn_handler    , -1 },
    { "PAGEDOWN",      pgdn_handler    , -1 },
    { "INS",           ins_handler     , -1 },
    { "INSERT",        ins_handler     , -1 },
    { "HOME",          home_handler    , -1 },
    { "REPEAT",        repeat_handler  , -1 },
    { "JUMP",          goto_handler    , -1 },
    { "GOTO",          goto_handler    , -1 },
    { "CALL",          call_handler    , -1 },
    { "GOSUB",         call_handler    , -1 },
    { "RET",           ret_handler     , -1 },
    { "RETURN",        ret_handler     , -1 },
    { "INC",           inc_handler     , -1 },
    { "DEC",           dec_handler     , -1 },
    { "IF",            if_handler      , -1 },
    { "OUT",           out_handler     , -1 },
    { "DELAY",         delay_handler   , -1 },
    { "DEFAULT_DELAY", dfldel_handler  , -1 },
    { "STRING_DELAY",  strdel_handler  , -1 },
    { "LOG",           log_handler     , -1 },
    { "LOGVAR",        logvar_handler  , -1 },
    { "REM",           rem_handler     , -1 },
    { "//",            rem_handler     , -1 },
    { "QUIT",          quit_handler    , -1 },
    { "EXIT",          quit_handler    , -1 },
    { "F1",            f1_handler      , -1 },
    { "F2",            f2_handler      , -1 },
    { "F3",            f3_handler      , -1 },
    { "F4",            f4_handler      , -1 },
    { "F5",            f5_handler      , -1 },
    { "F6",            f6_handler      , -1 },
    { "F7",            f7_handler      , -1 },
    { "F8",            f8_handler      , -1 },
    { "F9",            f9_handler      , -1 },
    { "10",            f10_handler     , -1 },
    { "F11",           f11_handler     , -1 },
    { "F12",           f12_handler     , -1 },
};

/* once again, this lookup table is implemented with a perfect hash map */

#define TOKMAP_SIZE 217
struct token_t tokmap[TOKMAP_SIZE];

uint32_t tok_hash(const char *str)
{
    uint32_t hash = 6722;
    char c;
    while((c = *str++))
    {
        hash = 129 * hash ^ c;
    }

    return hash % TOKMAP_SIZE;
}

void tokmap_insert(struct token_t *tok)
{
    uint32_t hash = tok_hash(tok->tok) % TOKMAP_SIZE;
    if(tokmap[hash].tok)
        error("hash map collision %lu: %s VS %s", hash, tok->tok, tokmap[hash].tok);
    rb->memcpy(tokmap+hash, tok, sizeof(*tok));
}

void init_tokmap(void)
{
    rb->memset(tokmap, 0, sizeof(tokmap));
    for(unsigned int i = 0; i < ARRAYLEN(tokens); ++i)
    {
        tokens[i].len = rb->strlen(tokens[i].tok);
        tokmap_insert(tokens+i);
    }
}

enum plugin_status plugin_start(const void *param)
{
    if(!param)
    {
        rb->splash(2*HZ, "Execute a DuckyScript file!");
        return PLUGIN_ERROR;
    }

    rb->lcd_clear_display();
    rb->lcd_update();

    /* first, make sure that USB HID is enabled */
    if(!rb->global_settings->usb_hid)
    {
        rb->splash(5*HZ, "USB HID is required for this plugin to function. Please enable it and try again.");
        return PLUGIN_ERROR;
    }

    rb->lcd_setfont(FONT_SYSFIXED);

    vid_write("*** DS-2 INIT ***");
    vid_write("QUACK AT YOUR OWN RISK!");
    vid_write("The author assumes no liability for any damages caused by this program.");

    const char *path = (const char*) param;
    int fd = rb->open(path, O_RDONLY);
    file_des = fd;

    atexit(exit_handler);

    if(fd < 0)
        error("invalid filename");
    vid_writef("File: %s", path);

    line_offset = index_lines(fd, &num_lines);
    vid_write("Indexing complete.");

    init_optable();
    init_tokmap();

    /* initialize the "." variable */
    setVariable(".", 0);
    struct varnode_t *dot_var = lookup_var(".");

    /* make sure USB isn't already inserted */

    if(!rb->usb_inserted())
    {
        /* wait for a USB connection */

        vid_write("Waiting for USB...");
        vid_write("Hold any key to skip...");

        int oldbutton = 0;
        int ticks_held = 0;
        long last_tick = 0;
        while(1)
        {

            int button = rb->button_get(true);
            if(button == SYS_USB_CONNECTED)
            {
                vid_write("Please wait...");
                break;
            }
            else if(button)
            {
                /* check if a key's being held down */

                if(oldbutton == 0)
                {
                    oldbutton = button;

                    ticks_held = 0;
                    last_tick = *rb->current_tick;
                }
                else if(button == oldbutton || button == (oldbutton | BUTTON_REPEAT))
                {
                    int dt = *rb->current_tick - last_tick;
                    if(dt)
                    {
                        ticks_held += dt;
                        last_tick = *rb->current_tick;
                        if(ticks_held >= FORCE_EXEC_THRES)
                            break;
                    }
                }
            }
        }

        /* wait a bit to let the host recognize us... */
        rb->sleep(HZ / 2);
    }

    vid_write("Executing...");

    long start_time = *rb->current_tick;
    int repeats_left = 0;

    while(1)
    {
        char instr_buf[MAX_LINE_LEN];
        memset(instr_buf, 0, sizeof(instr_buf));
        if(rb->read_line(fd, instr_buf, sizeof(instr_buf)) == 0)
            goto done;
        char *tok = NULL, *save = NULL;

        ++current_line;
        dot_var->val = current_line;
        ++lines_executed;

        char *buf = instr_buf;

        /* the RB HID driver handles up to 4 keys simultaneously,
           1 in each byte of the id */

        int key_state = 0;
        unsigned num_keys = 0;

        /* execute all the commands on this line/instruction */
        do {
            tok = rb->strtok_r(buf, " -\t", &save);
            buf = NULL;

            if(!tok)
                break;

            if(rb->strlen(tok) == 1)
            {
                add_char(&key_state, &num_keys, tok[0]);
            }
            else
            {
                int hash = tok_hash(tok);
                if(rb->strcmp(tokmap[hash].tok, tok) == 0)
                    switch(tokmap[hash].func(&save, &key_state, &num_keys, &repeats_left))
                    {
                    case OK:
                        break;
                    case BREAK:
                        goto break_out;
                    case DONE:
                        goto done;
                    case NEXT:
                        goto next_line;
                    default:
                        error("unknown return");
                    }
                else
                {
                    error("unknown token `%s` on line %d %d, %d", tok, current_line, hash, tok_hash("CTRL"));
                    goto done;
                }
            }
        } while(tok);
    break_out:

        /* all the key states are packed into an int, therefore the total
           number of keys down at one time can't exceed sizeof(int) */
        if(num_keys > sizeof(int))
        {
            error("too many keys depressed");
            goto done;
        }

        if(key_state)
        {
            send(key_state);

            if(default_delay)
            {
                rb->sleep(default_delay);
                rb->yield();
            }
        }

        if(repeats_left > 0)
        {
            --repeats_left;
            if(repeats_left)
                jump_line(fd, current_line);
            else
                jump_line(fd, current_line + 2);
        }
    next_line:
        ;
    }

    long total;
done:
    total = *rb->current_tick - start_time;

    /* clear the keyboard state */
    send(0);
    rb->yield();

    vid_writef("Done in %ld.%02ld seconds", total / HZ, total % HZ);
    if(total)
        vid_writef("Keys/second: %d", (int)((double)keys_sent/((double)total/HZ)));

    rb->action_userabort(-1);

    /* exit handlers close file descriptors */
    rb->close(fd);
    return PLUGIN_OK;
}
