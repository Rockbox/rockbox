/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2003 Lee Pilgrim
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 **************************************************************************/

 /******************************************************
  * Quick note: throughout this code you'll see
  * comments like this one, inside of a box. They are
  * used to indicate new functions, not because I
  * think you're blind. Only here to help.
  ******************************************************/

#include "plugin.h"

#ifdef HAVE_LCD_BITMAP

static struct plugin_api* rb;

/* This table is used to define x positions on a logarithmic (dBfs) scale.
   There are 112 of them, one for each pixel.
   The formula I used to figure these out was log(x)*32.09,
   where x was the original x position. */
static unsigned char db_scale[] = {0,0,10,15,19,22,25,27,29,31,32,33,35,36,37,38,39,39,40,41,42,42,
                                    43,44,44,45,45,46,46,47,47,48,48,49,49,50,50,50,51,51,51,52,52,
                                    52,53,53,53,54,54,54,55,55,55,55,56,56,56,56,56,66, 71,75,78,81,
                                    83,85,87,88,89,91,92,93,94,95,95,96,97,98,98,99,100,100,101,101,
                                    102,102,103,103,104,104,105,105,106,106,106,107,107,107,108,108,
                                    108,109,109,109,110,110,110,111,111,111,111,112,112,112};

/* Define's y positions, and makes it look like an arch, like a real needle would. */
static unsigned char y_values[] = {32,31,30,29,28,27,26,25,24,24,23,22,22,21,21,20,19,19,18,18,18,18,18,17,17,17,17,17,
                                   17,17,17,17,17,18,18,18,18,18,19,19,20,21,21,22,22,23,24,24,25,26,27,28,29,30,31,32,
                                   32,31,30,29,28,27,26,25,24,24,23,22,22,21,21,20,19,19,18,18,18,18,18,17,17,17,17,17,
                                   17,17,17,17,17,18,18,18,18,18,19,19,20,21,21,22,22,23,24,24,25,26,27,28,29,30,31,32};

/***************************************************
 * LCD Bitmaps/Icons
 ***************************************************/

/* Linear mode bitmap icon. */
unsigned char mode_linear[] = {0xDF,0x10,0xD0,0x00,0xDF,0x00,0xDF,0x02,0xC4,0x08,0xDF,
                               0x00,0xDF,0x15,0xD1,0x00,0xDE,0x05,0xDE,0x00,0xDF,0x05,
                               0xDA};

/* Logarithmic (dBfs) mode bitmap icon. */
unsigned char mode_dbfs[] =   {0xC8,0x14,0x14,0x1F,0x00,0xDF,0x15,0x15,0x0A,0xC0,0x1E,
                               0x05,0x05,0xC0,0x16,0x15,0xD5,0x09,0xC0,0x00,0xF0,0x00,
                               0xFE};

/* Channel indicator - no level     [4x8] */
static unsigned char sound_speaker[] = {0x18,0x24,0x42,0xFF};

/* Channel indicator - low level    [2x8] */
static unsigned char sound_low_level[] = {0x24,0x18};

/* Channel indicator - medium level [2x8] */
static unsigned char sound_med_level[] = {0x42,0x3C};

/* Channel indicator - high level   [2x8] */
static unsigned char sound_high_level[] = {0x81,0x7E};

/* Channel indicator - maxed out    [3x8] */
static unsigned char sound_max_level[] = {0x0E,0xDF,0x0E};

/* Volume level icon                [4x7] */
static unsigned char volume_indicator[] = {0x1C,0x1C,0x22,0x7F};

/* RIGHT ARROW icon (from icons.c)  [7x8] */
static unsigned char right_arrow[] = {0x7F,0x3E,0x1C,0x7F,0x3E,0x1C,0x08};

/* LEFT ARROW icon (from icons.c)   [7x8] */
static unsigned char left_arrow[] = {0x08,0x1C,0x3E,0x7F,0x1C,0x3E,0x7F};

/*************************
 * DEFINES
 *************************/
#define MAX_PEAK 0x7FFF
#define NEEDLE_BOTTOM_Y 54

/******************************
 * LEFT NEEDLE ints
 ******************************/
#define LEFT_NEEDLE_BOTTOM_X 28
int left_needle_top_y;
int left_needle_top_x;
int old_left_needle_top_x = 0;
int left_needle_top_x_no_log;

/*******************************
 * RIGHT NEEDLE ints
 *******************************/
#define RIGHT_NEEDLE_BOTTOM_X 84
int right_needle_top_y;
int right_needle_top_x;
int old_right_needle_top_x = 56;
int right_needle_top_x_no_log;

/*********************
 * LEFT MINIMETER ints
 *********************/
