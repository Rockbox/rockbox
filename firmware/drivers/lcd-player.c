/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "hwcompat.h"

#ifdef HAVE_LCD_CHARCELLS

#include "lcd.h"
#include "kernel.h"
#include "thread.h"
#include <string.h>
#include <stdlib.h>
#include "file.h"
#include "debug.h"
#include "system.h"
#include "font.h"
#include "lcd-player-charset.h"

/*** definitions ***/

#define OLD_LCD_CONTRAST_SET ((char)0xA8)
#define OLD_LCD_CRAM         ((char)0xB0) /* Characters */
#define OLD_LCD_PRAM         ((char)0x80) /*  Patterns  */
#define OLD_LCD_IRAM         ((char)0xE0) /*    Icons   */

#define NEW_LCD_CONTRAST_SET ((char)0x50)
#define NEW_LCD_CRAM         ((char)0x80) /* Characters */
#define NEW_LCD_PRAM         ((char)0xC0) /*  Patterns  */
#define NEW_LCD_IRAM         ((char)0x40) /*    Icons   */
#define NEW_LCD_FUNCTION_SET                    ((char)0x10)
#define NEW_LCD_POWER_SAVE_MODE_OSC_CONTROL_SET ((char)0x0c)
#define NEW_LCD_POWER_CONTROL_REGISTER_SET      ((char)0x20)
#define NEW_LCD_DISPLAY_CONTROL_SET             ((char)0x28)

#define LCD_CURSOR(x,y)      ((char)(lcd_cram+((y)*16+(x))))
#define LCD_ICON(i)          ((char)(lcd_iram+i))

#define SCROLLABLE_LINES 2

#define SCROLL_MODE_OFF   0
#define SCROLL_MODE_PAUSE 1
#define SCROLL_MODE_RUN   2

extern unsigned short new_lcd_rocklatin1_to_xlcd[];
extern unsigned short old_lcd_rocklatin1_to_xlcd[];
extern const unsigned char lcd_player_extended_lcd_to_rocklatin1[];
extern unsigned char extended_font_player[NO_EXTENDED_LCD_CHARS][8];

/*** generic code ***/

struct scrollinfo {
    int mode;
    char text[MAX_PATH];
    int textlen;
    int offset;
    int turn_offset;
    int startx;
    int starty;
    long scroll_start_tick;
    int direction; /* +1 for right or -1 for left*/
    int jump_scroll;
    int jump_scroll_steps;
};

#define MAX_CURSOR_CHARS 8
struct cursorinfo {
    int len;
    char text[MAX_CURSOR_CHARS];
    int textpos;
    int y_pos;
    int x_pos;
    int divider;
    int downcount;
} cursor;

static void scroll_thread(void);
static char scroll_stack[DEFAULT_STACK_SIZE];
static const char scroll_name[] = "scroll";
static char scroll_ticks = 12; /* # of ticks between updates */
static int scroll_delay = HZ/2; /* delay before starting scroll */
static int jump_scroll_delay = HZ/4; /* delay between jump scroll jumps */
static char scroll_spacing = 3; /* spaces between end and start of text */
static int bidir_limit = 50;  /* percent */
static int jump_scroll = 0; /* 0=off, 1=once, ..., JUMP_SCROLL_ALWAYS */

static struct scrollinfo scroll[SCROLLABLE_LINES];

static char extended_chars_mapped[NO_EXTENDED_LCD_CHARS];
static char extended_pattern_content[8]; /* Which char is mapped in pattern */
static char extended_pattern_usage[8]; /* Counting number of times used */
static char pattern_size; /* Last pattern, 3 for old LCD, 7 for new LCD */

static bool new_lcd;

unsigned short *lcd_ascii;
static char lcd_contrast_set;
static char lcd_cram;
static char lcd_pram;
static char lcd_iram;

unsigned short buffer_xlcd[11][2];
unsigned short buffer_lcd_mirror[11][2];

