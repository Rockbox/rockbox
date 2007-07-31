/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2003 Zakk Roberts
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/*****************************
 * RELEASE NOTES

***** VERSION 4.00 **
New almost entirely bitmap based display. Scales to all resolutions. Combined
Digital and LCD modes into one. Use Rockbox menu code for the menu. Removed
count-down option. A couple new options. Source code reworked, improved, cleaned
up.

***** VERSION 3.10 **
Drawing now scales for the display - still needs updated bitmaps for the binary
and plain mode.  The Time's Up logo could also be updated.

***** VERSION 3.00 **
New, simpler UI - every screen can be accessed from the new Main Menu.
Huge code cleanup - many major functions rewritten and optimized,
targeting scalability. Number of variables reduced majorly.
New clock mode: Plain (simple, large text). ON now controls counter
(press toggle/hold reset). Fancier credits roll. New logo. iRiver and iPod ports
are working but not yet scaled to fit their LCDs.

***** VERSION 2.60 **
Fixed general settings typo, split up settings function, added cursor animations,
and updated cursor look (rounded edges).

***** VERSION 2.51 **
"Show Counter" option is now saved to disk

***** VERSION 2.50 **
New general settings mode added, reworked options screen, cleaned up a few
things and removed redundant code, faster load_settings(), fixed a
help-screen bug (thanks to zeekoe)

***** VERSION 2.40 **
Cleaned and optimized code, removed unused code/bitmaps, credits screen updated,
centered text all over, general settings added at ON+F3,
new arrow bitmap for general settings and mode selector,
bugfix: 12:00AM is no longer 00:00AM

***** VERSION 2.31 **
Fixed credits roll - now displays all names. Features
improved animations. Also revised release notes.

***** VERSION 2.30 **
Tab indentation removed, and Counter screen added
at ON+F2, with countdown options

***** VERSION 2.22 **
Fixed two bugs:
Digital settings are now independent of LCD settings
12/24h "Analog" settings are now displayed correctly.

***** VERSION 2.21 **
-Changed the behaviour of F2

***** VERSION 2.20 **
Few small bugs taken care of. New features:
New binary mode, new mode selector, "counter", and redesigned help screen.

***** VERSION 2.10 **
New bug fixes, and some new features:
an LCD imitation mode, and American and European date modes.

***** VERSION 2.00 **
Major update, lots of bugfixes and new features.
Fullscreen mode introduced, modes have independent settings, credit roll
added, options screen reworked, logo selector, and -much- cleaner code.

***** VERSION 1.0 **
Original release, featuring analog/digital modes and a few options.
*****************************/
#include "plugin.h"
#include "time.h"
#include "checkbox.h"
#include "xlcd.h"
#include "oldmenuapi.h"
#include "fixedpoint.h"
#include "pluginlib_actions.h"

PLUGIN_HEADER

/* External bitmap references */
#include "clock_digits.h"
#include "clock_smalldigits.h"
#include "clock_smallsegments.h"
#include "clock_messages.h"
#include "clock_logo.h"
#include "clock_segments.h"

/* Bitmap positions/deltas, per LCD size */
#if (LCD_WIDTH >= 112) && (LCD_HEIGHT >= 64) && (LCD_DEPTH >= 1) /* Archos */
#define LCD_OFFSET 1
#define HAND_W 2
#else
#define LCD_OFFSET 1.5
#define HAND_W 3
#endif

#define DIGIT_WIDTH BMPWIDTH_clock_digits
#define DIGIT_HEIGHT (BMPHEIGHT_clock_digits/15)
#define SMALLDIGIT_WIDTH BMPWIDTH_clock_smalldigits
#define SMALLDIGIT_HEIGHT (BMPHEIGHT_clock_smalldigits/13)
#define SMALLSEG_WIDTH BMPWIDTH_clock_smallsegments
#define SMALLSEG_HEIGHT (BMPHEIGHT_clock_smallsegments/13)
#define MESSAGE_WIDTH BMPWIDTH_clock_messages
#define MESSAGE_HEIGHT (BMPHEIGHT_clock_messages/6)
#define LOGO_WIDTH BMPWIDTH_clock_logo
#define LOGO_HEIGHT BMPHEIGHT_clock_logo

/* Parts of larger bitmaps */
#define COLON      10
#define DOT_FILLED 11
#define DOT_EMPTY  12
#define ICON_PM    13
#define ICON_AM    14
#define SEGMENT_AM 11
#define SEGMENT_PM 12
#define SLASH      11
#define PERIOD     12

/* Message names/values */
#define MESSAGE_LOADING 0
#define MESSAGE_LOADED  1
#define MESSAGE_ERRLOAD 2
#define MESSAGE_SAVING  3
#define MESSAGE_SAVED   4
#define MESSAGE_ERRSAVE 5

/* Some macros to simplify drawing et al */
#define draw_digit( num, x, y )\
    rb->lcd_bitmap_part( clock_digits, 0, num * DIGIT_HEIGHT, \
                         DIGIT_WIDTH, x, y, DIGIT_WIDTH, DIGIT_HEIGHT )
#define draw_smalldigit( num, x, y )\
    rb->lcd_bitmap_part( clock_smalldigits, 0, num * SMALLDIGIT_HEIGHT, \
                         SMALLDIGIT_WIDTH, x, y, SMALLDIGIT_WIDTH, SMALLDIGIT_HEIGHT )
#define draw_segment( num, x, y )\
    rb->lcd_bitmap_part( clock_segments, 0, num * DIGIT_HEIGHT, \
                         DIGIT_WIDTH, x, y, DIGIT_WIDTH, DIGIT_HEIGHT )
#define draw_smallsegment( num, x, y )\
    rb->lcd_bitmap_part( clock_smallsegments, 0, num * SMALLSEG_HEIGHT, \
                         SMALLSEG_WIDTH, x, y, SMALLSEG_WIDTH, SMALLSEG_HEIGHT )
#define draw_message( msg, ypos )\
    rb->lcd_bitmap_part( clock_messages, 0, msg*MESSAGE_HEIGHT, MESSAGE_WIDTH, \
                         0, LCD_HEIGHT-(MESSAGE_HEIGHT*ypos), MESSAGE_WIDTH, MESSAGE_HEIGHT )
#define DIGIT_XOFS(x) (LCD_WIDTH-x*DIGIT_WIDTH)/2
#define DIGIT_YOFS(x) (LCD_HEIGHT-x*DIGIT_HEIGHT)/2
#define SMALLDIGIT_XOFS(x) (LCD_WIDTH-x*SMALLDIGIT_WIDTH)/2
#define SMALLDIGIT_YOFS(x) (LCD_HEIGHT-x*SMALLDIGIT_HEIGHT)/2
#define SMALLSEG_XOFS(x) (LCD_WIDTH-x*SMALLSEG_WIDTH)/2
#define SMALLSEG_YOFS(x) (LCD_HEIGHT-x*SMALLSEG_HEIGHT)/2

/* Keymaps */
const struct button_mapping* plugin_contexts[]={
    generic_actions,
    generic_directions
};

#define ACTION_COUNTER_TOGGLE PLA_FIRE
#define ACTION_COUNTER_RESET PLA_FIRE_REPEAT
#define ACTION_MENU PLA_MENU
#define ACTION_EXIT PLA_QUIT
#define ACTION_MODE_NEXT PLA_RIGHT
#define ACTION_MODE_PREV PLA_LEFT

/************
 * Prototypes
 ***********/
void save_settings(bool interface);

/********************
 * Misc counter stuff
 *******************/
int start_tick = 0;
int passed_time = 0;
int counter = 0;
int displayed_value = 0;
int count_h, count_m, count_s;
bool counting = false;

/********************
 * Everything else...
 *******************/
bool idle_poweroff = true; /* poweroff activated or not? */
bool done = false; /* used for most of the while loops */
bool exit_clock = false; /* when true, the main plugin loop will exit */

static struct plugin_api* rb;

/***********************************************************************
 * Used for hands to define lengths at a given time, analog + fullscreen
 **********************************************************************/
unsigned int xminute[61];
unsigned int yminute[61];
unsigned int yhour[61];
unsigned int xhour[61];
unsigned int xminute_full[61];
unsigned int yminute_full[61];
unsigned int xhour_full[61];
unsigned int yhour_full[61];

/* settings are saved to this location */
static const char default_filename[] = "/.rockbox/rocks/.clock_settings";

/*********************************************************
 * Some arrays/definitions for drawing settings/menu text.
 ********************************************************/
#define ANALOG      1
#define DIGITAL     2
#define FULLSCREEN  3
#define BINARY      4
#define PLAIN       5
#define CLOCK_MODES 5

