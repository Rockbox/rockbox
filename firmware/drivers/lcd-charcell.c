/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Jens Arnold
 * Based on the work of Alan Korr, Kjell Ericson and others
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

#include "lcd.h"
#include "kernel.h"
#include "thread.h"
#include <string.h>
#include <stdlib.h>
#include "file.h"
#include "debug.h"
#include "system.h"
#include "lcd-charcell.h"
#include "rbunicode.h"

/** definitions **/

#define SCROLLABLE_LINES LCD_HEIGHT
#define VARIABLE_XCHARS 16 /* number of software user-definable characters */

#define NO_PATTERN (-1)

static int find_xchar(unsigned long ucs);

/** globals **/

/* The "frame"buffer */
unsigned char lcd_charbuffer[LCD_HEIGHT][LCD_WIDTH];
struct pattern_info lcd_patterns[MAX_HW_PATTERNS];
struct cursor_info lcd_cursor;

static unsigned char xfont_variable[VARIABLE_XCHARS][HW_PATTERN_SIZE];
static bool xfont_variable_locked[VARIABLE_XCHARS];
static int xspace; /* stores xhcar id of ' ' - often needed */

static int xmargin = 0;
static int ymargin = 0;

/* scrolling */
static volatile int scrolling_lines=0; /* Bitpattern of which lines are scrolling */
static void scroll_thread(void);
static char scroll_stack[DEFAULT_STACK_SIZE];
static const char scroll_name[] = "scroll";
static int scroll_ticks = 12; /* # of ticks between updates */
static int scroll_delay = HZ/2; /* delay before starting scroll */
static int bidir_limit = 50;  /* percent */
static int jump_scroll_delay = HZ/4; /* delay between jump scroll jumps */
static int jump_scroll = 0; /* 0=off, 1=once, ..., JUMP_SCROLL_ALWAYS */
static struct scrollinfo scroll[SCROLLABLE_LINES];

static const char scroll_tick_table[16] = {
 /* Hz values:
    1, 1.25, 1.55, 2, 2.5, 3.12, 4, 5, 6.25, 8.33, 10, 12.5, 16.7, 20, 25, 33 */
    100, 80, 64, 50, 40, 32, 25, 20, 16, 12, 10, 8, 6, 5, 4, 3
};

/* LCD init */
void lcd_init (void)
{
    lcd_init_device();
    lcd_charset_init();
    memset(lcd_patterns, 0, sizeof(lcd_patterns));
    xspace = find_xchar(' ');
    memset(lcd_charbuffer, xchar_info[xspace].hw_char, sizeof(lcd_charbuffer));

    create_thread(scroll_thread, scroll_stack,
                  sizeof(scroll_stack), scroll_name
                  IF_PRIO(, PRIORITY_USER_INTERFACE)
		          IF_COP(, CPU, false));
}

/** parameter handling **/

void lcd_setmargins(int x, int y)
{
    xmargin = x;
    ymargin = y;
}

int lcd_getxmargin(void)
{
    return xmargin;
}

int lcd_getymargin(void)
{
    return ymargin;
}

int lcd_getstringsize(const unsigned char *str, int *w, int *h)
{                     
    int width = utf8length(str);
    
    if (w)
        *w = width;
    if (h)
        *h = 1;
        
    return width;
}

/** low-level functions **/

static int find_xchar(unsigned long ucs)
{
    int low = 0;
    int high = xchar_info_size - 1;

    do
    {
        int probe = (low + high) >> 1;

        if (xchar_info[probe].ucs < ucs)
            low = probe + 1;
        else if (xchar_info[probe].ucs > ucs)
            high = probe - 1;
        else
            return probe;
    }
    while (low <= high);

    /* Not found: return index of no-char symbol (last symbol, hardcoded). */
    return xchar_info_size - 1;
}

static int xchar_to_pat(int xchar)
{
    int i;

    for (i = 0; i < lcd_pattern_count; i++)
        if (lcd_patterns[i].xchar == xchar)
            return i;

    return NO_PATTERN;
}

