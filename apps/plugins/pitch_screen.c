/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Björn Stenberg
 *
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"
#include "lib/icon_helper.h"
#include "lib/arg_helper.h"

#define ICON_BORDER 12 /* icons are currently 7x8, so add ~2 pixels  */
                       /*   on both sides when drawing */

#define PITCH_MAX         (200 * PITCH_SPEED_PRECISION)
#define PITCH_MIN         (50 * PITCH_SPEED_PRECISION)
#define PITCH_SMALL_DELTA (PITCH_SPEED_PRECISION / 10)      /* .1% */
#define PITCH_BIG_DELTA   (PITCH_SPEED_PRECISION)           /*  1% */
#define PITCH_NUDGE_DELTA (2 * PITCH_SPEED_PRECISION)       /*  2% */

#define SPEED_SMALL_DELTA (PITCH_SPEED_PRECISION / 10)      /* .1% */
#define SPEED_BIG_DELTA   (PITCH_SPEED_PRECISION)           /*  1% */

#define SEMITONE_SMALL_DELTA (PITCH_SPEED_PRECISION / 10)  /* 10 cents   */
#define SEMITONE_BIG_DELTA   PITCH_SPEED_PRECISION         /* 1 semitone */

#define PVAR_VERBOSE 0x01
#define PVAR_GUI     0x02
struct pvars
{
    int32_t speed;
    int32_t pitch;
    int32_t stretch;
    int32_t flags;
};
static struct pvars pitch_vars;

enum
{
    PITCH_TOP = 0,
    PITCH_MID,
    PITCH_BOTTOM,
    PITCH_ITEM_COUNT,
};

/* This is a table of semitone percentage values of the appropriate
   precision (based on PITCH_SPEED_PRECISION).  Note that these are
   all constant expressions, which will be evaluated at compile time,
   so no need to worry about how complex the expressions look.
   That's just to get the precision right.

   I calculated these values, starting from 50, as

   x(n) = 50 * 2^(n/12)

   All that math in each entry simply converts the float constant
   to an integer equal to PITCH_SPEED_PRECISION times the float value,
   with as little precision loss as possible (i.e. correctly rounding
   the last digit).
*/
#define TO_INT_WITH_PRECISION(x) \
    ( (unsigned short)(((x) * PITCH_SPEED_PRECISION * 10 + 5) / 10) )

static const unsigned short semitone_table[] =
{
    TO_INT_WITH_PRECISION(50.00000000), /* Octave lower */
    TO_INT_WITH_PRECISION(52.97315472),
    TO_INT_WITH_PRECISION(56.12310242),
    TO_INT_WITH_PRECISION(59.46035575),
    TO_INT_WITH_PRECISION(62.99605249),
    TO_INT_WITH_PRECISION(66.74199271),
    TO_INT_WITH_PRECISION(70.71067812),
    TO_INT_WITH_PRECISION(74.91535384),
    TO_INT_WITH_PRECISION(79.37005260),
    TO_INT_WITH_PRECISION(84.08964153),
    TO_INT_WITH_PRECISION(89.08987181),
    TO_INT_WITH_PRECISION(94.38743127),
    TO_INT_WITH_PRECISION(100.0000000), /* Normal sound */
    TO_INT_WITH_PRECISION(105.9463094),
    TO_INT_WITH_PRECISION(112.2462048),
    TO_INT_WITH_PRECISION(118.9207115),
    TO_INT_WITH_PRECISION(125.9921049),
    TO_INT_WITH_PRECISION(133.4839854),
    TO_INT_WITH_PRECISION(141.4213562),
    TO_INT_WITH_PRECISION(149.8307077),
    TO_INT_WITH_PRECISION(158.7401052),
    TO_INT_WITH_PRECISION(168.1792831),
    TO_INT_WITH_PRECISION(178.1797436),
    TO_INT_WITH_PRECISION(188.7748625),
    TO_INT_WITH_PRECISION(200.0000000)  /* Octave higher */
};

#define NUM_SEMITONES   ((int)(sizeof(semitone_table)/sizeof(semitone_table[0])))
#define SEMITONE_END    (NUM_SEMITONES/2)
#define SEMITONE_START  (-SEMITONE_END)

/* A table of values for approximating the cent curve with
   linear interpolation.  Multipy the next lowest semitone
   by this much to find the corresponding cent percentage.

   These values were calculated as
   x(n) = 100 * 2^(n * 20/1200)
*/

static const unsigned short cent_interp[] =
{
    TO_INT_WITH_PRECISION(100.0000000),
    TO_INT_WITH_PRECISION(101.1619440),
    TO_INT_WITH_PRECISION(102.3373892),
    TO_INT_WITH_PRECISION(103.5264924),
    TO_INT_WITH_PRECISION(104.7294123),
    /* this one's the next semitone but we have it here for convenience */
    TO_INT_WITH_PRECISION(105.9463094),
};

int viewport_get_nb_lines(const struct viewport *vp)
{
    return vp->height/rb->font_get(vp->font)->height;
}
#if 0 /* replaced with cbmp_get_icon(CBMP_Mono_7x8, Icon_ABCD, &w, &h) */
enum icons_7x8 {
    Icon_Plug,
    Icon_USBPlug,
    Icon_Mute,
    Icon_Play,
    Icon_Stop,
    Icon_Pause,
    Icon_FastForward,
    Icon_FastBackward,
    Icon_Record,
    Icon_RecPause,
    Icon_Radio,
    Icon_Radio_Mute,
    Icon_Repeat,
    Icon_RepeatOne,
    Icon_Shuffle,
    Icon_DownArrow,
    Icon_UpArrow,
    Icon_RepeatAB,
    Icon7x8Last
};

