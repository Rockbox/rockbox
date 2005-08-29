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
#include "plugin.h"

#if defined(HAVE_LCD_BITMAP) && (CONFIG_CODEC != SWCODEC)

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define VUMETER_QUIT BUTTON_OFF
#define VUMETER_HELP BUTTON_ON
#define VUMETER_MENU BUTTON_F1
#define VUMETER_MENU_EXIT BUTTON_F1
#define VUMETER_MENU_EXIT2 BUTTON_OFF

#elif CONFIG_KEYPAD == ONDIO_PAD
#define VUMETER_QUIT BUTTON_OFF
#define VUMETER_HELP_PRE BUTTON_MENU
#define VUMETER_HELP (BUTTON_MENU | BUTTON_REL)
#define VUMETER_MENU_PRE BUTTON_MENU
#define VUMETER_MENU (BUTTON_MENU | BUTTON_REPEAT)
#define VUMETER_MENU_EXIT BUTTON_MENU
#define VUMETER_MENU_EXIT2 BUTTON_OFF

#endif

const struct plugin_api* rb;

#ifdef SIMULATOR
#define mas_codec_readreg(x) rand()%MAX_PEAK
#endif

/* Defines x positions on a logarithmic (dBfs) scale. */
const unsigned char analog_db_scale[] =
{0,0,10,15,19,22,25,27,29,31,32,33,35,36,37,38,39,39,40,41,42,42,43,44,44,45,45,46,
46,47,47,48,48,49,49,50,50,50,51,51,51,52,52,52,53,53,53,54,54,54,55,55,55,55,56,56};

const unsigned char digital_db_scale[] =
{0,2,3,5,5,6,6,6,7,7,7,7,8,8,8,8,8,9,9,9,9,9,9,9,9,10,10,
10,10,10,10,10,10,10,10,10,11,11,11,11,11,11,11,11};

/* Define's y positions, to make the needle arch, like a real needle would. */
const unsigned char y_values[] =
{34,34,33,33,32,32,31,31,30,30,29,29,28,28,28,27,27,27,26,26,26,26,25,25,25,25,25,25,
25,25,25,25,25,25,26,26,26,26,27,27,27,28,28,28,29,29,30,30,31,31,32,32,33,33,34,34};

const unsigned char led[] = {0x0e, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x1f, 0x0e};

const unsigned char needle_cover[] =
{0x18, 0x1c, 0x1c, 0x1e, 0x1e, 0x1f, 0x1f, 0x1f, 0x1e, 0x1e, 0x1c, 0x1c, 0x18};

const unsigned char sound_speaker[] = {0x18,0x24,0x42,0xFF};
const unsigned char sound_low_level[] = {0x24,0x18};
const unsigned char sound_med_level[] = {0x42,0x3C};
const unsigned char sound_high_level[] = {0x81,0x7E};
const unsigned char sound_max_level[] = {0x0E,0xDF,0x0E};

#define ANALOG 1 /* The two meter types */
#define DIGITAL 2

int left_needle_top_y;
int left_needle_top_x;
int last_left_needle_top_x;
int right_needle_top_y;
int right_needle_top_x;
int last_right_needle_top_x = 56;

int num_left_leds;
int num_right_leds;
int last_num_left_leds;
int last_num_right_leds;

int i;

#define MAX_PEAK 0x7FFF

struct saved_settings {
    int meter_type;
    bool analog_use_db_scale;
    bool digital_use_db_scale;
    bool analog_minimeters;
    bool digital_minimeters;
    int analog_decay;
    int digital_decay; 
} settings;

void reset_settings(void) {    
    settings.meter_type=ANALOG;
    settings.analog_use_db_scale=true;
    settings.digital_use_db_scale=true;
    settings.analog_minimeters=true;
    settings.digital_minimeters=false;
    settings.analog_decay=3;
    settings.digital_decay=0; 
}

