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
 *            0-9 and whitespace characters may NOT be used.
 *
 * NOTE: in the lines below, a * before a variable denotes that it is a constant
 * NOTE 2: the highest resolution timing achievable is bounded by HZ (so 10ms).
 *
 * "STRING_DELAY *X" - sets the delay between keystrokes in a string to X ms
 *
 * "JUMP *X" - jumps to line X in the file
 *
 * "IF X == 0 *Y" - if X == 0, execute the rest of the line
 * "IF X != 0 *Y" - if X != 0, execute the rest of the line
 * "IF X < 0 *Y" - if X < 0, execute the rest of the line
 * "IF X > 0 *Y" - if X > 0, execute the rest of the line
 * "IF X <= 0 *Y" - if X <= 0, execute the rest of the line
 * "IF X >= 0 *Y" - if X >= 0, execute the rest of the line
 * "IF X < Y *Z" - if X < Y, execute the rest of the line
 * "IF X > Y *Z" - if X > Y, execute the rest of the line
 * "IF X <= Y *Z" - if X <= Y, execute the rest of the line
 * "IF X >= Y *Z" - if X >= Y, execute the rest of the line
 * "IF X == Y *Z" - if X == Y, execute the rest of the line
 * "IF X != Y *Z" - if X != Y, execute the rest of the line
 *
 * "SET X *Y" - loads constant Y into variable X
 * "MOV X Y" - Y = X
 * "SWP X Y" - X <-> Y
 *
 * "OUT X" - outputs the value of variable X in base 10
 * "ASCII X" - outputs the ASCII equivalent of variable X, or nothing if invalid
 *
 * "NEG X"   - X = 0 - X
 * "ADD X Y" - Y = X + Y
 * "SUB X Y" - Y = X - Y
 * "MUL X Y" - Y = X * Y
 * "DIV X Y" - Y = X / Y
 * "MOD X Y" - Y = X % Y
 *
 * "LOG ..." - outputs any remaining text to the device's screen
 ******************************************************************************/

#define DEFAULT_DELAY 1
#define STRING_DELAY 1
#define TOKEN_IS(str) (rb->strcmp(tok, str) == 0)
#define MAX_LINE_LEN 512

/*** Variables ***/

off_t *line_offset = NULL;
int default_delay = DEFAULT_DELAY, string_delay = STRING_DELAY;

unsigned lines_executed = 0, current_line = 0, num_lines;

int log_fd;

/*** Utility functions ***/

