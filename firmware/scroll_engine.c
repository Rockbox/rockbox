/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 by Michael Sevakis
 *
 * LCD scrolling driver and scheduler
 *
 * Much collected and combined from the various Rockbox LCD drivers.
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "config.h"
#include "cpu.h"
#include "kernel.h"
#include "thread.h"
#include "usb.h"
#include "lcd.h"
#include "font.h"
#ifdef HAVE_REMOTE_LCD
#include "lcd-remote.h"
#endif
#include "scroll_engine.h"

static const char scroll_tick_table[16] = {
 /* Hz values:
    1, 1.25, 1.55, 2, 2.5, 3.12, 4, 5, 6.25, 8.33, 10, 12.5, 16.7, 20, 25, 33 */
    100, 80, 64, 50, 40, 32, 25, 20, 16, 12, 10, 8, 6, 5, 4, 3
};

static void scroll_thread(void);
static char scroll_stack[DEFAULT_STACK_SIZE*3];
static const char scroll_name[] = "scroll";

static struct scrollinfo lcd_scroll[LCD_SCROLLABLE_LINES];

#ifdef HAVE_REMOTE_LCD
struct scrollinfo lcd_remote_scroll[LCD_REMOTE_SCROLLABLE_LINES];
struct event_queue scroll_queue;
#endif

struct scroll_screen_info lcd_scroll_info =
{
    .scroll       = lcd_scroll,
    .lines        = 0,
    .ticks        = 12,
    .delay        = HZ/2,
    .bidir_limit  = 50,
#ifdef HAVE_LCD_BITMAP
    .step         = 6,
#endif
#ifdef HAVE_LCD_CHARCELLS
    .jump_scroll_delay = HZ/4,
    .jump_scroll       = 0,
#endif
};

#ifdef HAVE_REMOTE_LCD
struct scroll_screen_info lcd_remote_scroll_info =
{
    .scroll       = lcd_remote_scroll,
    .lines        = 0,
    .ticks        = 12,
    .delay        = HZ/2,
    .bidir_limit  = 50,
    .step         = 6,
};
#endif /* HAVE_REMOTE_LCD */

void lcd_stop_scroll(void)
{
    lcd_scroll_info.lines = 0;
}

/* Stop scrolling line y in the specified viewport, or all lines if y < 0 */
void lcd_scroll_stop_line(struct viewport* current_vp, int y)
{
    int i = 0;

    while (i < lcd_scroll_info.lines)
    {
        if ((lcd_scroll_info.scroll[i].vp == current_vp) && 
            ((y < 0) || (lcd_scroll_info.scroll[i].y == y)))
        {
            /* If i is not the last active line in the array, then move
               the last item to position i */
            if ((i + 1) != lcd_scroll_info.lines)
            {
                lcd_scroll_info.scroll[i] = lcd_scroll_info.scroll[lcd_scroll_info.lines-1];
            }
            lcd_scroll_info.lines--;

            /* A line can only appear once, so we're done. */
            return ;
        }
        else
        {
            i++;
        }
    }
}

/* Stop all scrolling lines in the specified viewport */
void lcd_scroll_stop(struct viewport* vp)
{
    lcd_scroll_stop_line(vp, -1);
}

void lcd_scroll_speed(int speed)
{
    lcd_scroll_info.ticks = scroll_tick_table[speed];
}

#if defined(HAVE_LCD_BITMAP)
void lcd_scroll_step(int step)
{
    lcd_scroll_info.step = step;
}
#endif

void lcd_scroll_delay(int ms)
{
    lcd_scroll_info.delay = ms / (HZ / 10);
}

void lcd_bidir_scroll(int percent)
{
    lcd_scroll_info.bidir_limit = percent;
}

#ifdef HAVE_LCD_CHARCELLS
void lcd_jump_scroll(int mode) /* 0=off, 1=once, ..., JUMP_SCROLL_ALWAYS */
{
    lcd_scroll_info.jump_scroll = mode;
}

void lcd_jump_scroll_delay(int ms)
{
    lcd_scroll_info.jump_scroll_delay = ms / (HZ / 10);
}
#endif

#ifdef HAVE_REMOTE_LCD
void lcd_remote_stop_scroll(void)
{
    lcd_remote_scroll_info.lines = 0;
}

