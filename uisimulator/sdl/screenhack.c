/* xscreensaver, Copyright (c) 1992, 1995, 1997, 1998
 *  Jamie Zawinski <jwz@jwz.org>
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that
 * copyright notice and this permission notice appear in supporting
 * documentation.  No representations are made about the suitability of this
 * software for any purpose.  It is provided "as is" without express or 
 * implied warranty.
 *
 * And remember: X Windows is to graphics hacking as roman numerals are to
 * the square root of pi.
 */

/* This file contains simple code to open a window or draw on the root.
   The idea being that, when writing a graphics hack, you can just link
   with this .o to get all of the uninteresting junk out of the way.

   -  create a procedure `screenhack(dpy, window)'

   -  create a variable `char *progclass' which names this program's
      resource class.

   -  create a variable `char defaults []' for the default resources, and
      null-terminate it.

   -  create a variable `XrmOptionDescRec options[]' for the command-line,
      and null-terminate it.

   And that's it...
 */

#include <stdio.h>
#include <string.h>
#include <SDL.h>
#include "lcd-x11.h"
#include "screenhack.h"
#include "version.h"

#include "debug.h"
#include "config.h"

#ifndef isupper
# define isupper(c)  ((c) >= 'A' && (c) <= 'Z')
#endif
#ifndef _tolower
# define _tolower(c)  ((c) - 'A' + 'a')
#endif

#define KEYBOARD_GENERIC \
  "Keyboard   Rockbox\n" \
  "--------   ------------\n" \
  "4, Left    LEFT\n" \
  "6, Right   RIGHT\n"

#if CONFIG_KEYPAD == PLAYER_PAD
#define KEYBOARD_SPECIFIC \
  "8, Up      PLAY\n" \
  "2, Down    STOP\n" \
  "+, Q       ON\n" \
  "., INS     MENU\n"

#elif CONFIG_KEYPAD == RECORDER_PAD
#define KEYBOARD_SPECIFIC \
  "8, Up      UP\n" \
  "2, Down    DOWN\n" \
  "5, Space   PLAY\n" \
  "+, Q       ON\n" \
  "Enter, A   OFF\n" \
  "/, (F1)    F1\n" \
  "*, (F2)    F2\n" \
  "-, (F3)    F3\n" 

#elif CONFIG_KEYPAD == ONDIO_PAD
#define KEYBOARD_SPECIFIC \
  "8, Up      UP\n" \
  "2, Down    DOWN\n" \
  "., INS     MENU\n" \
  "Enter, A   OFF\n"

#elif CONFIG_KEYPAD == IRIVER_H100_PAD
#define KEYBOARD_SPECIFIC \
  "8, Up      UP\n" \
  "2, Down    DOWN\n" \
  "5, Space   SELECT\n" \
  "+, Q       ON\n" \
  "Enter, A   OFF\n" \
  "., INS     MODE\n" \
  "/, (F1)    RECORD\n"

#elif CONFIG_KEYPAD == IRIVER_H300_PAD
#define KEYBOARD_SPECIFIC \
  "8, Up      UP\n" \
  "2, Down    DOWN\n" \
  "4, Left    LEFT\n" \
  "6, Right   RIGHT\n" \
  "5, Space   SELECT\n" \
  "+, Q       ON\n" \
  "Enter, A   OFF\n" \
  "., INS     MODE\n" \
  "/, (F1)     RECORD\n"

#elif CONFIG_KEYPAD == GMINI100_PAD
#define KEYBOARD_SPECIFIC \
  "8, Up      UP\n" \
  "2, Down    DOWN\n" \
  "5, Space   PLAY\n" \
  "+, Q       ON\n" \
  "Enter, A   OFF\n" \
  "., INS     MENU\n"

#elif (CONFIG_KEYPAD == IPOD_4G_PAD)
#define KEYBOARD_SPECIFIC \
  "[not written yet]"

#elif (CONFIG_KEYPAD == IAUDIO_X5_PAD)
#define KEYBOARD_SPECIFIC \
  "[not written yet]"

#endif


SDL_Surface *surface;

char having_new_lcd = true;

extern int display_zoom;

/* Dead-trivial event handling.
 */
int screenhack_handle_event(SDL_Event *event, bool *release)
{
    int key = 0;

    *release = false;

    switch (event->type) {
        case SDL_KEYDOWN:
            {
                key = event->key.keysym.sym;
                *release = false;
            }
            break;
        case SDL_KEYUP:
            {
                key = event->key.keysym.sym;
                *release = true;
            }
            break;
        case SDL_QUIT:
            {
                SDL_Quit();
                exit(0);
            }
            break;
        default:
            break;
    }

    return key;
}

int screenhack_handle_events(bool *release)
{
    int key = 0;
    SDL_Event event;

    if (SDL_PollEvent(&event)) {
        key = screenhack_handle_event(&event, release);
    }

    return key;
}


int main (int argc, char **argv)
{
    int window_width;
    int window_height;

    if (argc > 1)
    {
        int x;
        for (x=1; x<argc; x++) {
            if (!strcmp("--old_lcd", argv[x])) {
                having_new_lcd=false;
                printf("Using old LCD layout.\n");
            } else if (!strcmp("--zoom", argv[x])) {
                if (++x < argc) {
                    display_zoom=atoi(argv[x]);
                    printf("Window zoom is %d\n", display_zoom);
                    if (display_zoom < 1 || display_zoom > 5) {
                        printf("fatal: --zoom argument must be between 1 and 5\n");
                        exit(0);
                    }
                } else {
                    printf("fatal: --zoom requires an integer argument\n");
                    exit(0);
                }
            } else {
                printf("rockboxui\n");
                printf("Arguments:\n");
                printf("  --old_lcd \t [Player] simulate old playermodel (ROM version<4.51)\n");
                printf("  --zoom [1-5]\t window zoom\n");
                printf(KEYBOARD_GENERIC KEYBOARD_SPECIFIC);
                exit(0);
            }
        }
    }

    window_width = (LCD_WIDTH + 2*MARGIN_X) * display_zoom;
    window_height = (LCD_HEIGHT + 2*MARGIN_Y) * display_zoom;
#ifdef LCD_REMOTE_HEIGHT
    window_height += (LCD_REMOTE_HEIGHT + 2*MARGIN_Y) * display_zoom;
#endif

    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO)) {
        fprintf(stderr, "fatal: %s", SDL_GetError());
        exit(-1);
    }

    atexit(SDL_Quit);

    if ((surface = SDL_SetVideoMode(window_width, window_height, 24, 0)) == NULL) {
        fprintf(stderr, "fatal: %s", SDL_GetError());
        exit(-1);
    }

    screenhack(); /* doesn't return */
    return 0;
}
