/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Wincent Balin
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
#include "pdbox.h"

#include "lib/xlcd.h"

/* Button colors. */
#define BTNCOLOR_DARK (LCD_DARKGRAY)
#define BTNCOLOR_LIGHT (LCD_LIGHTGRAY)

/* Variables in the main code. */
extern char* filename;
extern bool quit;

/* Screen multiplier. */
static float screen_multiplier;

/* Back- and foreground colors. */
static unsigned bgcolor;
static unsigned fgcolor;

/* Displacement of the slanted corner in the contour of the number widget. */
static int number_corner;

/* Button flags. */
static bool play_on;
static bool previous_on;
static bool next_on;
static bool menu_on;
static bool action_on;

/* Pause flag. */
static bool paused;


/* Draw circle using midpoint circle algorithm.
   Adapted from http://en.wikipedia.org/wiki/Midpoint_circle_algorithm. */
void drawcircle(int x, int y, int r)
{
    int f = 1 - r;
    int ddfx = 1;
    int ddfy = -2 * r;
    int xp = 0;
    int yp = r;

    /* Draw outer points. */
    rb->lcd_drawpixel(x, y + r);
    rb->lcd_drawpixel(x, y + r);
    rb->lcd_drawpixel(x + r, y);
    rb->lcd_drawpixel(x - r, y);

    /* Calculate coordinates of points in one octant. */
    while(xp < yp)
    {
        /* ddfx == 2 * xp + 1;
           ddfy == -2 * yp;
           f == xp*xp + yp*yp - r*r + 2*xp - yp + 1; */
        if(f >= 0)
        {
            yp--;
            ddfy += 2;
            f += ddfy;
        }

        xp++;
        ddfx += 2;
        f += ddfx;

        /* Draw pixels in all octants. */
        rb->lcd_drawpixel(x + xp, y + yp);
        rb->lcd_drawpixel(x + xp, y - yp);
        rb->lcd_drawpixel(x - xp, y + yp);
        rb->lcd_drawpixel(x - xp, y - yp);
        rb->lcd_drawpixel(x + yp, y + xp);
        rb->lcd_drawpixel(x + yp, y - xp);
        rb->lcd_drawpixel(x - yp, y + xp);
        rb->lcd_drawpixel(x - yp, y - xp);
    }
}

/* Fill circle. */
void fillcircle(int x, int y, int r)
{
    int f = 1 - r;
    int ddfx = 1;
    int ddfy = -2 * r;
    int xp = 0;
    int yp = r;

    /* Draw outer points. */
    rb->lcd_drawpixel(x, y + r);
    rb->lcd_drawpixel(x, y + r);
    rb->lcd_drawpixel(x + r, y);
    rb->lcd_drawpixel(x - r, y);

    /* Calculate coordinates of points in one octant. */
    while(xp < yp)
    {
        /* ddfx == 2 * xp + 1;
           ddfy == -2 * yp;
           f == xp*xp + yp*yp - r*r + 2*xp - yp + 1; */
        if(f >= 0)
        {
            yp--;
            ddfy += 2;
            f += ddfy;
        }

        xp++;
        ddfx += 2;
        f += ddfx;

        /* Fill circle with horizontal lines. */
        rb->lcd_hline(x - xp, x + xp, y - yp);
        rb->lcd_hline(x - xp, x + xp, y + yp);
        rb->lcd_hline(x - yp, x + yp, y - xp);
        rb->lcd_hline(x - yp, x + yp, y + xp);
    }

    /* Draw last horizontal line (central one). */
    rb->lcd_hline(x - r, x + r, y);
}

/* Initialize GUI. */
void pd_gui_init(void)
{
    /* Reset button flags. */
    play_on = false;
    previous_on = false;
    next_on = false;
    menu_on = false;
    action_on = false;

    /* Unpause Pure Data. */
    paused = false;

    /* Calculate dimension factors. */
    screen_multiplier = ((float) LCD_WIDTH) / 160.0f;
    number_corner = 5 * screen_multiplier;

    /* Get back- and foreground colors. */
    bgcolor = rb->lcd_get_background();
    fgcolor = rb->lcd_get_foreground();

    /* Clear background. */
    rb->lcd_clear_display();

    /* Draw background of appropriate color. */
    rb->lcd_set_foreground(bgcolor);
    rb->lcd_fillrect(0, 0, LCD_WIDTH, LCD_HEIGHT);
    rb->lcd_set_foreground(fgcolor);

    /* Update display. */
    rb->lcd_update();
}

