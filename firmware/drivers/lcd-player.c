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

#define LCD_CURSOR(x,y)    ((char)(lcd_cram+((y)*16+(x))))
#define LCD_ICON(i)        ((char)(lcd_iram+i))

#define SCROLLABLE_LINES 2

#define SCROLL_MODE_OFF   0
#define SCROLL_MODE_PAUSE 1
#define SCROLL_MODE_RUN   2

extern unsigned short new_lcd_rocklatin1_to_xlcd[];
extern unsigned short old_lcd_rocklatin1_to_xlcd[];
extern unsigned char lcd_player_extended_lcd_to_rocklatin1[];
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
};

static void scroll_thread(void);
static char scroll_stack[DEFAULT_STACK_SIZE];
static char scroll_name[] = "scroll";
static char scroll_speed = 8; /* updates per second */
static int scroll_delay = HZ/2; /* delay before starting scroll */
static char scroll_spacing = 3; /* spaces between end and start of text */
static bool allow_bidirectional_scrolling = true;

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
#endif
#define NO_CHAR -1

static void lcd_free_pat(int map_ch)
{
    int x, y;
    unsigned short substitute_char;

    int pat;
    pat=extended_chars_mapped[map_ch];
    if (pat!=NO_CHAR) {

      substitute_char=lcd_player_extended_lcd_to_rocklatin1[map_ch];

      for (x=0; x<11; x++) {
        for (y=0; y<2; y++)  {
          if (map_ch==lcd_ascii[buffer_xlcd[x][y]]-512) {
            buffer_xlcd[x][y]=substitute_char;
            buffer_lcd_mirror[x][y]=substitute_char;
#ifdef SIMULATOR
            hardware_buffer_lcd[x][y]=substitute_char;
#else
            lcd_write(true, LCD_CURSOR(x, y));
            lcd_write(false, substitute_char);
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

    pat=last_pat;
    for (loop=0; loop<=pattern_size; loop++) {
        pat=(pat+1)&pattern_size; /* Keep 'pat' within limits */
        if (extended_pattern_usage[pat]==0) {
            int map_ch=extended_pattern_content[pat];
            if (map_ch != NO_CHAR) {
                extended_chars_mapped[map_ch]=NO_CHAR;
                extended_pattern_content[pat]=NO_CHAR;
            }
            last_pat=pat;
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
                } else {
                    map_ch=map_ch-512;
                    if (extended_chars_mapped[map_ch]!=NO_CHAR) {
                        hw_ch=extended_chars_mapped[map_ch];
                        extended_pattern_usage[hw_ch]++;
                    } else {
                        int pat;
                        pat=lcd_get_free_pat(map_ch);
                        if (pat<0) {
                            DEBUGF("Substitute for %02x (map 0x%02x) is used.\n", ch, map_ch);
                            /* Find substitute char */
                            map_ch=lcd_player_extended_lcd_to_rocklatin1[map_ch];
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
                                                  extended_font_player[map_ch], 8);
                            hw_ch=pat;
                        }
                    }
                }
            }
#ifdef SIMULATOR
            hardware_buffer_lcd[x][y]=hw_ch;
#else
            lcd_write(true,LCD_CURSOR(x,y));
            lcd_write(false, hw_ch);
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
  lcd_update();
#else
  lcd_write(true, LCD_CURSOR(x, y));
  lcd_write(false, lcd_char);
#endif
  return false;
}

void lcd_clear_display(void)
{
    int i;
    bool update=false; 
    DEBUGF("lcd_clear_display()\n");
    for (i=0;i<22;i++)
      update|=lcdx_putc(i%11, i/11, ' ');
    if (update)
      xlcd_update();
}

void lcd_puts(int x, int y, unsigned char *string)
{
    bool update=false; 
//    lcd_write(true,LCD_CURSOR(x,y));
    DEBUGF("lcd_puts(%d, %d, \"", x, y);
    for (; *string && x<11; x++)
        {
#ifdef DEBUGF
        if (*string>=32 && *string<128)
            {DEBUGF("%c", *string);}
        else
            {DEBUGF("(0x%02x)", *string);}
#endif
        /* We should check if char is over 256 */
        update|=lcdx_putc(x, y, *(unsigned char*)string++);
        }
    DEBUGF("\")\n");
    
    for (; x<11; x++)
        update|=lcdx_putc(x, y, ' ');
    if (update)
      xlcd_update();
}

void lcd_putc(int x, int y, unsigned short ch)
{
    bool update;
    DEBUGF("lcd_putc(%d, %d, %d '0x%02x')\n", x, y, ch, ch);
    if (x<0 || y<0) {
      return;
    }
//    lcd_write(true,LCD_CURSOR(x,y));
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

void lcd_define_pattern(int pat, char *pattern)
{
  int i;
  for (i=0; i<7; i++) {
    extended_font_player[pat][i]=pattern[i];
  }
  if (extended_chars_mapped[pat]!=NO_CHAR) {
      lcd_define_hw_pattern(pat*8, pattern, 7);
  }
}

#ifndef SIMULATOR
void lcd_define_hw_pattern (int which,char *pattern,int length)
{
    int i;
    lcd_write(true,lcd_pram | which);
    for (i=0;i<length;i++)
        lcd_write(false,pattern[i]);
}

void lcd_double_height(bool on)
{
    if(new_lcd)
        lcd_write(true,on?9:8);
}

static char icon_pos[] =
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

static char icon_mask[] =
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
    
    lcd_write(true, LCD_ICON(pos));
    
    if(enable)
        icon_mirror[pos] |= mask;
    else
        icon_mirror[pos] &= ~mask;
    
    lcd_write(false, icon_mirror[pos]);
}

void lcd_set_contrast(int val)
{
    lcd_write(true, lcd_contrast_set);
    lcd_write(false, 31-val);
}
#endif /* SIMULATOR */

void lcd_init (void)
{
    new_lcd = has_new_lcd();
    memset(extended_chars_mapped, NO_CHAR, sizeof(extended_chars_mapped));
    memset(extended_pattern_content, NO_CHAR,sizeof(extended_pattern_content));
    memset(extended_pattern_usage, 0, sizeof(extended_pattern_usage));

    if(new_lcd) {
        lcd_ascii = new_lcd_rocklatin1_to_xlcd;
        lcd_contrast_set = NEW_LCD_CONTRAST_SET;
        lcd_cram = NEW_LCD_CRAM;
        lcd_pram = NEW_LCD_PRAM;
        lcd_iram = NEW_LCD_IRAM;
        pattern_size=7; /* Last pattern, 3 for old LCD, 7 for new LCD */
    } else {
        lcd_ascii = old_lcd_rocklatin1_to_xlcd;
        lcd_contrast_set = OLD_LCD_CONTRAST_SET;
        lcd_cram = OLD_LCD_CRAM;
        lcd_pram = OLD_LCD_PRAM;
        lcd_iram = OLD_LCD_IRAM;
        pattern_size=3; /* Last pattern, 3 for old LCD, 7 for new LCD */
    }
    
    create_thread(scroll_thread, scroll_stack,
                  sizeof(scroll_stack), scroll_name);
}

void lcd_puts_scroll(int x, int y, unsigned char* string )
{
    struct scrollinfo* s;
    int i;

    DEBUGF("lcd_puts_scroll(%d, %d, %s)\n", x, y, string);

    s = &scroll[y];

    lcd_puts(x,y,string);
    s->textlen = strlen(string);

    if ( s->textlen > 11-x ) {
        s->mode = SCROLL_MODE_RUN;
        s->scroll_start_tick = current_tick + scroll_delay;
        s->offset=0;
        s->startx=x;
        s->starty=y;
        s->direction=+1;
        strncpy(s->text,string,sizeof s->text);
        s->turn_offset=-1;
        if (allow_bidirectional_scrolling) {
            if ( s->textlen + x > 11+4)
                s->turn_offset=s->textlen-x-11+4;
        }

        for (i=0; i<scroll_spacing && s->textlen<(int)sizeof(s->text); i++) {
            s->text[s->textlen++]=' ';
        }
        if (s->textlen<(int)sizeof(s->text))
            s->text[s->textlen]=' ';
        s->text[sizeof s->text - 1] = 0;
    } else
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
            s->mode = SCROLL_MODE_OFF;
        }
    }

    lcd_update();
}

void lcd_stop_scroll_line(int line)
{
    struct scrollinfo* s;
    
    s = &scroll[line];
    if ( s->mode == SCROLL_MODE_RUN ||
         s->mode == SCROLL_MODE_PAUSE ) {
        /* restore scrolled row */
        lcd_puts(s->startx, s->starty, s->text);
        s->mode = SCROLL_MODE_OFF;
    }

    lcd_update();
}

void lcd_scroll_pause(void)
{
    struct scrollinfo* s;
    int index;

    for ( index = 0; index < SCROLLABLE_LINES; index++ ) {
        s = &scroll[index];
        if ( s->mode == SCROLL_MODE_RUN ) {
            s->mode = SCROLL_MODE_PAUSE;
        }
    }
}

void lcd_scroll_pause_line(int line)
{
    struct scrollinfo* s;

    s = &scroll[line];
    if ( s->mode == SCROLL_MODE_RUN ) {
        s->mode = SCROLL_MODE_PAUSE;
    }
}

void lcd_scroll_resume(void)
{
    struct scrollinfo* s;
    int index;

    for ( index = 0; index < SCROLLABLE_LINES; index++ ) {
        s = &scroll[index];
        if ( s->mode == SCROLL_MODE_PAUSE ) {
            s->mode = SCROLL_MODE_RUN;
        }
    }
}

void lcd_scroll_resume_line(int line)
{
    struct scrollinfo* s;

    s = &scroll[line];
    if (s->mode == SCROLL_MODE_PAUSE ) {
        s->mode = SCROLL_MODE_RUN;
    }
}

void lcd_allow_bidirectional_scrolling(bool on)
{
    allow_bidirectional_scrolling=on;
}

void lcd_scroll_speed(int speed)
{
    scroll_speed = speed;
}

void lcd_scroll_delay(int ms)
{
    scroll_delay = ms / (HZ / 10);
    DEBUGF("scroll_delay=%d (ms=%d, HZ=%d)\n", scroll_delay, ms, HZ);
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
                    update = true;
                    DEBUGF("offset=%d, turn_offset=%d, len=%d",
                           s->offset, s->turn_offset, s->textlen);
                    if ( s->offset < s->textlen-1 ) {
                        s->offset+=s->direction;
                        if (s->offset==0) {
                            s->direction=+1;
                            s->scroll_start_tick = current_tick + scroll_delay;
                        } else if (s->offset==s->turn_offset) {
                            s->direction=-1;
                            s->scroll_start_tick = current_tick + scroll_delay;
                        }
                    }
                    else {
                            s->offset = 0;
                    }

                    i=0;
                    o=s->offset;
                    while (i<11) {
                        buffer[i++]=s->text[o++];
                        if (o==s->textlen)
                            break;
                    }
                    o=0;
                    while (i<11) {
                        buffer[i++]=s->text[o++];
                    }
                    buffer[11]=0;
                    lcd_puts(s->startx, s->starty, buffer);
                }
            }

            if (update) {
                lcd_update();
            }
        }

        sleep(HZ/scroll_speed);
    }
}

#endif /* HAVE_LCD_CHARCELLS */

/* -----------------------------------------------------------------
 * local variables:
 * eval: (load-file "../rockbox-mode.el")
 * end:
 */