const unsigned char bitmap_icons_7x8[][7] =
{
    {0x08,0x1c,0x3e,0x3e,0x3e,0x14,0x14}, /* Power plug */
    {0x1c,0x14,0x3e,0x2a,0x22,0x1c,0x08}, /* USB plug */
    {0x01,0x1e,0x1c,0x3e,0x7f,0x20,0x40}, /* Speaker mute */
    {0x00,0x7f,0x7f,0x3e,0x1c,0x08,0x00}, /* Play */
    {0x7f,0x7f,0x7f,0x7f,0x7f,0x7f,0x7f}, /* Stop */
    {0x00,0x7f,0x7f,0x00,0x7f,0x7f,0x00}, /* Pause */
    {0x7f,0x3e,0x1c,0x7f,0x3e,0x1c,0x08}, /* Fast forward */
    {0x08,0x1c,0x3e,0x7f,0x1c,0x3e,0x7f}, /* Fast backward */
    {0x1c,0x3e,0x7f,0x7f,0x7f,0x3e,0x1c}, /* Record */
    {0x1c,0x3e,0x7f,0x00,0x7f,0x3e,0x1c}, /* Record pause */
    {0x40,0xa0,0xa0,0xa0,0x7f,0x02,0x02}, /* Radio on */
    {0x42,0xa4,0xa8,0xb0,0x7f,0x22,0x42}, /* Radio mute */
    {0x44,0x4e,0x5f,0x44,0x44,0x44,0x38}, /* Repeat playmode */
    {0x44,0x4e,0x5f,0x44,0x38,0x02,0x7f}, /* Repeat-one playmode */
    {0x3e,0x41,0x51,0x41,0x45,0x41,0x3e}, /* Shuffle playmode (dice) */
    {0x04,0x0c,0x1c,0x3c,0x1c,0x0c,0x04}, /* Down-arrow */
    {0x20,0x30,0x38,0x3c,0x38,0x30,0x20}, /* Up-arrow */
    {0x7f,0x04,0x4e,0x5f,0x44,0x38,0x7f}  /* Repeat-AB playmode */
};
#endif

/* Number of cents between entries in the cent_interp table */
#define CENT_INTERP_INTERVAL 20
#define CENT_INTERP_NUM      ((int)(sizeof(cent_interp)/sizeof(cent_interp[0])))

/* This stores whether the pitch and speed are at their own limits */
/* or that of the timestretching algorithm                         */
static bool at_limit = false;

/*
 *
 * The pitchscreen is divided into 3 viewports (each row is a viewport)
 * Then each viewport is again divided into 3 colums, each showsing some infos
 * Additionally, on touchscreen, each cell represents a button
 *
 * Below a sketch describing what each cell will show (what's drawn on it)
 * --------------------------
 * |      |        |        | <-- pitch up in the middle (text and button)
 * |      |        |        | <-- arrows for mode toggling on the sides for touchscreen
 * |------------------------|
 * |      |        |        | <-- semitone/speed up/down on the sides
 * |      |        |        | <-- reset pitch&speed in the middle
 * |------------------------|
 * |      |        |        | <-- pitch down in the middle
 * |      |        |        | <-- Two "OK" for exit on the sides for touchscreen
 * |------------------------|
 *
 *
 */

static void speak_pitch_mode(bool enqueue)
{
    bool timestretch_mode = rb->global_settings->pitch_mode_timestretch && rb->dsp_timestretch_available();
    if (timestretch_mode)
        rb->talk_id(VOICE_PITCH_TIMESTRETCH_MODE, enqueue);
    if (rb->global_settings->pitch_mode_semitone)
        rb->talk_id(VOICE_PITCH_SEMITONE_MODE, timestretch_mode ? true : enqueue);
    else
        rb->talk_id(VOICE_PITCH_ABSOLUTE_MODE, timestretch_mode ? true : enqueue);
    return;
}

/*
 * Fixes the viewports so they represent the 3 rows, and adds a little margin
 * on all sides for the icons (which are drawn outside of the grid
 *
 * The modified viewports need to be passed to the touchscreen handling function
 **/
static void pitchscreen_fix_viewports(struct viewport *parent,
        struct viewport pitch_viewports[PITCH_ITEM_COUNT])
{
    int i, font_height;
    font_height = rb->font_get(parent->font)->height;
    for (i = 0; i < PITCH_ITEM_COUNT; i++)
    {
        pitch_viewports[i] = *parent;
        pitch_viewports[i].height = parent->height / PITCH_ITEM_COUNT;
        pitch_viewports[i].x += ICON_BORDER;
        pitch_viewports[i].width -= 2*ICON_BORDER;
    }
    pitch_viewports[PITCH_TOP].y      += ICON_BORDER;
    pitch_viewports[PITCH_TOP].height -= ICON_BORDER;

    if(pitch_viewports[PITCH_MID].height < font_height * 2)
        pitch_viewports[PITCH_MID].height = font_height * 2;

    pitch_viewports[PITCH_MID].y = pitch_viewports[PITCH_TOP].y
                                 + pitch_viewports[PITCH_TOP].height;

    pitch_viewports[PITCH_BOTTOM].y = pitch_viewports[PITCH_MID].y
                                    + pitch_viewports[PITCH_MID].height;

    pitch_viewports[PITCH_BOTTOM].height -= ICON_BORDER;
}

/* must be called before pitchscreen_draw, or within
 * since it neither clears nor updates the display */
static void pitchscreen_draw_icons(struct screen *display,
                                   struct viewport *parent)
{

    display->set_viewport(parent);
    int w, h;
    const unsigned char* uparrow = cbmp_get_icon(CBMP_Mono_7x8, Icon_UpArrow, &w, &h);
    if (uparrow)
        display->mono_bitmap(uparrow, parent->width/2 - 3, 2, w, h);