/* Load PD patch. */
unsigned int pd_gui_load_patch(struct pd_widget* wg, unsigned int max_widgets)
{
    int fd;
    char line[100];
    char* saveptr;
    unsigned int widgets = 0;

    /* Open PD patch. */
    fd = open(filename, O_RDONLY);

    /* Check for I/O error. */
    if(!fd)
    {
        /* Show error message and make plug-in quit. */
        rb->splash(HZ, "Error opening .pd file!");
        quit = true;
        return 0;
    }

    /* Read lines from PD file. */
    while(rb->read_line(fd, line, sizeof(line)) > 0)
    {
        /* Check whether we got too many widgets. */
        if(widgets >= max_widgets)
        {
            rb->splash(HZ, "Too many widgets!");
            quit = true;
            return 0;
        }

        /* Search for key strings in the line. */
        if((strstr(line, "floatatom") != NULL) &&
           (strstr(line, "pod_") != NULL))
        {
            wg->id = PD_NUMBER;

            strtok_r(line, " ", &saveptr);
            strtok_r(NULL, " ", &saveptr);
            wg->x = atoi(strtok_r(NULL, " ", &saveptr)) * screen_multiplier;
            wg->y = atoi(strtok_r(NULL, " ", &saveptr)) * screen_multiplier;
            wg->w = 7 * atoi(strtok_r(NULL, " ", &saveptr)) *
                                                          screen_multiplier;
            wg->h = 16 * screen_multiplier;
            strtok_r(NULL, " ", &saveptr);
            strtok_r(NULL, " ", &saveptr);
            strtok_r(NULL, " ", &saveptr);
            strtok_r(NULL, " ", &saveptr);
            strncpy(wg->name, strtok_r(NULL, " ", &saveptr), sizeof(wg->name));

            /* Reset value. */
            wg->value = 0;

            /* We got one more widget. */
            wg++;
            widgets++;
        }
        else if((strstr(line, "symbolatom") != NULL) &&
                (strstr(line, "pod_") != NULL))
        {
            wg->id = PD_SYMBOL;

            strtok_r(line, " ", &saveptr);
            strtok_r(NULL, " ", &saveptr);
            wg->x = atoi(strtok_r(NULL, " ", &saveptr)) * screen_multiplier;
            wg->y = atoi(strtok_r(NULL, " ", &saveptr)) * screen_multiplier;
            wg->w = 7 * atoi(strtok_r(NULL, " ", &saveptr)) *
                                                          screen_multiplier;
            wg->h = 16 * screen_multiplier;
            strtok_r(NULL, " ", &saveptr);
            strtok_r(NULL, " ", &saveptr);
            strtok_r(NULL, " ", &saveptr);
            strtok_r(NULL, " ", &saveptr);
            strncpy(wg->name, strtok_r(NULL, " ", &saveptr), sizeof(wg->name));

            /* Reset value. */
            wg->value = 0;

            /* We got one more widget. */
            wg++;
            widgets++;
        }
        else if((strstr(line, "vsl") != NULL) &&
                (strstr(line, "pod_") != NULL))
        {
            wg->id = PD_VSLIDER;

            strtok_r(line, " ", &saveptr);
            strtok_r(NULL, " ", &saveptr);
            wg->x = atoi(strtok_r(NULL, " ", &saveptr)) * screen_multiplier;
            wg->y = atoi(strtok_r(NULL, " ", &saveptr)) * screen_multiplier;
            strtok_r(NULL, " ", &saveptr);
            wg->w = atoi(strtok_r(NULL, " ", &saveptr)) * screen_multiplier;
            wg->h = atoi(strtok_r(NULL, " ", &saveptr)) * screen_multiplier;
            wg->min = atoi(strtok_r(NULL, " ", &saveptr));
            wg->max = atoi(strtok_r(NULL, " ", &saveptr));
            strtok_r(NULL, " ", &saveptr);
            strtok_r(NULL, " ", &saveptr);
            strtok_r(NULL, " ", &saveptr);
            strncpy(wg->name, strtok_r(NULL, " ", &saveptr), sizeof(wg->name));

            /* Reset value. */
            wg->value = 0;

            /* We got one more widget. */
            wg++;
            widgets++;
        }
        else if((strstr(line, "hsl") != NULL) &&
                (strstr(line, "pod_") != NULL))
        {
            wg->id = PD_HSLIDER;

            strtok_r(line, " ", &saveptr);
            strtok_r(NULL, " ", &saveptr);
            wg->x = atoi(strtok_r(NULL, " ", &saveptr)) * screen_multiplier;
            wg->y = atoi(strtok_r(NULL, " ", &saveptr)) * screen_multiplier;
            strtok_r(NULL, " ", &saveptr);
            wg->w = atoi(strtok_r(NULL, " ", &saveptr)) * screen_multiplier;
            wg->h = atoi(strtok_r(NULL, " ", &saveptr)) * screen_multiplier;
            wg->min = atoi(strtok_r(NULL, " ", &saveptr));
            wg->max = atoi(strtok_r(NULL, " ", &saveptr));
            strtok_r(NULL, " ", &saveptr);
            strtok_r(NULL, " ", &saveptr);
            strtok_r(NULL, " ", &saveptr);
            strncpy(wg->name, strtok_r(NULL, " ", &saveptr), sizeof(wg->name));

            /* Reset value. */
            wg->value = 0;

            /* We got one more widget. */
            wg++;
            widgets++;
        }
        else if((strstr(line, "vradio") != NULL) &&
                (strstr(line, "pod_") != NULL))
        {
            wg->id = PD_VRADIO;

            strtok_r(line, " ", &saveptr);
            strtok_r(NULL, " ", &saveptr);
            wg->x = atoi(strtok_r(NULL, " ", &saveptr)) * screen_multiplier;
            wg->y = atoi(strtok_r(NULL, " ", &saveptr)) * screen_multiplier;
            strtok_r(NULL, " ", &saveptr);
            wg->w = atoi(strtok_r(NULL, " ", &saveptr)) * screen_multiplier;
            strtok_r(NULL, " ", &saveptr);
            strtok_r(NULL, " ", &saveptr);
            wg->min = 0;
            wg->max = atoi(strtok_r(NULL, " ", &saveptr));
            wg->h = wg->w * wg->max;
            strtok_r(NULL, " ", &saveptr);
            strncpy(wg->name, strtok_r(NULL, " ", &saveptr), sizeof(wg->name));
            wg->max--;

            /* Reset value. */
            wg->value = 0;

            /* We got one more widget. */
            wg++;
            widgets++;
        }
        else if((strstr(line, "hradio") != NULL) &&
                (strstr(line, "pod_") != NULL))
        {
            wg->id = PD_HRADIO;

            strtok_r(line, " ", &saveptr);
            strtok_r(NULL, " ", &saveptr);
            wg->x = atoi(strtok_r(NULL, " ", &saveptr)) * screen_multiplier;
            wg->y = atoi(strtok_r(NULL, " ", &saveptr)) * screen_multiplier;
            strtok_r(NULL, " ", &saveptr);
            wg->h = atoi(strtok_r(NULL, " ", &saveptr)) * screen_multiplier;
            strtok_r(NULL, " ", &saveptr);
            strtok_r(NULL, " ", &saveptr);
            wg->min = 0;
            wg->max = atoi(strtok_r(NULL, " ", &saveptr));
            wg->w = wg->h * wg->max;
            strtok_r(NULL, " ", &saveptr);
            strncpy(wg->name, strtok_r(NULL, " ", &saveptr), sizeof(wg->name));
            wg->max--;

            /* Reset value. */
            wg->value = 0;

            /* We got one more widget. */
            wg++;
            widgets++;
        }
        else if((strstr(line, "bng") != NULL) &&
                (strstr(line, "pod_") != NULL))
        {
            wg->id = PD_BANG;

            strtok_r(line, " ", &saveptr);
            strtok_r(NULL, " ", &saveptr);
            wg->x = atoi(strtok_r(NULL, " ", &saveptr)) * screen_multiplier;
            wg->y = atoi(strtok_r(NULL, " ", &saveptr)) * screen_multiplier;
            strtok_r(NULL, " ", &saveptr);
            wg->w = atoi(strtok_r(NULL, " ", &saveptr)) * screen_multiplier;
            wg->h = wg->w;
            strtok_r(NULL, " ", &saveptr);
            strtok_r(NULL, " ", &saveptr);
            strtok_r(NULL, " ", &saveptr);
            strtok_r(NULL, " ", &saveptr);
            strncpy(wg->name, strtok_r(NULL, " ", &saveptr), sizeof(wg->name));
            wg->min = 0;
            wg->max = 1;

            /* Reset value. */
            wg->value = 0;

            /* Clear timeout flag. */
            wg->timeout = 0;

            /* We got one more widget. */
            wg++;
            widgets++;
        }
        else if(strstr(line, "text") != NULL)
        {
            wg->id = PD_TEXT;

            strtok_r(line, " ", &saveptr);
            strtok_r(NULL, " ", &saveptr);
            wg->x = atoi(strtok_r(NULL, " ", &saveptr)) * screen_multiplier;
            wg->y = atoi(strtok_r(NULL, " ", &saveptr)) * screen_multiplier;
            strncpy(wg->name, strtok_r(NULL, " ", &saveptr), sizeof(wg->name));
            char* w = strtok_r(NULL, " ", &saveptr);
            while(w != NULL)
            {
                strcat(wg->name, w);
                strcat(wg->name, " ");
                w = strtok_r(NULL, " ", &saveptr);
            }
            /* Cut off unneeded characters (';' and '\n'). */
            int namelen = strlen(wg->name);
            if(namelen > 1)
            {
                /* Cut off '\n'. */
                wg->name[namelen-1] = '\0';
                namelen--;
                /* Cut the last semi-colon, if there is one. */
                if(wg->name[namelen-1] == ';')
                    wg->name[namelen-1] = '\0';
            }


            /* We got one more widget. */
            wg++;
            widgets++;
        }
    }

    /* Close PD patch. */
    close(fd);

    /* Return amount of loaded widgets. */
    return widgets;
}

