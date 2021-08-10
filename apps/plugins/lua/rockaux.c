/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 Dan Everton (safetydan)
 * Copyright (C) 2018 William Wilgus
 * String function implementations taken from dietlibc 0.31 (GPLv2 License)
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

#include "plugin.h"
#define _ROCKCONF_H_ /* Protect against unwanted include */
#include "lua.h"
#include "lib/pluginlib_actions.h"

extern long strtol(const char *nptr, char **endptr, int base);
extern const char *strpbrk_n(const char *s, int smax, const char *accept);

#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
int errno = 0;
#endif

char *strerror(int errnum)
{
    (void) errnum;

    DEBUGF("strerror(%d)\n", errnum);

    return NULL;
}

/* splash string and allow user to scroll around
 * provides rudimentary text reflow
 * timeout is disabled on user interaction
 * returns the action that caused quit
 * [ACTION_NONE, ACTION_STD_CANCEL, ACTION_STD_OK]
 * !ACTION_NONE (only on initial timeout)!
 * TIMEOUT can be TIMEOUT_BLOCK or time in ticks
*/
int splash_scroller(int timeout, const char* str)
{
    if (!str)
        str = "[nil]";
    int w, ch_w, ch_h;
    rb->lcd_getstringsize("W", &ch_w, &ch_h);

    const int max_w = LCD_WIDTH - (ch_w * 2);
    const int max_lines = LCD_HEIGHT / ch_h - 1;
    const int wrap_thresh = (LCD_WIDTH / 3);
    const char *ch;
    const char *brk;

    const int max_ch = (LCD_WIDTH / ch_w - 1) * 2;
    char line[max_ch + 2]; /* display buffer */
    const char break_chars[] = "@#$%^&*+-{}[]()/\\|<>:;.,? _\n\r\t";

    int linepos, curline, linesdisp, realline, chars_next_break;
    int action = ACTION_NONE;
    int firstline = 0;
    int cycles = 2; /* get action timeout returns immediately on first call */

    while (cycles > 0)
    {
        /* walk whole buffer every refresh, only display relevant portion */
        rb->lcd_clear_display();
        curline = 0;
        linepos = 0;
        linesdisp = 0;
        ch = str;
        for (; *ch && linepos < max_ch; ch++)
        {
            if (ch[0] == '\t')
            {
                line[linepos++] = ' ';
                line[linepos] = ' ';
            }
            else if (ch[0] == '\b' && timeout > 0)
            {
                line[linepos] = ' ';
                rb->beep_play(1000, HZ, 1000);
            }
            else if (ch[0] < ' ') /* Dont copy control characters */
                line[linepos] = (linepos == 0) ? '\0' : ' ';
            else
                line[linepos] = ch[0];

            line[linepos + 1] = '\0'; /* terminate to check text extent */
            rb->lcd_getstringsize(line, &w, NULL);

            /* try to not split in middle of words */
            if (w + wrap_thresh >= max_w &&
                strpbrk_n(ch, 1, break_chars))
            {
                brk = strpbrk_n(ch+1, max_ch, break_chars);
                chars_next_break = (brk - ch);
                if (brk &&
                 (chars_next_break < 2 || w + (ch_w * chars_next_break) > max_w))
                {
                    if (!isprint(line[linepos]))
                    {
                        line[linepos] = '\0';
                        ch--; /* back-up we want it on the next line */
                    }
                    w += max_w;
                }
            }

            if (w > max_w ||
               (ch[0] >= '\n' && iscntrl(ch[0])) ||
                ch[1] == '\0')
            {
                realline = curline - firstline;
                if (realline >= 0 && realline < max_lines)
                {
                    rb->lcd_putsxy(0, realline * ch_h, line);
                    linesdisp++;
                }
                linepos = 0;
                curline++;
                continue;
            }
            linepos++;
        }

        rb->lcd_update();
        if (timeout >= TIMEOUT_BLOCK)
        {
            action = rb->get_action(CONTEXT_STD, timeout);
            switch(action)
            {
                case ACTION_STD_OK:
                case ACTION_STD_CANCEL:
                    cycles--;
                /* Fall Through */
                case ACTION_NONE:
                    cycles--;
                    break;
                case ACTION_STD_PREV:
                    timeout = TIMEOUT_BLOCK; /* disable timeout */
                    if(firstline > 0)
                        firstline--;
                    break;
                case ACTION_STD_NEXT:
                    timeout = TIMEOUT_BLOCK; /* disable timeout */
                    if (linesdisp == max_lines)
                        firstline++;
                    break;
            }
        }
        else
            break;
    }
    return action;
}