void load_settings(void) {
    int fp = rb->open("/.rockbox/rocks/.vu_meter", O_RDONLY);
    if(fp>=0) {
            rb->read(fp, &settings, sizeof(struct saved_settings));
            rb->close(fp);
    }
    else {
        reset_settings();
#if CONFIG_KEYPAD == RECORDER_PAD
        rb->splash(HZ, true, "Press ON for help");
#elif CONFIG_KEYPAD == ONDIO_PAD
        rb->splash(HZ, true, "Press MODE for help");
#endif
    }
}

void save_settings(void) {
    int fp = rb->creat("/.rockbox/rocks/.vu_meter", O_WRONLY);
    if(fp >= 0) {
        rb->write (fp, &settings, sizeof(struct saved_settings));
        rb->close(fp);
    }
}

void change_volume(int delta) {
    char curr_vol[4];
    int vol = rb->global_settings->volume + delta;

    if (vol>100) vol = 100;
    else if (vol < 0) vol = 0;
    if (vol != rb->global_settings->volume) {
        rb->sound_set(SOUND_VOLUME, vol);
        rb->global_settings->volume = vol;
        rb->snprintf(curr_vol, sizeof(curr_vol), "%d", vol);
        rb->lcd_putsxy(0,0, curr_vol);
        rb->lcd_update();
        rb->sleep(HZ/12);
    }
}

void change_settings(void)
{
    int selected_setting=0;
    bool quit=false;
    while(!quit)
    {
        rb->lcd_clear_display();

        rb->lcd_putsxy(33, 0, "Settings");

        rb->lcd_putsxy(0, 8, "Meter type:");
        if(settings.meter_type==ANALOG)
            rb->lcd_putsxy(67, 8, "Analog");
        else
            rb->lcd_putsxy(67, 8, "Digital");

        if(settings.meter_type==ANALOG) {
            rb->lcd_putsxy(0, 16, "Scale:");
            if(settings.analog_use_db_scale)
                rb->lcd_putsxy(36, 16, "dBfs");
            else
                rb->lcd_putsxy(36, 16, "Linear");

            rb->lcd_putsxy(0, 24, "Minimeters:");
            if(settings.analog_minimeters)
                rb->lcd_putsxy(65, 24, "On");
            else        
                rb->lcd_putsxy(65, 24, "Off");

            rb->lcd_putsxy(0, 32, "Decay Speed:");
            switch(settings.analog_decay) {
                case 0: rb->lcd_putsxy(10, 40, "No Decay");    break;
                case 1: rb->lcd_putsxy(10, 40, "Very Fast");   break;
                case 2: rb->lcd_putsxy(10, 40, "Fast");        break;
                case 3: rb->lcd_putsxy(10, 40, "Medium");      break;
                case 4: rb->lcd_putsxy(10, 40, "Medium-Slow"); break;
                case 5: rb->lcd_putsxy(10, 40, "Slow");        break;
                case 6: rb->lcd_putsxy(10, 40, "Very Slow");   break;
            }
        }
        else {
            rb->lcd_putsxy(0, 16, "Scale:");
            if(settings.digital_use_db_scale)
                rb->lcd_putsxy(36, 16, "dBfs");
            else
                rb->lcd_putsxy(36, 16, "Linear");

            rb->lcd_putsxy(0, 24, "Minimeters:");
            if(settings.digital_minimeters)
                rb->lcd_putsxy(65, 24, "On");
            else        
                rb->lcd_putsxy(65, 24, "Off");

            rb->lcd_putsxy(0, 32, "Decay Speed:");
            switch(settings.digital_decay) {
                case 0: rb->lcd_putsxy(10, 40, "No Decay");    break;
                case 1: rb->lcd_putsxy(10, 40, "Very Fast");   break;
                case 2: rb->lcd_putsxy(10, 40, "Fast");        break;
                case 3: rb->lcd_putsxy(10, 40, "Medium");      break;
                case 4: rb->lcd_putsxy(10, 40, "Medium-Slow"); break;
                case 5: rb->lcd_putsxy(10, 40, "Slow");        break;
                case 6: rb->lcd_putsxy(10, 40, "Very Slow");   break;
            }
        }

        rb->lcd_set_drawmode(DRMODE_COMPLEMENT);
        rb->lcd_fillrect(0, selected_setting*8+8,111,8);
        rb->lcd_set_drawmode(DRMODE_SOLID);
        rb->lcd_update();

        switch(rb->button_get_w_tmo(1))
        {
            case VUMETER_MENU_EXIT:
            case VUMETER_MENU_EXIT2:
                quit = true;
                break;

            case BUTTON_LEFT:
                if(selected_setting==0)
                    settings.meter_type == DIGITAL ? settings.meter_type = ANALOG : settings.meter_type++;
                if(settings.meter_type==ANALOG) {
                    if(selected_setting==1)
                        settings.analog_use_db_scale = !settings.analog_use_db_scale;    
                    if(selected_setting==2)                    
                        settings.analog_minimeters = !settings.analog_minimeters;
                    if(selected_setting==3)
                        settings.analog_decay == 0 ? settings.analog_decay = 6 : settings.analog_decay--;
                }
                else {
                    if(selected_setting==1)
                        settings.digital_use_db_scale = !settings.digital_use_db_scale;    
                    if(selected_setting==2)                    
                        settings.digital_minimeters = !settings.digital_minimeters;
                    if(selected_setting==3)
                        settings.digital_decay == 0 ? settings.digital_decay = 6 : settings.digital_decay--;
                }
                break;

            case BUTTON_RIGHT:
                if(selected_setting==0)
                    settings.meter_type == DIGITAL ? settings.meter_type = ANALOG : settings.meter_type++;
                if(settings.meter_type==ANALOG) {
                    if(selected_setting==1)
                        settings.analog_use_db_scale = !settings.analog_use_db_scale;
                    if(selected_setting==2)
                        settings.analog_minimeters = !settings.analog_minimeters;
                    if(selected_setting==3)
                        settings.analog_decay == 6 ? settings.analog_decay = 0 : settings.analog_decay++;
                }
                else {
                    if(selected_setting==1)
                        settings.digital_use_db_scale = !settings.digital_use_db_scale;
                    if(selected_setting==2)
                        settings.digital_minimeters = !settings.digital_minimeters;
                    if(selected_setting==3)
                        settings.digital_decay == 6 ? settings.digital_decay = 0 : settings.digital_decay++;
                }
                break;

            case BUTTON_DOWN:
                selected_setting == 3 ? selected_setting=0 : selected_setting++;
                break;

            case BUTTON_UP:
                selected_setting == 0 ? selected_setting=3 : selected_setting--;
        }
    }
}