/* Draw standard user interface. */
void pd_gui_draw_standard(void)
{
    /* Draw main circle. */
    rb->lcd_set_foreground(fgcolor);
    fillcircle(LCD_WIDTH / 2,
               LCD_HEIGHT / 2,
               2 * MIN(LCD_WIDTH, LCD_HEIGHT) / 5);

    /* Draw center circle. */
    rb->lcd_set_foreground(action_on ? BTNCOLOR_DARK : BTNCOLOR_LIGHT);
    fillcircle(LCD_WIDTH / 2,
               LCD_HEIGHT / 2,
               MIN(LCD_WIDTH, LCD_HEIGHT) / 8);

    /* Draw pressed buttons. */
    if(play_on)
        fillcircle(LCD_WIDTH / 2,
                   3 * LCD_HEIGHT / 4,
                   MIN(LCD_WIDTH, LCD_HEIGHT) / 8);

    if(previous_on)
        fillcircle(LCD_WIDTH / 3,
                   LCD_HEIGHT / 2,
                   MIN(LCD_WIDTH, LCD_HEIGHT) / 8);

    if(next_on)
        fillcircle(2 * LCD_WIDTH / 3 + 1,
                   LCD_HEIGHT / 2,
                   MIN(LCD_WIDTH, LCD_HEIGHT) / 8);

    if(menu_on)
        fillcircle(LCD_WIDTH / 2,
                   LCD_HEIGHT / 4,
                   MIN(LCD_WIDTH, LCD_HEIGHT) / 8);

    /* Restore foreground color. */
    rb->lcd_set_foreground(fgcolor);
}

