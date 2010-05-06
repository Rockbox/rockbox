/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 by Jens Arnold
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"
#include "lib/grey.h"
#include "lib/helper.h"

//#define TEST_GREYLIB  /* Uncomment for testing greylib instead of core gfx */

#ifdef TEST_GREYLIB
#define MYLCD(fn) grey_ ## fn
GREY_INFO_STRUCT
static unsigned char *gbuf;
static size_t gbuf_size = 0;
#else
#define MYLCD(fn) rb->lcd_ ## fn
#endif

#define DURATION (HZ) /* longer duration gives more precise results */
#define RND_SEED 0x43A678C3     /* arbirary */

PLUGIN_HEADER

static uint16_t rand_table[0x400];
static int log_fd;


static int log_init(void)
{
    char logfilename[MAX_PATH];
    int fd;

    rb->create_numbered_filename(logfilename, "/", "test_gfx_log_", ".txt",
                                 2 IF_CNFN_NUM_(, NULL));
    fd = rb->open(logfilename, O_RDWR|O_CREAT|O_TRUNC, 0666);
    return fd;
}

static void init_rand_table(void)
{
    int i;
    
    rb->srand(RND_SEED); /* make it reproducable */
    for (i = 0; i < 0x400; i++)
        rand_table[i] = rb->rand();
}

static void time_drawpixel(void)
{
    long time_start;  /* start tickcount */
    long time_end;    /* end tickcount */
    int count1, count2, count3, count4;

    /* Test 1: DRMODE_SOLID */
    MYLCD(set_drawmode)(DRMODE_SOLID);
    count1 = 0;
    rb->sleep(0); /* sync to tick */
    time_start = *rb->current_tick;
    while((time_end = *rb->current_tick) - time_start < DURATION)
    {
        unsigned rnd = rand_table[count1++ & 0x3ff];
        MYLCD(drawpixel)((rnd >> 8) & 0x3f, rnd & 0x3f);
    }

    /* Test 2: DRMODE_FG */
    MYLCD(set_drawmode)(DRMODE_FG);
    count2 = 0;
    rb->sleep(0); /* sync to tick */
    time_start = *rb->current_tick;
    while((time_end = *rb->current_tick) - time_start < DURATION)
    {
        unsigned rnd = rand_table[count2++ & 0x3ff];
        MYLCD(drawpixel)((rnd >> 8) & 0x3f, rnd & 0x3f);
    }
    /* Test 3: DRMODE_BG */
    MYLCD(set_drawmode)(DRMODE_BG);
    count3 = 0;
    rb->sleep(0); /* sync to tick */
    time_start = *rb->current_tick;
    while((time_end = *rb->current_tick) - time_start < DURATION)
    {
        unsigned rnd = rand_table[count3++ & 0x3ff];
        MYLCD(drawpixel)((rnd >> 8) & 0x3f, rnd & 0x3f);
    }
    /* Test 4: DRMODE_COMPLEMENT */
    MYLCD(set_drawmode)(DRMODE_COMPLEMENT);
    count4 = 0;
    rb->sleep(0); /* sync to tick */
    time_start = *rb->current_tick;
    while((time_end = *rb->current_tick) - time_start < DURATION)
    {
        unsigned rnd = rand_table[count4++ & 0x3ff];
        MYLCD(drawpixel)((rnd >> 8) & 0x3f, rnd & 0x3f);
    }

    rb->fdprintf(log_fd, "lcd_drawpixel (pixels/s): %d/%d/%d/%d\n",
                 count1, count2, count3, count4);
}

