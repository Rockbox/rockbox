/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: clock.c,v 1.0 2003/12/8
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
#include "plugin.h"
#include "time.h"

#ifdef HAVE_LCD_BITMAP

void draw_7seg_time(int hour, int minute, int x, int y, int width, int height,
bool colon);
void loadSettings(void);

static struct plugin_api* rb;

/* Used for hands to define lengths at a given time */
static unsigned char yhour[] = {
47,47,46,46,46,45,44,43,42,41,39,38,36,35,33,32,31,29,28,26,25,23,22,21,20,19,18,18,18,17,17,17,18,18,18,19,20,21,22,23,25,26,28,29,31,32,33,35,36,38,39,41,42,43,44,45,46,46,46,47,
};
static unsigned char yminute[] = {
55,54,54,53,53,51,50,49,47,45,43,41,39,36,34,32,30,28,25,23,21,19,17,15,14,13,11,11,10,10,9,10,10,11,11,13,14,15,17,19,21,23,25,28,30,32,34,36,39,41,43,45,47,49,50,51,53,53,54,54,
};
static unsigned char xhour[] = {
56,58,59,61,63,65,67,68,70,71,72,73,74,74,75,75,75,74,74,73,72,71,70,68,67,65,63,61,59,58,56,54,53,51,49,47,45,44,42,41,40,39,38,38,37,37,37,38,38,39,40,41,42,44,45,47,49,51,53,54,
};
static unsigned char xminute[] = {
56,59,61,64,67,70,72,75,77,79,80,82,83,84,84,84,84,84,83,82,80,79,77,75,72,70,67,64,61,59,56,53,51,48,45,42,40,37,35,33,32,30,29,28,28,28,28,28,29,30,32,33,35,37,40,42,45,48,51,53,
};

static char default_filename[] = "/.rockbox/rocks/.clock_settings";

struct saved_settings
{
	bool is_date_displayed;
	bool are_digits_displayed;
	bool is_time_displayed;
	bool is_rect_displayed;
	bool analog_clock;
} settings;


void save_settings(void)
{
    int fd;
    
    fd = rb->creat(default_filename, O_WRONLY);
    if(fd >= 0)
    {
        rb->write (fd, &settings, sizeof(struct saved_settings));
        rb->close(fd);
    }
    else
    {
	rb->splash(HZ, true, "Setting save failed");
    }
}