/* Draw custom user interface. */
void pd_gui_draw_custom(struct pd_widget* wg, unsigned int widgets)
{
    unsigned int i;
    int j;
    int v;

    for(i = 0; i < widgets; wg++, i++)
    {
        switch(wg->id)
        {
            case PD_BANG:
                /* Clear area to (re-)draw. */
                rb->lcd_set_foreground(bgcolor);
                rb->lcd_fillrect(wg->x, wg->y, wg->w, wg->h);
                rb->lcd_set_foreground(fgcolor);
                /* Draw border (rectangle). */
                rb->lcd_drawrect(wg->x, wg->y, wg->w, wg->h);
                /* Draw button (circle), being filled depending on value. */
                if(((int) wg->value) == 0) /* Button not pressed. */
                    drawcircle(wg->x + wg->w/2,
                               wg->y + wg->w/2,
                               wg->w/2 - 1);
                else /* Button pressed. */
                    fillcircle(wg->x + wg->w/2,
                               wg->y + wg->w/2,
                               wg->w/2 - 1);
                break;

            case PD_VSLIDER:
                /* Clear area to (re-)draw. */
                rb->lcd_set_foreground(bgcolor);
                rb->lcd_fillrect(wg->x, wg->y, wg->w, wg->h);
                rb->lcd_set_foreground(fgcolor);
                /* Draw border. */
                rb->lcd_drawrect(wg->x, wg->y, wg->w, wg->h);
                /* Draw slider. */
                v = ((float) wg->h / (wg->max - wg->min)) *
                    (wg->max - wg->value);
                rb->lcd_fillrect(wg->x, wg->y + v, wg->w, 2);
                break;

            case PD_HSLIDER:
                /* Clear area to (re-)draw. */
                rb->lcd_set_foreground(bgcolor);
                rb->lcd_fillrect(wg->x, wg->y, wg->w, wg->h);
                rb->lcd_set_foreground(fgcolor);
                /* Draw border. */
                rb->lcd_drawrect(wg->x, wg->y, wg->w, wg->h);
                /* Draw slider. */
                v = ((float) wg->w / (wg->max - wg->min)) *
                    (wg->max - wg->value);
                rb->lcd_fillrect(wg->x + wg->w - v, wg->y, 2, wg->h);
                break;

            case PD_HRADIO:
                /* Clear area to (re-)draw. */
                rb->lcd_set_foreground(bgcolor);
                rb->lcd_fillrect(wg->x, wg->y, wg->w, wg->h);
                rb->lcd_set_foreground(fgcolor);
                for(j = 0; j < wg->w / wg->h; j++)
                {
                    /* Draw border. */
                    rb->lcd_drawrect(wg->x + wg->h * j, wg->y, wg->h, wg->h);
                    /* If marked, draw button. */
                    if(((int) wg->value) == j)
                        rb->lcd_fillrect(wg->x + wg->h * j + 2, wg->y + 2,
                                         wg->h - 4, wg->h - 4);
                }
                break;

            case PD_VRADIO:
                /* Clear area to (re-)draw. */
                rb->lcd_set_foreground(bgcolor);
                rb->lcd_fillrect(wg->x, wg->y, wg->w, wg->h);
                rb->lcd_set_foreground(fgcolor);
                for(j = 0; j < wg->h / wg->w; j++)
                {
                    /* Draw border. */
                    rb->lcd_drawrect(wg->x, wg->y + wg->w * j, wg->w, wg->w);
                    /* If marked, draw button. */
                    if(((int) wg->value) == j)
                        rb->lcd_fillrect(wg->x + 2, wg->y + wg->w * j + 2,
                                         wg->w - 4, wg->w - 4);
                }
                break;

            case PD_NUMBER:
                rb->lcd_hline(wg->x,
                              wg->x + wg->w - number_corner,
                              wg->y);
                rb->lcd_drawline(wg->x + wg->w - number_corner,
                                 wg->y,
                                 wg->x + wg->w,
                                 wg->y + number_corner);
                rb->lcd_vline(wg->x + wg->w,
                              wg->y + number_corner,
                              wg->y + wg->h);
                rb->lcd_hline(wg->x,
                              wg->x + wg->w,
                              wg->y + wg->h);
                rb->lcd_vline(wg->x,
                              wg->y,
                              wg->y + wg->h);
                char sbuf[8];
                ftoan(wg->value, sbuf, 8);
                rb->lcd_putsxy(wg->x + 2, wg->y + 2, sbuf);
                break;

            case PD_TEXT:
                rb->lcd_putsxy(wg->x + 2, wg->y, wg->name);
                break;

            case PD_SYMBOL:
                break;
        }
    }
}