#ifdef SIMULATOR
unsigned char hardware_buffer_lcd[11][2];
#else
static unsigned char lcd_data_byte; /* global write buffer */
#endif

#define NO_CHAR -1

static void lcd_free_pat(int map_ch)
{
    int x, y;
    unsigned char substitute_char;

    int pat;
    pat=extended_chars_mapped[map_ch];
    if (pat!=NO_CHAR) {

        substitute_char=lcd_player_extended_lcd_to_rocklatin1[map_ch];

        /* TODO: use a define for the screen width! */
        for (x=0; x<11; x++) {
            /* TODO: use a define for the screen height! */
            for (y=0; y<2; y++)  {
                if (map_ch==lcd_ascii[buffer_xlcd[x][y]]-512) {
                    buffer_xlcd[x][y]=substitute_char;
                    buffer_lcd_mirror[x][y]=substitute_char;
#ifdef SIMULATOR
                    hardware_buffer_lcd[x][y]=substitute_char;
#else
                    lcd_write_command(LCD_CURSOR(x, y));
                    lcd_write_data(&substitute_char, 1);
#endif
                }
            }
        }
        extended_chars_mapped[map_ch]=NO_CHAR;
        extended_pattern_content[pat]=NO_CHAR;
        extended_pattern_usage[pat]=0;
    }
#ifdef SIMULATOR
    lcd_update();
#endif
}

static int lcd_get_free_pat(int ch)
{
    int pat;
    int last_pat=0;
    static int last_used_pat=0;
    int loop;

    pat=last_used_pat;
    for (loop=0; loop<=pattern_size; loop++) {
        pat=(pat+1)&pattern_size; /* Keep 'pat' within limits */
        if (extended_pattern_usage[pat]==0) {
            int map_ch=extended_pattern_content[pat];
            if (map_ch != NO_CHAR) {
                extended_chars_mapped[map_ch]=NO_CHAR;
                extended_pattern_content[pat]=NO_CHAR;
            }
            last_used_pat=pat;
            return pat;
        }
        if (extended_pattern_content[pat]>extended_pattern_content[last_pat])
            last_pat=pat;
    }
    if (ch<32) { /* Prioritized char */
        /* Remove last_pat */
        lcd_free_pat(extended_pattern_content[last_pat]);
        last_used_pat=last_pat;
        return last_pat;
    }
    return NO_CHAR;

}

void xlcd_update(void)
{
    int x, y;
    for (x=0; x<11; x++) {
        for (y=0; y<2; y++)  {
            unsigned short ch=buffer_xlcd[x][y];
            unsigned char hw_ch=0xff;
            if (ch==buffer_lcd_mirror[x][y])
                continue; /* No need to redraw */
            buffer_lcd_mirror[x][y]=ch;
            if (ch>=256 && ch<512) {
                hw_ch=ch-256;
            } else {
                int map_ch=lcd_ascii[ch];
                if (map_ch<512) {
                    hw_ch=map_ch;
                }
                else {
                    map_ch=map_ch-512;
                    if (extended_chars_mapped[map_ch]!=NO_CHAR) {
                        hw_ch=extended_chars_mapped[map_ch];
                        extended_pattern_usage[hw_ch]++;
                    }
                    else {
                        int pat;
                        pat=lcd_get_free_pat(map_ch);
                        if (pat<0) {
                            DEBUGF("Substitute for %02x (map 0x%02x)"
                                   " is used.\n", ch, map_ch);
                            /* Find substitute char */
                            map_ch=
                                lcd_player_extended_lcd_to_rocklatin1[map_ch];
                            hw_ch=lcd_ascii[map_ch];
                        } else {
#ifdef DEBUG
                            if (extended_pattern_usage[pat]!=0) {
                                DEBUGF("***Pattern %d is not zero!\n", pat);
                            }
#endif
                            extended_chars_mapped[map_ch]=pat;
                            extended_pattern_content[pat]=map_ch;
                            extended_pattern_usage[pat]=1;
                            lcd_define_hw_pattern(pat*8, 
                                                  extended_font_player[map_ch],
                                                  8);
                            hw_ch=pat;
                        }
                    }
                }
            }
#ifdef SIMULATOR
            hardware_buffer_lcd[x][y]=hw_ch;
#else
            lcd_write_command(LCD_CURSOR(x,y));
            lcd_write_data(&hw_ch, 1);
#endif
        }
    }
    lcd_update();
}

