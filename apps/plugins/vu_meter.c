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

#if defined(HAVE_LCD_BITMAP)

PLUGIN_HEADER

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define VUMETER_QUIT BUTTON_OFF
#define VUMETER_HELP BUTTON_ON
#define VUMETER_MENU BUTTON_F1
#define VUMETER_MENU_EXIT BUTTON_F1
#define VUMETER_MENU_EXIT2 BUTTON_OFF
#define VUMETER_LEFT BUTTON_LEFT
#define VUMETER_RIGHT BUTTON_RIGHT
#define VUMETER_UP BUTTON_UP
#define VUMETER_DOWN BUTTON_DOWN

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD
#define VUMETER_QUIT BUTTON_OFF
#define VUMETER_HELP BUTTON_ON
#define VUMETER_MENU BUTTON_F1
#define VUMETER_MENU_EXIT BUTTON_F1
#define VUMETER_MENU_EXIT2 BUTTON_OFF
#define VUMETER_LEFT BUTTON_LEFT
#define VUMETER_RIGHT BUTTON_RIGHT
#define VUMETER_UP BUTTON_UP
#define VUMETER_DOWN BUTTON_DOWN

#elif CONFIG_KEYPAD == ONDIO_PAD
#define VUMETER_QUIT BUTTON_OFF
#define VUMETER_HELP_PRE BUTTON_MENU
#define VUMETER_HELP (BUTTON_MENU | BUTTON_REL)
#define VUMETER_MENU_PRE BUTTON_MENU
#define VUMETER_MENU (BUTTON_MENU | BUTTON_REPEAT)
#define VUMETER_MENU_EXIT BUTTON_MENU
#define VUMETER_MENU_EXIT2 BUTTON_OFF
#define VUMETER_LEFT BUTTON_LEFT
#define VUMETER_RIGHT BUTTON_RIGHT
#define VUMETER_UP BUTTON_UP
#define VUMETER_DOWN BUTTON_DOWN

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || \
      (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define VUMETER_QUIT BUTTON_OFF
#define VUMETER_HELP BUTTON_ON
#define VUMETER_MENU BUTTON_SELECT
#define VUMETER_MENU_EXIT BUTTON_SELECT
#define VUMETER_MENU_EXIT2 BUTTON_OFF
#define VUMETER_LEFT BUTTON_LEFT
#define VUMETER_RIGHT BUTTON_RIGHT
#define VUMETER_UP BUTTON_UP
#define VUMETER_DOWN BUTTON_DOWN

#define VUMETER_RC_QUIT BUTTON_RC_STOP

#elif (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_4G_PAD)
#define VUMETER_QUIT BUTTON_MENU
#define VUMETER_HELP BUTTON_PLAY
#define VUMETER_MENU BUTTON_SELECT
#define VUMETER_MENU_EXIT BUTTON_SELECT
#define VUMETER_MENU_EXIT2 BUTTON_MENU
#define VUMETER_LEFT BUTTON_LEFT
#define VUMETER_RIGHT BUTTON_RIGHT
#define VUMETER_UP BUTTON_SCROLL_FWD
#define VUMETER_DOWN BUTTON_SCROLL_BACK

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define VUMETER_QUIT BUTTON_POWER
#define VUMETER_HELP BUTTON_A
#define VUMETER_MENU BUTTON_SELECT
#define VUMETER_MENU_EXIT BUTTON_SELECT
#define VUMETER_MENU_EXIT2 BUTTON_POWER
#define VUMETER_LEFT BUTTON_LEFT
#define VUMETER_RIGHT BUTTON_RIGHT
#define VUMETER_UP BUTTON_UP
#define VUMETER_DOWN BUTTON_DOWN

#elif (CONFIG_KEYPAD == SANSA_E200_PAD)
#define VUMETER_QUIT BUTTON_POWER
#define VUMETER_HELP BUTTON_REC
#define VUMETER_MENU BUTTON_SELECT
#define VUMETER_MENU_EXIT BUTTON_SELECT
#define VUMETER_MENU_EXIT2 BUTTON_POWER
#define VUMETER_LEFT BUTTON_LEFT
#define VUMETER_RIGHT BUTTON_RIGHT
#define VUMETER_UP BUTTON_UP
#define VUMETER_DOWN BUTTON_DOWN

#elif CONFIG_KEYPAD == IAUDIO_X5_PAD
#define VUMETER_QUIT BUTTON_POWER
#define VUMETER_HELP BUTTON_PLAY
#define VUMETER_MENU BUTTON_SELECT
#define VUMETER_MENU_EXIT BUTTON_SELECT
#define VUMETER_MENU_EXIT2 BUTTON_POWER
#define VUMETER_LEFT BUTTON_LEFT
#define VUMETER_RIGHT BUTTON_RIGHT
#define VUMETER_UP BUTTON_UP
#define VUMETER_DOWN BUTTON_DOWN

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define VUMETER_QUIT BUTTON_POWER
#define VUMETER_HELP BUTTON_PLAY
#define VUMETER_MENU BUTTON_REW
#define VUMETER_MENU_EXIT BUTTON_REW
#define VUMETER_MENU_EXIT2 BUTTON_POWER
#define VUMETER_LEFT BUTTON_LEFT
#define VUMETER_RIGHT BUTTON_RIGHT
#define VUMETER_UP BUTTON_SCROLL_UP
#define VUMETER_DOWN BUTTON_SCROLL_DOWN

#endif

const struct plugin_api* rb;

#if SIMULATOR && (CONFIG_CODEC != SWCODEC)
#define mas_codec_readreg(x) rand()%MAX_PEAK
#endif

/* Defines x positions on a logarithmic (dBfs) scale. */
unsigned char analog_db_scale[LCD_WIDTH/2];

/* Define's y positions, to make the needle arch, like a real needle would. */
unsigned char y_values[LCD_WIDTH/2];

const unsigned char digital_db_scale[] =
{0,2,3,5,5,6,6,6,7,7,7,7,8,8,8,8,8,9,9,9,9,9,9,9,9,10,10,
10,10,10,10,10,10,10,10,10,11,11,11,11,11,11,11,11};

const unsigned char needle_cover[] =
{0x18, 0x1c, 0x1c, 0x1e, 0x1e, 0x1f, 0x1f, 0x1f, 0x1e, 0x1e, 0x1c, 0x1c, 0x18};

const unsigned char sound_speaker[] = {0x18,0x24,0x42,0xFF};
const unsigned char sound_low_level[] = {0x24,0x18};
const unsigned char sound_med_level[] = {0x42,0x3C};
const unsigned char sound_high_level[] = {0x81,0x7E};
const unsigned char sound_max_level[] = {0x0E,0xDF,0x0E};

const int half_width = LCD_WIDTH / 2;
const int quarter_width = LCD_WIDTH / 4;
const int half_height = LCD_HEIGHT / 2;

/* approx ratio of the previous hard coded values */
const int analog_mini_1 = (LCD_WIDTH / 2)*0.1;
const int analog_mini_2 = (LCD_WIDTH / 2)*0.25;
const int analog_mini_3 = (LCD_WIDTH / 2)*0.4;
const int analog_mini_4 = (LCD_WIDTH / 2)*0.75;

const int digital_block_width = LCD_WIDTH / 11;
const int digital_block_gap = (int)(LCD_WIDTH / 11) / 10;
/* ammount to lead in on left so 11x blocks are centered - is often 0 */
const int digital_lead = (LCD_WIDTH - (((int)(LCD_WIDTH / 11))*11) ) / 2;

const int digital_block_height = (LCD_HEIGHT - 54) / 2 ;

#define ANALOG 1 /* The two meter types */
#define DIGITAL 2

int left_needle_top_y;
int left_needle_top_x;
int last_left_needle_top_x;
int right_needle_top_y;
int right_needle_top_x;
int last_right_needle_top_x = LCD_WIDTH / 2;

int num_left_leds;
int num_right_leds;
int last_num_left_leds;
int last_num_right_leds;

int i;

#define MAX_PEAK 0x8000

/* gap at the top for left/right etc */
#define NEEDLE_TOP 25

/* pow(M_E, 5) * 65536 */
#define E_POW_5 9726404

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

/* taken from http://www.quinapalus.com/efunc.html */
int fxlog(int x) {
    int t,y;

    y=0xa65af;
    if(x<0x00008000) x<<=16,              y-=0xb1721;
    if(x<0x00800000) x<<= 8,              y-=0x58b91;
    if(x<0x08000000) x<<= 4,              y-=0x2c5c8;
    if(x<0x20000000) x<<= 2,              y-=0x162e4;
    if(x<0x40000000) x<<= 1,              y-=0x0b172;
    t=x+(x>>1); if((t&0x80000000)==0) x=t,y-=0x067cd;
    t=x+(x>>2); if((t&0x80000000)==0) x=t,y-=0x03920;
    t=x+(x>>3); if((t&0x80000000)==0) x=t,y-=0x01e27;
    t=x+(x>>4); if((t&0x80000000)==0) x=t,y-=0x00f85;
    t=x+(x>>5); if((t&0x80000000)==0) x=t,y-=0x007e1;
    t=x+(x>>6); if((t&0x80000000)==0) x=t,y-=0x003f8;
    t=x+(x>>7); if((t&0x80000000)==0) x=t,y-=0x001fe;
    x=0x80000000-x;
    y-=x>>15;
    return y;
}

/*
 * Integer square root routine, good for up to 32-bit values.
 * Note that the largest square root (that of 0xffffffff) is
 * 0xffff, so the result fits in a regular unsigned and need
 * not be `long'.
 *
 * Original code from Tomas Rokicki (using a well known algorithm).
 * This version by Chris Torek, University of Maryland.
 *
 * This code is in the public domain.
 */
unsigned int root(unsigned long v)
{
        register unsigned long t = 1L << 30, r = 0, s;  /* 30 = 15*2 */

#define STEP(k) \
        s = t + r; \
        r >>= 1; \
        if (s <= v) { \
                v -= s; \
                r |= t; \
        }
        STEP(15); t >>= 2;
        STEP(14); t >>= 2;
        STEP(13); t >>= 2;
        STEP(12); t >>= 2;
        STEP(11); t >>= 2;
        STEP(10); t >>= 2;
        STEP(9); t >>= 2;
        STEP(8); t >>= 2;
        STEP(7); t >>= 2;
        STEP(6); t >>= 2;
        STEP(5); t >>= 2;
        STEP(4); t >>= 2;
        STEP(3); t >>= 2;
        STEP(2); t >>= 2;
        STEP(1); t >>= 2;
        STEP(0);
        return r;

}

void calc_scales(void)
{
    unsigned int fx_log_factor = E_POW_5/half_width;
    unsigned int y,z;

    for (i=1; i <= half_width; i++)
    {
        y = (half_width/5)*fxlog(i*fx_log_factor);

        /* better way of checking for negative values? */
        z = y>>16;
        if (z > LCD_WIDTH)
            z = 0;

        analog_db_scale[i-1] = z;
        /* play nice */
        rb->yield();
    }

    long j;
    long k;
    unsigned int l;
    int nh = LCD_HEIGHT - NEEDLE_TOP;
    long nh2 = nh*nh;
    for (i=1; i<=half_width; i++)
    {
        j = i - (int)(half_width/2);
        k = nh2 - ( j * j );
        /* +1 seems to give a closer approximation */
        l = root(k) + 1;
        l = LCD_HEIGHT - l;

        y_values[i-1] = l;
        rb->yield();
    }

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
    int fp = rb->creat("/.rockbox/rocks/.vu_meter");
    if(fp >= 0) {
        rb->write (fp, &settings, sizeof(struct saved_settings));
        rb->close(fp);
    }
}

void change_volume(int delta) {
    char curr_vol[5];
    int minvol = rb->sound_min(SOUND_VOLUME);
    int maxvol = rb->sound_max(SOUND_VOLUME);
    int vol = rb->global_settings->volume + delta;

    if (vol > maxvol) vol = maxvol;
    else if (vol < minvol) vol = minvol;
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

            case VUMETER_LEFT:
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

            case VUMETER_RIGHT:
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

            case VUMETER_UP:
                selected_setting == 3 ? selected_setting=0 : selected_setting++;
                break;

            case VUMETER_DOWN:
                selected_setting == 0 ? selected_setting=3 : selected_setting--;
        }
    }
}