/* Draw the GUI. */
void pd_gui_draw(struct pd_widget* wg, unsigned int widgets)
{
    /* Draw GUI. */
    if(widgets == 0)
        pd_gui_draw_standard();
    else
        pd_gui_draw_custom(wg, widgets);

    /* Update display. */
    rb->lcd_update();
}

/* Parse buttons, if needed, send messages to the Pure Data code. */
bool pd_gui_parse_buttons(unsigned int widgets)
{
    static long last_bv = 0;
    long bv = rb->button_get(false);

    /* Extract differences between current and previous button values. */
    long diff_bv = bv ^ last_bv;

    /* If no difference since the last button value, return here. */
    if(diff_bv == 0)
        return false;

    /* Query whether we have to quit. */
    if(bv == PDPOD_QUIT)
    {
        /* No need to send the quitting message to Pure Data core,
           as setting the quit flag ends threads and jumps
           to the cleanup code. */
        quit = true;
        return false;
    }

    /* Check the action (shift, mode) button. */
    if(diff_bv & PDPOD_ACTION)
    {
        if(bv & PDPOD_ACTION)
        {
            SEND_TO_CORE("b;\n");

            if(widgets == 0)
                action_on = true;
        }
        else
        {
            if(widgets == 0)
                action_on = false;
        }
    }

    /* Check play button. */
    if(diff_bv & PDPOD_PLAY)
    {
        if(bv & PDPOD_PLAY)
        {
            /* Action + play = pause. */
            if(action_on)
            {
                /* Switch paused state. */
                paused = !paused;
                SEND_TO_CORE(paused ? "p 1;\n" : "p 0;\n");
            }

            if(!action_on && !paused)
                SEND_TO_CORE("d 1;\n");

            if(widgets == 0)
                play_on = true;
        }
        else
        {
            if(!action_on && !paused)
                SEND_TO_CORE("d 0;\n");

            if(widgets == 0)
                play_on = false;
        }
    }

    /* Check rewind (previous) button. */
    if(diff_bv & PDPOD_PREVIOUS)
    {
        if(bv & PDPOD_PREVIOUS)
        {
            if(!paused)
                SEND_TO_CORE("w 1;\n");

            if(widgets == 0)
                previous_on = true;
        }
        else
        {
            if(!paused)
                SEND_TO_CORE("w 0;\n");

            if(widgets == 0)
                previous_on = false;
        }
    }

    /* Check forward (next) button. */
    if(diff_bv & PDPOD_NEXT)
    {
        if(bv & PDPOD_NEXT)
        {
            if(!paused)
                SEND_TO_CORE("f 1;\n");

            if(widgets == 0)
                next_on = true;
        }
        else
        {
            if(!paused)
                SEND_TO_CORE("f 0;\n");

            if(widgets == 0)
                next_on = false;
        }
    }

    /* Check menu (select) button. */
    if(diff_bv & PDPOD_MENU)
    {
        if(bv & PDPOD_MENU)
        {
            if(!action_on && !paused)
                SEND_TO_CORE("m 1;\n");

            if(widgets == 0)
                menu_on = true;
        }
        else
        {
            if(!action_on && !paused)
                SEND_TO_CORE("m 0;\n");

            if(widgets == 0)
                menu_on = false;
        }
    }

    /* Check scroll right (or up) button. */
    if(diff_bv & PDPOD_WHEELRIGHT)
    {
        if(bv & PDPOD_WHEELRIGHT)
        {
            SEND_TO_CORE(action_on ? "r 10;\n" : "r 1;\n");
        }
    }

    /* Check scroll left (or down) button. */
    if(diff_bv & PDPOD_WHEELLEFT)
    {
        if(bv & PDPOD_WHEELLEFT)
        {
            SEND_TO_CORE(action_on ? "l 10;\n" : "l 1;\n");
        }
    }

    /* Backup button value. */
    last_bv = bv;

    /* GUI has to be updated. */
    return true;
}