#define analog_date 0
#define analog_secondhand 1
#define analog_time 2
#define digital_seconds 0
#define digital_date 1
#define digital_blinkcolon 2
#define digital_format 3
#define fullscreen_border 0
#define fullscreen_secondhand 1
#define binary_mode 0
#define plain_format 0
#define plain_seconds 1
#define plain_date 2
#define plain_blinkcolon 3
#define general_counter 0
#define general_savesetting 1
#define general_backlight 2

/* Menu structs (lists of menu items) */
static const struct menu_item main_menu_items[] = {
    { "View Clock", NULL },
    { "Mode Selector",  NULL },
    { "Mode Settings", NULL },
    { "General Settings", NULL },
    { "Exit Plugin", NULL }
};
static const struct menu_item mode_selector_items[] = {
    { "Analog", NULL },
    { "Digital/LCD",  NULL },
    { "Full-screen", NULL },
    { "Binary", NULL },
    { "Plain", NULL }
};
static const struct menu_item general_settings_items[] = {
    { "Reset Settings", NULL },
    { "Save Settings Now",  NULL },
    { "Save On Exit", NULL },
    { "Show Counter", NULL },
    { "Backlight Settings", NULL },
    { "Idle Poweroff (temporary)", NULL }
};
static const struct menu_item analog_items[] = {
    { "Show Date",  NULL },
    { "Show Second Hand", NULL },
    { "Show Time Readout", NULL }
};
static const struct menu_item digital_items[] = {
    { "Show Seconds", NULL },
    { "Show Date",  NULL },
    { "Blinking Colon", NULL },
    { "Time Format", NULL }
};
static const struct menu_item fullscreen_items[] = {
    { "Show Border", NULL },
    { "Show Second Hand",  NULL }
};
static const struct menu_item binary_items[] = {
    { "Display Mode", NULL }
};
static const struct menu_item plain_items[] = {
    { "Show Date",  NULL },
    { "Show Seconds", NULL },
    { "Blinking Colon", NULL },
    { "Time Format", NULL }
};

/* Option structs (possible selections per each option) */
static const struct opt_items noyes_text[2] = {
    { "No", -1 },
    { "Yes", -1 }
};

static const struct opt_items backlight_settings_text[3] = {
    { "Always Off", -1 },
    { "Rockbox setting", -1 },
    { "Always On", -1 }
};
static const struct opt_items idle_poweroff_text[2] = {
    { "Disabled", -1 },
    { "Enabled", -1 }
};
static const struct opt_items counting_direction_text[2] = {
    {"Down",   -1},
    {"Up", -1}
};
static const struct opt_items date_format_text[] = {
    { "No", -1 },
    { "American format", -1 },
    { "European format", -1 }
};

static const struct opt_items analog_time_text[3] = {
    { "No", -1 },
    { "24-hour Format", -1 },
    { "12-hour Format", -1 }
};

static const struct opt_items time_format_text[2] = {
    { "24-hour Format", -1 },
    { "12-hour Format", -1 }
};

static const struct opt_items binary_mode_text[2] = {
    { "Numbers", -1 },
    { "Dots", -1 }
};

/*****************************************
 * All settings, saved to default_filename
 ****************************************/
struct saved_settings
{
    int clock; /* clock mode */
    int general[4]; /* general settings*/
    int analog[3];
    int digital[4];
    int fullscreen[2];
    int binary[1];
    int plain[4];
} settings;

/************************
 * Setting default values
 ***********************/
void reset_settings(void)
{
    settings.clock = 1;
    settings.general[general_counter] = 1;
    settings.general[general_savesetting] = 1;
    settings.general[general_backlight] = 2;
    settings.analog[analog_date] = 0;
    settings.analog[analog_secondhand] = true;
    settings.analog[analog_time] = false;
    settings.digital[digital_seconds] = 1;
    settings.digital[digital_date] = 1;
    settings.digital[digital_blinkcolon] = false;
    settings.digital[digital_format] = true;
    settings.fullscreen[fullscreen_border] = true;
    settings.fullscreen[fullscreen_secondhand] = true;
    settings.plain[plain_format] = true;
    settings.plain[plain_seconds] = true;
    settings.plain[plain_date] = 1;
    settings.plain[plain_blinkcolon] = false;
}

/**************************************************************
 * Simple function to check if we're switching to digital mode,
 * and if so, set bg/fg colors appropriately.
 *************************************************************/
void set_digital_colors(void)
{
#ifdef HAVE_LCD_COLOR /* color LCDs.. */
    if(settings.clock == DIGITAL)
    {
        rb->lcd_set_foreground(LCD_WHITE);
        rb->lcd_set_background(LCD_BLACK);
    }
    else
    {
        rb->lcd_set_foreground(LCD_BLACK);
        rb->lcd_set_background(LCD_RGBPACK(180,200,230));
    }
#elif LCD_DEPTH >= 2
    if(settings.clock == DIGITAL)
    {
        rb->lcd_set_foreground(LCD_WHITE);
        rb->lcd_set_background(LCD_BLACK);
    }
    else
    {
        rb->lcd_set_foreground(LCD_BLACK);
        rb->lcd_set_background(LCD_WHITE);
    }
#endif
}

/*************************************************************
 * Simple function to set standard black-on-light blue colors.
 ************************************************************/
void set_standard_colors(void)
{
#ifdef HAVE_LCD_COLOR /* color LCDs only.. */
    rb->lcd_set_foreground(LCD_BLACK);
    rb->lcd_set_background(LCD_RGBPACK(180,200,230));
#elif LCD_DEPTH >= 2
    rb->lcd_set_foreground(LCD_BLACK);
    rb->lcd_set_background(LCD_WHITE);
#endif
}

/**************************
 * Cleanup on plugin return
 *************************/
void cleanup(void *parameter)
{
    (void)parameter;

    if(settings.general[general_savesetting] == 1)
        save_settings(true);

    /* restore set backlight timeout */
    rb->backlight_set_timeout(rb->global_settings->backlight_timeout);
}

/****************
 * Shows the logo
 ***************/
void show_clock_logo(void)
{
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_BLACK);
    rb->lcd_set_background(LCD_RGBPACK(180,200,230));
#endif

    rb->lcd_clear_display();;

    rb->lcd_bitmap(clock_logo, 0, 0, LOGO_WIDTH, LOGO_HEIGHT);

    rb->lcd_update();
}

/********************************
 * Saves "saved_settings" to disk
 *******************************/
void save_settings(bool interface)
{
    int fd;

    if(interface)
    {
        rb->lcd_clear_display();
        show_clock_logo();

        draw_message(MESSAGE_SAVING, 1);

        rb->lcd_update();
    }

    fd = rb->creat(default_filename); /* create the settings file */

    if(fd >= 0) /* file exists, save successful */
    {
        rb->write (fd, &settings, sizeof(struct saved_settings));
        rb->close(fd);

        if(interface)
        {
            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
            rb->lcd_fillrect(0, LCD_HEIGHT-MESSAGE_HEIGHT, LCD_WIDTH, MESSAGE_HEIGHT);
            rb->lcd_set_drawmode(DRMODE_SOLID);
            draw_message(MESSAGE_SAVED, 1);
        }
    }
    else /* couldn't save for some reason */
    {
        if(interface)
        {
            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
            rb->lcd_fillrect(0, LCD_HEIGHT-MESSAGE_HEIGHT, LCD_WIDTH, MESSAGE_HEIGHT);
            rb->lcd_set_drawmode(DRMODE_SOLID);
            draw_message(MESSAGE_ERRSAVE, 1);
        }
    }

    if(interface)
    {
        rb->lcd_update();
        rb->sleep(HZ); /* pause a second */
    }
}

/**********************************
 * Loads "saved_settings" from disk
 *********************************/
void load_settings(void)
{
    /* open the settings file */
    int fd;
    fd = rb->open(default_filename, O_RDONLY);

    rb->lcd_clear_display();
    show_clock_logo();

    draw_message(MESSAGE_LOADING, 1);

    rb->lcd_update();

    if(fd >= 0) /* does file exist? */
    {
        if(rb->filesize(fd) == sizeof(struct saved_settings)) /* if so, is it the right size? */
        {
            rb->read(fd, &settings, sizeof(struct saved_settings));
            rb->close(fd);

            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
            rb->lcd_fillrect(0, LCD_HEIGHT-MESSAGE_HEIGHT, LCD_WIDTH, MESSAGE_HEIGHT);
            rb->lcd_set_drawmode(DRMODE_SOLID);
            draw_message(MESSAGE_LOADED, 1);
        }
        else /* must be invalid, bail out */
        {
            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
            rb->lcd_fillrect(0, LCD_HEIGHT-MESSAGE_HEIGHT, LCD_WIDTH, MESSAGE_HEIGHT);
            rb->lcd_set_drawmode(DRMODE_SOLID);
            draw_message(MESSAGE_ERRLOAD, 1);

            reset_settings();
        }
    }
    else /* must be missing, bail out */
    {
        rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        rb->lcd_fillrect(0, LCD_HEIGHT-MESSAGE_HEIGHT, LCD_WIDTH, MESSAGE_HEIGHT);
        rb->lcd_set_drawmode(DRMODE_SOLID);
        draw_message(MESSAGE_ERRLOAD, 1);

        reset_settings();
    }

    rb->lcd_update();

#ifndef SIMULATOR
    rb->ata_sleep();
#endif

    rb->sleep(HZ);
}