void load_settings(void)
{
    int fd;
    fd = rb->open(default_filename, O_RDONLY);

    /* if file exists, then load. */
    if(fd >= 0)
    {
        rb->read (fd, &settings, sizeof(struct saved_settings));
        rb->close(fd);
    }
    /* Else, loading failed */
    else
    {
	rb->splash(HZ, true, "Setting load failed, using default settings.");
    }
}

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    /* time ints */
    int i;
    int hour;
    int minute;
    int second;
    int last_second = -1;
    int pos = 0;

    /* date ints */
    int year;
    int day;
    int month;

    char moday[6];
    char dateyr[5];
    char tmhrmin[5];
    char tmsec[2];
    char current_date[12];

    struct tm* current_time;

    bool exit = false;
    bool used = false;

    TEST_PLUGIN_API(api);
    (void)parameter;
    rb = api;

    /* load settings from disk */
    load_settings();

    rb->lcd_clear_display();
    rb->splash(HZ, true, "F1 for INFO");

    while (!PLUGIN_OK)
    {
        /* universal font for better display */
        rb->lcd_setfont(FONT_SYSFIXED);

        /* start all the clock stuff */

        /* Time info */
        current_time = rb->get_time();
        hour = current_time->tm_hour;
        minute = current_time->tm_min;
        second = current_time->tm_sec;

        /* Date info */
        year = current_time->tm_year+1900;
        day = current_time->tm_mday;
        month = current_time->tm_mon+1;

        if(second != last_second)
        {
            rb->lcd_clear_display();

            rb->snprintf( moday, 5, "%d/%d", month, day );
            rb->snprintf( dateyr, 4, "%d", year );
            rb->snprintf( tmhrmin, 5, "%02d:%02d", hour, minute );
            rb->snprintf( tmsec, 2, "%02d", second );
            rb->snprintf( current_date, 12, "%d/%d/%d", month, day, year );

            if(settings.analog_clock)
            {
                /* Now draw the analog clock */
                pos = 90-second;
                if(pos >= 60)
                    pos -= 60;

                /* Second hand */
                rb->lcd_drawline((LCD_WIDTH/2), (LCD_HEIGHT/2),
                                 xminute[pos], yminute[pos]);

                pos = 90-minute;
                if(pos >= 60)
                    pos -= 60;

                /* Minute hand, thicker than the second hand */
                rb->lcd_drawline(LCD_WIDTH/2, LCD_HEIGHT/2,
                                 xminute[pos], yminute[pos]);
                rb->lcd_drawline(LCD_WIDTH/2-1, LCD_HEIGHT/2-1,
                                 xminute[pos], yminute[pos]);
                rb->lcd_drawline(LCD_WIDTH/2+1, LCD_HEIGHT/2+1,
                                 xminute[pos], yminute[pos]);
                rb->lcd_drawline(LCD_WIDTH/2-1, LCD_HEIGHT/2+1,
                                 xminute[pos], yminute[pos]);
                rb->lcd_drawline(LCD_WIDTH/2+1, LCD_HEIGHT/2-1,
                                 xminute[pos], yminute[pos]);

                if(hour > 12)
                    hour -= 12;

                hour = hour*5 + minute/12;;
                pos = 90-hour;
                if(pos >= 60)
                    pos -= 60;

                /* Hour hand, thick as the minute hand but shorter */
                rb->lcd_drawline(LCD_WIDTH/2, LCD_HEIGHT/2, xhour[pos], yhour[pos]);
                rb->lcd_drawline(LCD_WIDTH/2-1, LCD_HEIGHT/2-1,
                                 xhour[pos], yhour[pos]);
                rb->lcd_drawline(LCD_WIDTH/2+1, LCD_HEIGHT/2+1,
                                 xhour[pos], yhour[pos]);
                rb->lcd_drawline(LCD_WIDTH/2-1, LCD_HEIGHT/2+1,
                                 xhour[pos], yhour[pos]);
                rb->lcd_drawline(LCD_WIDTH/2+1, LCD_HEIGHT/2-1,
                                 xhour[pos], yhour[pos]);

                /* Draw the circle */
                for(i=0; i < 60; i+=5)
                {
                    rb->lcd_drawpixel(xminute[i],
                                      yminute[i]);
                    rb->lcd_drawrect(xminute[i]-1,
                                     yminute[i]-1,
                                     3, 3);
                }

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
                rb->lcd_drawpixel(LCD_WIDTH/2, LCD_HEIGHT/2);

                /* Draw the digits around the clock */
                if (settings.are_digits_displayed)
                {
                    rb->lcd_putsxy((LCD_WIDTH/2)-6, 1, "12");
                    rb->lcd_putsxy(20, (LCD_HEIGHT/2)-4, "9");
                    rb->lcd_putsxy((LCD_WIDTH/2)-4, 56, "6");
                    rb->lcd_putsxy(86, (LCD_HEIGHT/2)-4, "3");
                }

            	/* Draw digital readout */
            	if (settings.is_time_displayed)
                {
                    /* HH:MM */
                    rb->lcd_putsxy(1, 4, tmhrmin);
                    /* SS */
                    rb->lcd_putsxy(10, 12, tmsec);
                }
            }
            else
            {
                draw_7seg_time(hour, minute, 8, 16, 16, 32, second & 1);
            }

            /* Draw the border */
            if (settings.is_rect_displayed)
            {
                rb->lcd_drawrect(0, 0, 112, 64);

                /* top left corner */
                rb->lcd_drawpixel(1, 1);
                rb->lcd_drawpixel(2, 2);
                rb->lcd_drawpixel(1, 2);
                rb->lcd_drawpixel(1, 3);
                rb->lcd_drawpixel(2, 1);
                rb->lcd_drawpixel(3, 1);

                /* bottom left corner */
                rb->lcd_drawpixel(1, 62);
                rb->lcd_drawpixel(2, 61);
                rb->lcd_drawpixel(1, 61);
                rb->lcd_drawpixel(1, 60);
                rb->lcd_drawpixel(2, 62);
                rb->lcd_drawpixel(3, 62);

                /* top right corner */
                rb->lcd_drawpixel(110, 1);
                rb->lcd_drawpixel(109, 2);
                rb->lcd_drawpixel(110, 2);
                rb->lcd_drawpixel(110, 3);
                rb->lcd_drawpixel(109, 1);
                rb->lcd_drawpixel(108, 1);

                /* bottom right corner */
                rb->lcd_drawpixel(110, 62);
                rb->lcd_drawpixel(109, 61);
                rb->lcd_drawpixel(110, 61);
                rb->lcd_drawpixel(110, 60);
                rb->lcd_drawpixel(109, 62);
                rb->lcd_drawpixel(108, 62);
            }

            /* Draw the date */
            if (settings.is_date_displayed)
            {
                if(settings.analog_clock)
                {
                    rb->lcd_putsxy(3, 53, moday);
                    rb->lcd_putsxy(80, 53, dateyr);
                }
                else
                    rb->lcd_putsxy(25, 3, current_date);
            }

            if (settings.is_time_displayed)
            {
                if(settings.analog_clock)
                {
                    /* HH:MM */
                    rb->lcd_putsxy(1, 4, tmhrmin);
                    /* SS */
                    rb->lcd_putsxy(10, 12, tmsec);
                }
                else
                {
                    /* Just seconds */
                    rb->lcd_putsxy(50, 55, tmsec);
                }
            }

            /* Draw all to LCD */
            rb->lcd_update();
        }

        switch (rb->button_get_w_tmo(HZ/6))
        {
            /* OFF exits */
            case BUTTON_OFF:

            	/* Tell the user what's going on */
                rb->lcd_clear_display();
            	rb->splash(HZ/2, true, "Saving settings...");
            	/* Save to disk */
            	save_settings();
            	rb->lcd_clear_display();
                rb->splash(HZ, true, "Saved!");
                /* ...and exit. */
                return PLUGIN_OK;
                break;

                /* F1 & OFF exits without saving */
            case BUTTON_F1 | BUTTON_OFF:
                return PLUGIN_OK;
                break;

                /* F1 screen - plugin info */
            case BUTTON_F1:
                while (!exit)
                {
                    if(settings.analog_clock)
                    {
                        rb->lcd_clear_display();
                        rb->lcd_puts(0, 0, "F1   > Help");
                        rb->lcd_puts(0, 1, "PLAY > Digital");
                        rb->lcd_puts(0, 2, "FF   > Show Time");
                        rb->lcd_puts(0, 3, "DOWN > Border");
                        rb->lcd_puts(0, 4, "REW  > Digits");
                        rb->lcd_puts(0, 5, "UP   > Date");
                        rb->lcd_puts(0, 6, "OFF  > Save & Exit");
                    	rb->lcd_update();
                    }
                    else
                    {
                        rb->lcd_clear_display();
                        rb->lcd_puts(0, 0, "F1   > Help");
                        rb->lcd_puts(0, 1, "PLAY > Analog");
                        rb->lcd_puts(0, 3, "DOWN > Border");
                        rb->lcd_puts(0, 4, "UP   > Date");
                        rb->lcd_puts(0, 5, "FF   > Show Seconds");
                        rb->lcd_puts(0, 7, "OFF  > Save & Exit");
                    	rb->lcd_update();
                    }

                    switch (rb->button_get(true))
                    {
                        case BUTTON_F1 | BUTTON_REL:
                            if (used)
                                exit = true;
                            used = true;
                            break;

                        case BUTTON_OFF:
                            used = true;
                            break;

                        case SYS_USB_CONNECTED:
                            return PLUGIN_USB_CONNECTED;
                            rb->usb_screen();
                            break;
                    }
                }
                break;

                /* options screen */
            case BUTTON_F3:
                while (!exit)
                {
                    if(settings.analog_clock)
                    {
                        rb->lcd_clear_display();
                        rb->lcd_putsxy(7, 0,  "FF  > Show Time");
                        rb->lcd_putsxy(7, 10, "DOWN> Show Frame");
                        rb->lcd_putsxy(7, 20, "UP  > Show Date");
                        rb->lcd_putsxy(7, 30, "REW > Show Digits");

                        rb->lcd_putsxy(15, 46, "OFF > All OFF");
                        rb->lcd_putsxy(17, 56, "ON  > All ON");

                        rb->lcd_drawline(0, 41, 112, 41);

                        /* draw checkmarks */
                        if(settings.is_time_displayed)
                        {
                            rb->lcd_drawline(0, 4, 3, 7);
                            rb->lcd_drawline(3, 7, 5, 0);
                        }
                        if(settings.is_rect_displayed)
                        {
                            rb->lcd_drawline(0, 14, 3, 17);
                            rb->lcd_drawline(3, 17, 5, 10);
                        }
                        if(settings.is_date_displayed)
                        {
                            rb->lcd_drawline(0, 24, 3, 27);
                            rb->lcd_drawline(3, 27, 5, 20);
                        }
                        if(settings.are_digits_displayed)
                        {
                            rb->lcd_drawline(0, 34, 3, 37);
                            rb->lcd_drawline(3, 37, 5, 30);
                        }

                    	/* update the LCD after all this madness */
                    	rb->lcd_update();

                        /* ... and finally, setting options from screen: */
                        switch (rb->button_get(true))
                        {
                    	    case BUTTON_F3 | BUTTON_REL:
                    	        if (used)
                    	            exit = true;
                    	        used = true;
                    	        break;

                            case BUTTON_RIGHT:
                                settings.is_time_displayed =
                                    !settings.is_time_displayed;
                                break;

                            case BUTTON_DOWN:
                                settings.is_rect_displayed =
                                    !settings.is_rect_displayed;
                                break;

                            case BUTTON_UP:
                                settings.is_date_displayed =
                                    !settings.is_date_displayed;
                                break;

                            case BUTTON_LEFT:
                                settings.are_digits_displayed =
                                    !settings.are_digits_displayed;
                                break;

                            case BUTTON_ON:
                                settings.is_time_displayed = true;
                                settings.is_rect_displayed = true;
                                settings.is_date_displayed = true;
                                settings.are_digits_displayed = true;
                                break;

                            case BUTTON_OFF:
                                settings.is_time_displayed = false;
                                settings.is_rect_displayed = false;
                                settings.is_date_displayed = false;
                                settings.are_digits_displayed = false;
                                break;

                            case SYS_USB_CONNECTED:
                                return PLUGIN_USB_CONNECTED;
                                rb->usb_screen();
                                break;
                        }
                    }
                    else /* 7-Segment clock shown, less options */
                    {
                        rb->lcd_clear_display();
                        rb->lcd_putsxy(7, 0,  "DOWN> Show Frame");
                        rb->lcd_putsxy(7, 10, "UP  > Show Date");
                        rb->lcd_putsxy(7, 20, "FF  > Seconds");
                        rb->lcd_putsxy(15, 46, "OFF > All OFF");
                        rb->lcd_putsxy(17, 56, "ON  > All ON");
                        rb->lcd_drawline(0, 41, 112, 41);

                        /* draw checkmarks */
                        if(settings.is_rect_displayed)
                        {
                            rb->lcd_drawline(0, 4, 3, 7);
                            rb->lcd_drawline(3, 7, 5, 0);
                        }
                        if(settings.is_date_displayed)
                        {
                            rb->lcd_drawline(0, 14, 3, 17);
                            rb->lcd_drawline(3, 17, 5, 10);
                        }
                        if(settings.is_time_displayed)
                        {
                            rb->lcd_drawline(0, 24, 3, 27);
                            rb->lcd_drawline(3, 27, 5, 20);
                        }

                        /* And finally, update the LCD */
                        rb->lcd_update();

                        switch (rb->button_get(true))
                        {
                    	    case BUTTON_F3 | BUTTON_REL:
                    	        if (used)
                    	            exit = true;
                    	        used = true;
                    	        break;

                            case BUTTON_DOWN:
                                settings.is_rect_displayed =
                                    !settings.is_rect_displayed;
                                break;

                            case BUTTON_UP:
                                settings.is_date_displayed =
                                    !settings.is_date_displayed;
                                break;

                            case BUTTON_RIGHT:
                                settings.is_time_displayed =
                                    !settings.is_time_displayed;
                                break;

                            case BUTTON_ON:
                                settings.is_time_displayed = true;
                                settings.is_rect_displayed = true;
                                settings.is_date_displayed = true;
                                break;

                            case BUTTON_OFF:
                                settings.is_time_displayed = false;
                                settings.is_rect_displayed = false;
                                settings.is_date_displayed = false;
                                break;

                            case SYS_USB_CONNECTED:
                                return PLUGIN_USB_CONNECTED;
                                rb->usb_screen();
                                break;
                        }
                    }
                }
                break;

                /* Toggle analog/digital */
            case BUTTON_PLAY:
                settings.analog_clock = !settings.analog_clock;
                break;

                /* Show time */
            case BUTTON_RIGHT:
                settings.is_time_displayed = !settings.is_time_displayed;
                break;

                /* Show border */
            case BUTTON_DOWN:
                settings.is_rect_displayed = !settings.is_rect_displayed ;
                break;

                /* Show date */
            case BUTTON_UP:
                settings.is_date_displayed = !settings.is_date_displayed;
                break;

                /* Show digits */
            case BUTTON_LEFT:
                settings.are_digits_displayed = !settings.are_digits_displayed;
                break;

                /* USB plugged? */
            case SYS_USB_CONNECTED:
                rb->usb_screen();
                return PLUGIN_USB_CONNECTED;
                break;
        }
    }
}