bool lcdx_putc(int x, int y, unsigned short ch)
{
    int lcd_char;
    if (buffer_xlcd[x][y]==ch)
        return false; /* Same char, ignore any update */
    lcd_char=lcd_ascii[buffer_xlcd[x][y]];
    if (lcd_char>=512) {
        /* The removed char is a defined pattern, count down the reference. */
        extended_pattern_usage[(int)extended_chars_mapped[lcd_char-512]]--;
#ifdef DEBUG
        if (extended_pattern_usage[(int)extended_chars_mapped[lcd_char]]<0) {
            DEBUGF("**** Mapped char %02x is less than 0!\n", lcd_char);
        }
#endif
    }

    buffer_xlcd[x][y]=ch;

    lcd_char=lcd_ascii[ch];
    if (lcd_char>=256)
        return true; /* Caller shall call xlcd_update() when done */

    buffer_lcd_mirror[x][y]=lcd_char;
#ifdef SIMULATOR
    hardware_buffer_lcd[x][y]=lcd_char;
#else
    lcd_data_byte = (unsigned char) lcd_char;
    lcd_write_command(LCD_CURSOR(x, y));
    lcd_write_data(&lcd_data_byte, 1);
#endif
    return false;
}

int lcd_default_contrast(void)
{
    return 30;
}

void lcd_clear_display(void)
{
    int i;
    bool update=false; 
    DEBUGF("lcd_clear_display()\n");
    lcd_stop_scroll();
    cursor.len=0; /* Stop cursor */
    for (i=0;i<22;i++)
        update|=lcdx_putc(i%11, i/11, ' ');
    if (update)
        xlcd_update();
}

static void lcd_puts_cont_scroll(int x, int y, const unsigned char *string)
{
    bool update=false; 
    DEBUGF("lcd_puts_cont_scroll(%d, %d, \"", x, y);

    for (; *string && x<11; x++)
    {
#ifdef DEBUGF
        if (*string>=32 && *string<128)
        {
            DEBUGF("%c", *string);
        }
        else
        {
            DEBUGF("(0x%02x)", *string);
        }
#endif
        /* We should check if char is over 256 */
        update|=lcdx_putc(x, y, *(unsigned char*)string++);
    }
    DEBUGF("\")\n");
    
    for (; x<11; x++)
        update|=lcdx_putc(x, y, ' ');
    if (update)
        xlcd_update();
#ifdef SIMULATOR
    lcd_update();
#endif
}
void lcd_puts(int x, int y, const unsigned char *string)
{
    DEBUGF("lcd_puts(%d, %d) -> ", x, y);
    scroll[y].mode=SCROLL_MODE_OFF;
    return lcd_puts_cont_scroll(x, y, string);
}

void lcd_put_cursor(int x, int y, char cursor_char)
{
    if (cursor.len == 0) {
        cursor.text[0]=buffer_xlcd[x][y];
        cursor.text[1]=cursor_char;
        cursor.len=2;
        cursor.textpos=0;
        cursor.y_pos=y;
        cursor.x_pos=x;
        cursor.downcount=0;
        cursor.divider=4;
    }
}

void lcd_remove_cursor(void)
{
    if (cursor.len!=0) {
        bool up;
        cursor.len=0;
        up = lcdx_putc(cursor.x_pos, cursor.y_pos, cursor.text[0]);
#ifdef SIMULATOR
        if(up)
            lcd_update();
#endif
    }
}

void lcd_putc(int x, int y, unsigned short ch)
{
    bool update;
    DEBUGF("lcd_putc(%d, %d, %d '0x%02x')\n", x, y, ch, ch);
    if (x<0 || y<0) {
        return;
    }
    update=lcdx_putc(x, y, ch);

    if (update)
        xlcd_update();
}