/*******************************
 * Init clock, set up x/y tables
 ******************************/
void init_clock(void)
{
    #define ANALOG_VALUES 60
    #define ANALOG_YCENTER (LCD_HEIGHT/2)
    #define ANALOG_XCENTER (LCD_WIDTH/2)
    #define ANALOG_MIN_RADIUS MIN(LCD_HEIGHT/2 -10, LCD_WIDTH/2 -10)
    #define ANALOG_HR_RADIUS ((2 * ANALOG_MIN_RADIUS)/3)

    #define PI 3.141592
    int i;

    rb->lcd_setfont(FONT_SYSFIXED); /* universal font */

    load_settings();

    /* set backlight timeout */
    if(settings.general[general_backlight] == 0)
        rb->backlight_set_timeout(0);
    else if(settings.general[general_backlight] == 1)
        rb->backlight_set_timeout(rb->global_settings->backlight_timeout);
    else if(settings.general[general_backlight] == 2)
        rb->backlight_set_timeout(1);

    for(i=0; i<ANALOG_VALUES; i++)
    {
        xminute[i] = ((sin_int(360 * i / ANALOG_VALUES)
                        * ANALOG_MIN_RADIUS) >> 14) + ANALOG_XCENTER;
        yminute[i] = ((sin_int(360*i/ ANALOG_VALUES-90)
                        * ANALOG_MIN_RADIUS) >> 14) + ANALOG_YCENTER;
        xhour[i] = ((sin_int(360 * i / ANALOG_VALUES)
                        * ANALOG_HR_RADIUS) >> 14) + ANALOG_XCENTER;
        yhour[i] = ((sin_int(360 * i / ANALOG_VALUES-90)
                        * ANALOG_HR_RADIUS) >> 14) + ANALOG_YCENTER;

        /* Fullscreen initialization */
        if(i==0)
        {
            xminute_full[i] = LCD_WIDTH/2;
            xhour_full[i] = LCD_WIDTH/2;
            yminute_full[i] = 1;
            yhour_full[i] = LCD_HEIGHT/6;
        }
        else if(i<10 || (i>50 && i <60) )
        {
            xminute_full[i] = xminute_full[i-1]+LCD_WIDTH/20;
            xhour_full[i] = xhour_full[i-1]+(LCD_WIDTH/30);
            yminute_full[i] = 1;
            yhour_full[i] = LCD_HEIGHT/6;
        }

        else if (i>=10 && i < 20)
        {
            xminute_full[i] = LCD_WIDTH-2;
            xhour_full[i] = (5*LCD_WIDTH)/6;
            yminute_full[i] = ((i-10)*LCD_HEIGHT)/10;
            yhour_full[i] = (LCD_HEIGHT/6)+((i-10)*(LCD_HEIGHT))/15;
        }

        else if(i>=20&&i<40)
        {
            xminute_full[i] = ((40-i)*LCD_WIDTH)/20;
            xhour_full[i] = (LCD_WIDTH/6)+((40-i)*(LCD_WIDTH))/30;
            yminute_full[i] = LCD_HEIGHT-2;
            yhour_full[i] = (5*LCD_HEIGHT)/6;

        }
        else
        {
            xminute_full[i] = 1;
            xhour_full[i] = LCD_WIDTH/6;
            yminute_full[i] = ((50-i)*LCD_HEIGHT)/10;
            yhour_full[i] = LCD_HEIGHT/6 + ((50-i)*LCD_HEIGHT)/15;
        }
    }
}

/*******************
 * Analog clock mode
 ******************/
void analog_clock(int hour, int minute, int second)
{
    if(hour >= 12)
        hour -= 12;

    int i;
    int hourpos = (hour*5) + (minute/12);

    /* Crappy fake antialiasing (color LCDs only)!
     * how this works is we draw a large mid-gray hr/min/sec hand,
     * then the actual (slightly smaller) hand on top of those.
     * End result: mid-gray edges to the black hands, smooths them out. */
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_RGBPACK(100,110,125));

    /* second hand */
    if(settings.analog[analog_secondhand])
    {
        xlcd_filltriangle(LCD_WIDTH/2, LCD_HEIGHT/2-2, LCD_WIDTH/2, LCD_HEIGHT/2+2,
                          xminute[second], yminute[second]);
        xlcd_filltriangle(LCD_WIDTH/2-2, LCD_HEIGHT/2, LCD_WIDTH/2+2, LCD_HEIGHT/2,
                          xminute[second], yminute[second]);
    }

    /* minute hand */
    xlcd_filltriangle(LCD_WIDTH/2, LCD_HEIGHT/2-4, LCD_WIDTH/2, LCD_HEIGHT/2+4,
                      xminute[minute], yminute[minute]);
    xlcd_filltriangle(LCD_WIDTH/2-4, LCD_HEIGHT/2, LCD_WIDTH/2+4, LCD_HEIGHT/2,
                      xminute[minute], yminute[minute]);

    /* hour hand */
    xlcd_filltriangle(LCD_WIDTH/2, LCD_HEIGHT/2-4, LCD_WIDTH/2, LCD_HEIGHT/2+4,
                      xhour[hourpos], yhour[hourpos]);
    xlcd_filltriangle(LCD_WIDTH/2-4, LCD_HEIGHT/2, LCD_WIDTH/2+4, LCD_HEIGHT/2,
                      xhour[hourpos], yhour[hourpos]);

    rb->lcd_set_foreground(LCD_BLACK);
#endif

    /* second hand, if needed */
    if(settings.analog[analog_secondhand])
    {
        xlcd_filltriangle(LCD_WIDTH/2, LCD_HEIGHT/2-1, LCD_WIDTH/2, LCD_HEIGHT/2+1,
                          xminute[second], yminute[second]);
        xlcd_filltriangle(LCD_WIDTH/2-1, LCD_HEIGHT/2, LCD_WIDTH/2+1, LCD_HEIGHT/2,
                          xminute[second], yminute[second]);
    }

    /* minute hand */
    xlcd_filltriangle(LCD_WIDTH/2, LCD_HEIGHT/2-HAND_W, LCD_WIDTH/2,
                      LCD_HEIGHT/2+HAND_W, xminute[minute], yminute[minute]);
    xlcd_filltriangle(LCD_WIDTH/2-HAND_W, LCD_HEIGHT/2, LCD_WIDTH/2
                      +HAND_W, LCD_HEIGHT/2, xminute[minute], yminute[minute]);

    /* hour hand */
    xlcd_filltriangle(LCD_WIDTH/2, LCD_HEIGHT/2-HAND_W, LCD_WIDTH/2,
                      LCD_HEIGHT/2+HAND_W, xhour[hourpos], yhour[hourpos]);
    xlcd_filltriangle(LCD_WIDTH/2-HAND_W, LCD_HEIGHT/2, LCD_WIDTH/2
                      +HAND_W, LCD_HEIGHT/2, xhour[hourpos], yhour[hourpos]);

    /* Draw the circle */
    for(i=0; i < 60; i+=5)
        rb->lcd_fillrect(xminute[i]-1, yminute[i]-1, 3, 3);

    /* Draw the cover over the center */
    rb->lcd_drawline((LCD_WIDTH/2)-1, (LCD_HEIGHT/2)+3,
                     (LCD_WIDTH/2)+1, (LCD_HEIGHT/2)+3);
    rb->lcd_drawline((LCD_WIDTH/2)-3, (LCD_HEIGHT/2)+2,
                     (LCD_WIDTH/2)+3, (LCD_HEIGHT/2)+2);
    rb->lcd_drawline((LCD_WIDTH/2)-4, (LCD_HEIGHT/2)+1,
                     (LCD_WIDTH/2)+4, (LCD_HEIGHT/2)+1);
    rb->lcd_drawline((LCD_WIDTH/2)-4, LCD_HEIGHT/2,
                     (LCD_WIDTH/2)+4, LCD_HEIGHT/2);
    rb->lcd_drawline((LCD_WIDTH/2)-4, (LCD_HEIGHT/2)-1,
                     (LCD_WIDTH/2)+4, (LCD_HEIGHT/2)-1);
    rb->lcd_drawline((LCD_WIDTH/2)-3, (LCD_HEIGHT/2)-2,
                     (LCD_WIDTH/2)+3, (LCD_HEIGHT/2)-2);
    rb->lcd_drawline((LCD_WIDTH/2)-1, (LCD_HEIGHT/2)-3,
                     (LCD_WIDTH/2)+1, (LCD_HEIGHT/2)-3);
}