    const unsigned char* dnarrow = cbmp_get_icon(CBMP_Mono_7x8, Icon_DownArrow, &w, &h);
    if (dnarrow)
        display->mono_bitmap(dnarrow, parent->width /2 - 3, parent->height - 10, w, h);

    const unsigned char* fastfwd = cbmp_get_icon(CBMP_Mono_7x8, Icon_FastForward, &w, &h);
    if (fastfwd)
        display->mono_bitmap(fastfwd, parent->width - 10, parent->height /2 - 4, 7, 8);

    const unsigned char* fastrew = cbmp_get_icon(CBMP_Mono_7x8, Icon_FastBackward, &w, &h);
    if (fastrew)
        display->mono_bitmap(fastrew, 2, parent->height /2 - 4, w, h);

    display->update_viewport();

}

static void pitchscreen_draw(struct screen *display, int max_lines,
                             struct viewport pitch_viewports[PITCH_ITEM_COUNT],
                             int32_t pitch, int32_t semitone
                             ,int32_t speed
                             )
{
    const char* ptr;
    char buf[32];
    int w, h;
    bool show_lang_pitch;
    struct viewport *last_vp = NULL;

     /* "Pitch up/Pitch down" - hide for a small screen,
      * the text is drawn centered automatically
      *
      * note: this assumes 5 lines always fit on a touchscreen (should be
      * reasonable) */
    if (max_lines >= 5)
    {
        int w, h;
        struct viewport *vp = &pitch_viewports[PITCH_TOP];
        last_vp = display->set_viewport(vp);
        display->clear_viewport();
#ifdef HAVE_TOUCHSCREEN
        /* two arrows in the top row, left and right column */
        char *arrows[] = { "<", ">"};
        display->getstringsize(arrows[0], &w, &h);
        display->putsxy(0, vp->height/2 - h/2, arrows[0]);
        display->putsxy(vp->width - w, vp->height/2 - h/2, arrows[1]);
#endif
        /* UP: Pitch Up */
        if (rb->global_settings->pitch_mode_semitone)
            ptr = rb->str(LANG_PITCH_UP_SEMITONE);
        else
            ptr = rb->str(LANG_PITCH_UP);

        display->getstringsize(ptr, &w, NULL);
        /* draw text */
        display->putsxy(vp->width/2 - w/2, 0, ptr);
        display->update_viewport();

        /* DOWN: Pitch Down */
        vp = &pitch_viewports[PITCH_BOTTOM];
        display->set_viewport(vp);
        display->clear_viewport();

#ifdef HAVE_TOUCHSCREEN
        ptr = rb->str(LANG_KBD_OK);
        display->getstringsize(ptr, &w, &h);
        /* one OK in the middle first column of the vp (at half height) */
        display->putsxy(vp->width/6 - w/2, vp->height/2 - h/2, ptr);
        /* one OK in the middle of the last column of the vp (at half height) */
        display->putsxy(5*vp->width/6 - w/2, vp->height/2 - h/2, ptr);
#endif
        if (rb->global_settings->pitch_mode_semitone)
            ptr = rb->str(LANG_PITCH_DOWN_SEMITONE);
        else
            ptr = rb->str(LANG_PITCH_DOWN);
        display->getstringsize(ptr, &w, &h);
        /* draw text */
        display->putsxy(vp->width/2 - w/2, vp->height - h, ptr);
        display->update_viewport();
    }

    /* Middle section */
    display->set_viewport(&pitch_viewports[PITCH_MID]);
    display->clear_viewport();
    int width_used = 0;

    /* Middle section upper line - hide for a small screen */
    if ((show_lang_pitch = (max_lines >= 3)))
    {
        if(rb->global_settings->pitch_mode_timestretch)
        {
            /* Pitch:XXX.X% */
            if(rb->global_settings->pitch_mode_semitone)
            {
                rb->snprintf(buf, sizeof(buf), "%s: %s%d.%02d", rb->str(LANG_PITCH),
                         semitone >= 0 ? "+" : "-",
                         abs(semitone / PITCH_SPEED_PRECISION),
                         abs((semitone % PITCH_SPEED_PRECISION) /
                                         (PITCH_SPEED_PRECISION / 100))
                        );
            }
            else
            {
                rb->snprintf(buf, sizeof(buf), "%s: %ld.%ld%%", rb->str(LANG_PITCH),
                         pitch / PITCH_SPEED_PRECISION,
                         (pitch % PITCH_SPEED_PRECISION) /
                         (PITCH_SPEED_PRECISION / 10));
            }
        }
        else
        {
            /* Rate */
            rb->snprintf(buf, sizeof(buf), "%s:", rb->str(LANG_PLAYBACK_RATE));
        }
        display->getstringsize(buf, &w, &h);
        display->putsxy((pitch_viewports[PITCH_MID].width  / 2) - (w / 2),
                        (pitch_viewports[PITCH_MID].height / 2) - h, buf);
        if (w > width_used)
            width_used = w;
    }

    /* Middle section lower line */
    /* "Speed:XXX%" */
    if(rb->global_settings->pitch_mode_timestretch)
    {
        rb->snprintf(buf, sizeof(buf), "%s: %ld.%ld%%", rb->str(LANG_SPEED),
                 speed / PITCH_SPEED_PRECISION,
                 (speed % PITCH_SPEED_PRECISION) / (PITCH_SPEED_PRECISION / 10));
    }
    else
    {
        if(rb->global_settings->pitch_mode_semitone)
        {
            rb->snprintf(buf, sizeof(buf), "%s%d.%02d",
                     semitone >= 0 ? "+" : "-",
                     abs(semitone / PITCH_SPEED_PRECISION),
                     abs((semitone % PITCH_SPEED_PRECISION) /
                                     (PITCH_SPEED_PRECISION / 100))
                    );
        }
        else
        {
            rb->snprintf(buf, sizeof(buf), "%ld.%ld%%",
                     pitch / PITCH_SPEED_PRECISION,
                     (pitch  % PITCH_SPEED_PRECISION) / (PITCH_SPEED_PRECISION / 10));
        }
    }

    display->getstringsize(buf, &w, &h);
    display->putsxy((pitch_viewports[PITCH_MID].width / 2) - (w / 2),
        show_lang_pitch ? (pitch_viewports[PITCH_MID].height / 2) :
                          (pitch_viewports[PITCH_MID].height / 2) - (h / 2),
        buf);
    if (w > width_used)
        width_used = w;

    /* "limit" and "timestretch" labels */
    if (max_lines >= 7)
    {
        if(at_limit)
        {
            const char * const p = rb->str(LANG_STRETCH_LIMIT);
            display->getstringsize(p, &w, &h);
            display->putsxy((pitch_viewports[PITCH_MID].width / 2) - (w / 2),
                            (pitch_viewports[PITCH_MID].height / 2) + h, p);
            if (w > width_used)
                width_used = w;
        }
    }

    /* Middle section left/right labels */
    const char *leftlabel = "-2%";
    const char *rightlabel = "+2%";
    if (rb->global_settings->pitch_mode_timestretch)
    {
        leftlabel = "<<";
        rightlabel = ">>";
    }

    /* Only display if they fit */
    display->getstringsize(leftlabel, &w, &h);
    width_used += w;
    display->getstringsize(rightlabel, &w, &h);
    width_used += w;

    if (width_used <= pitch_viewports[PITCH_MID].width)
    {
        display->putsxy(0, (pitch_viewports[PITCH_MID].height / 2) - (h / 2),
                        leftlabel);
        display->putsxy((pitch_viewports[PITCH_MID].width - w),
                        (pitch_viewports[PITCH_MID].height / 2) - (h / 2),
                        rightlabel);
    }
    display->update_viewport();
    display->set_viewport(last_vp);
}