void draw_analog_minimeters(void) {
    rb->lcd_mono_bitmap(sound_speaker, 0, 12, 4, 8);
    rb->lcd_set_drawmode(DRMODE_FG);
    if(5<left_needle_top_x)
        rb->lcd_mono_bitmap(sound_low_level, 5, 12, 2, 8);
    if(12<left_needle_top_x)
        rb->lcd_mono_bitmap(sound_med_level, 7, 12, 2, 8);
    if(24<left_needle_top_x)
        rb->lcd_mono_bitmap(sound_high_level, 9, 12, 2, 8);
    if(40<left_needle_top_x)
        rb->lcd_mono_bitmap(sound_max_level, 12, 12, 3, 8);

    rb->lcd_set_drawmode(DRMODE_SOLID);
    rb->lcd_mono_bitmap(sound_speaker, 54, 12, 4, 8);
    rb->lcd_set_drawmode(DRMODE_FG);
    if(5<(right_needle_top_x-56))
        rb->lcd_mono_bitmap(sound_low_level, 59, 12, 2, 8);
    if(12<(right_needle_top_x-56))
        rb->lcd_mono_bitmap(sound_med_level, 61, 12, 2, 8);
    if(24<(right_needle_top_x-56))
        rb->lcd_mono_bitmap(sound_high_level, 63, 12, 2, 8);
    if(40<(right_needle_top_x-56))
        rb->lcd_mono_bitmap(sound_max_level, 66, 12, 3, 8);
    rb->lcd_set_drawmode(DRMODE_SOLID);
}