unsigned char lcd_get_locked_pattern(void)
{
    unsigned char pat=1;
    while (pat<LAST_RESERVED_CHAR) {
        if (lcd_ascii[pat]==RESERVED_CHAR) {
            lcd_ascii[pat]=0x200+pat;
            return pat;
        }
        pat++;
    }
    return 0;
}

void lcd_unlock_pattern(unsigned char pat)
{
    lcd_ascii[pat]=RESERVED_CHAR;
    lcd_free_pat(pat);
}

void lcd_define_pattern(int pat, const char *pattern)
{
    int i;
    for (i=0; i<7; i++) {
        extended_font_player[pat][i]=pattern[i];
    }
    if (extended_chars_mapped[pat]!=NO_CHAR) {
        lcd_define_hw_pattern(extended_chars_mapped[pat]*8, pattern, 7);
    }
}

#ifndef SIMULATOR
void lcd_define_hw_pattern (int which,const char *pattern,int length)
{
    lcd_write_command(lcd_pram | which);
    lcd_write_data(pattern, length);
}

void lcd_double_height(bool on)
{
    if(new_lcd)
        lcd_write_command(on?9:8);
}

static const char icon_pos[] =
{
    0, 0, 0, 0, /* Battery */
    2, /* USB */
    3, /* Play */
    4, /* Record */
    5, /* Pause */
    5, /* Audio */
    6, /* Repeat */
    7, /* 1 */
    9,  /* Volume */
    9,  /* Volume 1 */
    9,  /* Volume 2 */
    10, /* Volume 3 */
    10, /* Volume 4 */
    10, /* Volume 5 */
    10, /* Param */
};

static const char icon_mask[] =
{
    0x02, 0x08, 0x04, 0x10, /* Battery */
    0x04, /* USB */
    0x10, /* Play */
    0x10, /* Record */
    0x02, /* Pause */
    0x10, /* Audio */
    0x02, /* Repeat */
    0x01, /* 1 */
    0x04, /* Volume */
    0x02, /* Volume 1 */
    0x01, /* Volume 2 */
    0x08, /* Volume 3 */
    0x04, /* Volume 4 */
    0x01, /* Volume 5 */
    0x10, /* Param */
};

void lcd_icon(int icon, bool enable)
{
    static unsigned char icon_mirror[11] = {0};
    int pos, mask;

    pos = icon_pos[icon];
    mask = icon_mask[icon];
    
    lcd_write_command(LCD_ICON(pos));
    
    if(enable)
        icon_mirror[pos] |= mask;
    else
        icon_mirror[pos] &= ~mask;
    
    lcd_write_data(&icon_mirror[pos], 1);
}

void lcd_set_contrast(int val)
{
    lcd_data_byte = (unsigned char) (31 - val);
    lcd_write_command(lcd_contrast_set);
    lcd_write_data(&lcd_data_byte, 1);
}
#endif /* SIMULATOR */