/********************
 * Digital clock mode
 *******************/
void digital_clock(int hour, int minute, int second, bool colon)
{
    int x_ofs=0;

    /* this basically detects if we draw an AM or PM bitmap.
     * if we don't, we center the hh:mm display. */
    if(!settings.digital[digital_format])
        x_ofs=DIGIT_WIDTH/2;

#if LCD_DEPTH == 1
    rb->lcd_fillrect(0,0,112,64);
#endif

    if(settings.digital[digital_format])
    {
        /* draw the AM or PM bitmap */
        if(hour<12)
            draw_segment(SEGMENT_AM,DIGIT_XOFS(6)+DIGIT_WIDTH*5, 0);
        else
            draw_segment(SEGMENT_PM,DIGIT_XOFS(6)+DIGIT_WIDTH*5, 0);

        /* and then readjust the hour to 12-hour format
         * ( 13:00+ -> 1:00+ ) */
        if(hour>12)
        hour -= 12;
    }

    /* hour */
    draw_segment(hour/10,   DIGIT_XOFS(6)+x_ofs,               0);
    draw_segment(hour%10,   DIGIT_XOFS(6)+DIGIT_WIDTH+x_ofs,   0);

    /* colon */
    if(colon)
        draw_segment(COLON, DIGIT_XOFS(6)+2*DIGIT_WIDTH+x_ofs, 0);

    /* minutes */
    draw_segment(minute/10, DIGIT_XOFS(6)+3*DIGIT_WIDTH+x_ofs, 0);
    draw_segment(minute%10, DIGIT_XOFS(6)+4*DIGIT_WIDTH+x_ofs, 0);

    if(settings.digital[digital_seconds])
    {
        draw_segment(second/10, DIGIT_XOFS(2),             DIGIT_HEIGHT);
        draw_segment(second%10, DIGIT_XOFS(2)+DIGIT_WIDTH, DIGIT_HEIGHT);
    }
}

/***********************
 * Fullscreen clock mode
 **********************/
void fullscreen_clock(int hour, int minute, int second)
{
    if(hour >= 12)
        hour -= 12;

    int i;
    int hourpos = (hour*5) + (minute/12);

    /* Crappy fake antialiasing (color LCDs only)!
     * how this works is we draw a large mid-gray hr/min/sec hand,
     * then the actual (slightly smaller) hand on top of those.
     * End result: mid-gray edges to the black hands, smooths them out. */
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_foreground(LCD_RGBPACK(100,110,125));

    /* second hand */
    if(settings.analog[analog_secondhand])
    {
        xlcd_filltriangle(LCD_WIDTH/2, LCD_HEIGHT/2-2, LCD_WIDTH/2, LCD_HEIGHT/2+2,
                          xminute_full[second], yminute_full[second]);
        xlcd_filltriangle(LCD_WIDTH/2-2, LCD_HEIGHT/2, LCD_WIDTH/2+2, LCD_HEIGHT/2,
                          xminute_full[second], yminute_full[second]);
    }

    /* minute hand */
    xlcd_filltriangle(LCD_WIDTH/2, LCD_HEIGHT/2-4, LCD_WIDTH/2, LCD_HEIGHT/2+4,
                      xminute_full[minute], yminute_full[minute]);
    xlcd_filltriangle(LCD_WIDTH/2-4, LCD_HEIGHT/2, LCD_WIDTH/2+4, LCD_HEIGHT/2,
                      xminute_full[minute], yminute_full[minute]);

    /* hour hand */
    xlcd_filltriangle(LCD_WIDTH/2, LCD_HEIGHT/2-4, LCD_WIDTH/2, LCD_HEIGHT/2+4,
                      xhour_full[hourpos], yhour_full[hourpos]);
    xlcd_filltriangle(LCD_WIDTH/2-4, LCD_HEIGHT/2, LCD_WIDTH/2+4, LCD_HEIGHT/2,
                      xhour_full[hourpos], yhour_full[hourpos]);

    rb->lcd_set_foreground(LCD_BLACK);
#endif

    /* second hand, if needed */
    if(settings.analog[analog_secondhand])
    {
        xlcd_filltriangle(LCD_WIDTH/2, LCD_HEIGHT/2-1, LCD_WIDTH/2, LCD_HEIGHT/2+1,
                          xminute_full[second], yminute_full[second]);
        xlcd_filltriangle(LCD_WIDTH/2-1, LCD_HEIGHT/2, LCD_WIDTH/2+1, LCD_HEIGHT/2,
                          xminute_full[second], yminute_full[second]);
    }

    /* minute hand */
    xlcd_filltriangle(LCD_WIDTH/2, LCD_HEIGHT/2-3, LCD_WIDTH/2, LCD_HEIGHT/2+3,
                      xminute_full[minute], yminute_full[minute]);
    xlcd_filltriangle(LCD_WIDTH/2-3, LCD_HEIGHT/2, LCD_WIDTH/2+3, LCD_HEIGHT/2,
                      xminute_full[minute], yminute_full[minute]);

    /* hour hand */
    xlcd_filltriangle(LCD_WIDTH/2, LCD_HEIGHT/2-3, LCD_WIDTH/2, LCD_HEIGHT/2+3,
                      xhour_full[hourpos], yhour_full[hourpos]);
    xlcd_filltriangle(LCD_WIDTH/2-3, LCD_HEIGHT/2, LCD_WIDTH/2+3, LCD_HEIGHT/2,
                      xhour_full[hourpos], yhour_full[hourpos]);

    /* Draw the circle */
    for(i=0; i < 60; i+=5)
        rb->lcd_fillrect(xminute_full[i]-1, yminute_full[i]-1, 3, 3);

    /* Draw the cover over the center */
    rb->lcd_drawline((LCD_WIDTH/2)-1, (LCD_HEIGHT/2)+3,
                     (LCD_WIDTH/2)+1, (LCD_HEIGHT/2)+3);
    rb->lcd_drawline((LCD_WIDTH/2)-3, (LCD_HEIGHT/2)+2,
                     (LCD_WIDTH/2)+3, (LCD_HEIGHT/2)+2);
    rb->lcd_drawline((LCD_WIDTH/2)-4, (LCD_HEIGHT/2)+1,
                     (LCD_WIDTH/2)+4, (LCD_HEIGHT/2)+1);
    rb->lcd_drawline((LCD_WIDTH/2)-4, LCD_HEIGHT/2,
                     (LCD_WIDTH/2)+4, LCD_HEIGHT/2);
    rb->lcd_drawline((LCD_WIDTH/2)-4, (LCD_HEIGHT/2)-1,
                     (LCD_WIDTH/2)+4, (LCD_HEIGHT/2)-1);
    rb->lcd_drawline((LCD_WIDTH/2)-3, (LCD_HEIGHT/2)-2,
                     (LCD_WIDTH/2)+3, (LCD_HEIGHT/2)-2);
    rb->lcd_drawline((LCD_WIDTH/2)-1, (LCD_HEIGHT/2)-3,
                     (LCD_WIDTH/2)+1, (LCD_HEIGHT/2)-3);
}

/*******************
 * Binary clock mode
 ******************/
void binary_clock(int hour, int minute, int second)
{
    int i, xpos=0;
    int mode_var[3]; /* pointers to h, m, s arguments */
    int mode; /* 0 = hour, 1 = minute, 2 = second */

    mode_var[0] = hour;
    mode_var[1] = minute;
    mode_var[2] = second;

    for(mode = 0; mode < 3; mode++)
    {
        for(i = 32; i > 0; i /= 2)
        {
            if(mode_var[mode] >= i)
            {
                if(settings.binary[binary_mode])
                    draw_digit(DOT_FILLED, xpos*DIGIT_WIDTH+DIGIT_XOFS(6), DIGIT_HEIGHT*mode+DIGIT_YOFS(3));
                else
                    draw_digit(1, xpos*DIGIT_WIDTH+DIGIT_XOFS(6), DIGIT_HEIGHT*mode+DIGIT_YOFS(3));
                mode_var[mode] -= i;
            }
            else
            {
                if(settings.binary[binary_mode])
                    draw_digit(DOT_EMPTY, xpos*DIGIT_WIDTH+DIGIT_XOFS(6), DIGIT_HEIGHT*mode+DIGIT_YOFS(3));
                else
                    draw_digit(0, xpos*DIGIT_WIDTH+DIGIT_XOFS(6), DIGIT_HEIGHT*mode+DIGIT_YOFS(3));
            }

            xpos++;
        }

        xpos=0; /* reset the x-pos for next mode */
    }
}