void draw_analog_minimeters(void) {
    rb->lcd_mono_bitmap(sound_speaker, quarter_width-28, 12, 4, 8);
    rb->lcd_set_drawmode(DRMODE_FG);
    if(analog_mini_1<left_needle_top_x)
        rb->lcd_mono_bitmap(sound_low_level, quarter_width-23, 12, 2, 8);
    if(analog_mini_2<left_needle_top_x)
        rb->lcd_mono_bitmap(sound_med_level, quarter_width-21, 12, 2, 8);
    if(analog_mini_3<left_needle_top_x)
        rb->lcd_mono_bitmap(sound_high_level, quarter_width-19, 12, 2, 8);
    if(analog_mini_4<left_needle_top_x)
        rb->lcd_mono_bitmap(sound_max_level, quarter_width-16, 12, 3, 8);

    rb->lcd_set_drawmode(DRMODE_SOLID);
    rb->lcd_mono_bitmap(sound_speaker, quarter_width+half_width-30, 12, 4, 8);
    rb->lcd_set_drawmode(DRMODE_FG);
    if(analog_mini_1<(right_needle_top_x-half_width))
        rb->lcd_mono_bitmap(sound_low_level, quarter_width+half_width-25, 12, 2, 8);
    if(analog_mini_2<(right_needle_top_x-half_width))
        rb->lcd_mono_bitmap(sound_med_level, quarter_width+half_width-23, 12, 2, 8);
    if(analog_mini_3<(right_needle_top_x-half_width))
        rb->lcd_mono_bitmap(sound_high_level, quarter_width+half_width-21, 12, 2, 8);
    if(analog_mini_4<(right_needle_top_x-half_width))
        rb->lcd_mono_bitmap(sound_max_level, quarter_width+half_width-18, 12, 3, 8);
    rb->lcd_set_drawmode(DRMODE_SOLID);
}