/* 7 Segment LED imitation code by Linus Nielsen Feltzing */

/*
       a     0     b
        #########
       #         #
       #         #
      1#         #2
       #         #
       #    3    #
      c ######### d
       #         #
       #         #
      4#         #5
       #         #
       #    6    #
      e ######### f
*/

/* The coordinates of each end point (a-f) of the segment lines (0-6) */
static unsigned int point_coords[6][2] =
{
    {0, 0}, /* a */
    {1, 0}, /* b */
    {0, 1}, /* c */
    {1, 1}, /* d */
    {0, 2}, /* e */
    {1, 2}  /* f */
};

/* The end points (a-f) for each segment line */
static unsigned int seg_points[7][2] =
{
    {0,1}, /* a to b */
    {0,2}, /* a to c */
    {1,3}, /* b to d */
    {2,3}, /* c to d */
    {2,4}, /* c to e */
    {3,5}, /* d to f */
    {4,5}  /* e to f */
};

/* Lists that tell which segments (0-6) to enable for each digit (0-9),
   the list is terminated with -1 */
static int digit_segs[10][8] =
{
    {0,1,2,4,5,6, -1},   /* 0 */
    {2,5, -1},           /* 1 */
    {0,2,3,4,6, -1},     /* 2 */
    {0,2,3,5,6, -1},     /* 3 */
    {1,2,3,5, -1},       /* 4 */
    {0,1,3,5,6, -1},     /* 5 */
    {0,1,3,4,5,6, -1},   /* 6 */
    {0,2,5, -1},         /* 7 */
    {0,1,2,3,4,5,6, -1}, /* 8 */
    {0,1,2,3,5,6, -1}    /* 9 */
};