/******************
 * Plain clock mode
 *****************/
void plain_clock(int hour, int minute, int second, bool colon)
{

    int x_ofs=0;

    /* this basically detects if we draw an AM or PM bitmap.
     * if we don't, we center the hh:mm display. */
    if(!settings.plain[plain_format])
        x_ofs=DIGIT_WIDTH/2;

    if(settings.plain[plain_format])
    {
        /* draw the AM or PM bitmap */
        if(hour<12)
            draw_digit(ICON_AM, DIGIT_XOFS(6)+(DIGIT_WIDTH*5)+x_ofs, 0);
        else
            draw_digit(ICON_PM, DIGIT_XOFS(6)+(DIGIT_WIDTH*5)+x_ofs, 0);

        /* and then readjust the hour to 12-hour format
         * ( 13:00+ -> 1:00+ ) */
        if(hour>12)
        hour -= 12;
    }


    draw_digit(hour/10,   DIGIT_XOFS(6)+(DIGIT_WIDTH*0)+x_ofs, 0);
    draw_digit(hour%10,   DIGIT_XOFS(6)+(DIGIT_WIDTH*1)+x_ofs, 0);

    if(colon)
        draw_digit(COLON, DIGIT_XOFS(6)+(DIGIT_WIDTH*2)+x_ofs, 0);

    draw_digit(minute/10, DIGIT_XOFS(6)+(DIGIT_WIDTH*3)+x_ofs, 0);
    draw_digit(minute%10, DIGIT_XOFS(6)+(DIGIT_WIDTH*4)+x_ofs, 0);

    if(settings.plain[plain_seconds])
    {
        draw_digit(second/10, DIGIT_XOFS(2),               DIGIT_HEIGHT);
        draw_digit(second%10, DIGIT_XOFS(2)+(DIGIT_WIDTH), DIGIT_HEIGHT);
    }
}

/**********************
 * Analog settings menu
 *********************/
void analog_settings_menu(void)
{
    int m=0, result;

    done = false;

#ifdef HAVE_LCD_COLOR
    rb->lcd_set_background(LCD_RGBPACK(180,200,230));
    rb->lcd_set_foreground(LCD_BLACK);
#endif

    m = menu_init(rb, analog_items, sizeof(analog_items) / sizeof(*analog_items),
                      NULL, NULL, NULL, NULL);

    while(!done)
    {
        result = menu_show(m);

        switch(result)
        {
            case 0:
                rb->set_option("Show Date", &settings.analog[analog_date],
                               INT, date_format_text, 3, NULL);
                break;
            case 1:
                rb->set_option("Show Second Hand", &settings.analog[analog_secondhand],
                               INT, noyes_text, 2, NULL);
                break;
            case 2:
                rb->set_option("Show Time", &settings.analog[analog_time],
                               INT, analog_time_text, 3, NULL);
                break;

            default:
                done = true;
                break;
        }

        menu_exit(m);
    }
}

/***********************
 * Digital settings menu
 **********************/
void digital_settings_menu(void)
{
    int m=0, result;

    done = false;

#ifdef HAVE_LCD_COLOR
    rb->lcd_set_background(LCD_RGBPACK(180,200,230));
    rb->lcd_set_foreground(LCD_BLACK);
#endif

    m = menu_init(rb, digital_items, sizeof(digital_items) / sizeof(*digital_items),
                      NULL, NULL, NULL, NULL);

    while(!done)
    {
        result = menu_show(m);

        switch(result)
        {
            case 0:
                rb->set_option("Show Seconds", &settings.digital[digital_seconds],
                               INT, noyes_text, 2, NULL);
                break;
            case 1:
                rb->set_option("Show Date", &settings.digital[digital_date],
                               INT, date_format_text, 3, NULL);
                break;
            case 2:
                rb->set_option("Blinking Colon", &settings.digital[digital_blinkcolon],
                               INT, noyes_text, 2, NULL);
                break;
            case 3:
                rb->set_option("Time Format", &settings.digital[digital_format],
                               INT, time_format_text, 2, NULL);
                break;

            default:
                done = true;
                break;
        }

        menu_exit(m);
    }
}

/**************************
 * Fullscreen settings menu
 *************************/
void fullscreen_settings_menu(void)
{
    int m=0, result;

    done = false;

#ifdef HAVE_LCD_COLOR
    rb->lcd_set_background(LCD_RGBPACK(180,200,230));
    rb->lcd_set_foreground(LCD_BLACK);
#endif

    m = menu_init(rb, fullscreen_items, sizeof(fullscreen_items) / sizeof(*fullscreen_items),
                      NULL, NULL, NULL, NULL);

    while(!done)
    {
        result = menu_show(m);

        switch(result)
        {
            case 0:
                rb->set_option("Show Border", &settings.fullscreen[fullscreen_border],
                               INT, noyes_text, 2, NULL);
                break;
            case 1:
                rb->set_option("Show Second Hand", &settings.fullscreen[fullscreen_secondhand],
                               INT, noyes_text, 2, NULL);
                break;

            default:
                done = true;
                break;
        }

        menu_exit(m);
    }
}

/**********************
 * Binary settings menu
 *********************/
void binary_settings_menu(void)
{
    int m=0, result;

    done = false;

#ifdef HAVE_LCD_COLOR
    rb->lcd_set_background(LCD_RGBPACK(180,200,230));
    rb->lcd_set_foreground(LCD_BLACK);
#endif

    m = menu_init(rb,binary_items, sizeof(binary_items) / sizeof(*binary_items),
                      NULL, NULL, NULL, NULL);

    while(!done)
    {
        result = menu_show(m);

        switch(result)
        {
            case 0:
                rb->set_option("Display Mode", &settings.binary[binary_mode],
                               INT, binary_mode_text, 2, NULL);
                break;

            default:
                done = true;
                break;
        }

        menu_exit(m);
    }
}

/*********************
 * Plain settings menu
 ********************/
void plain_settings_menu(void)
{
    int m=0, result;

    done = false;

#ifdef HAVE_LCD_COLOR
    rb->lcd_set_background(LCD_RGBPACK(180,200,230));
    rb->lcd_set_foreground(LCD_BLACK);
#endif

    m = menu_init(rb,plain_items, sizeof(plain_items) / sizeof(*plain_items),
                      NULL, NULL, NULL, NULL);

    while(!done)
    {
        result = menu_show(m);

        switch(result)
        {
            case 0:
                rb->set_option("Show Date", &settings.plain[plain_date],
                               INT, date_format_text, 3, NULL);
                break;
            case 1:
                rb->set_option("Show Seconds", &settings.plain[plain_seconds],
                               INT, noyes_text, 2, NULL);
                break;
            case 2:
                rb->set_option("Blinking Colon", &settings.plain[plain_blinkcolon],
                               INT, noyes_text, 2, NULL);
                break;
            case 3:
                rb->set_option("Time Format", &settings.plain[plain_format],
                               INT, time_format_text, 2, NULL);
                break;

            default:
                done = true;
                break;
        }

        menu_exit(m);
    }
}

/**************************************
 * Settings screen for the current mode
 *************************************/
void settings_screen(void)
{
    set_standard_colors();

    if(settings.clock == ANALOG)
        analog_settings_menu();
    else if(settings.clock == DIGITAL)
        digital_settings_menu();
    else if(settings.clock == FULLSCREEN)
        fullscreen_settings_menu();
    else if(settings.clock == BINARY)
        binary_settings_menu();
    else if(settings.clock == PLAIN)
        plain_settings_menu();

    set_digital_colors();

}

/***********************************************************
 * Confirm resetting of settings, used in general_settings()
 **********************************************************/
void confirm_reset(void)
{
    int result=0;

    rb->set_option("Reset all settings?", &result, INT, noyes_text, 2, NULL);

    if(result == 1) /* reset! */
    {
        reset_settings();
        rb->splash(HZ, "Settings reset!");
    }
    else
        rb->splash(HZ, "Settings NOT reset.");
}

/************************************
 * General settings. Reset, save, etc
 ***********************************/
