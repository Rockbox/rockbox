/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Torne Wuff
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
#include "dumb_frotz.h"
#include "lib/pluginlib_exit.h"
#include "lib/pluginlib_actions.h"

PLUGIN_HEADER

extern int frotz_main(void);
extern bool hot_key_quit(void);

f_setup_t f_setup;
extern char save_name[], auxilary_name[], script_name[], command_name[];
extern int story_fp, sfp, rfp, pfp;

static struct viewport vp[NB_SCREENS];

static void atexit_cleanup(void);

enum plugin_status plugin_start(const void* parameter)
{
    int i;
    char* ext;

    PLUGINLIB_EXIT_INIT_ATEXIT(atexit_cleanup);

    if (!parameter)
        return PLUGIN_ERROR;

    rb->lcd_setfont(FONT_SYSFIXED);
#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif
    rb->lcd_clear_display();

    FOR_NB_SCREENS(i)
        rb->viewport_set_defaults(&vp[i], i);

    story_name = parameter;
    strcpy(save_name, story_name);
    ext = rb->strrchr(save_name, '.');
    if (ext)
        *ext = '\0';
    strcpy(auxilary_name, save_name);
    strcpy(script_name, save_name);
    strcpy(command_name, save_name);
    rb->strlcat(save_name, ".sav", MAX_PATH);
    rb->strlcat(auxilary_name, ".aux", MAX_PATH);
    rb->strlcat(script_name, ".scr", MAX_PATH);
    rb->strlcat(command_name, ".rec", MAX_PATH);

    frotz_main();

    return PLUGIN_OK;
}

void atexit_cleanup()
{
    if (story_fp != -1)
        fclose(story_fp);
    if (sfp != -1)
        fclose(sfp);
    if (rfp != -1)
        fclose(rfp);
    if (pfp != -1)
        fclose(pfp);
}

MENUITEM_STRINGLIST(ingame_menu, "Frotz", NULL, "Resume", "Undo", "Restart",
                    "Toggle input recording", "Play back input",
                    "Debug options", "Exit");

zchar menu(void)
{
    switch (rb->do_menu(&ingame_menu, NULL, vp, true))
    {
    case 0:
    default:
        dumb_dump_screen();
        return ZC_BAD;
    case 1:
        return ZC_HKEY_UNDO;
    case 2:
        return ZC_HKEY_RESTART;
    case 3:
        return ZC_HKEY_RECORD;
    case 4:
        return ZC_HKEY_PLAYBACK;
    case 5:
        return ZC_HKEY_DEBUG;
    case 6:
        return ZC_HKEY_QUIT;
    }
}

const struct button_mapping* plugin_contexts[]={generic_actions};

void wait_for_key()
{
    int action;

    dumb_show_screen(false);

    for (;;)
    {
        action = pluginlib_getaction(TIMEOUT_BLOCK,
                                     plugin_contexts, 1);
        switch (action)
        {
        case PLA_QUIT:
            hot_key_quit();
            break;
        
        case PLA_FIRE:
            return;
        }
    }
}

zchar do_input(int timeout, bool show_cursor)
{
    int action;
    long timeout_at;
    zchar menu_ret;

    dumb_show_screen(show_cursor);

    /* Convert timeout (tenths of a second) to ticks */
    if (timeout > 0)
        timeout = (timeout * HZ) / 10;
    else
        timeout = TIMEOUT_BLOCK;

    timeout_at = *rb->current_tick + timeout;

    for (;;)
    {
        action = pluginlib_getaction(timeout,
                                     plugin_contexts, 1);
        switch (action)
        {
        case PLA_QUIT:
            return ZC_HKEY_QUIT;
        
        case PLA_MENU:
            menu_ret = menu();
            if (menu_ret != ZC_BAD)
                return menu_ret;
            timeout_at = *rb->current_tick + timeout;
            break;

        case PLA_FIRE:
            return ZC_RETURN;

        case PLA_START:
            return ZC_BAD;

        default:
            if (timeout != TIMEOUT_BLOCK && 
                    !TIME_BEFORE(*rb->current_tick, timeout_at))
                return ZC_TIME_OUT;
        }
    }
}

zchar os_read_key(int timeout, bool show_cursor)
{
    int r;
    char inputbuf[5];
    short key;
    zchar zkey;

    for(;;)
    {
        zkey = do_input(timeout, show_cursor);
        if (zkey != ZC_BAD)
            return zkey;

        inputbuf[0] = '\0';
        r = rb->kbd_input(inputbuf, 5);
        rb->lcd_setfont(FONT_SYSFIXED);
        dumb_dump_screen();
        if (!r)
        {
            rb->utf8decode(inputbuf, &key);
            if (key > 0 && key < 256)
                return (zchar)key;
        }
    }
}

zchar os_read_line(int max, zchar *buf, int timeout, int width, int continued)
{
    (void)continued;
    int r;
    char inputbuf[256];
    const char *in;
    char *out;
    short key;
    zchar zkey;

    for(;;)
    {
        zkey = do_input(timeout, true);
        if (zkey != ZC_BAD)
            return zkey;

        if (max > width)
            max = width;
        strcpy(inputbuf, buf);
        r = rb->kbd_input(inputbuf, 256);
        rb->lcd_setfont(FONT_SYSFIXED);
        dumb_dump_screen();
        if (!r)
        {
            in = inputbuf;
            out = buf;
            while (*in && max)
            {
                in = rb->utf8decode(in, &key);
                if (key > 0 && key < 256)
                {
                    *out++ = key;
                    max--;
                }
            }
            *out = '\0';
            os_display_string(buf);
            return ZC_RETURN;
        }
    }
}

bool read_yes_or_no(const char *s)
{
    char message_line[50];
    const char *message_lines[] = {message_line};
    const struct text_message message = {message_lines, 1};

    rb->strlcpy(message_line, s, 49);
    rb->strcat(message_line, "?");

    if (rb->gui_syncyesno_run(&message, NULL, NULL) == YESNO_YES)
        return TRUE;
    else
        return FALSE;
}

zchar os_read_mouse(void)
{
    return 0;
}

int os_read_file_name(char *file_name, const char *default_name, int flag)
{
    (void)flag;
    strcpy(file_name, default_name);
    return TRUE;
}

void os_beep(int volume)
{
    rb->splashf(HZ/2, "[%s-PITCHED BEEP]", (volume==1) ? "HIGH" : "LOW");
}

static unsigned char unget_buf;
static int unget_file;

int frotz_ungetc(int c, int f)
{
    unget_file = f;
    unget_buf = c;
    return c;
}

int frotz_fgetc(int f)
{
    unsigned char cb;
    if (unget_file == f)
    {
        unget_file = -1;
        return unget_buf;
    }
    if (rb->read(f, &cb, 1) != 1)
        return EOF;
    return cb;
}

int frotz_fputc(int c, int f)
{
    unsigned char cb = c;
    if (rb->write(f, &cb, 1) != 1)
        return EOF;
    return cb;
}
