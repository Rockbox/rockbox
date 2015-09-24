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
 * Kudos to foolsh and hak5darren for hardware and the idea.
 * Expression-parsing code is based on:
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

#include "ducky.h"

#include <tlsf.h>

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
#define FORCE_EXEC_THRES (HZ / 3)

#define DEFAULT_DELAY 0
#define STRING_DELAY 0
#define TOKEN_IS(str) (rb->strcmp(tok, str) == 0)
#define MAX_LINE_LEN 512

/*** Globals ***/

static off_t *line_offset = NULL;
int default_delay = DEFAULT_DELAY, string_delay = STRING_DELAY;

static unsigned lines_executed = 0, current_line = 0, num_lines;

static unsigned call_stack[CALL_STACK_SZ];
static unsigned stack_frame = 0;

int file_des;

static int log_fd = -1;

int keys_sent = 0;

struct varnode_t {
    char name[VARNAME_MAX + 1];
    vartype val;
    bool constant; /* used by setVariable */
    struct varnode_t *next;
};

void error(const char *fmt, ...) __attribute__((noreturn)) ATTRIBUTE_PRINTF(1,2);
void vid_write(const char *str);
void vid_writef(const char *fmt, ...) ATTRIBUTE_PRINTF(1,2);
inline bool isValidVariable(const char *c);

/* variables are stored in a chained hash map */
/* collisions are manageable, but should be minimized */

struct varnode_t *var_map[VARMAP_SIZE] = { 0 };

/* simple DJB hash */
static inline uint32_t var_hash(const char *str)
{
    uint32_t hash = 5381;
    char c;
    while((c = *str++))
    {
        hash = ((hash << 5) + hash) ^ c;
    }

    return hash;
}

static struct varnode_t *lookup_var(const char *name)
{
    static bool firstrun = true;
    if(firstrun)
    {
        firstrun = false;
        rb->memset(var_map, 0, sizeof(var_map));
    }
    vid_writef("lookup up variable %s", name);
    uint32_t hash = var_hash(name) % VARMAP_SIZE;

    struct varnode_t *iter = var_map[hash];
    struct varnode_t *last = NULL;

    while(iter)
    {
        if(rb->strcmp(iter->name, name) == 0)
            return iter;
        last = iter;
        iter = iter->next;
    }

    /* not found in this bucket, so add it to the linked list */
    struct varnode_t *new = tlsf_malloc(sizeof(struct varnode_t));

    rb->memset(new, 0, sizeof(struct varnode_t));
    rb->strlcpy(new->name, name, sizeof(new->name));
    new->val = 0;
    new->constant = false;
    new->next = NULL;

    if(!last)
        var_map[hash] = new;
    else
        last->next = new;

    return new;
}

static vartype getVariable(const char *name)
{
    return lookup_var(name)->val;
}

static void setVariable(const char *name, vartype val)
{
    struct varnode_t *node = lookup_var(name);
    if(!node->constant)
        node->val = val;
    else
        error("attempted to modify a constant variable");
}

static void setConst(const char *name, bool c)
{
    lookup_var(name)->constant = c;
}

static void incVar(const char *name)
{
    struct varnode_t *node = lookup_var(name);
    if(!node->constant)
        ++lookup_var(name)->val;
    else
        error("attempted to modify a constant variable");
}

static void decVar(const char *name)
{
    struct varnode_t *node = lookup_var(name);
    if(!node->constant)
        --lookup_var(name)->val;
    else
        error("attempted to modify a constant variable");
}

/*** Utility functions ***/

static void exit_handler(void)
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
    char fmtbuf[256];

    va_list ap;
    va_start(ap, fmt);
    rb->vsnprintf(fmtbuf, sizeof(fmtbuf), fmt, ap);
    vid_write(fmtbuf);
    va_end(ap);
}

void __attribute__((noreturn,format(printf,1,2))) error(const char *fmt, ...)
{
    char fmtbuf[256];

    va_list ap;
    va_start(ap, fmt);
    rb->vsnprintf(fmtbuf, sizeof(fmtbuf), fmt, ap);
    if(current_line)
        vid_writef("Line %d: ", current_line);
    vid_writef("ERROR: %s",  fmtbuf);
    va_end(ap);

    rb->action_userabort(-1);
    exit(PLUGIN_ERROR);
}

void __attribute__((format(printf,1,2))) warning(const char *fmt, ...)
{
    char fmtbuf[256];

    va_list ap;
    va_start(ap, fmt);
    rb->vsnprintf(fmtbuf, sizeof(fmtbuf), fmt, ap);
    vid_writef("Line %d: WARNING: %s", current_line, fmtbuf);
    va_end(ap);
}

/* index_lines() precalculates the offset of each line for faster jumping */
/* also it does a quick pass to index all the labels */

off_t *index_lines(int fd, unsigned *numlines)
{
    vid_write("Indexing file...");

    size_t sz = sizeof(off_t);
    off_t *data = tlsf_malloc(sz);

    /* this uses 1-indexed line numbers, so the first indice is wasted */
    unsigned idx = 1;

    while(1)
    {
        sz += sizeof(off_t);
        data = tlsf_realloc(data, sz);
        data[idx] = rb->lseek(fd, 0, SEEK_CUR);

        char buf[MAX_LINE_LEN];

        if(!rb->read_line(fd, buf, sizeof(buf)))
            break;

        char *save = NULL;
        char *tok = rb->strtok_r(buf, " \t", &save);
        if(rb->strcmp(tok, "LABEL") == 0 || rb->strcmp("LBL", tok) == 0)
        {
            tok = rb->strtok_r(NULL, " \t", &save);
            if(tok && isValidVariable(tok))
            {
                setVariable(tok, idx);
                lookup_var(tok)->constant = true;
            }
        }

        ++idx;
    }

    rb->lseek(fd, 0, SEEK_SET);

    *numlines = idx - 1;

    return data;
}

void send(int status)
{
#ifdef USB_ENABLE_HID
    rb->usb_hid_send(HID_USAGE_PAGE_KEYBOARD_KEYPAD, status);
#endif
    ++keys_sent;
}

static inline void jump_line(int fd, unsigned where)
{
    if(1 <= where && where <= num_lines)
    {
        rb->lseek(fd, line_offset[where], SEEK_SET);
    }
    else
        error("JUMP target out of range (%u)", where);
    current_line = where - 1;
}