long rb_pow(long x, long n)
{
    long pow = 1;
    unsigned long u;

    if(n <= 0)
    {
        if(n == 0 || x == 1)
            return 1;

        if(x != -1)
            return x != 0 ? 1/x : 0;

        n = -n;
    }

    u = n;
    while(1)
    {
        if(u & 01)
            pow *= x;

        if(u >>= 1)
            x *= x;
        else
            break;
    }

    return pow;
}

int strcoll(const char * str1, const char * str2)
{
    return rb->strcmp(str1, str2);
}

struct tm * gmtime(const time_t *timep)
{
    static struct tm time;
    return rb->gmtime_r(timep, &time);
}

int get_current_path(lua_State *L, int level)
{
    lua_Debug ar;

    if(lua_getstack(L, level, &ar))
    {
        /* Try determining the base path of the current Lua chunk
            and write it to dest. */
        lua_getinfo(L, "S", &ar);

        const char* curfile = &ar.source[1];
        const char* pos = rb->strrchr(curfile, '/');
        if(pos != NULL)
        {
            lua_pushlstring (L, curfile, pos - curfile + 1);
            return 1;
        }
    }

    lua_pushnil(L);
    return 1;
}

/* filetol()
   reads long int from an open file, skips preceding
   whitespaces returns -1 if error, 1 on success.
   *num set to LONG_MAX or LONG_MIN on overflow.
   If number of digits is > than LUAI_MAXNUMBER2STR
   filepointer will continue till the next non digit
   but buffer will stop being filled with characters.
   Preceding zero is ignored
*/
int filetol(int fd, long *num)
{
    static char buffer[LUAI_MAXNUMBER2STR];
    int    retn = -1;
    char   chbuf = 0;
    size_t count = 0;
    bool   neg   = false;
    long   val;

    while (rb->read(fd, &chbuf, 1) == 1)
    {
        if(retn || !isspace(chbuf))
        {
            switch(chbuf)
            {
                case '-':
                {
                    if (retn > 0) /* 0 preceeds, this negative sign must be in error */
                        goto get_digits;
                    neg = true;
                    continue;
                }
                case '0':  /* strip preceeding zeros */
                {
                    *num = 0;
                    retn = 1;
                    continue;
                }
                default:
                    goto get_digits;
            }
        }
    }

    while (rb->read(fd, &chbuf, 1) == 1)
    {
get_digits:
        if(!isdigit(chbuf))
        {
            rb->lseek(fd, -1, SEEK_CUR);
            break;
        }
        else if (count < LUAI_MAXNUMBER2STR - 2)
            buffer[count++] = chbuf;
    }

    if(count)
    {
        buffer[count] = '\0';
        val = strtol(buffer, NULL, 10);
        *num = (neg)? -val:val;
        retn = 1;
    }

    return retn;
}

int get_plugin_action(int timeout, bool with_remote)
{
    static const struct button_mapping *m1[] = { pla_main_ctx };
    int btn;

#ifndef HAVE_REMOTE_LCD
    (void) with_remote;
#else
    static const struct button_mapping *m2[] = { pla_main_ctx, pla_remote_ctx };

    if (with_remote)
        btn = pluginlib_getaction(timeout, m2, 2);
    else
#endif
    btn = pluginlib_getaction(timeout, m1, 1);

    return btn;
}