void draw_digital_minimeters(void) {
    rb->lcd_mono_bitmap(sound_speaker, 34, 24, 4, 8);
    rb->lcd_set_drawmode(DRMODE_FG);
    if(1<num_left_leds)
        rb->lcd_mono_bitmap(sound_low_level, 39, 24, 2, 8);
    if(2<num_left_leds)
        rb->lcd_mono_bitmap(sound_med_level, 41, 24, 2, 8);
    if(5<num_left_leds)
        rb->lcd_mono_bitmap(sound_high_level, 43, 24, 2, 8);
    if(8<num_left_leds)
        rb->lcd_mono_bitmap(sound_max_level, 46, 24, 3, 8);

    rb->lcd_set_drawmode(DRMODE_SOLID);
    rb->lcd_mono_bitmap(sound_speaker, 34, 40, 4, 8);
    rb->lcd_set_drawmode(DRMODE_FG);
    if(1<(num_right_leds))
        rb->lcd_mono_bitmap(sound_low_level, 39, 40, 2, 8);
    if(2<(num_right_leds))
        rb->lcd_mono_bitmap(sound_med_level, 41, 40, 2, 8);
    if(5<(num_right_leds))
        rb->lcd_mono_bitmap(sound_high_level, 43, 40, 2, 8);
    if(8<(num_right_leds))
        rb->lcd_mono_bitmap(sound_max_level, 46, 40, 3, 8);
    rb->lcd_set_drawmode(DRMODE_SOLID);
}

void analog_meter(void) {
    if(settings.analog_use_db_scale) {
        left_needle_top_x = analog_db_scale[rb->mas_codec_readreg(0xC)*56/MAX_PEAK];
        right_needle_top_x = analog_db_scale[rb->mas_codec_readreg(0xD)*56/MAX_PEAK]+56;
    }

    else {
        left_needle_top_x = rb->mas_codec_readreg(0xC) * 56 / MAX_PEAK;
        right_needle_top_x = (rb->mas_codec_readreg(0xD) * 56 / MAX_PEAK)+56;
    }

    /* Makes a decay on the needle */
    left_needle_top_x = (left_needle_top_x+last_left_needle_top_x*settings.analog_decay)/(settings.analog_decay+1);
    right_needle_top_x = (right_needle_top_x+last_right_needle_top_x*settings.analog_decay)/(settings.analog_decay+1);

    last_left_needle_top_x = left_needle_top_x;
    last_right_needle_top_x = right_needle_top_x;

    left_needle_top_y = y_values[left_needle_top_x];
    right_needle_top_y = y_values[right_needle_top_x-56];

    /* Needles */
    rb->lcd_drawline(28, 63, left_needle_top_x, left_needle_top_y);
    rb->lcd_drawline(84, 63, right_needle_top_x, right_needle_top_y);

    if(settings.analog_minimeters)
        draw_analog_minimeters();

    /* Needle covers */
    rb->lcd_set_drawmode(DRMODE_FG);
    rb->lcd_mono_bitmap(needle_cover, 22, 59, 13, 5);
    rb->lcd_mono_bitmap(needle_cover, 78, 59, 13, 5);
    rb->lcd_set_drawmode(DRMODE_SOLID);

    /* Show Left/Right */
    rb->lcd_putsxy(16, 12, "Left");
    rb->lcd_putsxy(70, 12, "Right");

    /* Line above/below  the Left/Right text */
    rb->lcd_drawline(0,9,111,9);
    rb->lcd_drawline(0,21,111,21);

    for(i=0; i<56; i++) {
        rb->lcd_drawpixel(i, (y_values[i])-2);
        rb->lcd_drawpixel(i+56, (y_values[i])-2);
    }
}