static inline void sub_call(int fd, unsigned where)
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

static inline void sub_return(int fd)
{
    if(stack_frame > 0)
    {
        --stack_frame;
        jump_line(fd, call_stack[stack_frame]);
    }
}

/* Rockbox's HID driver supports up to 4 keys simultaneously, 1 in each byte */

void add_key(int *keystate, unsigned *nkeys, int newkey)
{
    *keystate = (*keystate << 8) | newkey;
    if(nkeys)
        (*nkeys)++;
}

#ifdef USB_ENABLE_HID
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
#endif

void add_char(int *keystate, unsigned *nkeys, char c)
{
    (void) keystate; (void) nkeys; (void) c;
#ifdef USB_ENABLE_HID
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
#endif
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

/* arrays are implemented as UNARY OPERATORS */

struct op_s {
    const char *op;
    int prec;
    int assoc;
    int unary;
    vartype (*eval)(vartype a1, vartype a2);
    unsigned int len;
} ops[] = {
    {"+",     20,  ASSOC_LEFT,   0,  eval_add,       -1},
    {"-",     20,  ASSOC_LEFT,   0,  eval_sub,       -1},
    {"**",    40,  ASSOC_RIGHT,  0,  eval_exp,       -1},
    {"*",     30,  ASSOC_LEFT,   0,  eval_mul,       -1},
    {"&&",    8,   ASSOC_LEFT,   0,  eval_log_and,   -1},
    {"&",     11,  ASSOC_LEFT,   0,  eval_bit_and,   -1},
    {"<<",    15,  ASSOC_LEFT,   0,  eval_lsh,       -1},
    {">>",    15,  ASSOC_LEFT,   0,  eval_rsh,       -1},
    {"<=",    14,  ASSOC_LEFT,   0,  eval_leq,       -1},
    {">=",    14,  ASSOC_LEFT,   0,  eval_geq,       -1},
    {"<",     14,  ASSOC_LEFT,   0,  eval_lt,        -1},
    {">",     14,  ASSOC_LEFT,   0,  eval_gt,        -1},
    {"/",     30,  ASSOC_LEFT,   0,  eval_div,       -1},
    {"||",    7,   ASSOC_LEFT,   0,  eval_log_or,    -1},
    {"|",     9,   ASSOC_LEFT,   0,  eval_bit_or,    -1},
    {"==",    12,  ASSOC_LEFT,   0,  eval_eq,        -1},
    {"!=",    12,  ASSOC_LEFT,   0,  eval_neq,       -1},
    {"%",     30,  ASSOC_LEFT,   0,  eval_mod,       -1},
    {"!",     50,  ASSOC_LEFT,   1,  eval_log_neg,   -1},
    {"~",     50,  ASSOC_LEFT,   1,  eval_bit_comp,  -1},
    {"^",     10,  ASSOC_LEFT,   0,  eval_bit_xor,   -1},
    {"(",     0,   ASSOC_NONE,   0,  NULL,           -1},
    {")",     0,   ASSOC_NONE,   0,  NULL,           -1},
    {"sqrt",  1,   ASSOC_LEFT,   1,  eval_sqrt,      -1},
};

#define OPMAP_SIZE 25

inline void op_hash_round(char c, uint32_t *hash)
{
    *hash *= 70;
    *hash ^= c;
}

inline uint32_t op_hash(const char *c)
{
    uint32_t hash = 4412;
    while(1)
    {
        if(!*c)
            return hash;
        op_hash_round(*c, &hash);
        ++c;
    }
}

/* optimized hash map for fast lookup of operators */
struct op_s op_map[OPMAP_SIZE];
size_t longest_op = 0;

void opmap_insert(struct op_s *op)
{
    if(op->len > longest_op)
        longest_op = op->len;

    uint32_t hash = op_hash(op->op) % OPMAP_SIZE;

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
        opmap_insert(ops+i);
    }
}

const struct op_s *getop(const char *ch, int *len)
{
    unsigned int i = 0;
    uint32_t hash = 4412;
    const struct op_s *poss = NULL;
    do {
        op_hash_round(ch[i], &hash);
        uint32_t modhash = hash % OPMAP_SIZE;

        if(op_map[modhash].op && rb->strncmp(ch, op_map[modhash].op, op_map[modhash].len) == 0)
        {
            *len = op_map[modhash].len;
            poss = op_map + modhash;
        }
    } while(ch[i++] && i < longest_op);
    return poss;
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

    vid_writef("eval_expr %s = "VARFORMAT, str, numstack[0]);
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

#ifdef USB_ENABLE_HID
#define KEY_HANDLER(KEY, NAME)                            \
    int NAME ## _handler(char **save, int *keystate,      \
                         int *numkeys, int *repeats_left) \
    {                                                     \
        (void) save; (void) repeats_left;                 \
        add_key(keystate, numkeys, KEY);                  \
        return 0;                                         \
    }
#else
#define KEY_HANDLER(KEY, NAME)                            \
    int NAME ## _handler(char **save, int *keystate,      \
                         int *numkeys, int *repeats_left) \
    {                                                     \
        (void) save; (void) repeats_left; (void) numkeys; \
        (void) keystate;                                  \
        return 0;                                         \
    }
#endif

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
KEY_HANDLER(HID_KEYBOARD_SPACEBAR, space);
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
    (void) save; (void) keystate; (void) nkeys; (void) repeats_left;
    char *str = rb->strtok_r(NULL, "", save);
    if(!str)
        return OK;
    send_string(str);
    return OK;
}