void general_settings(void)
{
    int m, result;
    done = false;

    set_standard_colors();

    m = menu_init(rb,general_settings_items, sizeof(general_settings_items) / sizeof(*general_settings_items),
                      NULL, NULL, NULL, NULL);

    while(!done)
    {
        result = menu_show(m);

        switch(result)
        {
            case 0:
                confirm_reset();
                break;

            case 1:
                save_settings(false);
                rb->splash(HZ, "Settings saved");
                break;

            case 2:
                rb->set_option("Save On Exit", &settings.general[general_savesetting],
                               INT, noyes_text, 2, NULL);

                /* if we no longer save on exit, we better save now to remember that */
                if(settings.general[general_savesetting] == 0)
                    save_settings(false);
                break;

            case 3:
                rb->set_option("Show Counter", &settings.general[general_counter],
                               INT, noyes_text, 2, NULL);
                break;

            case 4:
                rb->set_option("Backlight Settings", &settings.general[general_backlight],
                               INT, backlight_settings_text, 3, NULL);
                break;

            case 5:
                rb->set_option("Idle Poweroff (temporary)", &idle_poweroff,
                               BOOL, idle_poweroff_text, 2, NULL);
                break;

            default:
                done=true;
                break;
        }

        menu_exit(m);
    }

    set_digital_colors();
}

/****************************************
 * Draws the extras, IE border, digits...
 ***************************************/
void draw_extras(int year, int day, int month, int hour, int minute, int second)
{
    int i;

    struct tm* current_time = rb->get_time();

    char moday[8];
    char dateyr[6];
    char tmhrmin[7];
    char tmsec[3];

    /* american date readout */
    if(settings.analog[analog_date] == 1)
        rb->snprintf(moday, sizeof(moday), "%02d/%02d", month, day);
    else
        rb->snprintf(moday, sizeof(moday), "%02d.%02d", day, month);
    rb->snprintf(dateyr, sizeof(dateyr), "%d", year);
    rb->snprintf(tmhrmin, sizeof(tmhrmin), "%02d:%02d", hour, minute);
    rb->snprintf(tmsec, sizeof(tmsec), "%02d", second);

    /* Analog Extras */
    if(settings.clock == ANALOG)
    {
        if(settings.analog[analog_time] != 0) /* Digital readout */
        {
            draw_smalldigit(hour/10,   SMALLDIGIT_WIDTH*0, 0);
            draw_smalldigit(hour%10,   SMALLDIGIT_WIDTH*1, 0);
            draw_smalldigit(COLON,     SMALLDIGIT_WIDTH*2, 0);
            draw_smalldigit(minute/10, SMALLDIGIT_WIDTH*3, 0);
            draw_smalldigit(minute%10, SMALLDIGIT_WIDTH*4, 0);

            draw_smalldigit(second/10, SMALLDIGIT_WIDTH*1.5, SMALLDIGIT_HEIGHT);
            draw_smalldigit(second%10, SMALLDIGIT_WIDTH*2.5, SMALLDIGIT_HEIGHT);

            /* AM/PM indicator */
            if(settings.analog[analog_time] == 2)
            {
                if(current_time->tm_hour > 12) /* PM */
                    draw_digit(ICON_PM, LCD_WIDTH-DIGIT_WIDTH, DIGIT_HEIGHT/2-DIGIT_HEIGHT);
                else /* AM */
                    draw_digit(ICON_AM, LCD_WIDTH-DIGIT_WIDTH, DIGIT_HEIGHT/2-DIGIT_HEIGHT);
            }
        }
        if(settings.analog[analog_date] != 0) /* Date readout */
        {
            if(settings.analog[analog_date] == 1)
            {
                draw_smalldigit(month/10,      SMALLDIGIT_WIDTH*0,
                                LCD_HEIGHT-SMALLDIGIT_HEIGHT*2);
                draw_smalldigit(month%10,      SMALLDIGIT_WIDTH*1,
                                LCD_HEIGHT-SMALLDIGIT_HEIGHT*2);
                draw_smalldigit(SLASH,         SMALLDIGIT_WIDTH*2,
                                LCD_HEIGHT-SMALLDIGIT_HEIGHT*2);
                draw_smalldigit(day/10,        SMALLDIGIT_WIDTH*3,
                                LCD_HEIGHT-SMALLDIGIT_HEIGHT*2);
                draw_smalldigit(day%10,        SMALLDIGIT_WIDTH*4,
                                LCD_HEIGHT-SMALLDIGIT_HEIGHT*2);
                draw_smalldigit(year/1000,     SMALLDIGIT_WIDTH*0.5,
                                LCD_HEIGHT-SMALLDIGIT_HEIGHT);
                draw_smalldigit(year%1000/100, SMALLDIGIT_WIDTH*1.5,
                                LCD_HEIGHT-SMALLDIGIT_HEIGHT);
                draw_smalldigit(year%100/10,   SMALLDIGIT_WIDTH*2.5,
                                LCD_HEIGHT-SMALLDIGIT_HEIGHT);
                draw_smalldigit(year%10,       SMALLDIGIT_WIDTH*3.5,
                                LCD_HEIGHT-SMALLDIGIT_HEIGHT);
            }
            else if(settings.analog[analog_date] == 2)
            {

                draw_smalldigit(day/10,        SMALLDIGIT_WIDTH*0,
                                LCD_HEIGHT-SMALLDIGIT_HEIGHT*2);
                draw_smalldigit(day%10,        SMALLDIGIT_WIDTH*1,
                                LCD_HEIGHT-SMALLDIGIT_HEIGHT*2);
                draw_smalldigit(PERIOD,        SMALLDIGIT_WIDTH*2,
                                LCD_HEIGHT-SMALLDIGIT_HEIGHT*2);
                draw_smalldigit(month/10,      SMALLDIGIT_WIDTH*3,
                                LCD_HEIGHT-SMALLDIGIT_HEIGHT*2);
                draw_smalldigit(month%10,      SMALLDIGIT_WIDTH*4,
                                LCD_HEIGHT-SMALLDIGIT_HEIGHT*2);
                draw_smalldigit(year/1000,     SMALLDIGIT_WIDTH*0.5,
                                LCD_HEIGHT-SMALLDIGIT_HEIGHT);
                draw_smalldigit(year%1000/100, SMALLDIGIT_WIDTH*1.5,
                                LCD_HEIGHT-SMALLDIGIT_HEIGHT);
                draw_smalldigit(year%100/10,   SMALLDIGIT_WIDTH*2.5,
                                LCD_HEIGHT-SMALLDIGIT_HEIGHT);
                draw_smalldigit(year%10,       SMALLDIGIT_WIDTH*3.5,
                                LCD_HEIGHT-SMALLDIGIT_HEIGHT);
            }
        }
    }
    else if(settings.clock == DIGITAL)
    {
        /* Date readout */
        if(settings.digital[digital_date] == 1) /* american mode */
        {
            draw_smallsegment(month/10, SMALLSEG_WIDTH*0+SMALLSEG_XOFS(10),
                              LCD_HEIGHT-SMALLSEG_HEIGHT*LCD_OFFSET*2);
            draw_smallsegment(month%10, SMALLSEG_WIDTH*1+SMALLSEG_XOFS(10),
                              LCD_HEIGHT-SMALLSEG_HEIGHT*LCD_OFFSET*2);
            draw_smallsegment(SLASH, SMALLSEG_WIDTH*2+SMALLSEG_XOFS(10),
                              LCD_HEIGHT-SMALLSEG_HEIGHT*LCD_OFFSET*2);
            draw_smallsegment(day/10, SMALLSEG_WIDTH*3+SMALLSEG_XOFS(10),
                              LCD_HEIGHT-SMALLSEG_HEIGHT*LCD_OFFSET*2);
            draw_smallsegment(day%10, SMALLSEG_WIDTH*4+SMALLSEG_XOFS(10),
                              LCD_HEIGHT-SMALLSEG_HEIGHT*LCD_OFFSET*2);
            draw_smallsegment(SLASH, SMALLSEG_WIDTH*5+SMALLSEG_XOFS(10),
                              LCD_HEIGHT-SMALLSEG_HEIGHT*LCD_OFFSET*2);
            draw_smallsegment(year/1000, SMALLSEG_WIDTH*6+SMALLSEG_XOFS(10),
                              LCD_HEIGHT-SMALLSEG_HEIGHT*LCD_OFFSET*2);
            draw_smallsegment(year%1000/100, SMALLSEG_WIDTH*7+SMALLSEG_XOFS(10),
                              LCD_HEIGHT-SMALLSEG_HEIGHT*LCD_OFFSET*2);
            draw_smallsegment(year%100/10, SMALLSEG_WIDTH*8+SMALLSEG_XOFS(10),
                              LCD_HEIGHT-SMALLSEG_HEIGHT*LCD_OFFSET*2);
            draw_smallsegment(year%10, SMALLSEG_WIDTH*9+SMALLSEG_XOFS(10),
                              LCD_HEIGHT-SMALLSEG_HEIGHT*LCD_OFFSET*2);
        }
        else if(settings.digital[digital_date] == 2) /* european mode */
        {
            draw_smallsegment(day/10, SMALLSEG_WIDTH*0+SMALLSEG_XOFS(10),
                              LCD_HEIGHT-SMALLSEG_HEIGHT*LCD_OFFSET*2);
            draw_smallsegment(day%10, SMALLSEG_WIDTH*1+SMALLSEG_XOFS(10),
                              LCD_HEIGHT-SMALLSEG_HEIGHT*LCD_OFFSET*2);
            draw_smallsegment(PERIOD, SMALLSEG_WIDTH*2+SMALLSEG_XOFS(10),
                              LCD_HEIGHT-SMALLSEG_HEIGHT*LCD_OFFSET*2);
            draw_smallsegment(month/10, SMALLSEG_WIDTH*3+SMALLSEG_XOFS(10),
                              LCD_HEIGHT-SMALLSEG_HEIGHT*LCD_OFFSET*2);
            draw_smallsegment(month%10, SMALLSEG_WIDTH*4+SMALLSEG_XOFS(10),
                              LCD_HEIGHT-SMALLSEG_HEIGHT*LCD_OFFSET*2);
            draw_smallsegment(PERIOD, SMALLSEG_WIDTH*5+SMALLSEG_XOFS(10),
                              LCD_HEIGHT-SMALLSEG_HEIGHT*LCD_OFFSET*2);
            draw_smallsegment(year/1000, SMALLSEG_WIDTH*6+SMALLSEG_XOFS(10),
                              LCD_HEIGHT-SMALLSEG_HEIGHT*LCD_OFFSET*2);
            draw_smallsegment(year%1000/100, SMALLSEG_WIDTH*7+SMALLSEG_XOFS(10),
                              LCD_HEIGHT-SMALLSEG_HEIGHT*LCD_OFFSET*2);
            draw_smallsegment(year%100/10, SMALLSEG_WIDTH*8+SMALLSEG_XOFS(10),
                              LCD_HEIGHT-SMALLSEG_HEIGHT*LCD_OFFSET*2);
            draw_smallsegment(year%10, SMALLSEG_WIDTH*9+SMALLSEG_XOFS(10),
                              LCD_HEIGHT-SMALLSEG_HEIGHT*LCD_OFFSET*2);
        }
    }
    else if(settings.clock == FULLSCREEN) /* Fullscreen mode */
    {
        if(settings.fullscreen[fullscreen_border])
        {
            for(i=0; i < 60; i+=5) /* Draw the circle */
                rb->lcd_fillrect(xminute_full[i]-1, yminute_full[i]-1, 3, 3);
        }
    }
    else if(settings.clock == PLAIN) /* Plain mode */
    {
        /* Date readout */
        if(settings.plain[plain_date] == 1) /* american mode */
        {
            draw_smalldigit(month/10, SMALLDIGIT_WIDTH*0+SMALLDIGIT_XOFS(10),
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*LCD_OFFSET*2);
            draw_smalldigit(month%10, SMALLDIGIT_WIDTH*1+SMALLDIGIT_XOFS(10),
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*LCD_OFFSET*2);
            draw_smalldigit(SLASH, SMALLDIGIT_WIDTH*2+SMALLDIGIT_XOFS(10),
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*LCD_OFFSET*2);
            draw_smalldigit(day/10, SMALLDIGIT_WIDTH*3+SMALLDIGIT_XOFS(10),
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*LCD_OFFSET*2);
            draw_smalldigit(day%10, SMALLDIGIT_WIDTH*4+SMALLDIGIT_XOFS(10),
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*LCD_OFFSET*2);
            draw_smalldigit(SLASH, SMALLDIGIT_WIDTH*5+SMALLDIGIT_XOFS(10),
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*LCD_OFFSET*2);
            draw_smalldigit(year/1000, SMALLDIGIT_WIDTH*6+SMALLDIGIT_XOFS(10),
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*LCD_OFFSET*2);
            draw_smalldigit(year%1000/100, SMALLDIGIT_WIDTH*7+SMALLDIGIT_XOFS(10),
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*LCD_OFFSET*2);
            draw_smalldigit(year%100/10, SMALLDIGIT_WIDTH*8+SMALLDIGIT_XOFS(10),
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*LCD_OFFSET*2);
            draw_smalldigit(year%10, SMALLDIGIT_WIDTH*9+SMALLDIGIT_XOFS(10),
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*LCD_OFFSET*2);
        }
        else if(settings.plain[plain_date] == 2) /* european mode */
        {
            draw_smalldigit(day/10, SMALLDIGIT_WIDTH*0+SMALLDIGIT_XOFS(10),
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*LCD_OFFSET*2);
            draw_smalldigit(day%10, SMALLDIGIT_WIDTH*1+SMALLDIGIT_XOFS(10),
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*LCD_OFFSET*2);
            draw_smalldigit(PERIOD, SMALLDIGIT_WIDTH*2+SMALLDIGIT_XOFS(10),
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*LCD_OFFSET*2);
            draw_smalldigit(month/10, SMALLDIGIT_WIDTH*3+SMALLDIGIT_XOFS(10),
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*LCD_OFFSET*2);
            draw_smalldigit(month%10, SMALLDIGIT_WIDTH*4+SMALLDIGIT_XOFS(10),
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*LCD_OFFSET*2);
            draw_smalldigit(PERIOD, SMALLDIGIT_WIDTH*5+SMALLDIGIT_XOFS(10),
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*LCD_OFFSET*2);
            draw_smalldigit(year/1000, SMALLDIGIT_WIDTH*6+SMALLDIGIT_XOFS(10),
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*LCD_OFFSET*2);
            draw_smalldigit(year%1000/100, SMALLDIGIT_WIDTH*7+SMALLDIGIT_XOFS(10),
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*LCD_OFFSET*2);
            draw_smalldigit(year%100/10, SMALLDIGIT_WIDTH*8+SMALLDIGIT_XOFS(10),
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*LCD_OFFSET*2);
            draw_smalldigit(year%10, SMALLDIGIT_WIDTH*9+SMALLDIGIT_XOFS(10),
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*LCD_OFFSET*2);
        }
    }
}