void digital_meter(void) {
    if(settings.digital_use_db_scale) {
        num_left_leds = digital_db_scale[rb->mas_codec_readreg(0xC) * 44 / MAX_PEAK];
        num_right_leds = digital_db_scale[rb->mas_codec_readreg(0xD) * 44 / MAX_PEAK];
    }
    else {
        num_left_leds = rb->mas_codec_readreg(0xC) * 11 / MAX_PEAK;
        num_right_leds = rb->mas_codec_readreg(0xD) * 11 / MAX_PEAK;
    }

    num_left_leds = (num_left_leds+last_num_left_leds*settings.digital_decay)/(settings.digital_decay+1);
    num_right_leds = (num_right_leds+last_num_right_leds*settings.digital_decay)/(settings.digital_decay+1);
    
    last_num_left_leds = num_left_leds;
    last_num_right_leds = num_right_leds;

    rb->lcd_set_drawmode(DRMODE_FG);
    /* LEDS */
    for(i=0; i<num_left_leds; i++)
        rb->lcd_mono_bitmap(led, i*9+2+i, 14, 9, 5);

    for(i=0; i<num_right_leds; i++)
        rb->lcd_mono_bitmap(led, i*9+2+i, 52, 9, 5);

    rb->lcd_set_drawmode(DRMODE_SOLID);

    if(settings.digital_minimeters)
        draw_digital_minimeters();

    /* Lines above/below where the LEDS are */
    rb->lcd_drawline(0,12,111,12);
    rb->lcd_drawline(0,20,111,20);

    rb->lcd_drawline(0,50,111,50);
    rb->lcd_drawline(0,58,111,58);

    /* Show Left/Right */
    rb->lcd_putsxy(2, 24, "Left");
    rb->lcd_putsxy(2, 40, "Right");

    /* Line in the middle */
    rb->lcd_drawline(0,35,111,35);
}

enum plugin_status plugin_start(struct plugin_api* api, void* parameter) {
    int button;
    int lastbutton = BUTTON_NONE;
    
    TEST_PLUGIN_API(api);
    (void) parameter;
    rb = api;

    load_settings();
    rb->lcd_setfont(FONT_SYSFIXED);

    while (1)
    {
        rb->lcd_clear_display();

        rb->lcd_putsxy(33, 0, "VU Meter");

        if(settings.meter_type==ANALOG)
            analog_meter();
        else
            digital_meter();
            
        rb->lcd_update();

        button = rb->button_get_w_tmo(1);
        switch (button)
        {
            case VUMETER_QUIT:
                save_settings();
                return PLUGIN_OK;
                break;

            case VUMETER_HELP:
#ifdef VUMETER_HELP_PRE
                if (lastbutton != VUMETER_HELP_PRE)
                    break;
#endif
                rb->lcd_clear_display();
                rb->lcd_puts(0, 0, "OFF: Exit");
#if CONFIG_KEYPAD == RECORDER_PAD
                rb->lcd_puts(0, 1, "F1: Settings");
#elif CONFIG_KEYPAD == ONDIO_PAD
                rb->lcd_puts(0, 1, "MODE..: Settings");
#endif
                rb->lcd_puts(0, 2, "UP/DOWN: Volume");
                rb->lcd_update();
                rb->sleep(HZ*3);
                break;

            case VUMETER_MENU:
#ifdef VUMETER_MENU_PRE
                if (lastbutton != VUMETER_MENU_PRE)
                    break;
#endif
                change_settings();
                break;

            case BUTTON_UP:
            case BUTTON_UP | BUTTON_REPEAT:
                change_volume(1);
                break;

            case BUTTON_DOWN:
            case BUTTON_DOWN | BUTTON_REPEAT:
                change_volume(-1);
                break;

            default:
                if(rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    return PLUGIN_USB_CONNECTED;
                break;
        }
        if (button != BUTTON_NONE)
            lastbutton = button;
    }
}
#endif /* #ifdef HAVE_LCD_BITMAP and HWCODEC */