int mini_left_low;
int mini_left_med;
int mini_left_high;
int mini_left_clip;

/**********************
 * RIGHT MINIMETER ints
 **********************/
int mini_right_low;
int mini_right_med;
int mini_right_high;
int mini_right_clip;

/************************
 * GENERAL ints
 ************************/
char curr_vol[3];          /* CURRENT VOLUME */
int vol;                   /* CURENNT VOLUME int */
int on = 1;                /* MINIMETER OPTIONS int */
int needle_cover_mode = 1; /* NEEDLE COVER int (defaults to "rounded") */
int arch = 1;              /* ARCH int (defaults to "solid") */
int decay = 3;             /* DECAY int (defaults to "medium") */

/*************************
 * GENERAL bools
 *************************/
bool quit = false;
bool use_log_scale = true;
bool use_minimeters = true;

/********************************************
 * Change the volume - derived from VIDEO.C
 ********************************************/
void ChangeVolume(int delta)
{
    vol = rb->global_settings->volume + delta;

    if (vol > 100) vol = 100;
    else if (vol < 0) vol = 0;
    if (vol != rb->global_settings->volume)
    {
        rb->mpeg_sound_set(SOUND_VOLUME, vol);
        rb->global_settings->volume = vol;
        rb->snprintf(curr_vol, sizeof(curr_vol), "%d", vol);
    }
}

/*****************************************************
 * Draw the status bar
 *****************************************************/
void draw_status_bar(void)
{
    rb->lcd_setfont(FONT_SYSFIXED);

    rb->lcd_drawline(0, 10, 112, 10);
    rb->lcd_putsxy(1, 1, "VU Meter");
    rb->lcd_drawline(52, 1, 52, 8);
    rb->lcd_bitmap(volume_indicator, 60, 1, 4, 7, true);
    rb->lcd_putsxy(65, 1, curr_vol);
    rb->lcd_drawline(85, 1, 85, 8);

    if(use_log_scale)
        rb->lcd_bitmap(mode_dbfs, 88, 1, 23, 8, true);
    else
        rb->lcd_bitmap(mode_linear, 88, 1, 23, 8, true);

    /* The last line under the needle covers. */
    rb->lcd_drawline(0, 54, 111, 54);

    rb->lcd_update_rect(0, 0, 112, 10);
}

/******************************
 * DRAW left MiniMeters
 ******************************/

void draw_left_minimeters(void)
{
    rb->lcd_bitmap(sound_speaker, 22, 56, 4, 8, true);

    if(5<left_needle_top_x_no_log)
        rb->lcd_bitmap(sound_low_level, 27, 56, 2, 8, false);
    if(12<left_needle_top_x_no_log)
        rb->lcd_bitmap(sound_med_level, 30, 56, 2, 8, false);
    if(24<left_needle_top_x_no_log)
        rb->lcd_bitmap(sound_high_level, 33, 56, 2, 8, false);
    if(40<left_needle_top_x_no_log)
        rb->lcd_bitmap(sound_max_level, 36, 56, 3, 8, false);
}

/*******************************
 * DRAW right MiniMeters
 *******************************/
void draw_right_minimeters(void)
{
    rb->lcd_bitmap(sound_speaker, 79, 56, 4, 8, true);

    if(5<(right_needle_top_x_no_log-56))
        rb->lcd_bitmap(sound_low_level, 84, 56, 2, 8, false);
    if(12<(right_needle_top_x_no_log-56))
        rb->lcd_bitmap(sound_med_level, 87, 56, 2, 8, false);
    if(24<(right_needle_top_x_no_log-56))
        rb->lcd_bitmap(sound_high_level, 90, 56, 2, 8, false);
    if(40<(right_needle_top_x_no_log-56))
        rb->lcd_bitmap(sound_max_level, 93, 56, 3, 8, false);
}

/***************************************
 * GENERAL SETTINGS - F1
 ***************************************/