static int32_t pitch_increase(int32_t pitch, int32_t pitch_delta, bool allow_cutoff
                          /* need this to maintain correct pitch/speed caps */
                          , int32_t speed
                          )
{
    int32_t new_pitch;
    int32_t new_stretch;
    at_limit = false;

    if (pitch_delta < 0)
    {
        /* for large jumps, snap up to whole numbers */
        if(allow_cutoff && pitch_delta <= -PITCH_SPEED_PRECISION &&
           (pitch + pitch_delta) % PITCH_SPEED_PRECISION != 0)
        {
            pitch_delta += PITCH_SPEED_PRECISION - ((pitch + pitch_delta) % PITCH_SPEED_PRECISION);
        }

        new_pitch = pitch + pitch_delta;

        if (new_pitch < PITCH_MIN)
        {
            if (!allow_cutoff)
            {
                return pitch;
            }
            new_pitch = PITCH_MIN;
            at_limit = true;
        }
    }
    else if (pitch_delta > 0)
    {
        /* for large jumps, snap down to whole numbers */
        if(allow_cutoff && pitch_delta >= PITCH_SPEED_PRECISION &&
           (pitch + pitch_delta) % PITCH_SPEED_PRECISION != 0)
        {
            pitch_delta -= (pitch + pitch_delta) % PITCH_SPEED_PRECISION;
        }

        new_pitch = pitch + pitch_delta;

        if (new_pitch > PITCH_MAX)
        {
            if (!allow_cutoff)
                return pitch;
            new_pitch = PITCH_MAX;
            at_limit = true;
        }
    }
    else
    {
        /* pitch_delta == 0 -> no real change */
        return pitch;
    }
    if (rb->dsp_timestretch_available())
    {
        /* increase the multiple to increase precision of this calculation */
        new_stretch = GET_STRETCH(new_pitch, speed);
        if(new_stretch < STRETCH_MIN)
        {
            /* we have to ignore allow_cutoff, because we can't have the */
            /* stretch go higher than STRETCH_MAX                        */
            new_pitch = GET_PITCH(speed, STRETCH_MIN);
        }
        else if(new_stretch > STRETCH_MAX)
        {
            /* we have to ignore allow_cutoff, because we can't have the */
            /* stretch go higher than STRETCH_MAX                        */
            new_pitch = GET_PITCH(speed, STRETCH_MAX);
        }

        if(new_stretch >= STRETCH_MAX ||
           new_stretch <= STRETCH_MIN)
        {
            at_limit = true;
        }
    }

    rb->sound_set_pitch(new_pitch);

    return new_pitch;
}

static int32_t get_semitone_from_pitch(int32_t pitch)
{
    int semitone = 0;
    int32_t fractional_index = 0;

    while(semitone < NUM_SEMITONES - 1 &&
          pitch >= semitone_table[semitone + 1])
    {
        semitone++;
    }


    /* now find the fractional part */
    while(pitch > (cent_interp[fractional_index + 1] *
                   semitone_table[semitone] / PITCH_SPEED_100))
    {
        /* Check to make sure fractional_index isn't too big */
        /* This should never happen. */
        if(fractional_index >= CENT_INTERP_NUM - 1)
        {
            break;
        }
        fractional_index++;
    }

    int32_t semitone_pitch_a = cent_interp[fractional_index] *
                               semitone_table[semitone] /
                               PITCH_SPEED_100;
    int32_t semitone_pitch_b = cent_interp[fractional_index + 1] *
                               semitone_table[semitone] /
                               PITCH_SPEED_100;
    /* this will be the integer offset from the cent_interp entry */
    int32_t semitone_frac_ofs = (pitch - semitone_pitch_a) * CENT_INTERP_INTERVAL /
                            (semitone_pitch_b - semitone_pitch_a);
    semitone = (semitone + SEMITONE_START) * PITCH_SPEED_PRECISION +
                     fractional_index * CENT_INTERP_INTERVAL +
                     semitone_frac_ofs;

    return semitone;
}