static const unsigned char *xchar_to_glyph(int xchar)
{
    unsigned index = xchar_info[xchar].glyph;
    
    if (index & 0x8000)
        return xfont_variable[index & 0x7fff];
    else
        return xfont_fixed[index];
}

static void lcd_free_pat(int xchar)
{
    int x, y;
    unsigned char substitute;
    int pat = xchar_to_pat(xchar);

    if (pat != NO_PATTERN)
    {
        substitute = xchar_info[xchar].hw_char;

        for (x = 0; x < LCD_WIDTH; x++)
            for (y = 0; y < LCD_HEIGHT; y++)
                if (pat == lcd_charbuffer[y][x])
                    lcd_charbuffer[y][x] = substitute;

        if (lcd_cursor.enabled && pat == lcd_cursor.hw_char)
            lcd_cursor.hw_char = substitute;

        lcd_patterns[pat].count = 0;
    }
}

static int lcd_get_free_pat(int xchar)
{
    static int last_used_pat = 0;

    int pat = last_used_pat; /* start from last used pattern */
    int least_pat = pat;     /* pattern with least priority */
    int least_priority = xchar_info[lcd_patterns[pat].xchar].priority;
    int i;

    for (i = 0; i < lcd_pattern_count; i++)
    {
        if (++pat >= lcd_pattern_count)  /* Keep 'pat' within limits */
            pat = 0;
            
        if (lcd_patterns[pat].count == 0)
        {
            lcd_patterns[pat].xchar = xchar;
            last_used_pat = pat;
            return pat;
        }
        if (xchar_info[lcd_patterns[pat].xchar].priority < least_priority)
        {
            least_priority = xchar_info[lcd_patterns[pat].xchar].priority;
            least_pat = pat;
        }
    }
    if (xchar_info[xchar].priority > least_priority) /* prioritized char */
    {
        lcd_free_pat(lcd_patterns[least_pat].xchar);
        lcd_patterns[least_pat].xchar = xchar;
        last_used_pat = least_pat;
        return least_pat;
    }
    return NO_PATTERN;
}

static int map_xchar(int xchar)
{
    int pat;

    if (xchar_info[xchar].priority > 0)      /* soft char */
    {
        pat = xchar_to_pat(xchar);
 
        if (pat == NO_PATTERN)               /* not yet mapped */
        {
            pat = lcd_get_free_pat(xchar);   /* try to map */
            if (pat == NO_PATTERN)           /* failed: just use substitute */
                return xchar_info[xchar].hw_char;
            else                             /* define pattern */
                memcpy(lcd_patterns[pat].pattern, xchar_to_glyph(xchar),
                       HW_PATTERN_SIZE);
        }
        lcd_patterns[pat].count++;             /* increase reference count */
        return pat;
    }
    else                                     /* hardware char */
        return xchar_info[xchar].hw_char;
}

static void lcd_putxchar(int x, int y, int xchar)
{
    int lcd_char = lcd_charbuffer[y][x];

    if (lcd_char < lcd_pattern_count)         /* old char was soft */
        lcd_patterns[lcd_char].count--;        /* decrease old reference count */

    lcd_charbuffer[y][x] = map_xchar(xchar);
}

/** user-definable pattern handling **/

unsigned long lcd_get_locked_pattern(void)
{
    int i = 0;

    for (i = 0; i < VARIABLE_XCHARS; i++)
    {
        if (!xfont_variable_locked[i])
        {
            xfont_variable_locked[i] = true;
            return 0xe000 + i;  /* hard-coded */
        }
    }
    return 0;
}

void lcd_unlock_pattern(unsigned long ucs)
{
    int xchar = find_xchar(ucs);
    int index = xchar_info[xchar].glyph;

    if (index & 0x8000) /* variable extended char */
    {
        lcd_free_pat(xchar);
        xfont_variable_locked[index & 0x7fff] = false;
    }
}