static void time_drawline(void)
{
    long time_start;  /* start tickcount */
    long time_end;    /* end tickcount */
    int count1, count2, count3, count4;

    /* Test 1: DRMODE_SOLID */
    MYLCD(set_drawmode)(DRMODE_SOLID);
    count1 = 0;
    rb->sleep(0); /* sync to tick */
    time_start = *rb->current_tick;
    while((time_end = *rb->current_tick) - time_start < DURATION)
    {
        unsigned rnd1 = rand_table[count1++ & 0x3ff];
        unsigned rnd2 = rand_table[count1++ & 0x3ff];
        MYLCD(drawline)((rnd1 >> 8) & 0x3f, rnd1 & 0x3f,
                        (rnd2 >> 8) & 0x3f, rnd2 & 0x3f);
    }

    /* Test 2: DRMODE_FG */
    MYLCD(set_drawmode)(DRMODE_FG);
    count2 = 0;
    rb->sleep(0); /* sync to tick */
    time_start = *rb->current_tick;
    while((time_end = *rb->current_tick) - time_start < DURATION)
    {
        unsigned rnd1 = rand_table[count2++ & 0x3ff];
        unsigned rnd2 = rand_table[count2++ & 0x3ff];
        MYLCD(drawline)((rnd1 >> 8) & 0x3f, rnd1 & 0x3f,
                        (rnd2 >> 8) & 0x3f, rnd2 & 0x3f);
    }
    /* Test 3: DRMODE_BG */
    MYLCD(set_drawmode)(DRMODE_BG);
    count3 = 0;
    rb->sleep(0); /* sync to tick */
    time_start = *rb->current_tick;
    while((time_end = *rb->current_tick) - time_start < DURATION)
    {
        unsigned rnd1 = rand_table[count3++ & 0x3ff];
        unsigned rnd2 = rand_table[count3++ & 0x3ff];
        MYLCD(drawline)((rnd1 >> 8) & 0x3f, rnd1 & 0x3f,
                        (rnd2 >> 8) & 0x3f, rnd2 & 0x3f);
    }
    /* Test 4: DRMODE_COMPLEMENT */
    MYLCD(set_drawmode)(DRMODE_COMPLEMENT);
    count4 = 0;
    rb->sleep(0); /* sync to tick */
    time_start = *rb->current_tick;
    while((time_end = *rb->current_tick) - time_start < DURATION)
    {
        unsigned rnd1 = rand_table[count4++ & 0x3ff];
        unsigned rnd2 = rand_table[count4++ & 0x3ff];
        MYLCD(drawline)((rnd1 >> 8) & 0x3f, rnd1 & 0x3f,
                        (rnd2 >> 8) & 0x3f, rnd2 & 0x3f);
    }

    rb->fdprintf(log_fd, "lcd_drawline  (lines/s):  %d/%d/%d/%d\n",
                 count1, count2, count3, count4);
}

static void time_hline(void)
{
    long time_start;  /* start tickcount */
    long time_end;    /* end tickcount */
    int count1, count2, count3, count4;

    /* Test 1: DRMODE_SOLID */
    MYLCD(set_drawmode)(DRMODE_SOLID);
    count1 = 0;
    rb->sleep(0); /* sync to tick */
    time_start = *rb->current_tick;
    while((time_end = *rb->current_tick) - time_start < DURATION)
    {
        unsigned rnd1 = rand_table[count1++ & 0x3ff];
        unsigned rnd2 = rand_table[count1++ & 0x3ff];
        MYLCD(hline)((rnd1 >> 8) & 0x3f, rnd1 & 0x3f, rnd2 & 0x3f);
    }

    /* Test 2: DRMODE_FG */
    MYLCD(set_drawmode)(DRMODE_FG);
    count2 = 0;
    rb->sleep(0); /* sync to tick */
    time_start = *rb->current_tick;
    while((time_end = *rb->current_tick) - time_start < DURATION)
    {
        unsigned rnd1 = rand_table[count2++ & 0x3ff];
        unsigned rnd2 = rand_table[count2++ & 0x3ff];
        MYLCD(hline)((rnd1 >> 8) & 0x3f, rnd1 & 0x3f, rnd2 & 0x3f);
    }
    /* Test 3: DRMODE_BG */
    MYLCD(set_drawmode)(DRMODE_BG);
    count3 = 0;
    rb->sleep(0); /* sync to tick */
    time_start = *rb->current_tick;
    while((time_end = *rb->current_tick) - time_start < DURATION)
    {
        unsigned rnd1 = rand_table[count3++ & 0x3ff];
        unsigned rnd2 = rand_table[count3++ & 0x3ff];
        MYLCD(hline)((rnd1 >> 8) & 0x3f, rnd1 & 0x3f, rnd2 & 0x3f);
    }
    /* Test 4: DRMODE_COMPLEMENT */
    MYLCD(set_drawmode)(DRMODE_COMPLEMENT);
    count4 = 0;
    rb->sleep(0); /* sync to tick */
    time_start = *rb->current_tick;
    while((time_end = *rb->current_tick) - time_start < DURATION)
    {
        unsigned rnd1 = rand_table[count4++ & 0x3ff];
        unsigned rnd2 = rand_table[count4++ & 0x3ff];
        MYLCD(hline)((rnd1 >> 8) & 0x3f, rnd1 & 0x3f, rnd2 & 0x3f);
    }

    rb->fdprintf(log_fd, "lcd_hline     (lines/s):  %d/%d/%d/%d\n",
                 count1, count2, count3, count4);
}