void close_log_fd(void)
{
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

void vid_writef(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char buf[256];
    rb->vsnprintf(buf, sizeof(buf), fmt, ap);
    vid_write(buf);
    va_end(ap);
}

void error(const char *, ...) __attribute__((noreturn));

void error(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    char buf[256];
    rb->vsnprintf(buf, sizeof(buf), fmt, ap);
    vid_writef("Line %d: ERROR: %s", current_line, buf);
    va_end(ap);

    rb->action_userabort(HZ * 4);
    exit(PLUGIN_ERROR);
}

void warning(const char *fmt, ...)
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

int file_des;

void close_fd(void)
{
    rb->close(file_des);
}

void jump_line(int fd, unsigned where)
{
    if(1 <= where && where <= num_lines)
        rb->lseek(fd, line_offset[where], SEEK_SET);
    else
        error("JUMP target out of bounds (%d)", where);
    current_line = where - 1;
}

#define STR_EQ(a, b) (rb->strcmp(a, b) == 0)

int test_compare(int left, char *op, int right)
{
    if(STR_EQ(op, "=="))
        return left == right;
    if(STR_EQ(op, "!="))
        return left != right;
    if(STR_EQ(op, "<"))
        return left < right;
    if(STR_EQ(op, ">"))
        return left > right;
    if(STR_EQ(op, ">="))
        return left >= right;
    if(STR_EQ(op, "<="))
        return left <= right;
    error("invalid operator %s", op);
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

/* 255 variables so each ASCII code is assigned a variable */
/* the variable '0' is always zero */
int variables[0x100] = { 0 };

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

bool isDigit(char c)
{
    return '0' <= c && c <= '9';
}

bool isAllDigits(const char *str)
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

int keys_sent = 0;

inline void send(int status)
{
    vid_writef("Sending key %d", status);
    rb->usb_hid_send(HID_USAGE_PAGE_KEYBOARD_KEYPAD, status);
    ++keys_sent;
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
        ++current_line;
        ++lines_executed;

        vid_writef("Executing line %d", current_line);

        char *tok = NULL, *save = NULL;

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

            vid_writef("Executing token %s", tok);

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
                vid_writef("STRING %s", str);
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
                        rb->sleep(string_delay);
                    rb->yield();
                }
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
                tok = rb->strtok_r(NULL, " \t", &save);
                vid_writef("JUMP to line %s", tok);
                int where = rb->atoi(tok);
                jump_line(fd, where);
            }
            else if(TOKEN_IS("IF"))
            {
                int left_value, right_value;

                tok = rb->strtok_r(NULL, " \t", &save);
                if(isAllDigits(tok))
                    left_value = rb->atoi(tok);
                else if(tok && rb->strlen(tok) == 1)
                    left_value = variables[tok[0]];
                else
                    error("invalid 1-character left-hand operand for IF");

                tok = rb->strtok_r(NULL, " \t", &save);
                char op_buf[3]; /* longest comparison operator is 2 in length */

                if(tok && rb->strlen(tok) > 0)
                    rb->strlcpy(op_buf, tok, sizeof(op_buf));
                else
                    error("invalid comparison operator for IF");

                tok = rb->strtok_r(NULL, " \t", &save);
                if(isAllDigits(tok))
                    right_value = rb->atoi(tok);
                else if(tok && rb->strlen(tok) == 1)
                    right_value = variables[tok[0]];
                else
                    error("invalid right-hand operand for IF");

                vid_writef("Comparing %d %s %d", left_value, op_buf, right_value);

                /* break out of the do-while if the condition is false */
                if(!test_compare(left_value, op_buf, right_value))
                {
                    vid_write("FALSE, skipping line");
                    break;
                }
            }
            else if(TOKEN_IS("SET"))
            {
                char *varname = rb->strtok_r(NULL, " \t", &save);

                /* only 1-character variable names are supported */
                if(rb->strlen(varname) == 1)
                {
                    char var = varname[0];
                    char *value = rb->strtok_r(NULL, " \t", &save);
                    if(isDigit(var))
                        error("invalid variable name");
                    variables[(int)var] = rb->atoi(value);
                }
                else
                {
                    error("expected 1-character variable name for SET");
                    goto done;
                }
            }
            else if(TOKEN_IS("DELAY"))
            {
                /* delay N 1000ths of a sec */
                tok = rb->strtok_r(NULL, " \t", &save);
                rb->sleep((HZ / 100) * rb->atoi(tok) / 10);
            }
            else if(TOKEN_IS("DEFAULT_DELAY"))
            {
                /* sets time between instructions, 1000ths of a second */
                tok = rb->strtok_r(NULL, " \t", &save);
                default_delay = rb->atoi(tok) / 10 * (HZ / 100);
            }
            else if(TOKEN_IS("STRING_DELAY"))
            {
                tok = rb->strtok_r(NULL, " \t", &save);
                string_delay = rb->atoi(tok) / 10 * (HZ / 100);
            }
            else if(TOKEN_IS("REM") || TOKEN_IS("//"))
            {
                /* break out of the do-while to skip the rest of this line */
                break;
            }
            else if(TOKEN_IS("LOG"))
            {
                tok = rb->strtok_r(NULL, "", &save);

                vid_write(tok);
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
            error("too many keys depressed on line %d", current_line);
            goto done;
        }

        send(key_state);

        rb->yield();

        if(default_delay)
        {
            rb->sleep(default_delay);
        }
    }

    long total;
done:
    total = *rb->current_tick - start_time;

    /* clear the keyboard state */
    send(0);
    rb->yield();

    vid_writef("Done in %d.%02d seconds", total / HZ, total % HZ);
    vid_writef("Keys/second: %d", (int)((double)keys_sent/((double)total/HZ)));

    rb->action_userabort(-1);

    rb->close(fd);
    return PLUGIN_OK;
}