/* Stop scrolling line y in the specified viewport, or all lines if y < 0 */
void lcd_remote_scroll_stop_line(struct viewport* current_vp, int y)
{
    int i = 0;

    while (i < lcd_remote_scroll_info.lines)
    {
        if ((lcd_remote_scroll_info.scroll[i].vp == current_vp) && 
            ((y < 0) || (lcd_remote_scroll_info.scroll[i].y == y)))
        {
            /* If i is not the last active line in the array, then move
               the last item to position i */
            if ((i + 1) != lcd_remote_scroll_info.lines)
            {
                lcd_remote_scroll_info.scroll[i] = lcd_remote_scroll_info.scroll[lcd_remote_scroll_info.lines-1];
            }
            lcd_remote_scroll_info.lines--;

            /* A line can only appear once, so we're done. */
            return ;
        }
        else
        {
            i++;
        }
    }
}

/* Stop all scrolling lines in the specified viewport */
void lcd_remote_scroll_stop(struct viewport* vp)
{
    lcd_remote_scroll_stop_line(vp, -1);
}

void lcd_remote_scroll_speed(int speed)
{
    lcd_remote_scroll_info.ticks = scroll_tick_table[speed];
}

void lcd_remote_scroll_step(int step)
{
    lcd_remote_scroll_info.step = step;
}

void lcd_remote_scroll_delay(int ms)
{
    lcd_remote_scroll_info.delay = ms / (HZ / 10);
}

void lcd_remote_bidir_scroll(int percent)
{
    lcd_remote_scroll_info.bidir_limit = percent;
}

static void sync_display_ticks(void)
{
    lcd_scroll_info.last_scroll =
    lcd_remote_scroll_info.last_scroll = current_tick;
}

static bool scroll_process_message(int delay)
{
    struct queue_event ev;

    do
    {
        long tick = current_tick;
        queue_wait_w_tmo(&scroll_queue, &ev, delay);

        switch (ev.id)
        {
        case SYS_TIMEOUT:
            return false;
        case SYS_USB_CONNECTED:
            usb_acknowledge(SYS_USB_CONNECTED_ACK);
            usb_wait_for_disconnect(&scroll_queue);
            sync_display_ticks();
            return true;
#ifndef SIMULATOR
        case SYS_REMOTE_PLUGGED:
            if (!remote_initialized)
                sync_display_ticks();
#endif
        }

        delay -= current_tick - tick;
    }
    while (delay > 0);

    return false;
}
#endif /* HAVE_REMOTE_LCD */

static void scroll_thread(void) __attribute__((noreturn));
#ifdef HAVE_REMOTE_LCD

static void scroll_thread(void)
{
    enum
    {
        SCROLL_LCD        = 0x1,
        SCROLL_LCD_REMOTE = 0x2,
    };

    sync_display_ticks();

    while ( 1 )
    {
        long delay;
        int scroll;
        long tick_lcd, tick_remote;

        tick_lcd = lcd_scroll_info.last_scroll + lcd_scroll_info.ticks;
        delay = current_tick;

        if (
#ifndef SIMULATOR
            !remote_initialized ||
#endif
            (tick_remote = lcd_remote_scroll_info.last_scroll +
                           lcd_remote_scroll_info.ticks,
             TIME_BEFORE(tick_lcd, tick_remote)))
        {
            scroll = SCROLL_LCD;
            delay = tick_lcd - delay;
        }
        /* TIME_BEFORE(tick_remote, tick_lcd) */
        else if (tick_lcd != tick_remote)
        {
            scroll = SCROLL_LCD_REMOTE;
            delay = tick_remote - delay;
        }
        else
        {
            scroll = SCROLL_LCD | SCROLL_LCD_REMOTE;
            delay = tick_lcd - delay;
        }

        if (scroll_process_message(delay))
            continue;

        if (scroll & SCROLL_LCD)
        {
#ifdef HAVE_LCD_ENABLE
            if (lcd_enabled())
#endif
                lcd_scroll_fn();
            lcd_scroll_info.last_scroll = current_tick;
        }

        if (scroll == (SCROLL_LCD | SCROLL_LCD_REMOTE))
            yield();

        if (scroll & SCROLL_LCD_REMOTE)
        {
            lcd_remote_scroll_fn();
            lcd_remote_scroll_info.last_scroll = current_tick;
        }
    }
}
#else
static void scroll_thread(void)
{
    while (1)
    {
        sleep(lcd_scroll_info.ticks);
#ifdef HAVE_LCD_ENABLE
        if (lcd_enabled())
#endif
            lcd_scroll_fn();
    }
}
#endif /* HAVE_REMOTE_LCD */

void scroll_init(void)
{
#ifdef HAVE_REMOTE_LCD
    queue_init(&scroll_queue, true);
#endif
    create_thread(scroll_thread, scroll_stack,
                  sizeof(scroll_stack), 0, scroll_name
                  IF_PRIO(, PRIORITY_USER_INTERFACE)
                  IF_COP(, CPU));
}