void lcd_define_pattern(unsigned long ucs, const char *pattern)
{
    int xchar = find_xchar(ucs);
    int index = xchar_info[xchar].glyph;
    int pat;

    if (index & 0x8000) /* variable extended char */
    {
        memcpy(xfont_variable[index & 0x7fff], pattern, HW_PATTERN_SIZE);
        pat = xchar_to_pat(xchar);
        if (pat != NO_PATTERN)
        {
            memcpy(lcd_patterns[pat].pattern, pattern, HW_PATTERN_SIZE);
            lcd_update(); //FIXME: remove when lcd_update() calls are checked all over
        }
    }
}

/** output functions **/

/* Clear the whole display */
void lcd_clear_display(void)
{
    int x, y;

    lcd_stop_scroll();
    lcd_remove_cursor();

    for (x = 0; x < LCD_WIDTH; x++)
        for (y = 0; y < LCD_HEIGHT; y++)
            lcd_putxchar(x, y, xspace);

    lcd_update(); //FIXME: remove when lcd_update() calls are checked all over
}

/* Put an unicode character at the given position */
void lcd_putc(int x, int y, unsigned long ucs)
{
    if ((unsigned)x >= LCD_WIDTH || (unsigned)y >= LCD_HEIGHT)
        return;

    lcd_putxchar(x, y, find_xchar(ucs));
    lcd_update(); //FIXME: remove when lcd_update() calls are checked all over
}

/* Show cursor (alternating with existing character) at the given position */
void lcd_put_cursor(int x, int y, unsigned long cursor_ucs)
{
    if ((unsigned)x >= LCD_WIDTH || (unsigned)y >= LCD_HEIGHT
        || lcd_cursor.enabled)
        return;

    lcd_cursor.enabled = true;
    lcd_cursor.visible = false;
    lcd_cursor.hw_char = map_xchar(find_xchar(cursor_ucs));
    lcd_cursor.x = x;
    lcd_cursor.y = y;
    lcd_cursor.downcount = 0;
    lcd_cursor.divider = MAX((HZ/2) / scroll_ticks, 1);
}

/* Remove the cursor */
void lcd_remove_cursor(void)
{
    if (lcd_cursor.enabled)
    {
        if (lcd_cursor.hw_char < lcd_pattern_count)  /* soft char, unmap */
            lcd_patterns[lcd_cursor.hw_char].count--;

        lcd_cursor.enabled = lcd_cursor.visible = false;
    }
}

/* Put a string at a given position, skipping first ofs chars */
static int lcd_putsxyofs(int x, int y, int ofs, const unsigned char *str)
{
    unsigned short ucs;
    const unsigned char *utf8 = str;

    while (*utf8 && x < LCD_WIDTH)
    {
        utf8 = utf8decode(utf8, &ucs);
        
        if (ofs > 0)
        {
            ofs--;
            continue;
        }
        lcd_putxchar(x++, y, find_xchar(ucs));
    }
    return x;
}

/* Put a string at a given position */
void lcd_putsxy(int x, int y, const unsigned char *str)
{
    if ((unsigned)y >= LCD_HEIGHT)
        return;

    lcd_putsxyofs(x, y, 0, str);
    lcd_update(); //FIXME: remove when lcd_update() calls are checked all over
}

/*** Line oriented text output ***/

/* Put a string at a given char position */
void lcd_puts(int x, int y, const unsigned char *str)
{
    lcd_puts_offset(x, y, str, 0);
}

/* Put a string at a given char position,  skipping first offset chars */
void lcd_puts_offset(int x, int y, const unsigned char *str, int offset)
{
    x += xmargin;
    y += ymargin;

    if ((unsigned)y >= LCD_HEIGHT)
        return;

    /* make sure scrolling is turned off on the line we are updating */
    scrolling_lines &= ~(1 << y);

    x = lcd_putsxyofs(x, y, offset, str);
    while (x < LCD_WIDTH)
        lcd_putxchar(x++, y, xspace);

    lcd_update(); //FIXME: remove when lcd_update() calls are checked all over
}