static int32_t get_pitch_from_semitone(int32_t semitone)
{
    int32_t adjusted_semitone = semitone - SEMITONE_START * PITCH_SPEED_PRECISION;

    /* Find the index into the semitone table */
    int32_t semitone_index = (adjusted_semitone / PITCH_SPEED_PRECISION);

    /* set pitch to the semitone's integer part value */
    int32_t pitch = semitone_table[semitone_index];
    /* get the range of the cent modification for future calculation */
    int32_t pitch_mod_a =
        cent_interp[(adjusted_semitone % PITCH_SPEED_PRECISION) /
                    CENT_INTERP_INTERVAL];
    int32_t pitch_mod_b =
        cent_interp[(adjusted_semitone % PITCH_SPEED_PRECISION) /
                    CENT_INTERP_INTERVAL + 1];
    /* figure out the cent mod amount based on the semitone fractional value */
    int32_t pitch_mod = pitch_mod_a + (pitch_mod_b - pitch_mod_a) *
                   (adjusted_semitone % CENT_INTERP_INTERVAL) / CENT_INTERP_INTERVAL;

    /* modify pitch based on the mod amount we just calculated */
    return (pitch * pitch_mod  + PITCH_SPEED_100 / 2) / PITCH_SPEED_100;
}

static int32_t pitch_increase_semitone(int32_t pitch,
                                       int32_t current_semitone,
                                       int32_t semitone_delta
                                       , int32_t speed
                                      )
{
    int32_t new_semitone = current_semitone;

    /* snap to the delta interval */
    if(current_semitone % semitone_delta != 0)
    {
        if(current_semitone > 0 && semitone_delta > 0)
            new_semitone += semitone_delta;
        else if(current_semitone < 0 && semitone_delta < 0)
            new_semitone += semitone_delta;

        new_semitone -= new_semitone % semitone_delta;
    }
    else
        new_semitone += semitone_delta;

    /* clamp the pitch so it doesn't go beyond the pitch limits */
    if(new_semitone < (SEMITONE_START * PITCH_SPEED_PRECISION))
    {
        new_semitone = SEMITONE_START * PITCH_SPEED_PRECISION;
        at_limit = true;
    }
    else if(new_semitone > (SEMITONE_END * PITCH_SPEED_PRECISION))
    {
        new_semitone = SEMITONE_END * PITCH_SPEED_PRECISION;
        at_limit = true;
    }

    int32_t new_pitch = get_pitch_from_semitone(new_semitone);

    int32_t new_stretch = GET_STRETCH(new_pitch, speed);

    /* clamp the pitch so it doesn't go beyond the stretch limits */
    if( new_stretch > STRETCH_MAX)
    {
        new_pitch = GET_PITCH(speed, STRETCH_MAX);
        new_semitone = get_semitone_from_pitch(new_pitch);
        at_limit = true;
    }
    else if (new_stretch < STRETCH_MIN)
    {
        new_pitch = GET_PITCH(speed, STRETCH_MIN);
        new_semitone = get_semitone_from_pitch(new_pitch);
        at_limit = true;
    }

    pitch_increase(pitch, new_pitch - pitch, false
                   , speed
                   );

    return new_semitone;
}

#ifdef HAVE_TOUCHSCREEN
/*
 * Check for touchscreen presses as per sketch above in this file
 *
 * goes through each row of the, checks whether the touchscreen
 * was pressed in it. Then it looks the columns of each row for specific actions
 */
static int pitchscreen_do_touchscreen(struct viewport vps[])
{
    short x, y;
    struct viewport *this_vp = &vps[PITCH_TOP];
    int ret;
    static bool wait_for_release = false;
    ret = rb->action_get_touchscreen_press_in_vp(&x, &y, this_vp);

    /* top row */
    if (ret > ACTION_UNKNOWN)
    {   /* press on top row, left or right column
         * only toggle mode if released */
        int column = this_vp->width / 3;
        if ((x < column || x > (2*column)) && (ret == BUTTON_REL))
            return ACTION_PS_TOGGLE_MODE;


        else if (x >= column && x <= (2*column))
        {   /* center column pressed */
            if (ret == BUTTON_REPEAT)
                return ACTION_PS_INC_BIG;
            else if (ret & BUTTON_REL)
                return ACTION_PS_INC_SMALL;
        }
        return ACTION_NONE;
    }

    /* now the center row */
    this_vp = &vps[PITCH_MID];
    ret = rb->action_get_touchscreen_press_in_vp(&x, &y, this_vp);

    if (ret > ACTION_UNKNOWN)
    {
        int column = this_vp->width / 3;

        if (x < column)
        {   /* left column */
            if (ret & BUTTON_REL)
            {
                wait_for_release = false;
                return ACTION_PS_NUDGE_LEFTOFF;
            }
            else if (ret & BUTTON_REPEAT)
                return ACTION_PS_SLOWER;
            if (!wait_for_release)
            {
                wait_for_release = true;
                return ACTION_PS_NUDGE_LEFT;
            }
        }
        else if (x > (2*column))
        {   /* right column */
            if (ret & BUTTON_REL)
            {
                wait_for_release = false;
                return ACTION_PS_NUDGE_RIGHTOFF;
            }
            else if (ret & BUTTON_REPEAT)
                return ACTION_PS_FASTER;
            if (!wait_for_release)
            {
                wait_for_release = true;
                return ACTION_PS_NUDGE_RIGHT;
            }
        }
        else
            /* center column was pressed */
            return ACTION_PS_RESET;
    }

    /* now the bottom row */
    this_vp = &vps[PITCH_BOTTOM];
    ret = rb->action_get_touchscreen_press_in_vp(&x, &y, this_vp);

    if (ret > ACTION_UNKNOWN)
    {
        int column = this_vp->width / 3;

        /* left or right column is exit */
        if ((x < column || x > (2*column)) && (ret == BUTTON_REL))
            return ACTION_PS_EXIT;
        else if (x >= column && x <= (2*column))
        {   /* center column was pressed */
            if (ret & BUTTON_REPEAT)
                return ACTION_PS_DEC_BIG;
            else if (ret & BUTTON_REL)
                return ACTION_PS_DEC_SMALL;
        }
        return ACTION_NONE;
    }
    return ACTION_NONE;
}