/* Draws one segment */
void draw_seg(int seg, int x, int y, int width, int height)
{
    int p1 = seg_points[seg][0];
    int p2 = seg_points[seg][1];
    int x1 = point_coords[p1][0];
    int y1 = point_coords[p1][1];
    int x2 = point_coords[p2][0];
    int y2 = point_coords[p2][1];

    /* It draws parallel lines of different lengths for thicker segments */
    if(seg == 0 || seg == 3 || seg == 6)
    {
        rb->lcd_drawline(x + x1 * width + 1, y + y1 * height / 2,
                         x + x2 * width - 1 , y + y2 * height / 2);

        rb->lcd_drawline(x + x1 * width + 2, y + y1 * height / 2 - 1,
                         x + x2 * width - 2, y + y2 * height / 2 - 1);
        rb->lcd_drawline(x + x1 * width + 2, y + y1 * height / 2 + 1,
                         x + x2 * width - 2, y + y2 * height / 2 + 1);

        rb->lcd_drawline(x + x1 * width + 3, y + y1 * height / 2 - 2,
                         x + x2 * width - 3, y + y2 * height / 2 - 2);
        rb->lcd_drawline(x + x1 * width + 3, y + y1 * height / 2 + 2,
                         x + x2 * width - 3, y + y2 * height / 2 + 2);
    }
    else
    {
        rb->lcd_drawline(x + x1 * width, y + y1 * height / 2 + 1,
                         x + x2 * width , y + y2 * height / 2 - 1);

        rb->lcd_drawline(x + x1 * width - 1, y + y1 * height / 2 + 2,
                         x + x2 * width - 1, y + y2 * height / 2 - 2);
        rb->lcd_drawline(x + x1 * width + 1, y + y1 * height / 2 + 2,
                         x + x2 * width + 1, y + y2 * height / 2 - 2);

        rb->lcd_drawline(x + x1 * width - 2, y + y1 * height / 2 + 3,
                         x + x2 * width - 2, y + y2 * height / 2 - 3);
        
        rb->lcd_drawline(x + x1 * width + 2, y + y1 * height / 2 + 3,
                         x + x2 * width + 2, y + y2 * height / 2 - 3);
    }
}