/***************
 * Select a mode
 **************/
void mode_selector(void)
{
    int m, result;
    done = false;

    set_standard_colors();

    m = menu_init(rb,mode_selector_items, sizeof(mode_selector_items) / sizeof(*mode_selector_items),
                  NULL, NULL, NULL, NULL);

    while(!done)
    {
        result = menu_show(m);

        /* check for this, so if the user exits the menu without
         * making a selection, it won't change to some weird value. */
        if(result >= 0)
            settings.clock = result+1;

        done = true;

        menu_exit(m);
    }

    set_digital_colors();
}

/*********************
 * Display the counter
 ********************/
void show_counter(void)
{

    /* increment counter */
    if(counting)
        passed_time = *rb->current_tick - start_tick;
    else
        passed_time = 0;

    displayed_value = counter + passed_time;
    displayed_value = displayed_value / HZ;

    /* these are the REAL displayed values */
    count_s = displayed_value % 60;
    count_m = displayed_value % 3600 / 60;
    count_h = displayed_value / 3600;

    if(settings.general[general_counter])
    {
        if(settings.clock == ANALOG)
        {
            draw_smalldigit(count_h/10, LCD_WIDTH-SMALLDIGIT_WIDTH*5,
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*2);
            draw_smalldigit(count_h%10, LCD_WIDTH-SMALLDIGIT_WIDTH*4,
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*2);
            draw_smalldigit(COLON, LCD_WIDTH-SMALLDIGIT_WIDTH*3,
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*2);
            draw_smalldigit(count_m/10, LCD_WIDTH-SMALLDIGIT_WIDTH*2,
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*2);
            draw_smalldigit(count_m%10, LCD_WIDTH-SMALLDIGIT_WIDTH,
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*2);
            draw_smalldigit(count_s/10, LCD_WIDTH-SMALLDIGIT_WIDTH*3.5,
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT);
            draw_smalldigit(count_s%10, LCD_WIDTH-SMALLDIGIT_WIDTH*2.5,
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT);
        }
        else if(settings.clock == DIGITAL)
        {
            draw_smallsegment(count_h/10, SMALLSEG_WIDTH*0+SMALLSEG_XOFS(8),
                              LCD_HEIGHT-SMALLSEG_HEIGHT*LCD_OFFSET);
            draw_smallsegment(count_h%10, SMALLSEG_WIDTH*1+SMALLSEG_XOFS(8),
                              LCD_HEIGHT-SMALLSEG_HEIGHT*LCD_OFFSET);
            draw_smallsegment(COLON, SMALLSEG_WIDTH*2+SMALLSEG_XOFS(8),
                              LCD_HEIGHT-SMALLSEG_HEIGHT*LCD_OFFSET);
            draw_smallsegment(count_m/10, SMALLSEG_WIDTH*3+SMALLSEG_XOFS(8),
                              LCD_HEIGHT-SMALLSEG_HEIGHT*LCD_OFFSET);
            draw_smallsegment(count_m%10, SMALLSEG_WIDTH*4+SMALLSEG_XOFS(8),
                              LCD_HEIGHT-SMALLSEG_HEIGHT*LCD_OFFSET);
            draw_smallsegment(COLON, SMALLSEG_WIDTH*5+SMALLSEG_XOFS(8),
                              LCD_HEIGHT-SMALLSEG_HEIGHT*LCD_OFFSET);
            draw_smallsegment(count_s/10, SMALLSEG_WIDTH*6+SMALLSEG_XOFS(8),
                              LCD_HEIGHT-SMALLSEG_HEIGHT*LCD_OFFSET);
            draw_smallsegment(count_s%10, SMALLSEG_WIDTH*7+SMALLSEG_XOFS(8),
                              LCD_HEIGHT-SMALLSEG_HEIGHT*LCD_OFFSET);
        }
        else if(settings.clock == FULLSCREEN)
        {

            draw_smalldigit(count_h/10, SMALLDIGIT_WIDTH*0+SMALLDIGIT_XOFS(8),
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*1.5);
            draw_smalldigit(count_h%10, SMALLDIGIT_WIDTH*1+SMALLDIGIT_XOFS(8),
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*1.5);
            draw_smalldigit(COLON, SMALLDIGIT_WIDTH*2+SMALLDIGIT_XOFS(8),
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*1.5);
            draw_smalldigit(count_m/10, SMALLDIGIT_WIDTH*3+SMALLDIGIT_XOFS(8),
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*1.5);
            draw_smalldigit(count_m%10, SMALLDIGIT_WIDTH*4+SMALLDIGIT_XOFS(8),
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*1.5);
            draw_smalldigit(COLON, SMALLDIGIT_WIDTH*5+SMALLDIGIT_XOFS(8),
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*1.5);
            draw_smalldigit(count_s/10, SMALLDIGIT_WIDTH*6+SMALLDIGIT_XOFS(8),
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*1.5);
            draw_smalldigit(count_s%10, SMALLDIGIT_WIDTH*7+SMALLDIGIT_XOFS(8),
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*1.5);
        }
        else if(settings.clock == PLAIN)
        {
            draw_smalldigit(count_h/10, SMALLDIGIT_WIDTH*0+SMALLDIGIT_XOFS(8),
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*LCD_OFFSET);
            draw_smalldigit(count_h%10, SMALLDIGIT_WIDTH*1+SMALLDIGIT_XOFS(8),
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*LCD_OFFSET);
            draw_smalldigit(COLON, SMALLDIGIT_WIDTH*2+SMALLDIGIT_XOFS(8),
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*LCD_OFFSET);
            draw_smalldigit(count_m/10, SMALLDIGIT_WIDTH*3+SMALLDIGIT_XOFS(8),
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*LCD_OFFSET);
            draw_smalldigit(count_m%10, SMALLDIGIT_WIDTH*4+SMALLDIGIT_XOFS(8),
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*LCD_OFFSET);
            draw_smalldigit(COLON, SMALLDIGIT_WIDTH*5+SMALLDIGIT_XOFS(8),
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*LCD_OFFSET);
            draw_smalldigit(count_s/10, SMALLDIGIT_WIDTH*6+SMALLDIGIT_XOFS(8),
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*LCD_OFFSET);
            draw_smalldigit(count_s%10, SMALLDIGIT_WIDTH*7+SMALLDIGIT_XOFS(8),
                            LCD_HEIGHT-SMALLDIGIT_HEIGHT*LCD_OFFSET);
        }
    }
}