/* Emulate timer for widgets which use time-out. */
bool pd_gui_apply_timeouts(struct pd_widget* wg, unsigned int widgets)
{
    unsigned int i;
    bool result = false;

    for(i = 0; i < widgets; wg++, i++)
    {
        if(wg->id == PD_BANG)
        {
            if(wg->timeout > 0)
            {
                /* Decrement timeout value. */
                wg->timeout--;

                /* If zero reached, clear value. */
                if(wg->timeout == 0)
                {
                    wg->value = 0;
                    result = true;
                }
            }
        }
    }

    return result;
}

/* Parse and apply message from the Pure Data core. */
void pd_gui_parse_message(struct datagram* dg,
                          struct pd_widget* wg,
                          unsigned int widgets)
{
    unsigned int i;
    char* saveptr;
    char* object = NULL;
    char* argument = NULL;
    float argvalue = 0;

    object = strtok_r(dg->data, " ", &saveptr);
    argument = strtok_r(NULL, " ;\n", &saveptr);

    if(argument != NULL)
        argvalue = atof(argument);

    for(i = 0; i < widgets; wg++, i++)
    {
        if(strncmp(object, wg->name, strlen(wg->name)) == 0)
        {
            /* If object not a number, set boundaries. */
            if(wg->id != PD_NUMBER)
            {
                if(argvalue > wg->max)
                    argvalue = wg->max;
                else if(argvalue < wg->min)
                    argvalue = wg->min;
            }

            /* Set value. */
            if(wg->id == PD_BANG)
            {
                wg->value = 1;
                wg->timeout = HZ / 10;
            }
            else
            {
                wg->value = argvalue;
            }
        }
    }
}
