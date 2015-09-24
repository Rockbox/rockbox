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
 * "ASCII <EXPR>;" - outputs the ASCII equivalent of <EXPR> if typeable
 *
 * "LOG ..." - outputs any remaining text to the device's screen. Greedy.
 * "LOGVAR <EXPR>;" - outputs variable <EXPR> in decimal to the device's screen
 ******************************************************************************/

#define DEFAULT_DELAY 1
#define STRING_DELAY 1
#define TOKEN_IS(str) (rb->strcmp(tok, str) == 0)
#define MAX_LINE_LEN 512

/*** Variables ***/

typedef int vartype;

#define VARFORMAT "%d"

off_t *line_offset = NULL;
int default_delay = DEFAULT_DELAY, string_delay = STRING_DELAY;

unsigned lines_executed = 0, current_line = 0, num_lines;

int log_fd, file_des;

int keys_sent = 0;

/* 255 variables so each ASCII code is assigned a variable */
/* the variable '0' is always zero */
vartype variables[0x100] = { 0 };



/*** Utility functions ***/

void close_log_fd(void)
{
    rb->close(log_fd);
}

void close_fd(void)
{
    rb->close(file_des);
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
        log_fd = rb->open("/ducky.log", O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if(log_fd < 0)
            exit(PLUGIN_ERROR);
        atexit(close_log_fd);
    }

    rb->write(log_fd, str, rb->strlen(str));
    char newline = '\n';
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

off_t *index_lines(int fd, unsigned *numlines)
{
    vid_write("Indexing file...");

    /* allocate the rest of the plugin buffer to store line number info in */
    /* each index points to the offset within the file for that line */
    size_t pluginbuf_sz;
    off_t *data = rb->plugin_get_buffer(&pluginbuf_sz);
    size_t max_lines = pluginbuf_sz / sizeof(off_t);

    /* NOTE: this uses 1-indexed line numbers, so the first indice is wasted */
    unsigned idx = 1;
    data[idx++] = 0;

    while(1)
    {
        char buf[MAX_LINE_LEN];
        if(!rb->read_line(fd, buf, sizeof(buf)))
            break;
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

void jump_line(int fd, unsigned where)
{
    if(1 <= where && where <= num_lines)
        rb->lseek(fd, line_offset[where], SEEK_SET);
    else
        error("JUMP target out of bounds (%d)", where);
    current_line = where - 1;
}

/* Rockbox's HID driver supports up to 4 keys simultaneously, 1 in each byte */

void add_key(int *keystate, unsigned *nkeys, int newkey)
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

bool handle_fn_key(char *tok, int *keystate, unsigned *num_keys)
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

/* math expression parsing */

/* based on http://en.literateprograms.org/Shunting_yard_algorithm_%28C%29 */

#define MAXOPSTACK 64
#define MAXNUMSTACK 64

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
        exit(EXIT_FAILURE);
    }
    return a1/a2;
}
vartype eval_mod(vartype a1, vartype a2)
{
    if(!a2) {
        error("division by zero");
        exit(EXIT_FAILURE);
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


enum {ASSOC_NONE=0, ASSOC_LEFT, ASSOC_RIGHT};

/* order matters in this table, because operators can share prefixes */
/* operator precedence is based on that of C */

const struct op_s {
    const char *op;
    int prec;
    int assoc;
    int unary;
    vartype (*eval)(vartype a1, vartype a2);
} ops[]={
    {"~", 50, ASSOC_LEFT, 1, eval_bit_comp},
    {"_", 50, ASSOC_RIGHT,1, eval_uminus},
    {"**",40, ASSOC_RIGHT,0, eval_exp},
    {"*", 30, ASSOC_LEFT, 0, eval_mul},
    {"/", 30, ASSOC_LEFT, 0, eval_div},
    {"%", 30, ASSOC_LEFT, 0, eval_mod},
    {"+", 20, ASSOC_LEFT, 0, eval_add},
    {"-", 20, ASSOC_LEFT, 0, eval_sub},
    {"<<",15, ASSOC_LEFT, 0, eval_lsh},
    {">>",15, ASSOC_LEFT, 0, eval_rsh},
    {"<=",14, ASSOC_LEFT, 0, eval_leq},
    {">=",14, ASSOC_LEFT, 0, eval_geq},
    {"<", 14, ASSOC_LEFT, 0, eval_lt},
    {">", 14, ASSOC_LEFT, 0, eval_gt},
    {"==",12, ASSOC_LEFT, 0, eval_eq},
    {"!=",12, ASSOC_LEFT, 0, eval_neq},
    {"!", 50, ASSOC_LEFT, 1, eval_log_neg},
    {"&&",8,  ASSOC_LEFT, 0, eval_log_and},
    {"||",7,  ASSOC_LEFT, 0, eval_log_or},
    {"&", 11, ASSOC_LEFT, 0, eval_bit_and},
    {"^", 10, ASSOC_LEFT, 0, eval_bit_xor},
    {"|", 9,  ASSOC_LEFT, 0, eval_bit_or},
    {"(", 0, ASSOC_NONE, 0, NULL},
    {")", 0, ASSOC_NONE, 0, NULL},
};

const struct op_s *getop(const char *ch, int *len)
{
    unsigned int i;
    for(i=0; i<ARRAYLEN(ops); ++i) {
        if(rb->strncmp(ch, ops[i].op, rb->strlen(ops[i].op)) == 0)
        {
            if(len)
                *len = rb->strlen(ops[i].op);
            return ops + i;
        }
    }
    return NULL;
}

const struct op_s *opstack[MAXOPSTACK];
int nopstack;

vartype numstack[MAXNUMSTACK];
int nnumstack;

void push_opstack(const struct op_s *op)
{
    if(nopstack>MAXOPSTACK-1) {
        error("operator stack overflow");
    }
    opstack[nopstack++]=op;
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
    if(nnumstack>MAXNUMSTACK-1) {
        error("number stack overflow");
    }
    numstack[nnumstack++]=num;
}

vartype pop_numstack(void)
{
    if(!nnumstack) {
        error("number stack empty");
    }
    return numstack[--nnumstack];
}

bool isDigit(char c)
{
    return '0' <= c && c <= '9';
}

bool isValidNumber(char *str)
{
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

bool isSpace(char c)
{
    return (c == ' ') || (c == '\t');
}

bool isValidVariable(const char *c)
{
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

vartype getValue(const char *str)
{
    if(str && isValidVariable(str))
        return variables[(int)str[0]];
    return strtol(str, NULL, 0);
}

bool isValidNumberOrVariable(const char *c)
{
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
    if(rb->strcmp(op->op, "(") == 0) {
        push_opstack(op);
        return;
    } else if(rb->strcmp(op->op, ")") == 0) {
        while(nopstack>0 && rb->strcmp(opstack[nopstack-1]->op, "(") != 0) {
            pop=pop_opstack();
            n1=pop_numstack();
            if(pop->unary) push_numstack(pop->eval(n1, 0));
            else {
                n2=pop_numstack();
                push_numstack(pop->eval(n2, n1));
            }
        }
        if(!(pop=pop_opstack()) || rb->strcmp(pop->op,"(") != 0) {
            error("stack error. No matching \'(\'");
            exit(EXIT_FAILURE);
        }
        return;
    }

    if(op->assoc==ASSOC_RIGHT) {
        while(nopstack && op->prec<opstack[nopstack-1]->prec) {
            pop=pop_opstack();
            n1=pop_numstack();
            if(pop->unary) push_numstack(pop->eval(n1, 0));
            else {
                n2=pop_numstack();
                push_numstack(pop->eval(n2, n1));
            }
        }
    } else {
        while(nopstack && op->prec<=opstack[nopstack-1]->prec) {
            pop=pop_opstack();
            n1=pop_numstack();
            if(pop->unary) push_numstack(pop->eval(n1, 0));
            else {
                n2=pop_numstack();
                push_numstack(pop->eval(n2, n1));
            }
        }
    }
    push_opstack(op);
}

vartype eval_expr(char *str)
{
    char *expr;
    char *tstart=NULL;
    const struct op_s startop={"X", 0, ASSOC_NONE, 0, NULL};
    const struct op_s *op=NULL;
    vartype n1, n2;
    const struct op_s *lastop=&startop;
    nopstack = 0;
    nnumstack = 0;

    int len;
    for(expr = str; *expr; expr += len)
    {
        len = 1;
        if(!tstart)
        {
            if((op=getop(expr, &len)))
            {
                if(lastop && (lastop==&startop || rb->strcmp(lastop->op, ")") != 0))
                {
                    if(rb->strcmp(op->op, "-") == 0)
                        op=getop("_", &len);
                    else if(rb->strcmp(op->op, "(") != 0 && !op->unary)
                    {
                        error("illegal use of binary operator (%s)", op->op);
                    }
                }
                shunt_op(op);
                lastop=op;
            }
            else if(isValidNumberOrVariable(expr))
                tstart=expr;
            else if(!isSpace(*expr))
            {
                error("syntax error");
            }
        }
        else
        {
            if(isSpace(*expr))
            {
                push_numstack(getValue(tstart));
                tstart=NULL;
                lastop=NULL;
            }
            else if((op=getop(expr, &len)))
            {
                push_numstack(getValue(tstart));
                tstart=NULL;
                shunt_op(op);
                lastop=op;
            }
            else if(!isValidNumberOrVariable(expr))
            {
                error("syntax error");
            }
        }
    }
    if(tstart) push_numstack(getValue(tstart));

    while(nopstack) {
        op=pop_opstack();
        n1=pop_numstack();
        if(op->unary) push_numstack(op->eval(n1, 0));
        else {
            n2=pop_numstack();
            push_numstack(op->eval(n2, n1));
        }
    }
    if(nnumstack!=1) {
        error("number stack has %d elements after evaluation. Should be 1.", nnumstack);
    }
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

    atexit(close_fd);

    if(fd < 0)
        error("invalid filename");
    vid_writef("File: %s", path);

    line_offset = index_lines(fd, &num_lines);
    vid_write("Indexing complete.");

    /* make sure USB isn't already inserted */

    if(!rb->usb_inserted())
    {
        /* wait for a USB connection */

        vid_writef("VAL: %d", eval_expr("1 + 2"));

        vid_write("Waiting for USB...");

        while(1)
        {
            if(rb->button_get(true) == SYS_USB_CONNECTED)
            {
                vid_write("Please wait...");
                break;
            }
        }

        /* wait a bit to let the host recognize us... */
        rb->sleep(HZ / 2);
    }

    vid_write("Executing...");

    long start_time = *rb->current_tick;

    while(1)
    {
        char instr_buf[MAX_LINE_LEN];
        memset(instr_buf, 0, sizeof(instr_buf));
        if(rb->read_line(fd, instr_buf, sizeof(instr_buf)) == 0)
            goto done;
        char *tok = NULL, *save = NULL;

        ++current_line;
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
            else if(TOKEN_IS("STRING"))
            {
                /* get the rest of the string */
                char *str = rb->strtok_r(NULL, "", &save);
                if(!str)
                    break;
                send_string(str);
                continue;
            }
            else if(TOKEN_IS("ENTER") || TOKEN_IS("RETURN"))
                add_key(&key_state, &num_keys, HID_KEYBOARD_RETURN);
            else if(TOKEN_IS("DEL") || TOKEN_IS("DELETE"))
                add_key(&key_state, &num_keys, HID_KEYBOARD_DELETE);
            else if(TOKEN_IS("TAB"))
                add_key(&key_state, &num_keys, HID_KEYBOARD_TAB);
            else if(TOKEN_IS("ESC") || TOKEN_IS("ESCAPE"))
                add_key(&key_state, &num_keys, HID_KEYBOARD_ESCAPE);
            else if(TOKEN_IS("GUI"))
                add_key(&key_state, &num_keys, HID_KEYBOARD_LEFT_GUI);
            else if(TOKEN_IS("RGUI"))
                add_key(&key_state, &num_keys, HID_KEYBOARD_RIGHT_GUI);
            else if(TOKEN_IS("CTRL") || TOKEN_IS("CONTROL"))
                add_key(&key_state, &num_keys, HID_KEYBOARD_LEFT_CONTROL);
            else if(TOKEN_IS("RCTRL") || TOKEN_IS("RCONTROL"))
                add_key(&key_state, &num_keys, HID_KEYBOARD_RIGHT_CONTROL);
            else if(TOKEN_IS("ALT") || TOKEN_IS("META"))
                add_key(&key_state, &num_keys, HID_KEYBOARD_LEFT_ALT);
            else if(TOKEN_IS("RALT") || TOKEN_IS("RMETA"))
                add_key(&key_state, &num_keys, HID_KEYBOARD_RIGHT_ALT);
            else if(TOKEN_IS("SHIFT"))
                add_key(&key_state, &num_keys, HID_KEYBOARD_LEFT_SHIFT);
            else if(TOKEN_IS("RSHIFT"))
                add_key(&key_state, &num_keys, HID_KEYBOARD_RIGHT_SHIFT);
            else if(TOKEN_IS("MENU") || TOKEN_IS("APP"))
                add_key(&key_state, &num_keys, HID_KEYBOARD_MENU);
            else if(TOKEN_IS("PAUSE") || TOKEN_IS("BREAK"))
                add_key(&key_state, &num_keys, HID_KEYBOARD_PAUSE);
            else if(TOKEN_IS("PRINT_SCREEN") || TOKEN_IS("PRINTSCREEN") || TOKEN_IS("SYSRQ"))
                add_key(&key_state, &num_keys, HID_KEYBOARD_PRINT_SCREEN);
            else if(TOKEN_IS("NUMLOCK") || TOKEN_IS("NUM_LOCK"))
                add_key(&key_state, &num_keys, HID_KEYPAD_NUM_LOCK_AND_CLEAR);
            else if(TOKEN_IS("CAPS") || TOKEN_IS("CAPSLOCK") || TOKEN_IS("CAPS_LOCK"))
                add_key(&key_state, &num_keys, HID_KEYBOARD_CAPS_LOCK);
            else if(TOKEN_IS("SCROLL") || TOKEN_IS("SCROLLLOCK") || TOKEN_IS("SCROLL_LOCK"))
                add_key(&key_state, &num_keys, HID_KEYBOARD_SCROLL_LOCK);
            else if(TOKEN_IS("BKSP") || TOKEN_IS("BACKSPACE"))
                add_key(&key_state, &num_keys, HID_KEYBOARD_BACKSPACE);
            else if(TOKEN_IS("UP") || TOKEN_IS("UPARROW"))
                add_key(&key_state, &num_keys, HID_KEYBOARD_UP_ARROW);
            else if(TOKEN_IS("DOWN") || TOKEN_IS("DOWNARROW"))
                add_key(&key_state, &num_keys, HID_KEYBOARD_DOWN_ARROW);
            else if(TOKEN_IS("LEFT") || TOKEN_IS("LEFTARROW"))
                add_key(&key_state, &num_keys, HID_KEYBOARD_LEFT_ARROW);
            else if(TOKEN_IS("RIGHT") || TOKEN_IS("RIGHTARROW"))
                add_key(&key_state, &num_keys, HID_KEYBOARD_RIGHT_ARROW);
            else if(TOKEN_IS("PGUP") || TOKEN_IS("PAGEUP"))
                add_key(&key_state, &num_keys, HID_KEYBOARD_PAGE_UP);
            else if(TOKEN_IS("PGDN") || TOKEN_IS("PAGEDOWN"))
                add_key(&key_state, &num_keys, HID_KEYBOARD_PAGE_DOWN);
            else if(TOKEN_IS("INS") || TOKEN_IS("INSERT"))
                add_key(&key_state, &num_keys, HID_KEYBOARD_INSERT);
            else if(TOKEN_IS("HOME"))
                add_key(&key_state, &num_keys, HID_KEYBOARD_HOME);
            else if(TOKEN_IS("JUMP"))
            {
                tok = rb->strtok_r(NULL, ";", &save);
                if(tok)
                    jump_line(fd, eval_expr(tok));
                else
                    error("expected valid expression after JUMP");
            }
            else if(TOKEN_IS("INC"))
            {
                tok = rb->strtok_r(NULL, " \t", &save);
                if(isValidVariable(tok))
                    ++variables[(int)tok[0]];
            }
            else if(TOKEN_IS("DEC"))
            {
                tok = rb->strtok_r(NULL, " \t", &save);
                if(isValidVariable(tok))
                    --variables[(int)tok[0]];
            }
            else if(TOKEN_IS("IF"))
            {
                tok = rb->strtok_r(NULL, ";", &save);

                if(!tok)
                    error("expected conditional after IF");

                /* break out of the do-while if the condition is false */
                if(!eval_expr(tok))
                {
                    break;
                }
            }
            else if(TOKEN_IS("OUT"))
            {
                tok = rb->strtok_r(NULL, ";", &save);
                if(tok)
                {
                    char str[32];
                    rb->snprintf(str, sizeof(str), VARFORMAT, eval_expr(tok));
                    send_string(str);
                }
                else
                    error("expected valid variable name for OUT");
            }
            else if(TOKEN_IS("SWP") || TOKEN_IS("SWAP"))
            {
                char a, b;
                tok = rb->strtok_r(NULL, " \t", &save);
                if(tok && rb->strlen(tok) == 1)
                {
                    a = tok[0];
                    if(!isValidVariable(tok))
                        error("invalid variable %c", a);
                    tok = rb->strtok_r(NULL, " \t", &save);
                    if(tok && rb->strlen(tok) == 1)
                    {
                        b = tok[0];
                        if(!isValidVariable(tok))
                            error("invalid variable %c", b);
                        vartype temp;
                        temp = variables[(int)a];
                        variables[(int)a] = variables[(int)b];
                        variables[(int)b] = temp;
                    }
                    else
                        error("expected valid variable name for SWP");
                }
                else
                    error("expected valid variable name for SWP");
            }
            else if(TOKEN_IS("SET"))
            {
                char *varname = rb->strtok_r(NULL, " \t", &save);

                /* only 1-character variable names are supported */
                if(varname && rb->strlen(varname) == 1 && isValidVariable(varname))
                {
                    char var = varname[0];
                    tok = rb->strtok_r(NULL, ";", &save);
                    if(tok)
                        variables[(int)var] = eval_expr(tok);
                    else
                        error("exprected valid expression after SET");
                }
                else
                {
                    error("invalid variable name for SET");
                    goto done;
                }
            }
            else if(TOKEN_IS("DELAY"))
            {
                /* delay N 1000ths of a sec */
                tok = rb->strtok_r(NULL, ";", &save);
                if(tok)
                    rb->sleep((HZ / 100) * eval_expr(tok) / 10);
                else
                    error("expected valid expression after DELAY");
            }
            else if(TOKEN_IS("DEFAULT_DELAY"))
            {
                /* sets time between instructions, 1000ths of a second */
                tok = rb->strtok_r(NULL, ";", &save);
                if(tok)
                    default_delay = eval_expr(tok) / 10 * (HZ / 100);
                else
                    error("expected valid expression after DEFAULT_DELAY");
            }
            else if(TOKEN_IS("STRING_DELAY"))
            {
                tok = rb->strtok_r(NULL, ";", &save);
                if(tok)
                    string_delay = eval_expr(tok) / 10 * (HZ / 100);
                else
                    error("expected valid expression after STRING_DELAY");
            }
            else if(TOKEN_IS("LOG"))
            {
                tok = rb->strtok_r(NULL, "", &save);

                vid_write(tok);
            }
            else if(TOKEN_IS("LOGVAR"))
            {
                tok = rb->strtok_r(NULL, ";", &save);
                if(tok)
                    vid_writef("%d", eval_expr(tok));
                else
                    error("expected expression after LOGVAR");
            }
            else if(TOKEN_IS("REM") || TOKEN_IS("//"))
            {
                /* break out of the do-while to skip the rest of this line */
                break;
            }
            else
            {
                /* check if it's a function key */
                if(!handle_fn_key(tok, &key_state, &num_keys))
                {
                    error("unknown token `%s` on line %d", tok, current_line);
                    goto done;
                }
            }
        } while(tok);

        /* all the key states are packed into an int, therefore the total
           number of keys down at one time can't exceed sizeof(int) */
        if(num_keys > sizeof(int))
        {
            error("too many keys depressed");
            goto done;
        }

        send(key_state);

        if(default_delay)
        {
            rb->sleep(default_delay);
            rb->yield();
        }
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