void general_settings(void)
{
    char buf[15];
    int w, h;

    while(!quit)
    {
        rb->lcd_clear_display();

        /* Left hand side (needle covers) */
        rb->lcd_putsxy(0, 15, "Needle");
        rb->lcd_putsxy(0, 23, "Covers:");
        if(needle_cover_mode == 1)
            rb->lcd_putsxy(0, 32, "Rounded");
        else
            rb->lcd_putsxy(0, 32, "Pyramid");
        rb->lcd_bitmap(left_arrow, 43, 23, 7, 8, true);

        /* Right hand side (visible arch) */
        rb->snprintf(buf, sizeof(buf), "Visible");
        rb->lcd_getstringsize(buf, &w, &h);
        rb->lcd_putsxy(LCD_WIDTH-w, 15, buf);
        rb->snprintf(buf, sizeof(buf), "Arch:");
        rb->lcd_getstringsize(buf, &w, &h);
        rb->lcd_putsxy(LCD_WIDTH-w, 23, buf);
        rb->lcd_bitmap(right_arrow, 62, 23, 7, 8, true);
        if(arch == 1)
        {
            rb->snprintf(buf, sizeof(buf), "Solid");
            rb->lcd_getstringsize(buf, &w, &h);
            rb->lcd_putsxy(LCD_WIDTH-w, 32, buf);
        }
        else if(arch == 2)
        {
            rb->snprintf(buf, sizeof(buf), "Dotted");
            rb->lcd_getstringsize(buf, &w, &h);
            rb->lcd_putsxy(LCD_WIDTH-w, 32, buf);
        }
        else
        {
			rb->snprintf(buf, sizeof(buf), "Off");
			rb->lcd_getstringsize(buf, &w, &h);
			rb->lcd_putsxy(LCD_WIDTH-w, 32, buf);
		}

        rb->lcd_update();

        switch(rb->button_get_w_tmo(HZ/4))
        {
            case BUTTON_F1:
            case BUTTON_OFF:
                quit = true;
                break;

            case BUTTON_RIGHT:
                arch==3 ? arch=1 : arch++;
                break;

            case BUTTON_LEFT:
                needle_cover_mode==2 ? needle_cover_mode=1 : needle_cover_mode++;
                break;
        }
    }
}

/*****************************
 * User Decay settings
 *****************************/
void user_decay_settings(void)
{
    while(!quit)
    {
		rb->lcd_clear_display();

		rb->lcd_putsxy(2, 0, "User Decay Options");
		rb->lcd_putsxy(5, 15, "Release Speed:");

		switch(decay)
		{
			case 0: rb->lcd_putsxy(5, 23, "No Decay");    break;
			case 1: rb->lcd_putsxy(5, 23, "Very Fast");   break;
			case 2: rb->lcd_putsxy(5, 23, "Fast");        break;
			case 3: rb->lcd_putsxy(5, 23, "Medium");      break;
			case 4: rb->lcd_putsxy(5, 23, "Medium-Slow"); break;
			case 5: rb->lcd_putsxy(5, 23, "Slow");        break;
			case 6: rb->lcd_putsxy(5, 23, "Very Slow");   break;
			default: break;
		}

		rb->lcd_putsxy(2, 48, "UP/DOWN: Change");
		rb->lcd_putsxy(2, 56, "OFF/F2: Back to VU");

		rb->lcd_update();

		switch(rb->button_get_w_tmo(HZ/4))
		{
			case BUTTON_F2:
			case BUTTON_OFF:
			    quit = true;
			    break;

			case BUTTON_DOWN:
			    decay == 6 ? decay = 0 : decay++;
			    break;

			case BUTTON_UP:
			    decay == 0 ? decay = 6 : decay--;
			    break;
		}
	}
}