static void time_vline(void)
{
    long time_start;  /* start tickcount */
    long time_end;    /* end tickcount */
    int count1, count2, count3, count4;

    /* Test 1: DRMODE_SOLID */
    MYLCD(set_drawmode)(DRMODE_SOLID);
    count1 = 0;
    rb->sleep(0); /* sync to tick */
    time_start = *rb->current_tick;
    while((time_end = *rb->current_tick) - time_start < DURATION)
    {
        unsigned rnd1 = rand_table[count1++ & 0x3ff];
        unsigned rnd2 = rand_table[count1++ & 0x3ff];
        MYLCD(vline)((rnd1 >> 8) & 0x3f, rnd1 & 0x3f, rnd2 & 0x3f);
    }

    /* Test 2: DRMODE_FG */
    MYLCD(set_drawmode)(DRMODE_FG);
    count2 = 0;
    rb->sleep(0); /* sync to tick */
    time_start = *rb->current_tick;
    while((time_end = *rb->current_tick) - time_start < DURATION)
    {
        unsigned rnd1 = rand_table[count2++ & 0x3ff];
        unsigned rnd2 = rand_table[count2++ & 0x3ff];
        MYLCD(vline)((rnd1 >> 8) & 0x3f, rnd1 & 0x3f, rnd2 & 0x3f);
    }
    /* Test 3: DRMODE_BG */
    MYLCD(set_drawmode)(DRMODE_BG);
    count3 = 0;
    rb->sleep(0); /* sync to tick */
    time_start = *rb->current_tick;
    while((time_end = *rb->current_tick) - time_start < DURATION)
    {
        unsigned rnd1 = rand_table[count3++ & 0x3ff];
        unsigned rnd2 = rand_table[count3++ & 0x3ff];
        MYLCD(vline)((rnd1 >> 8) & 0x3f, rnd1 & 0x3f, rnd2 & 0x3f);
    }
    /* Test 4: DRMODE_COMPLEMENT */
    MYLCD(set_drawmode)(DRMODE_COMPLEMENT);
    count4 = 0;
    rb->sleep(0); /* sync to tick */
    time_start = *rb->current_tick;
    while((time_end = *rb->current_tick) - time_start < DURATION)
    {
        unsigned rnd1 = rand_table[count4++ & 0x3ff];
        unsigned rnd2 = rand_table[count4++ & 0x3ff];
        MYLCD(vline)((rnd1 >> 8) & 0x3f, rnd1 & 0x3f, rnd2 & 0x3f);
    }

    rb->fdprintf(log_fd, "lcd_vline     (lines/s):  %d/%d/%d/%d\n",
                 count1, count2, count3, count4);
}

static void time_fillrect(void)
{
    long time_start;  /* start tickcount */
    long time_end;    /* end tickcount */
    int count1, count2, count3, count4;

    /* Test 1: DRMODE_SOLID */
    MYLCD(set_drawmode)(DRMODE_SOLID);
    count1 = 0;
    rb->sleep(0); /* sync to tick */
    time_start = *rb->current_tick;
    while((time_end = *rb->current_tick) - time_start < DURATION)
    {
        unsigned rnd1 = rand_table[count1++ & 0x3ff];
        unsigned rnd2 = rand_table[count1++ & 0x3ff];
        MYLCD(fillrect)((rnd1 >> 8) & 0x3f, rnd1 & 0x3f,
                        (rnd2 >> 8) & 0x3f, rnd2 & 0x3f);
    }

    /* Test 2: DRMODE_FG */
    MYLCD(set_drawmode)(DRMODE_FG);
    count2 = 0;
    rb->sleep(0); /* sync to tick */
    time_start = *rb->current_tick;
    while((time_end = *rb->current_tick) - time_start < DURATION)
    {
        unsigned rnd1 = rand_table[count2++ & 0x3ff];
        unsigned rnd2 = rand_table[count2++ & 0x3ff];
        MYLCD(fillrect)((rnd1 >> 8) & 0x3f, rnd1 & 0x3f,
                        (rnd2 >> 8) & 0x3f, rnd2 & 0x3f);
    }
    /* Test 3: DRMODE_BG */
    MYLCD(set_drawmode)(DRMODE_BG);
    count3 = 0;
    rb->sleep(0); /* sync to tick */
    time_start = *rb->current_tick;
    while((time_end = *rb->current_tick) - time_start < DURATION)
    {
        unsigned rnd1 = rand_table[count3++ & 0x3ff];
        unsigned rnd2 = rand_table[count3++ & 0x3ff];
        MYLCD(fillrect)((rnd1 >> 8) & 0x3f, rnd1 & 0x3f,
                        (rnd2 >> 8) & 0x3f, rnd2 & 0x3f);
    }
    /* Test 4: DRMODE_COMPLEMENT */
    MYLCD(set_drawmode)(DRMODE_COMPLEMENT);
    count4 = 0;
    rb->sleep(0); /* sync to tick */
    time_start = *rb->current_tick;
    while((time_end = *rb->current_tick) - time_start < DURATION)
    {
        unsigned rnd1 = rand_table[count4++ & 0x3ff];
        unsigned rnd2 = rand_table[count4++ & 0x3ff];
        MYLCD(fillrect)((rnd1 >> 8) & 0x3f, rnd1 & 0x3f,
                        (rnd2 >> 8) & 0x3f, rnd2 & 0x3f);
    }

    rb->fdprintf(log_fd, "lcd_fillrect  (rects/s):  %d/%d/%d/%d\n",
                 count1, count2, count3, count4);
}