/* Draws one digit */
void draw_7seg_digit(int digit, int x, int y, int width, int height)
{
    int i;
    int c;

    for(i = 0;digit_segs[digit][i] >= 0;i++)
    {
        c = digit_segs[digit][i];

        draw_seg(c, x, y, width, height);
    }
}

/* Draws the entire 7-segment hour-minute time display */
void draw_7seg_time(int hour, int minute, int x, int y, int width, int height,
bool colon)
{
    int xpos = x;

    draw_7seg_digit(hour / 10, xpos, y, width, height);
    xpos += width + 6;
    draw_7seg_digit(hour % 10, xpos, y, width, height);
    xpos += width + 6;

    if(colon)
    {
        rb->lcd_drawline(xpos, y + height/3 + 2,
                         xpos, y + height/3 + 2);
        rb->lcd_drawline(xpos+1, y + height/3 + 1,
                         xpos+1, y + height/3 + 3);
        rb->lcd_drawline(xpos+2, y + height/3,
                         xpos+2, y + height/3 + 4);
        rb->lcd_drawline(xpos+3, y + height/3 + 1,
                         xpos+3, y + height/3 + 3);
        rb->lcd_drawline(xpos+4, y + height/3 + 2,
                         xpos+4, y + height/3 + 2);

        rb->lcd_drawline(xpos, y + height-height/3 + 2,
                         xpos, y + height-height/3 + 2);
        rb->lcd_drawline(xpos+1, y + height-height/3 + 1,
                         xpos+1, y + height-height/3 + 3);
        rb->lcd_drawline(xpos+2, y + height-height/3,
                         xpos+2, y + height-height/3 + 4);
        rb->lcd_drawline(xpos+3, y + height-height/3 + 1,
                         xpos+3, y + height-height/3 + 3);
        rb->lcd_drawline(xpos+4, y + height-height/3 + 2,
                         xpos+4, y + height-height/3 + 2);
    }

    xpos += 12;

    draw_7seg_digit(minute / 10, xpos, y, width, height);
    xpos += width + 6;
    draw_7seg_digit(minute % 10, xpos, y, width, height);
    xpos += width + 6;
}

#endif