void lcd_init (void)
{
    unsigned char data_vector[64];
    
    (void)data_vector;

    new_lcd = is_new_player();
    memset(extended_chars_mapped, NO_CHAR, sizeof(extended_chars_mapped));
    memset(extended_pattern_content, NO_CHAR,sizeof(extended_pattern_content));
    memset(extended_pattern_usage, 0, sizeof(extended_pattern_usage));

    if(new_lcd) {
        lcd_ascii = new_lcd_rocklatin1_to_xlcd;
        lcd_contrast_set = NEW_LCD_CONTRAST_SET;
        lcd_cram = NEW_LCD_CRAM;
        lcd_pram = NEW_LCD_PRAM;
        lcd_iram = NEW_LCD_IRAM;
        pattern_size=7; /* Last pattern, 7 for new LCD */

#ifndef SIMULATOR
        /* LCD init for cold start */
        PBCR2 &= 0xff00;                 /* Set PB0..PB3 to GPIO */
        or_b(0x0f, &PBIORL);             /* ... output */
        or_b(0x0f, &PBDRL);              /* ... and high */

        lcd_write_command(NEW_LCD_FUNCTION_SET + 1); /* CGRAM selected */
        lcd_write_command(NEW_LCD_CONTRAST_SET);
        lcd_data_byte = 0x08;
        lcd_write_data(&lcd_data_byte, 1);
        lcd_write_command(NEW_LCD_POWER_SAVE_MODE_OSC_CONTROL_SET + 2);
                                         /* oscillator on */
        lcd_write_command(NEW_LCD_POWER_CONTROL_REGISTER_SET + 7);
                                         /* opamp buffer + voltage booster on*/

        memset(data_vector, 0x20, 64);
        lcd_write_command(NEW_LCD_CRAM); /* Set DDRAM address */
        lcd_write_data(data_vector, 64); /* all spaces */

        memset(data_vector, 0, 64);
        lcd_write_command(NEW_LCD_PRAM); /* Set CGRAM address */
        lcd_write_data(data_vector, 64); /* zero out */
        lcd_write_command(NEW_LCD_IRAM); /* Set ICONRAM address */
        lcd_write_data(data_vector, 16); /* zero out */

        lcd_write_command(NEW_LCD_DISPLAY_CONTROL_SET + 1); /* display on */
#endif /* !SIMULATOR */
    }
    else {
        lcd_ascii = old_lcd_rocklatin1_to_xlcd;
        lcd_contrast_set = OLD_LCD_CONTRAST_SET;
        lcd_cram = OLD_LCD_CRAM;
        lcd_pram = OLD_LCD_PRAM;
        lcd_iram = OLD_LCD_IRAM;
        pattern_size=3; /* Last pattern, 3 for old LCD */

#ifndef SIMULATOR
#if 1
        /* LCD init for cold start */
        PBCR2 &= 0xff00;                 /* Set PB0..PB3 to GPIO */
        or_b(0x0f, &PBIORL);             /* ... output */
        or_b(0x0f, &PBDRL);              /* ... and high */

        lcd_write_command(0x61);
        lcd_write_command(0x42);
        lcd_write_command(0x57);

        memset(data_vector, 0x24, 13);
        lcd_write_command(OLD_LCD_CRAM); /* Set DDRAM address */
        lcd_write_data(data_vector, 13); /* all spaces */
        lcd_write_command(OLD_LCD_CRAM + 0x10);
        lcd_write_data(data_vector, 13);
        lcd_write_command(OLD_LCD_CRAM + 0x20);
        lcd_write_data(data_vector, 13);

        memset(data_vector, 0, 32);
        lcd_write_command(OLD_LCD_PRAM); /* Set CGRAM address */
        lcd_write_data(data_vector, 32); /* zero out */
        lcd_write_command(OLD_LCD_IRAM); /* Set ICONRAM address */
        lcd_write_data(data_vector, 13); /* zero out */
        lcd_write_command(OLD_LCD_IRAM + 0x10);
        lcd_write_data(data_vector, 13);

        lcd_write_command(0x31);
#else  
        /* archos look-alike code, left here for reference. As soon as the
         * rockbox version is confirmed working, this will go away */
        {
        int i;

        PBCR2 &= 0xc000;
        PBIOR |= 0x000f;
        PBDR |= 0x0002;
        PBDR |= 0x0001;
        PBDR |= 0x0004;
        PBDR |= 0x0008;

        for (i=0; i<200; i++) asm volatile ("nop"); /* wait 100 us */

        PBDR &= 0xfffd; /* CS low (assert) */

        for (i=0; i<100; i++) asm volatile ("nop"); /* wait 50 us */

        lcd_write_command(0x61);
        lcd_write_command(0x42);
        lcd_write_command(0x57);

        memset(data_vector, 0x24, 13);
        lcd_write_command(0xb0);         /* Set DDRAM address */
        lcd_write_data(data_vector, 13); /* all spaces */
        lcd_write_command(0xc0);
        lcd_write_data(data_vector, 13);
        lcd_write_command(0xd0);
        lcd_write_data(data_vector, 13);

        memset(data_vector, 0, 32);
        lcd_write_command(0x80);         /* Set CGRAM address */
        lcd_write_data(data_vector, 32); /* zero out */
        lcd_write_command(0xe0);         /* Set ICONRAM address */
        lcd_write_data(data_vector, 13); /* zero out */
        lcd_write_command(0xf0);
        lcd_write_data(data_vector, 13);

        for (i=0; i<300000; i++) asm volatile ("nop"); /* wait 150 ms */

        lcd_write_command(0x31);
        lcd_write_command(0xa8);         /* Set contrast control */
        lcd_data_byte = 0;
        lcd_write_data(&lcd_data_byte, 1); /* 0 */
        }
#endif
#endif /* !SIMULATOR */
    }
    
    lcd_set_contrast(lcd_default_contrast());

    create_thread(scroll_thread, scroll_stack,
                  sizeof(scroll_stack), scroll_name);
}

