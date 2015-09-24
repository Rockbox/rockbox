/***************************************************************************
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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

#include "plugin.h"

/*******************************************************************************
 * The script language implemented here is an extension of DuckyScript.
 * DuckyScript as it is now is limited to simple tasks, as it lacks good flow
 * control or variable support.
 *
 * These following extensions to DuckyScript are implemented:
 *
 * Variables: there are 26 available variables, A-Z, *case-insensitive*
 *
 * NOTE: in the lines below, a * before a variable denotes that it is a constant
 *
 * "STRING_DELAY *X" - sets the delay between keystrokes in a string to X ms
 *
 * "JUMP *X" - jumps to line X in the file
 * "JUMPZ X *Y" - if X == 0, jump to line Y
 * "JUMPLZ X *Y" - if X < 0, jump to line Y
 * "JUMPL X Y *Z" - if X < Y, jump to line Z
 *
 * "SET X *Y" - loads constant Y into variable X
 * "MOV X Y" - Y = X
 * "SWP X Y" - X <-> Y
 *
 * "NEG X"   - X = 0 - X
 * "ADD X Y" - Y = X + Y
 * "SUB X Y" - Y = X - Y
 * "MUL X Y" - Y = X * Y
 * "DIV X Y" - Y = X / Y
 * "MOD X Y" - Y = X % Y
 ******************************************************************************/

#define DEFAULT_DELAY 1
#define STRING_DELAY 1
#define TOKEN_IS(str) (rb->strcmp(tok, str) == 0)

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

        rb->splashf(2 * HZ, "WARNING: invalid character '%c'", c);
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
        }
    }
    return false;
}

int keys_sent = 0;

inline void send(int status)
{
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

    const char *path = (const char*) param;
    int fd = rb->open(path, O_RDONLY);

    int line = 0;

    int default_delay = DEFAULT_DELAY, string_delay = STRING_DELAY;

    /* make sure USB isn't already inserted */

    if(rb->usb_inserted())
        goto exec;

    /* wait for a USB connection */

    rb->splash(0, "Please connect USB to begin execution...");

    while(1)
    {
        if(rb->button_get(true) == SYS_USB_CONNECTED)
        {
            rb->lcd_clear_display();
            rb->lcd_update();
            rb->splash(0, "Please wait...");
            break;
        }
    }

    /* wait a bit to let the host recognize us... */
    rb->sleep(HZ / 2);

exec:

    rb->lcd_clear_display();
    rb->lcd_update();
    rb->splash(0, "Executing...");

    long start_time = *rb->current_tick;

    while(1)
    {
        char instr_buf[256];
        memset(instr_buf, 0, sizeof(instr_buf));
        if(rb->read_line(fd, instr_buf, sizeof(instr_buf)) == 0)
            goto done;
        ++line;

        char *tok = NULL, *save = NULL;

        char *buf = instr_buf;

        /* the RB HID driver handles up to 4 keys simultaneously,
           1 in each byte of the id */

        int key_state = 0;
        unsigned num_keys = 0;

        /* execute all the commands on this line/instruction */
        do {
            tok = rb->strtok_r(buf, " -", &save);
            buf = NULL;

            if(!tok)
                break;

            //rb->splashf(HZ * 2, "Executing token %s", tok);

            if(rb->strlen(tok) == 1) {
                add_char(&key_state, &num_keys, tok[0]);
            }
            else if(TOKEN_IS("STRING")) {
                /* get the rest of the string */
                char *str = rb->strtok_r(NULL, "", &save);
                if(!str)
                    break;
                while(*str)
                {
                    int string_state = 0;
                    if(!*str)
                        break;
                    add_char(&string_state, NULL, *str);

                    send(string_state);

                    ++str;
                    rb->sleep(string_delay);
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
            else if(TOKEN_IS("NUMLOCK"))
                add_key(&key_state, &num_keys, HID_KEYBOARD_LOCKING_NUM_LOCK);
            else if(TOKEN_IS("CAPS") || TOKEN_IS("CAPSLOCK") || TOKEN_IS("CAPS_LOCK"))
                add_key(&key_state, &num_keys, HID_KEYBOARD_LOCKING_CAPS_LOCK);
            else if(TOKEN_IS("SCROLL") || TOKEN_IS("SCROLLLOCK") || TOKEN_IS("SCROLL_LOCK"))
                add_key(&key_state, &num_keys, HID_KEYBOARD_LOCKING_SCROLL_LOCK);
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
            else if(TOKEN_IS("DELAY")) {
                /* delay N 1000ths of a sec */
                tok = rb->strtok_r(NULL, " ", &save);
                rb->sleep((HZ / 100) * rb->atoi(tok) / 10);
            }
            else if(TOKEN_IS("DEFAULT_DELAY")) {
                /* sets time between instructions, 1000ths of a second */
                tok = rb->strtok_r(NULL, " ", &save);
                default_delay = rb->atoi(tok) / 10 * (HZ / 100);
            }
            else if(TOKEN_IS("STRING_DELAY")) {
                tok = rb->strtok_r(NULL, " ", &save);
                string_delay = rb->atoi(tok) / 10 * (HZ / 100);
            }
            else if(TOKEN_IS("REM") || TOKEN_IS("//")) {
                /* break out of the do-while */
                break;
            }
            else {
                /* check if it's a function key */
                if(!handle_fn_key(tok, &key_state, &num_keys))
                {
                    rb->splashf(HZ*3, "ERROR: Unknown token `%s` on line %d", tok, line);
                    goto done;
                }
            }

            /* all the key states are packed into an int, therefore the total
               number of keys down at one time can't exceed sizeof(int) */
            if(num_keys > sizeof(int))
            {
                rb->splashf(2*HZ, "ERROR: Too many keys depressed on line %d", line);
                goto done;
            }

            send(key_state);

            rb->sleep(default_delay);
            rb->yield();
        } while(tok);
    }

    long total;
done:
    total = *rb->current_tick - start_time;

    /* clear the keyboard state */
    send(0);

    rb->splashf(HZ, "Total time: %ld.%02ld seconds", total / 100, total % 100);
    rb->splashf(HZ*2, "Keys sent: %d", keys_sent);

    rb->close(fd);
    return PLUGIN_OK;
}