void draw_digital_minimeters(void) {
    rb->lcd_mono_bitmap(sound_speaker, 34, half_height-8, 4, 8);
    rb->lcd_set_drawmode(DRMODE_FG);
    if(1<num_left_leds)
        rb->lcd_mono_bitmap(sound_low_level, 39, half_height-8, 2, 8);
    if(2<num_left_leds)
        rb->lcd_mono_bitmap(sound_med_level, 41, half_height-8, 2, 8);
    if(5<num_left_leds)
        rb->lcd_mono_bitmap(sound_high_level, 43, half_height-8, 2, 8);
    if(8<num_left_leds)
        rb->lcd_mono_bitmap(sound_max_level, 46, half_height-8, 3, 8);

    rb->lcd_set_drawmode(DRMODE_SOLID);
    rb->lcd_mono_bitmap(sound_speaker, 34, half_height+8, 4, 8);
    rb->lcd_set_drawmode(DRMODE_FG);
    if(1<(num_right_leds))
        rb->lcd_mono_bitmap(sound_low_level, 39, half_height+8, 2, 8);
    if(2<(num_right_leds))
        rb->lcd_mono_bitmap(sound_med_level, 41, half_height+8, 2, 8);
    if(5<(num_right_leds))
        rb->lcd_mono_bitmap(sound_high_level, 43, half_height+8, 2, 8);
    if(8<(num_right_leds))
        rb->lcd_mono_bitmap(sound_max_level, 46, half_height+8, 3, 8);
    rb->lcd_set_drawmode(DRMODE_SOLID);
}