/***********
 * Main menu
 **********/
void main_menu(void)
{
    int m, result;
    done = false;

    set_standard_colors();

    m = menu_init(rb,main_menu_items, sizeof(main_menu_items) / sizeof(*main_menu_items),
                  NULL, NULL, NULL, NULL);

    while(!done)
    {
        result = menu_show(m);

        switch(result)
        {
            case 0:
                rb->lcd_setfont(FONT_SYSFIXED);
                done = true;
                break;

            case 1:
                mode_selector();
                break;

            case 2:
                settings_screen();
                break;

            case 3:
                general_settings();
                break;

            case 4:
                exit_clock = true;
                done = true;
                break;

            default:
                done=true;
                break;
        }

        menu_exit(m);
    }

    rb->lcd_setfont(FONT_SYSFIXED);
    set_digital_colors();
}

/**********************************************************************
 * Plugin starts here
 **********************************************************************/
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int button;

    /* time/date ints */
    int hour, minute, second;
    int temphour;
    int last_second = -1;
    int year, day, month;

    bool counter_btn_held = false;

    struct tm* current_time;

    (void)parameter;
    rb = api;

#if LCD_DEPTH > 1
    rb->lcd_set_backdrop(NULL);
#endif

    init_clock();

    /* init xlcd functions */
    xlcd_init(rb);

    set_digital_colors();

    while(!exit_clock)
    {
        /*********************
         * Time info
         *********************/
        current_time = rb->get_time();
        hour = current_time->tm_hour;
        minute = current_time->tm_min;
        second = current_time->tm_sec;
        temphour = current_time->tm_hour;

        /*********************
         * Date info
         *********************/
        year = current_time->tm_year + 1900;
        day = current_time->tm_mday;
        month = current_time->tm_mon + 1;

        done = false;

        if(second != last_second)
        {
            rb->lcd_clear_display();

            /* Analog mode */
            if(settings.clock == ANALOG)
                analog_clock(hour, minute, second);
            /* Digital mode */
            else if(settings.clock == DIGITAL)
            {
                if(settings.digital[digital_blinkcolon])
                    digital_clock(hour, minute, second, second & 1);
                else
                    digital_clock(hour, minute, second, true);
            }
            /* Fullscreen mode */
            else if(settings.clock == FULLSCREEN)
                fullscreen_clock(hour, minute, second);
            /* Binary mode */
            else if(settings.clock == BINARY)
                binary_clock(hour, minute, second);
            /* Plain mode */
            else if(settings.clock == PLAIN)
            {
                if(settings.plain[plain_blinkcolon])
                    plain_clock(hour, minute, second, second & 1);
                else
                    plain_clock(hour, minute, second, true);
            }

            /* show counter */
            show_counter();
        }

        if(settings.analog[analog_time] == 2 && temphour == 0)
            temphour = 12;
        if(settings.analog[analog_time] == 2 && temphour > 12)
            temphour -= 12;

        /* all the "extras" - readouts/displays */
        draw_extras(year, day, month, temphour, minute, second);

        if(!idle_poweroff)
            rb->reset_poweroff_timer();

        rb->lcd_update();

        /*************************
         * Scan for button presses
         ************************/
        button =  pluginlib_getaction(rb, HZ/10, plugin_contexts, 2);
        switch (button)
        {
            case ACTION_COUNTER_TOGGLE: /* start/stop counter */
                if(settings.general[general_counter])
                {
                    if(!counter_btn_held) /* Ignore if the counter was reset */
                    {
                        if(counting)
                        {
                            counting = false;
                            counter += passed_time;
                        }
                        else
                        {
                            counting = true;
                            start_tick = *rb->current_tick;
                        }
                    }
                    counter_btn_held = false;
                }
                break;

            case ACTION_COUNTER_RESET: /* reset counter */
                if(settings.general[general_counter])
                {
                    counter_btn_held = true;  /* Ignore the release event */
                    counter = 0;
                    start_tick = *rb->current_tick;
                }
                break;

            case ACTION_MODE_NEXT:
                if(settings.clock < CLOCK_MODES)
                    settings.clock++;
                else
                    settings.clock = 1;

                set_digital_colors();
                break;

            case ACTION_MODE_PREV:
                if(settings.clock > 1)
                    settings.clock--;
                else
                    settings.clock = CLOCK_MODES;

                set_digital_colors();
                break;

            case ACTION_MENU: /* main menu */
                main_menu();
                break;

            case ACTION_EXIT: /* main menu */
                exit_clock=true;
                break;

            default:
                if(rb->default_event_handler_ex(button, cleanup, NULL)
                   == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                break;
        }
    }

    cleanup(NULL);
    return PLUGIN_OK;
}