int let_handler(char **save, int *keystate, int *nkeys, int *repeats_left)
{
    (void) save; (void) keystate; (void) nkeys; (void) repeats_left;
    char *varname = rb->strtok_r(NULL, "= \t", save);

    if(varname && isValidVariable(varname))
    {
        struct varnode_t *node = lookup_var(varname);
        if(node->constant)
            error("attempted to modify a constant variable");

        char *tok = rb->strtok_r(NULL, "=;", save);
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
    (void) save; (void) keystate; (void) nkeys; (void) repeats_left;
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
    (void) save; (void) keystate; (void) nkeys; (void) repeats_left;
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
    (void) save; (void) keystate; (void) nkeys; (void) repeats_left;
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
    (void) save; (void) keystate; (void) nkeys; (void) repeats_left;
    sub_return(file_des);
    return NEXT;
}

int inc_handler(char **save, int *keystate, int *nkeys, int *repeats_left)
{
    (void) save; (void) keystate; (void) nkeys; (void) repeats_left;
    char *tok = rb->strtok_r(NULL, " \t", save);
    if(isValidVariable(tok))
        incVar(tok);
    return OK;
}

int dec_handler(char **save, int *keystate, int *nkeys, int *repeats_left)
{
    (void) save; (void) keystate; (void) nkeys; (void) repeats_left;
    char *tok = rb->strtok_r(NULL, " \t", save);
    if(isValidVariable(tok))
        decVar(tok);
    return OK;
}

int if_handler(char **save, int *keystate, int *nkeys, int *repeats_left)
{
    (void) save; (void) keystate; (void) nkeys; (void) repeats_left;
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
    (void) save; (void) keystate; (void) nkeys; (void) repeats_left;
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
    (void) save; (void) keystate; (void) nkeys; (void) repeats_left;
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
    (void) save; (void) keystate; (void) nkeys; (void) repeats_left;
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
    (void) save; (void) keystate; (void) nkeys; (void) repeats_left;
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
    (void) save; (void) keystate; (void) nkeys; (void) repeats_left;
    char *tok = rb->strtok_r(NULL, "", save);

    vid_write(tok);
    return OK;
}

int logvar_handler(char **save, int *keystate, int *nkeys, int *repeats_left)
{
    (void) save; (void) keystate; (void) nkeys; (void) repeats_left;
    char *tok = rb->strtok_r(NULL, ";", save);
    if(tok)
    {
        vid_writef(VARFORMAT, eval_expr(tok));
        return OK;
    }
    else
        error("expected expression after LOGVAR");

}

int rem_handler(char **save, int *keystate, int *nkeys, int *repeats_left)
{
    (void) save; (void) keystate; (void) nkeys; (void) repeats_left;
    return BREAK;
}

int quit_handler(char **save, int *keystate, int *nkeys, int *repeats_left)
{
    (void) save; (void) keystate; (void) nkeys; (void) repeats_left;
    return DONE;
}

int logchar_handler(char **save, int *keystate, int *nkeys, int *repeats_left)
{
    (void) save; (void) keystate; (void) nkeys; (void) repeats_left;
    char *tok = rb->strtok_r(NULL, ";", save);
    if(tok)
    {
        vid_writef("%c", (char)eval_expr(tok));
        return OK;
    }
    else
        error("expected expression after LOGCHAR");

}

int newline_handler(char **save, int *keystate, int *nkeys, int *repeats_left)
{
    (void) save; (void) keystate; (void) nkeys; (void) repeats_left;
    return OK;
}

struct token_t {
    const char *tok;
    int (*func)(char **save, int *keystate, int *numkeys, int *repeats_left);
} tokens[] = {
    {  "//",             rem_handler       },
    {  "ALT",            alt_handler       },
    {  "APP",            menu_handler      },
    {  "BACKSPACE",      bksp_handler      },
    {  "BKSP",           bksp_handler      },
    {  "BREAK",          pause_handler     },
    {  "CALL",           call_handler      },
    {  "CAPS",           cpslck_handler    },
    {  "CAPSLOCK",       cpslck_handler    },
    {  "CAPS_LOCK",      cpslck_handler    },
    {  "CONTROL",        ctrl_handler      },
    {  "CTRL",           ctrl_handler      },
    {  "DEC",            dec_handler       },
    {  "DEFAULT_DELAY",  dfldel_handler    },
    {  "DEL",            del_handler       },
    {  "DELAY",          delay_handler     },
    {  "DELETE",         del_handler       },
    {  "DOWN",           down_handler      },
    {  "DOWNARROW",      down_handler      },
    {  "ENTER",          enter_handler     },
    {  "ESC",            esc_handler       },
    {  "ESCAPE",         esc_handler       },
    {  "EXIT",           quit_handler      },
    {  "F1",             f1_handler        },
    {  "F10",            f10_handler       },
    {  "F11",            f11_handler       },
    {  "F12",            f12_handler       },
    {  "F2",             f2_handler        },
    {  "F3",             f3_handler        },
    {  "F4",             f4_handler        },
    {  "F5",             f5_handler        },
    {  "F6",             f6_handler        },
    {  "F7",             f7_handler        },
    {  "F8",             f8_handler        },
    {  "F9",             f9_handler        },
    {  "GOSUB",          call_handler      },
    {  "GOTO",           goto_handler      },
    {  "GUI",            gui_handler       },
    {  "HOME",           home_handler      },
    {  "IF",             if_handler        },
    {  "INC",            inc_handler       },
    {  "INS",            ins_handler       },
    {  "INSERT",         ins_handler       },
    {  "JUMP",           goto_handler      },
    {  "LABEL",          rem_handler       },
    {  "LBL",            rem_handler       },
    {  "LEFT",           left_handler      },
    {  "LEFTARROW",      left_handler      },
    {  "LET",            let_handler       },
    {  "LOG",            log_handler       },
    {  "LOGCHAR",        logchar_handler   },
    {  "LOGVAR",         logvar_handler    },
    {  "MENU",           menu_handler      },
    {  "META",           alt_handler       },
    {  "NEWLINE",        newline_handler   },
    {  "NUMLOCK",        numlck_handler    },
    {  "NUM_LOCK",       numlck_handler    },
    {  "OUT",            out_handler       },
    {  "PAGEDOWN",       pgdn_handler      },
    {  "PAGEUP",         pgup_handler      },
    {  "PAUSE",          pause_handler     },
    {  "PGDOWN",         pgdn_handler      },
    {  "PGUP",           pgup_handler      },
    {  "PRINTSCREEN",    psn_handler       },
    {  "PRINT_SCREEN",   psn_handler       },
    {  "QUIT",           quit_handler      },
    {  "RALT",           ralt_handler      },
    {  "RCONTROL",       rctrl_handler     },
    {  "RCTRL",          rctrl_handler     },
    {  "REM",            rem_handler       },
    {  "REMTA",          ralt_handler      },
    {  "REPEAT",         repeat_handler    },
    {  "RET",            ret_handler       },
    {  "RETURN",         ret_handler       },
    {  "RGUI",           rgui_handler      },
    {  "RIGHT",          right_handler     },
    {  "RIGHTARROW",     right_handler     },
    {  "RSHIFT",         rshift_handler    },
    {  "SCROLL",         scrllck_handler   },
    {  "SCROLLLOCK",     scrllck_handler   },
    {  "SCROLL_LOCK",    scrllck_handler   },
    {  "SHIFT",          shift_handler     },
    {  "SPACE",          space_handler     },
    {  "SPACEBAR",       space_handler     },
    {  "STRING",         string_handler    },
    {  "STRING_DELAY",   strdel_handler    },
    {  "SYSRQ",          psn_handler       },
    {  "TAB",            tab_handler       },
    {  "UP",             up_handler        },
    {  "UPARROW",        up_handler        },
    {  "WIN",            gui_handler       },
    {  "WINDOWS",        gui_handler       },
};

/* once again, this lookup table is implemented with a perfect hash map */

#define TOKMAP_SIZE sizeof(tokens)
struct token_t tokmap[TOKMAP_SIZE];

#if 0
#define TOKMAP_SIZE 196
/* hand-optimized perfect hash function */
/* balances speed and memory usage */

uint32_t tok_hash(const char *str)
{
    uint32_t hash = 37638;
    char c;
    while((c = *str++))
    {
        hash = 368 * hash ^ c;
    }

    return hash % TOKMAP_SIZE;
}
#else
/* auto-generated minimal perfect hash function */
/* generated with mph 1.2 */
/* sacrifices memory for SPEED! */
/*
 * d=3
 * n=114
 * m=92
 * c=1.23
 * maxlen=13
 * minklen=2
 * maxklen=13
 * minchar=47
 * maxchar=95
 * loop=0
 * numiter=15
 * seed=
 */

static int g[] = {
          73,   52,   31,   -1,    9,   51,    3,    6,   82,    8,
           0,   39,   62,   -1,   19,   65,   28,   50,   28,   86,
          30,   -1,   86,   19,   28,   28,   10,    0,   -1,    4,
          31,   35,   31,   75,   68,   38,   11,   48,   44,   83,
           0,   75,   64,   28,    7,   75,   76,   33,    0,   -1,
           6,    9,   32,   36,   42,   55,    1,   82,   78,   20,
          34,    7,   36,   79,    3,   80,   38,   89,   74,   69,
           7,    0,   88,   19,    0,   50,   25,   55,    0,   45,
           8,   78,    0,    6,   63,   39,   81,   78,   87,   46,
          43,    0,   54,    0,   86,   72,   44,    0,   74,   -1,
          66,    0,    0,   45,    0,   85,   47,   34,   55,   55,
          -1,   60,   69,   -1,
};

static int T0[] = {
        0x5e, 0x6d, 0x2c, 0x08, 0x0b, 0x2e, 0x58, 0x3c, 0x33, 0x21,
        0x31, 0x49, 0x64, 0x29, 0x4f, 0x20, 0x20, 0x2d, 0x49, 0x2c,
        0x34, 0x3e, 0x0d, 0x49, 0x36, 0x1e, 0x03, 0x45, 0x24, 0x6a,
        0x6d, 0x21, 0x65, 0x37, 0x29, 0x0e, 0x03, 0x1f, 0x4a, 0x36,
        0x41, 0x09, 0x1d, 0x43, 0x42, 0x0b, 0x64, 0x62, 0x38, 0x2c,
        0x3e, 0x58, 0x4b, 0x4c, 0x1b, 0x06, 0x53, 0x33, 0x3a, 0x1d,
        0x04, 0x5a, 0x22, 0x6c, 0x5b, 0x56, 0x39, 0x55, 0x59, 0x56,
        0x64, 0x04, 0x16, 0x03, 0x71, 0x41, 0x3f, 0x3a, 0x46, 0x57,
        0x67, 0x13, 0x3e, 0x50, 0x5f, 0x59, 0x57, 0x50, 0x2a, 0x2f,
        0x6d, 0x2e, 0x17, 0x2d, 0x39, 0x11, 0x22, 0x00, 0x3c, 0x01,
        0x25, 0x0c, 0x1c, 0x49, 0x2f, 0x17, 0x0a, 0x3d, 0x01, 0x0b,
        0x4d, 0x66, 0x4b, 0x1c, 0x62, 0x05, 0x6d, 0x05, 0x13, 0x1c,
        0x3f, 0x55, 0x6e, 0x26, 0x49, 0x1b, 0x53, 0x39, 0x3c, 0x2d,
        0x4a, 0x61, 0x49, 0x66, 0x48, 0x06, 0x0b, 0x52, 0x54, 0x1c,
        0x6d, 0x2f, 0x21, 0x56, 0x5b, 0x11, 0x6c, 0x19, 0x06, 0x1b,
        0x14, 0x27, 0x21, 0x5d, 0x68, 0x41, 0x2e, 0x30, 0x30, 0x37,
        0x42, 0x02, 0x0b, 0x5d, 0x35, 0x66, 0x46, 0x54, 0x5a, 0x07,
        0x15, 0x2e, 0x47, 0x36, 0x0e, 0x35, 0x54, 0x10, 0x4e, 0x5a,
        0x3c, 0x63, 0x1f, 0x6d, 0x5e, 0x16, 0x3c, 0x2b, 0x46, 0x0a,
        0x00, 0x26, 0x0d, 0x0b, 0x21, 0x52, 0x39, 0x59, 0x09, 0x5e,
        0x21, 0x11, 0x15, 0x62, 0x16, 0x6e, 0x22, 0x05, 0x64, 0x22,
        0x03, 0x4d, 0x3c, 0x13, 0x31, 0x5c, 0x11, 0x4b, 0x3e, 0x16,
        0x43, 0x6c, 0x10, 0x0b, 0x1c, 0x0e, 0x19, 0x55, 0x05, 0x22,
        0x51, 0x26, 0x43, 0x66, 0x16, 0x59, 0x01, 0x38, 0x5f, 0x03,
        0x6b, 0x00, 0x60, 0x45, 0x23, 0x5b, 0x48, 0x09, 0x27, 0x24,
        0x10, 0x26, 0x1f, 0x11, 0x2d, 0x15, 0x46, 0x3d, 0x61, 0x13,
        0x0c, 0x48, 0x50, 0x54, 0x6a, 0x02, 0x27, 0x47, 0x01, 0x02,
        0x58, 0x47, 0x53, 0x53, 0x4e, 0x5d, 0x3d, 0x24, 0x66, 0x02,
        0x48, 0x14, 0x29, 0x67, 0x25, 0x66, 0x1b, 0x09, 0x32, 0x0a,
        0x2d, 0x4e, 0x52, 0x0b, 0x47, 0x36, 0x6f, 0x3f, 0x16, 0x33,
        0x0e, 0x6e, 0x58, 0x35, 0x15, 0x18, 0x6e, 0x28, 0x2b, 0x0a,
        0x3f, 0x0d, 0x2c, 0x67, 0x01, 0x39, 0x57, 0x65, 0x13, 0x66,
        0x27, 0x6f, 0x60, 0x13, 0x54, 0x36, 0x59, 0x51, 0x13, 0x6f,
        0x22, 0x21, 0x6c, 0x08, 0x67, 0x1f, 0x21, 0x63, 0x47, 0x5c,
        0x6e, 0x14, 0x69, 0x3c, 0x55, 0x45, 0x05, 0x6c, 0x33, 0x61,
        0x2a, 0x69, 0x0b, 0x43, 0x6f, 0x60, 0x03, 0x22, 0x64, 0x0e,
        0x5e, 0x1a, 0x5b, 0x07, 0x1f, 0x6a, 0x4f, 0x5e, 0x5c, 0x2b,
        0x62, 0x62, 0x1d, 0x41, 0x2c, 0x10, 0x24, 0x41, 0x1a, 0x58,
        0x41, 0x44, 0x5f, 0x4c, 0x16, 0x5c, 0x4b, 0x29, 0x0d, 0x4d,
        0x37, 0x09, 0x6e, 0x32, 0x46, 0x2d, 0x3b, 0x4d, 0x38, 0x4e,
        0x33, 0x03, 0x67, 0x07, 0x54, 0x5c, 0x6c, 0x40, 0x6b, 0x0f,
        0x64, 0x60, 0x58, 0x37, 0x53, 0x2f, 0x63, 0x44, 0x33, 0x50,
        0x1d, 0x71, 0x08, 0x29, 0x31, 0x4e, 0x56, 0x6c, 0x39, 0x1d,
        0x59, 0x0a, 0x30, 0x5e, 0x22, 0x23, 0x48, 0x1c, 0x63, 0x51,
        0x3b, 0x53, 0x0b, 0x4d, 0x30, 0x0c, 0x49, 0x11, 0x4b, 0x16,
        0x16, 0x11, 0x46, 0x19, 0x64, 0x57, 0x40, 0x20, 0x26, 0x5c,
        0x56, 0x32, 0x54, 0x51, 0x4c, 0x38, 0x4c, 0x49, 0x39, 0x63,
        0x6c, 0x34, 0x55, 0x05, 0x10, 0x13, 0x21, 0x69, 0x34, 0x0a,
        0x0d, 0x4a, 0x1b, 0x63, 0x01, 0x0e, 0x58, 0x42, 0x3e, 0x1c,
        0x02, 0x46, 0x22, 0x61, 0x62, 0x59, 0x65, 0x58, 0x31, 0x13,
        0x2f, 0x13, 0x0c, 0x2a, 0x25, 0x63, 0x12, 0x66, 0x3d, 0x0a,
        0x46, 0x4f, 0x4f, 0x4c, 0x6e, 0x1e, 0x2a, 0x60, 0x68, 0x68,
        0x60, 0x6a, 0x4c, 0x20, 0x59, 0x3c, 0x07, 0x4c, 0x22, 0x49,
        0x70, 0x62, 0x5c, 0x1a, 0x2a, 0x20, 0x0b, 0x3c, 0x14, 0x51,
        0x28, 0x67, 0x64, 0x13, 0x29, 0x04, 0x5b, 0x1b, 0x0f, 0x71,
        0x50, 0x38, 0x24, 0x23, 0x62, 0x6e, 0x6b, 0x65, 0x35, 0x01,
        0x08, 0x47, 0x49, 0x1b, 0x6d, 0x1f, 0x13, 0x3f, 0x1f, 0x26,
        0x2f, 0x57, 0x1b, 0x21, 0x6a, 0x54, 0x35, 0x63, 0x0e, 0x55,
        0x63, 0x5e, 0x2b, 0x25, 0x1f, 0x1c, 0x22, 0x18, 0x3e, 0x52,
        0x1a, 0x6d, 0x03, 0x32, 0x6f, 0x3b, 0x6a, 0x52, 0x32, 0x3d,
        0x4f, 0x22, 0x19, 0x41, 0x07, 0x02, 0x56, 0x27, 0x5c, 0x31,
        0x55, 0x66, 0x2b, 0x58, 0x50, 0x62, 0x38, 0x50, 0x4b, 0x05,
        0x30, 0x66, 0x00, 0x43, 0x36, 0x0d, 0x0c, 0x3e, 0x5f, 0x3e,
        0x1a, 0x3c, 0x61, 0x33, 0x1c, 0x06, 0x45,
};

static int T1[] = {
        0x1c, 0x1e, 0x26, 0x6a, 0x4b, 0x0d, 0x50, 0x47, 0x31, 0x56,
        0x11, 0x22, 0x65, 0x3d, 0x09, 0x46, 0x1b, 0x29, 0x59, 0x64,
        0x1c, 0x35, 0x19, 0x3a, 0x23, 0x46, 0x01, 0x45, 0x14, 0x45,
        0x1e, 0x30, 0x63, 0x44, 0x38, 0x4c, 0x51, 0x16, 0x21, 0x20,
        0x0b, 0x33, 0x42, 0x70, 0x0e, 0x4c, 0x54, 0x3a, 0x13, 0x2e,
        0x1f, 0x59, 0x3d, 0x09, 0x35, 0x0a, 0x66, 0x6c, 0x2c, 0x5e,
        0x38, 0x04, 0x4e, 0x38, 0x51, 0x4a, 0x37, 0x37, 0x0a, 0x63,
        0x58, 0x0c, 0x01, 0x66, 0x36, 0x23, 0x5a, 0x36, 0x00, 0x34,
        0x02, 0x1f, 0x2b, 0x3f, 0x39, 0x71, 0x59, 0x2d, 0x6b, 0x13,
        0x2a, 0x32, 0x28, 0x06, 0x08, 0x07, 0x50, 0x3f, 0x05, 0x30,
        0x3b, 0x32, 0x28, 0x4f, 0x54, 0x36, 0x0a, 0x13, 0x3f, 0x46,
        0x1d, 0x34, 0x64, 0x6c, 0x01, 0x00, 0x1f, 0x09, 0x3f, 0x4d,
        0x59, 0x11, 0x68, 0x55, 0x1a, 0x24, 0x15, 0x60, 0x05, 0x1b,
        0x1e, 0x40, 0x5d, 0x47, 0x1d, 0x40, 0x0b, 0x37, 0x63, 0x5b,
        0x0b, 0x0e, 0x2d, 0x6f, 0x08, 0x3e, 0x0d, 0x59, 0x67, 0x07,
        0x2b, 0x5f, 0x6e, 0x6b, 0x63, 0x25, 0x45, 0x41, 0x0e, 0x1f,
        0x32, 0x2c, 0x46, 0x36, 0x60, 0x1f, 0x13, 0x15, 0x6d, 0x4f,
        0x07, 0x24, 0x2b, 0x03, 0x6c, 0x1f, 0x4c, 0x2e, 0x16, 0x51,
        0x45, 0x51, 0x4e, 0x41, 0x4b, 0x3f, 0x66, 0x2e, 0x1f, 0x12,
        0x4e, 0x61, 0x3f, 0x32, 0x25, 0x3d, 0x3e, 0x55, 0x22, 0x1c,
        0x10, 0x51, 0x0f, 0x38, 0x0c, 0x1d, 0x69, 0x0a, 0x09, 0x64,
        0x6d, 0x4e, 0x33, 0x32, 0x71, 0x30, 0x2b, 0x45, 0x5f, 0x71,
        0x6f, 0x48, 0x62, 0x32, 0x28, 0x58, 0x66, 0x66, 0x3b, 0x16,
        0x20, 0x4c, 0x68, 0x40, 0x12, 0x02, 0x6d, 0x19, 0x1c, 0x04,
        0x0b, 0x17, 0x63, 0x3f, 0x4a, 0x08, 0x1f, 0x17, 0x4c, 0x53,
        0x55, 0x4a, 0x1c, 0x4c, 0x48, 0x11, 0x5e, 0x32, 0x26, 0x18,
        0x43, 0x10, 0x35, 0x46, 0x2b, 0x1a, 0x64, 0x66, 0x6f, 0x69,
        0x1c, 0x13, 0x33, 0x4b, 0x5c, 0x1c, 0x54, 0x19, 0x34, 0x2e,
        0x6d, 0x27, 0x06, 0x27, 0x01, 0x5e, 0x48, 0x70, 0x1e, 0x6e,
        0x26, 0x00, 0x0d, 0x5b, 0x34, 0x19, 0x56, 0x56, 0x30, 0x62,
        0x0f, 0x43, 0x69, 0x50, 0x4c, 0x4c, 0x2d, 0x3e, 0x2b, 0x5b,
        0x29, 0x52, 0x5a, 0x43, 0x33, 0x1d, 0x05, 0x34, 0x61, 0x09,
        0x59, 0x6a, 0x48, 0x07, 0x6d, 0x0b, 0x21, 0x51, 0x71, 0x61,
        0x51, 0x0e, 0x32, 0x49, 0x6e, 0x1d, 0x33, 0x29, 0x6b, 0x5f,
        0x22, 0x23, 0x3f, 0x07, 0x3f, 0x48, 0x56, 0x5a, 0x55, 0x16,
        0x1b, 0x71, 0x04, 0x56, 0x5a, 0x0e, 0x08, 0x21, 0x3f, 0x6e,
        0x36, 0x04, 0x5c, 0x20, 0x3a, 0x56, 0x36, 0x62, 0x0f, 0x2f,
        0x2a, 0x48, 0x05, 0x32, 0x4f, 0x54, 0x18, 0x44, 0x4d, 0x6e,
        0x5a, 0x68, 0x0b, 0x5e, 0x5d, 0x65, 0x0b, 0x65, 0x24, 0x5a,
        0x71, 0x6a, 0x1a, 0x3c, 0x11, 0x0a, 0x2b, 0x41, 0x3f, 0x04,
        0x64, 0x18, 0x25, 0x47, 0x05, 0x45, 0x27, 0x15, 0x3f, 0x56,
        0x6c, 0x61, 0x1e, 0x42, 0x46, 0x17, 0x14, 0x5b, 0x71, 0x4b,
        0x19, 0x5b, 0x1b, 0x43, 0x36, 0x2c, 0x4d, 0x61, 0x0c, 0x1b,
        0x03, 0x70, 0x43, 0x28, 0x45, 0x48, 0x6e, 0x0b, 0x6d, 0x4b,
        0x61, 0x34, 0x62, 0x6c, 0x3c, 0x33, 0x50, 0x29, 0x3b, 0x52,
        0x5b, 0x56, 0x48, 0x59, 0x10, 0x04, 0x1a, 0x19, 0x54, 0x5a,
        0x21, 0x38, 0x5a, 0x45, 0x5b, 0x31, 0x31, 0x4b, 0x42, 0x55,
        0x27, 0x09, 0x17, 0x27, 0x03, 0x64, 0x6a, 0x64, 0x2b, 0x33,
        0x44, 0x14, 0x18, 0x1a, 0x0b, 0x38, 0x2e, 0x25, 0x51, 0x20,
        0x6e, 0x52, 0x19, 0x11, 0x1c, 0x4b, 0x29, 0x59, 0x38, 0x47,
        0x0f, 0x5c, 0x2d, 0x58, 0x6b, 0x48, 0x2b, 0x59, 0x23, 0x34,
        0x3d, 0x4a, 0x3c, 0x34, 0x70, 0x43, 0x1a, 0x23, 0x01, 0x62,
        0x62, 0x0e, 0x52, 0x09, 0x1f, 0x0c, 0x55, 0x59, 0x65, 0x2b,
        0x2e, 0x13, 0x25, 0x6c, 0x09, 0x2e, 0x52, 0x44, 0x15, 0x43,
        0x62, 0x2c, 0x06, 0x65, 0x5e, 0x3a, 0x4b, 0x6b, 0x10, 0x34,
        0x6c, 0x1d, 0x4a, 0x6d, 0x45, 0x2c, 0x4a, 0x05, 0x27, 0x63,
        0x69, 0x32, 0x2c, 0x49, 0x62, 0x4f, 0x1c, 0x68, 0x67, 0x65,
        0x3a, 0x68, 0x30, 0x50, 0x5b, 0x1c, 0x18, 0x35, 0x26, 0x38,
        0x07, 0x20, 0x55, 0x62, 0x2b, 0x38, 0x2c, 0x14, 0x63, 0x0e,
        0x56, 0x15, 0x6b, 0x07, 0x6c, 0x54, 0x64, 0x2b, 0x2d, 0x05,
        0x50, 0x60, 0x32, 0x2c, 0x48, 0x59, 0x45, 0x3e, 0x56, 0x71,
        0x33, 0x3f, 0x35, 0x62, 0x61, 0x68, 0x6e, 0x0c, 0x10, 0x6f,
        0x2a, 0x66, 0x23, 0x23, 0x0c, 0x1d, 0x06, 0x0e, 0x58, 0x43,
        0x13, 0x36, 0x31, 0x46, 0x00, 0x18, 0x3d,
};

static int T2[] = {
        0x48, 0x48, 0x68, 0x71, 0x09, 0x2e, 0x0e, 0x07, 0x11, 0x3c,
        0x1b, 0x04, 0x11, 0x63, 0x1f, 0x07, 0x46, 0x54, 0x44, 0x64,
        0x49, 0x57, 0x22, 0x40, 0x1f, 0x3d, 0x08, 0x63, 0x3b, 0x4a,
        0x58, 0x11, 0x21, 0x4e, 0x10, 0x3a, 0x1a, 0x1e, 0x41, 0x3b,
        0x6b, 0x6c, 0x50, 0x1a, 0x5d, 0x6f, 0x22, 0x41, 0x62, 0x54,
        0x22, 0x06, 0x4d, 0x29, 0x18, 0x00, 0x5f, 0x12, 0x53, 0x3a,
        0x61, 0x12, 0x19, 0x27, 0x4b, 0x22, 0x16, 0x4b, 0x01, 0x15,
        0x00, 0x5b, 0x57, 0x0b, 0x36, 0x5d, 0x4c, 0x59, 0x29, 0x19,
        0x4b, 0x5c, 0x2f, 0x27, 0x23, 0x47, 0x37, 0x10, 0x59, 0x18,
        0x4a, 0x58, 0x2a, 0x02, 0x1e, 0x13, 0x24, 0x34, 0x25, 0x4f,
        0x36, 0x1b, 0x3f, 0x66, 0x2f, 0x12, 0x35, 0x1f, 0x5c, 0x6b,
        0x49, 0x42, 0x26, 0x02, 0x0b, 0x0b, 0x1d, 0x39, 0x19, 0x2e,
        0x69, 0x4b, 0x1a, 0x06, 0x0a, 0x09, 0x11, 0x41, 0x22, 0x46,
        0x1e, 0x68, 0x00, 0x6d, 0x5c, 0x2f, 0x0d, 0x20, 0x5e, 0x07,
        0x29, 0x35, 0x49, 0x50, 0x48, 0x55, 0x6b, 0x6f, 0x4f, 0x6c,
        0x1d, 0x4a, 0x0d, 0x1c, 0x65, 0x66, 0x67, 0x16, 0x5f, 0x2a,
        0x6a, 0x67, 0x48, 0x68, 0x57, 0x63, 0x2b, 0x16, 0x32, 0x55,
        0x70, 0x63, 0x5d, 0x10, 0x3e, 0x36, 0x5d, 0x52, 0x33, 0x3a,
        0x4c, 0x61, 0x22, 0x69, 0x0b, 0x25, 0x5d, 0x10, 0x4c, 0x4b,
        0x3a, 0x44, 0x50, 0x10, 0x4a, 0x36, 0x0f, 0x27, 0x12, 0x21,
        0x24, 0x20, 0x66, 0x51, 0x71, 0x2d, 0x4a, 0x28, 0x4b, 0x62,
        0x6f, 0x5e, 0x67, 0x16, 0x70, 0x0b, 0x2a, 0x3e, 0x57, 0x1e,
        0x62, 0x68, 0x25, 0x02, 0x08, 0x01, 0x4b, 0x27, 0x28, 0x5d,
        0x48, 0x5d, 0x1c, 0x4d, 0x3c, 0x1b, 0x18, 0x14, 0x53, 0x63,
        0x15, 0x61, 0x50, 0x0a, 0x05, 0x2f, 0x6b, 0x50, 0x01, 0x5b,
        0x01, 0x2f, 0x0b, 0x25, 0x4f, 0x66, 0x49, 0x13, 0x2f, 0x6c,
        0x62, 0x45, 0x61, 0x5c, 0x53, 0x08, 0x62, 0x2d, 0x6d, 0x2d,
        0x43, 0x3e, 0x4f, 0x4e, 0x43, 0x3e, 0x0b, 0x4c, 0x2d, 0x1d,
        0x35, 0x2e, 0x4c, 0x50, 0x63, 0x3a, 0x44, 0x4b, 0x4d, 0x12,
        0x45, 0x4d, 0x67, 0x44, 0x2a, 0x4c, 0x37, 0x4c, 0x47, 0x3a,
        0x2b, 0x30, 0x6f, 0x07, 0x12, 0x5e, 0x58, 0x20, 0x6a, 0x10,
        0x5f, 0x24, 0x02, 0x5e, 0x5d, 0x61, 0x58, 0x0d, 0x3c, 0x38,
        0x1e, 0x09, 0x58, 0x27, 0x5f, 0x10, 0x02, 0x24, 0x6c, 0x59,
        0x6e, 0x26, 0x17, 0x6b, 0x3d, 0x39, 0x67, 0x33, 0x59, 0x5f,
        0x53, 0x57, 0x22, 0x30, 0x56, 0x42, 0x2f, 0x42, 0x37, 0x03,
        0x5b, 0x35, 0x2c, 0x27, 0x43, 0x5d, 0x07, 0x2b, 0x22, 0x5c,
        0x32, 0x57, 0x15, 0x3c, 0x4f, 0x3c, 0x11, 0x2c, 0x46, 0x07,
        0x17, 0x61, 0x44, 0x0f, 0x1f, 0x38, 0x51, 0x5e, 0x09, 0x27,
        0x61, 0x02, 0x6c, 0x2b, 0x39, 0x3e, 0x27, 0x40, 0x69, 0x49,
        0x2a, 0x39, 0x6f, 0x5f, 0x3d, 0x1f, 0x49, 0x6f, 0x5e, 0x2a,
        0x6d, 0x13, 0x61, 0x4f, 0x16, 0x6d, 0x35, 0x24, 0x67, 0x66,
        0x28, 0x04, 0x3d, 0x03, 0x11, 0x6c, 0x0b, 0x65, 0x2a, 0x69,
        0x05, 0x56, 0x5b, 0x12, 0x53, 0x36, 0x31, 0x2a, 0x43, 0x2d,
        0x65, 0x3e, 0x40, 0x54, 0x2b, 0x56, 0x5f, 0x61, 0x08, 0x65,
        0x65, 0x22, 0x57, 0x33, 0x32, 0x2d, 0x46, 0x3e, 0x08, 0x41,
        0x56, 0x66, 0x6c, 0x0e, 0x23, 0x5b, 0x46, 0x2b, 0x10, 0x03,
        0x3d, 0x46, 0x33, 0x1f, 0x16, 0x12, 0x29, 0x24, 0x36, 0x3b,
        0x30, 0x3f, 0x5e, 0x16, 0x10, 0x1e, 0x43, 0x57, 0x6c, 0x4b,
        0x36, 0x61, 0x50, 0x40, 0x6f, 0x01, 0x29, 0x53, 0x2c, 0x39,
        0x21, 0x3d, 0x2f, 0x2b, 0x6e, 0x31, 0x21, 0x37, 0x69, 0x4b,
        0x6f, 0x56, 0x51, 0x3f, 0x1c, 0x2c, 0x67, 0x1e, 0x3b, 0x4d,
        0x40, 0x17, 0x05, 0x4a, 0x33, 0x67, 0x44, 0x59, 0x4b, 0x69,
        0x12, 0x0a, 0x34, 0x51, 0x36, 0x41, 0x10, 0x57, 0x06, 0x08,
        0x31, 0x14, 0x6e, 0x20, 0x63, 0x18, 0x5c, 0x58, 0x46, 0x37,
        0x16, 0x1a, 0x21, 0x22, 0x51, 0x1e, 0x10, 0x41, 0x43, 0x4f,
        0x4d, 0x65, 0x0e, 0x06, 0x09, 0x1d, 0x2b, 0x5b, 0x4d, 0x1e,
        0x4e, 0x0b, 0x19, 0x56, 0x2b, 0x61, 0x6f, 0x05, 0x39, 0x2a,
        0x4d, 0x60, 0x54, 0x6e, 0x10, 0x43, 0x1a, 0x30, 0x12, 0x5e,
        0x1d, 0x5f, 0x51, 0x2b, 0x04, 0x6a, 0x48, 0x3f, 0x45, 0x60,
        0x2d, 0x19, 0x4f, 0x49, 0x45, 0x61, 0x31, 0x2b, 0x3e, 0x57,
        0x59, 0x3b, 0x12, 0x6e, 0x1f, 0x57, 0x54, 0x33, 0x0e, 0x31,
        0x46, 0x42, 0x46, 0x61, 0x0c, 0x63, 0x50, 0x18, 0x03, 0x23,
        0x06, 0x30, 0x4c, 0x66, 0x07, 0x1f, 0x65, 0x48, 0x4a, 0x41,
        0x3e, 0x41, 0x0a, 0x50, 0x3d, 0x39, 0x46,
};

#define uchar unsigned char

int
tok_hash(const uchar *key)
{
        int i;
        unsigned f0, f1, f2;
        const uchar *kp = key;

        for (i=-47, f0=f1=f2=0; *kp; ++kp) {
                if (*kp < 47 || *kp > 95)
                        return -1;
                if (kp-key > 12)
                        return -1;
                f0 += T0[i + *kp];
                f1 += T1[i + *kp];
                f2 += T2[i + *kp];
                i += 49;
        }

        if (kp-key < 2)
                return -1;

        f0 %= 114;
        f1 %= 114;
        f2 %= 114;

        return (g[f0] + g[f1] + g[f2]) % 92;
}
#endif

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
        tokmap_insert(tokens+i);
    }
}