void lcd_jump_scroll (int mode) /* 0=off, 1=once, ..., JUMP_SCROLL_ALWAYS */
{
    jump_scroll=mode;
}

void lcd_bidir_scroll(int percent)
{
    bidir_limit = percent;
}

void lcd_puts_scroll(int x, int y, const unsigned char* string )
{
    struct scrollinfo* s;
    int i;

    DEBUGF("lcd_puts_scroll(%d, %d, %s)\n", x, y, string);

    s = &scroll[y];

    lcd_puts_cont_scroll(x,y,string);
    s->textlen = strlen(string);

    if ( s->textlen > 11-x ) {
        s->mode = SCROLL_MODE_RUN;
        s->scroll_start_tick = current_tick + scroll_delay;
        s->offset=0;
        s->startx=x;
        s->starty=y;
        s->direction=+1;
        s->jump_scroll=0;
        s->jump_scroll_steps=0;
        if (jump_scroll && jump_scroll_delay<scroll_ticks*(s->textlen-11+x)) {
            s->jump_scroll_steps=11-x;
            s->jump_scroll=jump_scroll;
        }
        strncpy(s->text,string,sizeof s->text);
        s->turn_offset=-1;
        if (bidir_limit && (s->textlen < ((11-x)*(100+bidir_limit))/100)) {
            s->turn_offset=s->textlen+x-11;
        }
        else {
            for (i=0; i<scroll_spacing &&
                     s->textlen<(int)sizeof(s->text); i++) {
                s->text[s->textlen++]=' ';
            }
        }
        if (s->textlen<(int)sizeof(s->text))
            s->text[s->textlen]=' ';
        s->text[sizeof s->text - 1] = 0;
    }
    else
        s->mode = SCROLL_MODE_OFF;
}

void lcd_stop_scroll(void)
{
    struct scrollinfo* s;
    int index;

    for ( index = 0; index < SCROLLABLE_LINES; index++ ) {
        s = &scroll[index];
        if ( s->mode == SCROLL_MODE_RUN ||
             s->mode == SCROLL_MODE_PAUSE ) {
            /* restore scrolled row */
            lcd_puts(s->startx, s->starty, s->text);
        }
    }

    lcd_update();
}

static const char scroll_tick_table[16] = {
 /* Hz values:
    1, 1.25, 1.55, 2, 2.5, 3.12, 4, 5, 6.25, 8.33, 10, 12.5, 16.7, 20, 25, 33 */
    100, 80, 64, 50, 40, 32, 25, 20, 16, 12, 10, 8, 6, 5, 4, 3
};

void lcd_scroll_speed(int speed)
{
    scroll_ticks = scroll_tick_table[speed];
}

void lcd_scroll_delay(int ms)
{
    scroll_delay = ms / (HZ / 10);
    DEBUGF("scroll_delay=%d (ms=%d, HZ=%d)\n", scroll_delay, ms, HZ);
}