void analog_meter(void) {

#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    int left_peak = rb->mas_codec_readreg(0xC);
    int right_peak = rb->mas_codec_readreg(0xD);
#elif (CONFIG_CODEC == SWCODEC)
    int left_peak, right_peak;
    rb->pcm_calculate_peaks(&left_peak, &right_peak);
#endif

    if(settings.analog_use_db_scale) {
        left_needle_top_x = analog_db_scale[left_peak * half_width / MAX_PEAK];
        right_needle_top_x = analog_db_scale[right_peak * half_width / MAX_PEAK] + half_width;
    }
    else {
        left_needle_top_x = left_peak * half_width / MAX_PEAK;
        right_needle_top_x = right_peak * half_width / MAX_PEAK + half_width;
    }

    /* Makes a decay on the needle */
    left_needle_top_x = (left_needle_top_x+last_left_needle_top_x*settings.analog_decay)/(settings.analog_decay+1);
    right_needle_top_x = (right_needle_top_x+last_right_needle_top_x*settings.analog_decay)/(settings.analog_decay+1);

    last_left_needle_top_x = left_needle_top_x;
    last_right_needle_top_x = right_needle_top_x;

    left_needle_top_y = y_values[left_needle_top_x];
    right_needle_top_y = y_values[right_needle_top_x-half_width];

    /* Needles */
    rb->lcd_drawline(quarter_width, LCD_HEIGHT-1, left_needle_top_x, left_needle_top_y);
    rb->lcd_drawline((quarter_width+half_width), LCD_HEIGHT-1, right_needle_top_x, right_needle_top_y);

    if(settings.analog_minimeters)
        draw_analog_minimeters();

    /* Needle covers */
    rb->lcd_set_drawmode(DRMODE_FG);
    rb->lcd_mono_bitmap(needle_cover, quarter_width-6, LCD_HEIGHT-5, 13, 5);
    rb->lcd_mono_bitmap(needle_cover, half_width+quarter_width-6, LCD_HEIGHT-5, 13, 5);
    rb->lcd_set_drawmode(DRMODE_SOLID);

    /* Show Left/Right */
    rb->lcd_putsxy(quarter_width-12, 12, "Left");
    rb->lcd_putsxy(half_width+quarter_width-12, 12, "Right");

    /* Line above/below  the Left/Right text */
    rb->lcd_drawline(0,9,LCD_WIDTH-1,9);
    rb->lcd_drawline(0,21,LCD_WIDTH-1,21);

    for(i=0; i<half_width; i++) {
        rb->lcd_drawpixel(i, (y_values[i])-2);
        rb->lcd_drawpixel(i+half_width, (y_values[i])-2);
    }
}