static void time_text(void) /* tests mono_bitmap performance */
{
    long time_start;  /* start tickcount */
    long time_end;    /* end tickcount */
    int count1, count2, count3, count4;
    
    rb->lcd_setfont(FONT_SYSFIXED);

    /* Test 1: DRMODE_SOLID */
    MYLCD(set_drawmode)(DRMODE_SOLID);
    count1 = 0;
    rb->sleep(0); /* sync to tick */
    time_start = *rb->current_tick;
    while((time_end = *rb->current_tick) - time_start < DURATION)
    {
        unsigned rnd = rand_table[count1++ & 0x3ff];
        MYLCD(putsxy)((rnd >> 8) & 0x3f, rnd & 0x3f, "Rockbox!");
    }

    /* Test 2: DRMODE_FG */
    MYLCD(set_drawmode)(DRMODE_FG);
    count2 = 0;
    rb->sleep(0); /* sync to tick */
    time_start = *rb->current_tick;
    while((time_end = *rb->current_tick) - time_start < DURATION)
    {
        unsigned rnd = rand_table[count2++ & 0x3ff];
        MYLCD(putsxy)((rnd >> 8) & 0x3f, rnd & 0x3f, "Rockbox!");
    }
    /* Test 3: DRMODE_BG */
    MYLCD(set_drawmode)(DRMODE_BG);
    count3 = 0;
    rb->sleep(0); /* sync to tick */
    time_start = *rb->current_tick;
    while((time_end = *rb->current_tick) - time_start < DURATION)
    {
        unsigned rnd = rand_table[count3++ & 0x3ff];
        MYLCD(putsxy)((rnd >> 8) & 0x3f, rnd & 0x3f, "Rockbox!");
    }
    /* Test 4: DRMODE_COMPLEMENT */
    MYLCD(set_drawmode)(DRMODE_COMPLEMENT);
    count4 = 0;
    rb->sleep(0); /* sync to tick */
    time_start = *rb->current_tick;
    while((time_end = *rb->current_tick) - time_start < DURATION)
    {
        unsigned rnd = rand_table[count4++ & 0x3ff];
        MYLCD(putsxy)((rnd >> 8) & 0x3f, rnd & 0x3f, "Rockbox!");
    }

    rb->fdprintf(log_fd, "lcd_putsxy   (strings/s): %d/%d/%d/%d\n",
                 count1, count2, count3, count4);
}

/* plugin entry point */
enum plugin_status plugin_start(const void* parameter)
{
#ifndef SIMULATOR
    int cpu_freq;
#endif

    /* standard stuff */
    (void)parameter;

    log_fd = log_init();
    if (log_fd < 0)
    {
        rb->splash(HZ, "Could not create logfile");
        return PLUGIN_ERROR;
    }
    rb->fdprintf(log_fd, "%s", 
#ifdef TEST_GREYLIB
                 "Greylib performance test.\n"
#else
                 "LCD driver performance test.\n"
#endif
                 "----------------------------\n\n"
                 "Results are printed in the following drawmode order:\n"
                 "solid/foreground/background/complement\n\n");

#ifdef TEST_GREYLIB
    /* get the remainder of the plugin buffer */
    gbuf = (unsigned char *) rb->plugin_get_buffer(&gbuf_size);

    /* initialize the greyscale buffer.*/
    if (!grey_init(gbuf, gbuf_size, GREY_BUFFERED|GREY_ON_COP,
                   LCD_WIDTH, LCD_HEIGHT, NULL))
    {
        rb->close(log_fd);
        rb->splash(HZ, "Couldn't init greyscale library");
        return PLUGIN_ERROR;
    }
#elif LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
    rb->lcd_clear_display();
#endif
    backlight_force_on(); /* backlight control in lib/helper.c */

    rb->splashf(0, "LCD driver performance test, please wait %d sec",
                6*4*DURATION/HZ);
    init_rand_table();

#ifndef SIMULATOR
    cpu_freq = *rb->cpu_frequency; /* remember CPU frequency */
#endif

    time_drawpixel();
    time_drawline();
    time_hline();
    time_vline();
    time_fillrect();
    time_text();

#ifndef SIMULATOR
    if (*rb->cpu_frequency != cpu_freq)
        rb->fdprintf(log_fd, "\nCPU: %s\n", "clock changed!");
    else
        rb->fdprintf(log_fd, "\nCPU: %d MHz\n",
                     (cpu_freq + 500000) / 1000000);
#endif
    rb->close(log_fd);
    backlight_use_settings();
#ifdef TEST_GREYLIB
    grey_release();
#endif

    return PLUGIN_OK;
}