void lcd_jump_scroll_delay(int ms)
{
    jump_scroll_delay = ms / (HZ / 10);
    DEBUGF("jump_scroll_delay=%d (ms=%d, HZ=%d)\n", jump_scroll_delay, ms, HZ);
}

static void scroll_thread(void)
{
    struct scrollinfo* s;
    int index;
    int i, o;
    bool update;

    /* initialize scroll struct array */
    for (index = 0; index < SCROLLABLE_LINES; index++) {
        scroll[index].mode = SCROLL_MODE_OFF;
    }

    while ( 1 ) {

        update = false;

        for ( index = 0; index < SCROLLABLE_LINES; index++ ) {
            s = &scroll[index];
            if ( s->mode == SCROLL_MODE_RUN ) {
                if ( TIME_AFTER(current_tick, s->scroll_start_tick) ) {
                    char buffer[12];
                    int jumping_scroll=s->jump_scroll;
                    update = true;
                    if (s->jump_scroll) {

                        /* Find new position to start jump scroll by
                         * finding last white space within
                         * jump_scroll_steps */
                        int i;
                        o = s->offset = s->offset + s->jump_scroll_steps;
                        for (i = 0; i < s->jump_scroll_steps; i++, o--) {
                            if (o < s->textlen &&
                                ((0x20 <= s->text[o] && s->text[o] <= 0x2f) || s->text[o] == '_'))
                            {
                                s->offset = o;
                                break;
                            }
                        }

                        s->scroll_start_tick = current_tick +
                            jump_scroll_delay;
                        /* Eat space */
                        while (s->offset < s->textlen &&
                               ((0x20 <= s->text[s->offset] && s->text[s->offset] <= 0x2f) ||
                                s->text[s->offset] == '_')) {
                            s->offset++;
                        }
                        if (s->offset >= s->textlen) {
                            s->offset=0;
                            s->scroll_start_tick = current_tick +
                                scroll_delay;
                            if (s->jump_scroll != JUMP_SCROLL_ALWAYS) {
                                s->jump_scroll--;
                                s->direction=1;
                            }
                        }
                    } else {
                        if ( s->offset < s->textlen-1 ) {
                            s->offset+=s->direction;
                            if (s->offset==0) {
                                s->direction=+1;
                                s->scroll_start_tick = current_tick +
                                    scroll_delay;
                            } else {
                                if (s->offset == s->turn_offset) {
                                    s->direction=-1;
                                    s->scroll_start_tick = current_tick +
                                        scroll_delay;
                                }
                            }
                        }
                        else {
                            s->offset = 0;
                        }
                    }

                    i=0;
                    o=s->offset;
                    while (i<11) {
                        buffer[i++]=s->text[o++];
                        if (o==s->textlen /* || (jump_scroll && buffer[i-1] == ' ') */)
                            break;
                    }
                    o=0;
                    if (s->turn_offset == -1 && !jumping_scroll) {
                        while (i<11) {
                            buffer[i++]=s->text[o++];
                        }
                    } else {
                        while (i<11) {
                            buffer[i++]=' ';
                        }
                    }
                    buffer[11]=0;
                    
                    lcd_puts_cont_scroll(s->startx, s->starty, buffer);
                }
            }
            if (cursor.len>0) {
                if (cursor.downcount--<0) {
                    cursor.downcount=cursor.divider;
                    cursor.textpos++;
                    if (cursor.textpos>=cursor.len)
                        cursor.textpos=0;
#ifdef SIMULATOR
                    lcdx_putc(cursor.x_pos, cursor.y_pos,
                              cursor.text[cursor.textpos]);
                    update=true;
#else
                    update|=lcdx_putc(cursor.x_pos, cursor.y_pos,
                                      cursor.text[cursor.textpos]);
#endif
                }
            }
            if (update) {
                lcd_update();
            }
        }

        sleep(scroll_ticks);
    }
}

#endif /* HAVE_LCD_CHARCELLS */