/***********************************************************************
 * PLUGIN STARTS HERE
 ***********************************************************************/
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    TEST_PLUGIN_API(api);
    (void) parameter;
    rb = api;

    /* Calculate the volume right away */
    vol = rb->global_settings->volume;
    rb->snprintf(curr_vol, sizeof(curr_vol), "%d", vol);

    rb->splash(HZ*2, true, "Press [ON] for help");
    while (!PLUGIN_OK)
    {
        quit = false;

        #ifdef SIMULATOR
        left_needle_top_x_no_log = rb->rand()%56;
        right_needle_top_x_no_log = (rb->rand()%56)+56;
        #else
        left_needle_top_x_no_log =
            (rb->mas_codec_readreg(0xC) * 56 / MAX_PEAK);

        right_needle_top_x_no_log =
            (rb->mas_codec_readreg(0xD) * 56 / MAX_PEAK) + 56;
        #endif

        rb->lcd_clear_display();

        if (use_log_scale)
        {
            left_needle_top_x = db_scale[left_needle_top_x_no_log];
            right_needle_top_x = db_scale[right_needle_top_x_no_log];
        }

        else
        {
            left_needle_top_x = left_needle_top_x_no_log;
            right_needle_top_x = right_needle_top_x_no_log;
        }

        /* Makes a decay on the needle (Custom Decay is at F2) */
        left_needle_top_x = (left_needle_top_x+old_left_needle_top_x*decay)/(decay+1);
        right_needle_top_x = (right_needle_top_x+old_right_needle_top_x*decay)/(decay+1);

        old_left_needle_top_x = left_needle_top_x;
        old_right_needle_top_x = right_needle_top_x;

        left_needle_top_y = y_values[left_needle_top_x];
        right_needle_top_y = y_values[right_needle_top_x];

        /* Draws the needles */
        rb->lcd_drawline(LEFT_NEEDLE_BOTTOM_X, NEEDLE_BOTTOM_Y,
                        left_needle_top_x, left_needle_top_y);

        rb->lcd_drawline(RIGHT_NEEDLE_BOTTOM_X, NEEDLE_BOTTOM_Y,
                         right_needle_top_x, right_needle_top_y);

        /* We've got four needle cover modes... */
        if(needle_cover_mode == 1)  /* Rounded needle cover. */
        {
            /* Left needle cover, top to bottom */
            rb->lcd_drawline(27, 49, 29, 49);
            rb->lcd_drawline(25, 50, 31, 50);
            rb->lcd_drawline(23, 51, 33, 51);
            rb->lcd_drawline(22, 52, 34, 52);
            rb->lcd_drawline(22, 53, 34, 53);

            /* Right needle cover, top to bottom */
            rb->lcd_drawline(83, 49, 85, 49);
            rb->lcd_drawline(81, 50, 87, 50);
            rb->lcd_drawline(79, 51, 89, 51);
            rb->lcd_drawline(78, 52, 90, 52);
            rb->lcd_drawline(78, 53, 90, 53);
        }

        else  /* Pyramid needle cover. */
        {
            /* Left needle cover, top to bottom */
            rb->lcd_drawpixel(28, 49);
            rb->lcd_drawline(27, 50, 29, 50);
            rb->lcd_drawline(26, 51, 30, 51);
            rb->lcd_drawline(25, 52, 31, 52);
            rb->lcd_drawline(24, 53, 32, 53);

            /* Right needle cover, top to bottom */
            rb->lcd_drawpixel(84, 49);
            rb->lcd_drawline(83, 50, 85, 50);
            rb->lcd_drawline(82, 51, 86, 51);
            rb->lcd_drawline(81, 52, 87, 52);
            rb->lcd_drawline(80, 53, 88, 53);
        }

        if(arch == 1) /* Solid */
        {
            int i;
            for(i=0; i<=112; i++)
                rb->lcd_drawpixel(i, (y_values[i])-2 );
        }
        else if(arch == 2) /* Dotted */
        {
            int i;
            for(i=0; i<=112; i+=2)
                rb->lcd_drawpixel(i, y_values[i]);
        }

        if(use_minimeters)
        {
            draw_left_minimeters();
            draw_right_minimeters();
        }

        draw_status_bar();

        rb->lcd_update();

        /* Using a faster timeout like "1" makes the
         * minimeters flicker, HZ/35 looks good. */
        switch (rb->button_get_w_tmo(HZ/35))
        {
            /* EXIT */
            case BUTTON_OFF:
                return PLUGIN_OK;
                break;

            /* INFO */
            case BUTTON_ON:
                rb->lcd_clear_display();
                while(!quit)
                {
                    rb->lcd_puts(0, 0, "OFF: Exit Plugin");
                    rb->lcd_puts(0, 1, "PLAY: Change Scale");
                    rb->lcd_puts(0, 2, "F1: Settings");
                    rb->lcd_puts(0, 3, "F2: Custom Decay");
                    rb->lcd_puts(0, 4, "F3: Mini-Meters");
                    rb->lcd_puts(0, 5, "UP/DOWN: Volume");
                    rb->lcd_puts(0, 7, "ON to exit...");

                    rb->lcd_update();

                    switch(rb->button_get_w_tmo(HZ/4))
                    {
                        case BUTTON_ON:
                            quit = true;
                            break;
                    }
                }
                break;

            /* SCALE switch */
            case BUTTON_PLAY:
                use_log_scale = !use_log_scale;
                break;

            /* GENERAL SETTINGS screen */
            case BUTTON_F1:
                general_settings();
                break;

            /* DECAY SETTINGS screen */
            case BUTTON_F2:
                user_decay_settings();
                break;

            /* MINIMETER SETTINGS screen */
            case BUTTON_F3:
                use_minimeters = !use_minimeters;
                break;

            /* VOLUME UP */
            case BUTTON_UP:
            case BUTTON_UP | BUTTON_REPEAT:
                ChangeVolume(1);
                break;

            /* VOLUME DOWN */
            case BUTTON_DOWN:
            case BUTTON_DOWN | BUTTON_REPEAT:
                ChangeVolume(-1);
                break;

            /* USB CONNECTED */
            case SYS_USB_CONNECTED:
                rb->usb_screen();
                return PLUGIN_USB_CONNECTED;
                break;
        }
    }
}
#endif /* #ifdef HAVE_LCD_BITMAP */