void init_tlsf(void)
{
    void *pluginbuf;
    size_t bufsz;
    pluginbuf = rb->plugin_get_buffer(&bufsz);
    init_memory_pool(bufsz, pluginbuf);
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

#ifdef USB_ENABLE_HID
    /* first, make sure that USB HID is enabled */
    if(!rb->global_settings->usb_hid)
    {
        rb->splash(5*HZ, "USB HID is required for this plugin to function. Please enable it and try again.");
        return PLUGIN_ERROR;
    }
#endif

    rb->lcd_setfont(FONT_SYSFIXED);

    vid_write("*** DS-2 INIT ***");
    vid_write("QUACK AT YOUR OWN RISK!");
    vid_write("The author assumes no liability for any damages caused by this program.");

    const char *path = (const char*) param;
    file_des = rb->open(path, O_RDONLY);

    atexit(exit_handler);

    init_tlsf();

    if(file_des < 0)
        error("invalid filename");
    vid_writef("File: %s", path);

    imm_t imm = read_imm();
    rb->lseek(file_des, 0, SEEK_SET);
    if(imm == DUCKY_MAGIC)
    {
        ducky_vm(file_des);
        return PLUGIN_OK;
    }

    init_optable();
    init_tokmap();

    /* initialize the "." variable, which is the line counter */
    setVariable(".", 0);
    setConst(".", true);

    struct varnode_t *dot_var = lookup_var(".");

    /* initialize some other constants */
    setVariable("true", 1);
    setConst("true", true);

    setVariable("false", 0);
    setConst("false", true);

    line_offset = index_lines(file_des, &num_lines);
    vid_writef("Indexing complete (%u lines).", num_lines);

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
        if(rb->read_line(file_des, instr_buf, sizeof(instr_buf)) == 0)
        {
            vid_writef("end of file");
            goto done;
        }
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
                int hash = tok_hash(tok) % TOKMAP_SIZE;
                if(hash >= 0 && rb->strcmp(tokmap[hash].tok, tok) == 0)
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
                else if(tok[0] == '#')
                    goto next_line;
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
            rb->yield();

            send(0);
            rb->yield();

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
                jump_line(file_des, current_line);
            else
                jump_line(file_des, current_line + 2);
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
    rb->close(file_des);
    return PLUGIN_OK;
}