#endif
/*
    returns:
    0 on exit
    1 if USB was connected
*/

int gui_syncpitchscreen_run(void)
{
    int button;
    int32_t pitch = rb->sound_get_pitch();
    int32_t semitone;

    int32_t new_pitch;
    int32_t pitch_delta;
    bool nudged = false;
    int i, updated = 4, decimals = 0;
    bool exit = false;
    /* should maybe be passed per parameter later, not needed for now */
    struct viewport parent[NB_SCREENS];
    struct viewport pitch_viewports[NB_SCREENS][PITCH_ITEM_COUNT];
    int max_lines[NB_SCREENS];

    //push_current_activity(ACTIVITY_PITCHSCREEN);

    int32_t new_speed = 0, new_stretch;

    /* the speed variable holds the apparent speed of the playback */
    int32_t speed;
    if (rb->dsp_timestretch_available())
    {
        speed = GET_SPEED(pitch, rb->dsp_get_timestretch());
    }
    else
    {
        speed = pitch;
    }



    /* Count decimals for speaking */
    for (i = PITCH_SPEED_PRECISION; i >= 10; i /= 10)
        decimals++;

    /* set the semitone index based on the current pitch */
    semitone = get_semitone_from_pitch(pitch);

    /* initialize pitchscreen vps */
    FOR_NB_SCREENS(i)
    {
        rb->viewport_set_defaults(&parent[i], i);
        max_lines[i] = viewport_get_nb_lines(&parent[i]);
        pitchscreen_fix_viewports(&parent[i], pitch_viewports[i]);
        rb->screens[i]->set_viewport(&parent[i]);
        rb->screens[i]->clear_viewport();

        /* also, draw the icons now, it's only needed once */
        pitchscreen_draw_icons(rb->screens[i], &parent[i]);
    }


    while (!exit)
    {
        FOR_NB_SCREENS(i)
            pitchscreen_draw(rb->screens[i], max_lines[i],
                              pitch_viewports[i], pitch, semitone
                              , speed
                              );
        pitch_delta = 0;
        new_speed = 0;

        if (rb->global_settings->talk_menu && updated)
        {
            rb->talk_shutup();
            switch (updated)
            {
            case 1:
                if (rb->global_settings->pitch_mode_semitone)
                    rb->talk_value_decimal(semitone, UNIT_SIGNED, decimals, false);
                else
                    rb->talk_value_decimal(pitch, UNIT_PERCENT, decimals, false);
                break;
            case 2:
                rb->talk_value_decimal(speed, UNIT_PERCENT, decimals, false);
                break;
            case 3:
                speak_pitch_mode(false);
                break;
            case 4:
                if (rb->global_settings->pitch_mode_timestretch && rb->dsp_timestretch_available())
                    rb->talk_id(LANG_PITCH, false);
                else
                    rb->talk_id(LANG_PLAYBACK_RATE, false);
                rb->talk_value_decimal(pitch, UNIT_PERCENT, decimals, true);
                if (rb->global_settings->pitch_mode_timestretch && rb->dsp_timestretch_available())
                {
                    rb->talk_id(LANG_SPEED, true);
                    rb->talk_value_decimal(speed, UNIT_PERCENT, decimals, true);
                }
                speak_pitch_mode(true);
                break;
            default:
                break;
            }
        }
        updated = 0;

        button = rb->get_action(CONTEXT_PITCHSCREEN, HZ);

#ifdef HAVE_TOUCHSCREEN
        if (button == ACTION_TOUCHSCREEN)
        {
            FOR_NB_SCREENS(i)
                button = pitchscreen_do_touchscreen(pitch_viewports[i]);
        }
#endif
        switch (button)
        {
            case ACTION_PS_INC_SMALL:
                if(rb->global_settings->pitch_mode_semitone)
                    pitch_delta = SEMITONE_SMALL_DELTA;
                else
                    pitch_delta = PITCH_SMALL_DELTA;
                updated = 1;
                break;

            case ACTION_PS_INC_BIG:
                if(rb->global_settings->pitch_mode_semitone)
                    pitch_delta = SEMITONE_BIG_DELTA;
                else
                    pitch_delta = PITCH_BIG_DELTA;
                updated = 1;
                break;

            case ACTION_PS_DEC_SMALL:
                if(rb->global_settings->pitch_mode_semitone)
                    pitch_delta = -SEMITONE_SMALL_DELTA;
                else
                    pitch_delta = -PITCH_SMALL_DELTA;
                updated = 1;
                break;

            case ACTION_PS_DEC_BIG:
                if(rb->global_settings->pitch_mode_semitone)
                    pitch_delta = -SEMITONE_BIG_DELTA;
                else
                    pitch_delta = -PITCH_BIG_DELTA;
                updated = 1;
                break;

            case ACTION_PS_NUDGE_RIGHT:
                if (!rb->global_settings->pitch_mode_timestretch)
                {
                    new_pitch = pitch_increase(pitch, PITCH_NUDGE_DELTA, false
                                               , speed
                        );
                    nudged = (new_pitch != pitch);
                    pitch = new_pitch;
                    semitone = get_semitone_from_pitch(pitch);
                    speed = pitch;
                    updated = nudged ? 1 : 0;
                    break;
                }
                else
                {
                    new_speed = speed + SPEED_SMALL_DELTA;
                    at_limit = false;
                    updated = 2;
                }
                break;

            case ACTION_PS_FASTER:
                if (rb->global_settings->pitch_mode_timestretch)
                {
                    new_speed = speed + SPEED_BIG_DELTA;
                    /* snap to whole numbers */
                    if(new_speed % PITCH_SPEED_PRECISION != 0)
                        new_speed -= new_speed % PITCH_SPEED_PRECISION;
                    at_limit = false;
                    updated = 2;
                }
                break;

            case ACTION_PS_NUDGE_RIGHTOFF:
                if (nudged)
                {
                    pitch = pitch_increase(pitch, -PITCH_NUDGE_DELTA, false
                                           , speed
                        );
                    speed = pitch;
                    semitone = get_semitone_from_pitch(pitch);
                    nudged = false;
                    updated = 1;
                }
                break;

            case ACTION_PS_NUDGE_LEFT:
                if (!rb->global_settings->pitch_mode_timestretch)
                {
                    new_pitch = pitch_increase(pitch, -PITCH_NUDGE_DELTA, false
                                               , speed
                        );
                    nudged = (new_pitch != pitch);
                    pitch = new_pitch;
                    semitone = get_semitone_from_pitch(pitch);
                    speed = pitch;
                    updated = nudged ? 1 : 0;
                    break;
                }
                else
                {
                    new_speed = speed - SPEED_SMALL_DELTA;
                    at_limit = false;
                    updated = 2;
                }
                break;

            case ACTION_PS_SLOWER:
                if (rb->global_settings->pitch_mode_timestretch)
                {
                    new_speed = speed - SPEED_BIG_DELTA;
                    /* snap to whole numbers */
                    if(new_speed % PITCH_SPEED_PRECISION != 0)
                        new_speed += PITCH_SPEED_PRECISION - speed % PITCH_SPEED_PRECISION;
                    at_limit = false;
                    updated = 2;
                }
                break;

            case ACTION_PS_NUDGE_LEFTOFF:
                if (nudged)
                {
                    pitch = pitch_increase(pitch, PITCH_NUDGE_DELTA, false
                                           , speed
                        );
                    speed = pitch;
                    semitone = get_semitone_from_pitch(pitch);
                    nudged = false;
                    updated = 1;
                }
                break;

            case ACTION_PS_RESET:
                pitch = PITCH_SPEED_100;
                rb->sound_set_pitch(pitch);
                speed = PITCH_SPEED_100;
                if (rb->dsp_timestretch_available())
                {
                    rb->dsp_set_timestretch(PITCH_SPEED_100);
                    at_limit = false;
                }
                semitone = get_semitone_from_pitch(pitch);
                updated = 4;
                break;

            case ACTION_PS_TOGGLE_MODE:
                rb->global_settings->pitch_mode_semitone = !rb->global_settings->pitch_mode_semitone;

                if (rb->dsp_timestretch_available() && !rb->global_settings->pitch_mode_semitone)
                {
                    rb->global_settings->pitch_mode_timestretch = !rb->global_settings->pitch_mode_timestretch;
                    if(!rb->global_settings->pitch_mode_timestretch)
                    {
                        /* no longer in timestretch mode.  Reset speed */
                        speed = pitch;
                        rb->dsp_set_timestretch(PITCH_SPEED_100);
                    }
                }
                rb->settings_save();
                updated = 3;
                break;

            case ACTION_PS_EXIT:
                exit = true;
                break;

            default:
                if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
                    return 1;
                break;
        }
        if (pitch_delta)
        {
            if (rb->global_settings->pitch_mode_semitone)
            {
                semitone = pitch_increase_semitone(pitch, semitone, pitch_delta
                                                , speed
                );
                pitch = get_pitch_from_semitone(semitone);
            }
            else
            {
                pitch = pitch_increase(pitch, pitch_delta, true
                                       , speed
                );
                semitone = get_semitone_from_pitch(pitch);
            }
            if (rb->global_settings->pitch_mode_timestretch)
            {
                /* do this to make sure we properly obey the stretch limits */
                new_speed = speed;
            }
            else
            {
                speed = pitch;
            }
        }

        if(new_speed)
        {
            new_stretch = GET_STRETCH(pitch, new_speed);

            /* limit the amount of stretch */
            if(new_stretch > STRETCH_MAX)
            {
                new_stretch = STRETCH_MAX;
                new_speed = GET_SPEED(pitch, new_stretch);
            }
            else if(new_stretch < STRETCH_MIN)
            {
                new_stretch = STRETCH_MIN;
                new_speed = GET_SPEED(pitch, new_stretch);
            }

            new_stretch = GET_STRETCH(pitch, new_speed);
            if(new_stretch >= STRETCH_MAX ||
               new_stretch <= STRETCH_MIN)
            {
                at_limit = true;
            }

            /* set the amount of stretch */
            rb->dsp_set_timestretch(new_stretch);

            /* update the speed variable with the new speed */
            speed = new_speed;

            /* Reset new_speed so we only call dsp_set_timestretch */
            /* when needed                                         */
            new_speed = 0;
        }
    }

    //rb->pcmbuf_set_low_latency(false);
    //pop_current_activity();

    /* Clean up */
    FOR_NB_SCREENS(i)
    {
        rb->screens[i]->set_viewport(NULL);
    }

    return 0;
}

