/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $$
 *
 * Copyright (C) 2005 Ray Lambert
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "abrepeat.h"

#include "settings.h"
#include "audio.h"
#include "kernel.h"

#ifdef HAVE_LCD_BITMAP
#include "lcd.h"
#endif

#ifdef AB_REPEAT_ENABLE

static int ab_audio_event_handler(unsigned short event, unsigned long data)
{
    int rc = AUDIO_EVENT_RC_IGNORED;
    if ( ab_repeat_mode_enabled() )
    {
        switch(event)
        {
            case AUDIO_EVENT_POS_REPORT:
            {
                if ( ! (audio_status() & AUDIO_STATUS_PAUSE) && ab_reached_B_marker(data) )
                {
                    ab_jump_to_A_marker();
                    rc = AUDIO_EVENT_RC_HANDLED;
                }
                break;
            }
            case AUDIO_EVENT_END_OF_TRACK:
            {
                if ( ab_A_marker_set() && ! ab_B_marker_set() )
                {
                    ab_jump_to_A_marker();
                    rc = AUDIO_EVENT_RC_HANDLED;
                }
                break;
            }
        }
    }
    return rc;
}

void ab_repeat_init(void)
{
    static bool ab_initialized = false;
    if ( ! ab_initialized )
    {
        ab_initialized = true;
        audio_register_event_handler(ab_audio_event_handler,
            AUDIO_EVENT_POS_REPORT | AUDIO_EVENT_END_OF_TRACK );
    }
}

static unsigned int ab_A_marker = AB_MARKER_NONE;
static unsigned int ab_B_marker = AB_MARKER_NONE;

bool ab_repeat_mode_enabled(void)
{
	extern struct user_settings global_settings;
    return global_settings.repeat_mode == REPEAT_AB;
}

bool ab_A_marker_set(void)
{
    return ab_A_marker != AB_MARKER_NONE;
}

bool ab_B_marker_set(void)
{
    return ab_B_marker != AB_MARKER_NONE;
}

unsigned int ab_get_A_marker(void)
{
    return ab_A_marker;
}

unsigned int ab_get_B_marker(void)
{
    return ab_B_marker;
}

bool ab_reached_B_marker(unsigned int song_position)
{
/* following is the size of the window in which we'll detect that the B marker
was hit; it must be larger than the frequency (in milliseconds) at which this 
function is called otherwise detection of the B marker will be unreliable;
we assume that this function will be called on each system tick and derive
the window size from this with a generous margin of error (note: the number 
of ticks per second is given by HZ) */
#define B_MARKER_DETECT_WINDOW ((1000/HZ)*10)
    if (ab_B_marker != AB_MARKER_NONE)
    {
        if ( (song_position >= ab_B_marker) 
        && (song_position <= (ab_B_marker+B_MARKER_DETECT_WINDOW)) )
            return true;
    }
    return false;
}

/* determines if the given song position is earlier than the A mark;
intended for use in handling the jump NEXT and PREV commands */
bool ab_before_A_marker(unsigned int song_position)
{
    return (ab_A_marker != AB_MARKER_NONE)
        && (song_position < ab_A_marker);
}

/* determines if the given song position is later than the A mark;
intended for use in handling the jump PREV command */
bool ab_after_A_marker(unsigned int song_position)
{
/* following is the size of the virtual A marker; we pretend that the A marker is
larger than a single instant in order to give the user time to hit PREV again to
jump back to the start of the song; it should be large enough to allow a
reasonable amount of time for the typical user to react */
#define A_MARKER_VIRTUAL_SIZE 1000
    return (ab_A_marker != AB_MARKER_NONE)
        && (song_position > (ab_A_marker+A_MARKER_VIRTUAL_SIZE));
}

void ab_jump_to_A_marker(void)
{
    bool paused = (audio_status() & AUDIO_STATUS_PAUSE) != 0;
    if ( ! paused )
        audio_pause();
    audio_ff_rewind(ab_A_marker);
    if ( ! paused )
        audio_resume();
}

void ab_reset_markers(void)
{
    ab_A_marker = AB_MARKER_NONE;
    ab_B_marker = AB_MARKER_NONE;
}

/* following is a fudge factor to help overcome the latency between
the time the user hears the passage they want to mark and the time
they actually press the button; the actual song position is adjusted
by this fudge factor when setting a mark */
#define EAR_TO_HAND_LATENCY_FUDGE 200

void ab_set_A_marker(unsigned int song_position)
{
    ab_A_marker = song_position;
    ab_A_marker = (ab_A_marker >= EAR_TO_HAND_LATENCY_FUDGE) 
        ? (ab_A_marker - EAR_TO_HAND_LATENCY_FUDGE) : 0;
    /* check if markers are out of order */
    if ( (ab_B_marker != AB_MARKER_NONE) && (ab_A_marker > ab_B_marker) )
        ab_B_marker = AB_MARKER_NONE;
}

void ab_set_B_marker(unsigned int song_position)
{
    ab_B_marker = song_position;
    ab_B_marker = (ab_B_marker >= EAR_TO_HAND_LATENCY_FUDGE) 
        ? (ab_B_marker - EAR_TO_HAND_LATENCY_FUDGE) : 0;
    /* check if markers are out of order */
    if ( (ab_A_marker != AB_MARKER_NONE) && (ab_B_marker < ab_A_marker) )
        ab_A_marker = AB_MARKER_NONE;
}

#ifdef HAVE_LCD_BITMAP

static int ab_calc_mark_x_pos(int mark, int capacity, int offset, int size)
{
    int w = size - offset;
    return offset + ( (w * mark) / capacity );
}

static void ab_draw_veritcal_line_mark(int x, int y, int h)
{
    lcd_set_drawmode(DRMODE_COMPLEMENT);
    lcd_vline(x, y, y+h-1);
}

#define DIRECTION_RIGHT 1
#define DIRECTION_LEFT -1

static void ab_draw_arrow_mark(int x, int y, int h, int direction)
{
    /* draw lines in decreasing size until a height of zero is reached */
    lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
    while( h > 0 )
    {
        lcd_vline(x, y, y+h-1);
        h -= 2;
        y++;
        x += direction;
        lcd_set_drawmode(DRMODE_COMPLEMENT);
    }
}

void ab_draw_markers(int capacity, int x, int y, int w, int h)
{
    /* if both markers are set, determine if they're far enough apart
    to draw arrows */
    if ( ab_A_marker_set() && ab_B_marker_set() )
    {
        int xa = ab_calc_mark_x_pos(ab_A_marker, capacity, x, w);
        int xb = ab_calc_mark_x_pos(ab_B_marker, capacity, x, w);
        int arrow_width = (h+1) / 2;
        if ( (xb-xa) < (arrow_width*2) )
        {
            ab_draw_veritcal_line_mark(xa, y, h);
            ab_draw_veritcal_line_mark(xb, y, h);
        }
        else
        {
            ab_draw_arrow_mark(xa, y, h, DIRECTION_RIGHT);
            ab_draw_arrow_mark(xb, y, h, DIRECTION_LEFT);
        }
    }
    else
    {
        if (ab_A_marker_set())
        {
            int xa = ab_calc_mark_x_pos(ab_A_marker, capacity, x, w);
            ab_draw_arrow_mark(xa, y, h, DIRECTION_RIGHT);
        }
        if (ab_B_marker_set())
        {
            int xb = ab_calc_mark_x_pos(ab_B_marker, capacity, x, w);
            ab_draw_arrow_mark(xb, y, h, DIRECTION_LEFT);
        }
    }
}

#endif /* HAVE_LCD_BITMAP */

#endif /* AB_REPEAT_ENABLE */