void digital_meter(void) {
#if (CONFIG_CODEC == MAS3587F) || (CONFIG_CODEC == MAS3539F)
    int left_peak = rb->mas_codec_readreg(0xC);
    int right_peak = rb->mas_codec_readreg(0xD);
#elif (CONFIG_CODEC == SWCODEC)
    int left_peak, right_peak;
    rb->pcm_calculate_peaks(&left_peak, &right_peak);
#endif

    if(settings.digital_use_db_scale) {
        num_left_leds = digital_db_scale[left_peak * 44 / MAX_PEAK];
        num_right_leds = digital_db_scale[right_peak * 44 / MAX_PEAK];
    }
    else {
        num_left_leds = left_peak * 11 / MAX_PEAK;
        num_right_leds = right_peak * 11 / MAX_PEAK;
    }

    num_left_leds = (num_left_leds+last_num_left_leds*settings.digital_decay)/(settings.digital_decay+1);
    num_right_leds = (num_right_leds+last_num_right_leds*settings.digital_decay)/(settings.digital_decay+1);

    last_num_left_leds = num_left_leds;
    last_num_right_leds = num_right_leds;

    rb->lcd_set_drawmode(DRMODE_FG);
    /* LEDS */
    for(i=0; i<num_left_leds; i++)
        rb->lcd_fillrect((digital_lead + (i*digital_block_width)),
            14, digital_block_width - digital_block_gap, digital_block_height);

    for(i=0; i<num_right_leds; i++)
        rb->lcd_fillrect((digital_lead + (i*digital_block_width)),
            (half_height + 20), digital_block_width - digital_block_gap, 
            digital_block_height);

    rb->lcd_set_drawmode(DRMODE_SOLID);

    if(settings.digital_minimeters)
        draw_digital_minimeters();

    /* Lines above/below where the LEDS are */
    rb->lcd_drawline(0,12,LCD_WIDTH-1,12);
    rb->lcd_drawline(0,half_height-12,LCD_WIDTH-1,half_height-12);

    rb->lcd_drawline(0,half_height+18,LCD_WIDTH-1,half_height+18);
    rb->lcd_drawline(0,LCD_HEIGHT-6,LCD_WIDTH-1,LCD_HEIGHT-6);

    /* Show Left/Right */
    rb->lcd_putsxy(2, half_height-8, "Left");
    rb->lcd_putsxy(2, half_height+8, "Right");

    /* Line in the middle */
    rb->lcd_drawline(0,half_height+3,LCD_WIDTH-1,half_height+3);
}

enum plugin_status plugin_start(struct plugin_api* api, void* parameter) {
    int button;
    int lastbutton = BUTTON_NONE;

    (void) parameter;
    rb = api;

    calc_scales();

    load_settings();
    rb->lcd_setfont(FONT_SYSFIXED);

    while (1)
    {
        rb->lcd_clear_display();

        rb->lcd_putsxy(half_width-23, 0, "VU Meter");

        if(settings.meter_type==ANALOG)
            analog_meter();
        else
            digital_meter();

        rb->lcd_update();

        button = rb->button_get_w_tmo(1);
        switch (button)
        {
#ifdef VUMETER_RC_QUIT
            case VUMETER_RC_QUIT:
#endif
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

            case VUMETER_UP:
            case VUMETER_UP | BUTTON_REPEAT:
                change_volume(1);
                break;

            case VUMETER_DOWN:
            case VUMETER_DOWN | BUTTON_REPEAT:
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
#endif /* #ifdef HAVE_LCD_BITMAP */