static int arg_callback(char argchar, const char **parameter)
{
    int ret;
    long num, dec;
    bool bret;
    //rb->splashf(100, "Arg: %c", argchar);
    while (*parameter[0] > '/' && ispunct(*parameter[0])) (*parameter)++;
    switch (tolower(argchar))
    {
        case 'q' :
            pitch_vars.flags &= ~PVAR_VERBOSE;
            break;
        case 'g' :
            pitch_vars.flags |= PVAR_GUI;
            break;
        case 'p' :
            ret = longnum_parse(parameter, &num, &dec);
            if (ret)
            {
                dec /= (ARGPARSE_FRAC_DEC_MULTIPLIER / PITCH_SPEED_PRECISION);
                if (num < 0)
                    dec = -dec;
                pitch_vars.pitch = (num * PITCH_SPEED_PRECISION + (dec % PITCH_SPEED_PRECISION));
            }
            break;
        case 'k' :
            ret = bool_parse(parameter, &bret);
            if (ret)
            {
                if(!bret && rb->dsp_timestretch_available())
                {
                    /* no longer in timestretch mode.  Reset speed */
                    rb->dsp_set_timestretch(PITCH_SPEED_100);
                }
                rb->dsp_timestretch_enable(bret);
                if ((pitch_vars.flags & PVAR_VERBOSE) == PVAR_VERBOSE)
                    rb->splashf(HZ, "Timestretch: %s", bret ? "true" : "false");
                int n = 10; /* 1 second */
                while (bret && n-- > 0 && !rb->dsp_timestretch_available())
                {
                    rb->sleep(HZ / 10);
                }
            }
            break;
        case 's' :
            ret = longnum_parse(parameter, &num, &dec);
            if (ret && rb->dsp_timestretch_available())
            {
                dec /= (ARGPARSE_FRAC_DEC_MULTIPLIER / PITCH_SPEED_PRECISION);
                if (num < 0)
                    break;
                pitch_vars.speed = (num * PITCH_SPEED_PRECISION + (dec % PITCH_SPEED_PRECISION));

            }
            break;
        default :
            rb->splashf(HZ, "Unknown switch '%c'",argchar);
            //return 0;
    }

    return 1;
}