/** scrolling **/

void lcd_stop_scroll(void)
{
    scrolling_lines=0;
}

void lcd_scroll_speed(int speed)
{
    scroll_ticks = scroll_tick_table[speed];
}

void lcd_scroll_delay(int ms)
{
    scroll_delay = ms / (HZ / 10);
}

void lcd_bidir_scroll(int percent)
{
    bidir_limit = percent;
}

void lcd_jump_scroll(int mode) /* 0=off, 1=once, ..., JUMP_SCROLL_ALWAYS */
{
    jump_scroll = mode;
}

void lcd_jump_scroll_delay(int ms)
{
    jump_scroll_delay = ms / (HZ / 10);
}

void lcd_puts_scroll(int x, int y, const unsigned char *string)
{
    lcd_puts_scroll_offset(x, y, string, 0);
}

void lcd_puts_scroll_offset(int x, int y, const unsigned char *string,
                            int offset)
{
    struct scrollinfo* s;
    int len;

    s = &scroll[y];

    s->start_tick = current_tick + scroll_delay;

    lcd_puts_offset(x, y, string, offset);
    len = utf8length(string);

    if (LCD_WIDTH - x - xmargin < len) 
    {
        /* prepare scroll line */
        char *end;

        memset(s->line, 0, sizeof s->line);
        strcpy(s->line, string);

        /* get width */
        s->len = utf8length(s->line);

        /* scroll bidirectional or forward only depending on the string width */
        if (bidir_limit)
        {
            s->bidir = s->len < (LCD_WIDTH - xmargin) * (100 + bidir_limit) / 100;
        }
        else
            s->bidir = false;

        if (!s->bidir)  /* add spaces if scrolling in the round */
        {
            strcat(s->line, "   ");
            /* get new width incl. spaces */
            s->len += SCROLL_SPACING;
        }

        end = strchr(s->line, '\0');
        strncpy(end, string, utf8seek(s->line, LCD_WIDTH));

        s->offset = offset;
        s->startx = xmargin + x;
        s->backward = false;
        scrolling_lines |= (1<<y);
    }
    else
        /* force a bit switch-off since it doesn't scroll */
        scrolling_lines &= ~(1<<y);
}

static void scroll_thread(void)
{
    struct scrollinfo* s;
    int index;
    int xpos, ypos;
    bool update;

    /* initialize scroll struct array */
    scrolling_lines = 0;

    while (1)
    {
        update = false;
        for (index = 0; index < SCROLLABLE_LINES; index++)
        {
            /* really scroll? */
            if (!(scrolling_lines&(1<<index)))
                continue;

            s = &scroll[index];

            /* check pause */
            if (TIME_BEFORE(current_tick, s->start_tick))
                continue;

            if (s->backward)
                s->offset--;
            else
                s->offset++;

            xpos = s->startx;
            ypos = ymargin + index;

            if (s->bidir)  /* scroll bidirectional */
            {
                if (s->offset <= 0) 
                {
                    /* at beginning of line */
                    s->offset = 0;
                    s->backward = false;
                    s->start_tick = current_tick + scroll_delay * 2;
                }
                if (s->offset >= s->len - (LCD_WIDTH - xpos)) 
                {
                    /* at end of line */
                    s->offset = s->len - (LCD_WIDTH - xpos);
                    s->backward = true;
                    s->start_tick = current_tick + scroll_delay * 2;
                }
            }
            else    /* scroll forward the whole time */
            {
                if (s->offset >= s->len)
                    s->offset -= s->len;
            }
            lcd_putsxyofs(xpos, ypos, s->offset, s->line);
            update = true;
        }
        if (lcd_cursor.enabled)
        {
            if (--lcd_cursor.downcount <= 0)
            {
                lcd_cursor.downcount = lcd_cursor.divider;
                lcd_cursor.visible = !lcd_cursor.visible;
                update = true;
            }
        }
        if (update)
            lcd_update();

        sleep(scroll_ticks);
    }
}