void fill_pitchvars(struct pvars *pv)
{
    if (!pv)
        return;
    pv->pitch = rb->sound_get_pitch();

    /* the speed variable holds the apparent speed of the playback */
    if (rb->dsp_timestretch_available())
    {
        pv->speed = GET_SPEED(pv->pitch, rb->dsp_get_timestretch());
    }
    else
    {
        pv->speed = pv->pitch;
    }

    pv->stretch = GET_STRETCH(pv->pitch, pv->speed);
    pv->flags |= PVAR_VERBOSE;

}
/* plugin entry point */
enum plugin_status plugin_start(const void* parameter)
{
 /* pitch_screen
 *  accepts args -q, -g, -p=, -s=, -k=; (= sign is optional)
 * -q silences output splash
 * -g runs the gui (DEFAULT)
 * -p100 would set pitch to 100%
 * -s=90 sets speed to 90% if timestrech is enabled
 * -k=true -k1 enables time stretch -k0 -kf-kn disables
*/
    bool gui = false;
    rb->pcmbuf_set_low_latency(true);

    /* Figure out whether to be in timestretch mode */
    if (parameter == NULL) /* gui mode */
    {
        if (rb->global_settings->pitch_mode_timestretch && !rb->dsp_timestretch_available())
        {
            rb->global_settings->pitch_mode_timestretch = false;
            rb->settings_save();
        }
        gui = true;
    }
    else
    {
        struct pvars cur;
        fill_pitchvars(&cur);
        fill_pitchvars(&pitch_vars);
        argparse((const char*) parameter, -1, &arg_callback);
        if (pitch_vars.pitch != cur.pitch)
        {
            rb->sound_set_pitch(pitch_vars.pitch);
            pitch_vars.pitch = rb->sound_get_pitch();
            if ((pitch_vars.flags & PVAR_VERBOSE) == PVAR_VERBOSE)
                rb->splashf(HZ, "pitch: %ld.%02ld%%",
                            pitch_vars.pitch / PITCH_SPEED_PRECISION,
                            pitch_vars.pitch % PITCH_SPEED_PRECISION);
        }
        if (pitch_vars.speed != cur.speed)
        {
                pitch_vars.stretch = GET_STRETCH(pitch_vars.pitch, pitch_vars.speed);

                /* limit the amount of stretch */
                if(pitch_vars.stretch > STRETCH_MAX)
                {
                    pitch_vars.stretch = STRETCH_MAX;
                    pitch_vars.speed = GET_SPEED(pitch_vars.pitch, pitch_vars.stretch);
                }
                else if(pitch_vars.stretch < STRETCH_MIN)
                {
                    pitch_vars.stretch = STRETCH_MIN;
                    pitch_vars.speed = GET_SPEED(pitch_vars.pitch, pitch_vars.stretch);
                }

                pitch_vars.stretch = GET_STRETCH(pitch_vars.pitch, pitch_vars.speed);
                if ((pitch_vars.flags & PVAR_VERBOSE) == PVAR_VERBOSE)
                    rb->splashf(HZ, "speed: %ld.%02ld%%",
                                pitch_vars.speed / PITCH_SPEED_PRECISION,
                                pitch_vars.speed % PITCH_SPEED_PRECISION);
                /* set the amount of stretch */
                rb->dsp_set_timestretch(pitch_vars.stretch);
        }
        gui = ((pitch_vars.flags & PVAR_GUI) == PVAR_GUI);
        if ((pitch_vars.flags & PVAR_VERBOSE) == PVAR_VERBOSE)
            rb->splashf(HZ, "GUI: %d", gui);

    }

    if (gui && gui_syncpitchscreen_run() == 1)
            return PLUGIN_USB_CONNECTED;
    rb->pcmbuf_set_low_latency(false);
    return PLUGIN_OK;
}
