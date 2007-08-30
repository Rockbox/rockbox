/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Robert Hak <rhak at ramapo.edu>
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"
#include "helper.h"

PLUGIN_HEADER

#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)

#define QUIT BUTTON_OFF
#define UP BUTTON_UP
#define DOWN BUTTON_DOWN

#elif CONFIG_KEYPAD == ONDIO_PAD

#define QUIT BUTTON_OFF
#define UP BUTTON_UP
#define DOWN BUTTON_DOWN

#elif CONFIG_KEYPAD == RECORDER_PAD

#define QUIT BUTTON_OFF
#define UP BUTTON_UP
#define DOWN BUTTON_DOWN

#elif CONFIG_KEYPAD == ARCHOS_AV300_PAD

#define QUIT BUTTON_OFF
#define UP BUTTON_UP
#define DOWN BUTTON_DOWN

#elif (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_4G_PAD)

#define QUIT BUTTON_MENU
#define UP BUTTON_SCROLL_BACK
#define DOWN BUTTON_SCROLL_FWD

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)

#define QUIT BUTTON_A
#define UP BUTTON_UP
#define DOWN BUTTON_DOWN

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD /* grayscale at the moment */

#define QUIT BUTTON_POWER
#define UP BUTTON_UP
#define DOWN BUTTON_DOWN

#elif CONFIG_KEYPAD == SANSA_E200_PAD

#define QUIT BUTTON_POWER
#define UP BUTTON_SCROLL_UP
#define DOWN BUTTON_SCROLL_DOWN

#elif CONFIG_KEYPAD == IRIVER_H10_PAD /* grayscale at the moment */

#define QUIT BUTTON_POWER
#define UP BUTTON_SCROLL_UP
#define DOWN BUTTON_SCROLL_DOWN

#elif CONFIG_KEYPAD == PLAYER_PAD

#define QUIT BUTTON_STOP
#define UP BUTTON_LEFT
#define DOWN BUTTON_RIGHT

#elif CONFIG_KEYPAD == IPOD_1G2G_PAD

#define QUIT (BUTTON_PLAY|BUTTON_REPEAT)
#define UP BUTTON_SCROLL_BACK
#define DOWN BUTTON_SCROLL_FWD

#else
#error Unsupported keypad
#endif


void roll_credits(void);
const char* const credits[] = {
#include "credits.raw" /* generated list of names from docs/CREDITS */
};

static struct plugin_api* rb;

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    int j = 0;
    int btn;

    (void)parameter;
    rb = api;

    /* Turn off backlight timeout */
    backlight_force_on(rb); /* backlight control in lib/helper.c */

    rb->show_logo();
#ifdef HAVE_LCD_CHARCELLS
    rb->lcd_double_height(false);
#endif

    /* Show the logo for about 3 secs allowing the user to stop */
    for (j = 0; j < 15; j++) {
        rb->sleep((HZ*2)/10);

        btn = rb->button_get(false);
        if (btn !=  BUTTON_NONE && (btn & QUIT))
            goto end_of_proc;
    }

    roll_credits();

end_of_proc:
    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings(rb); /* backlight control in lib/helper.c */

    return PLUGIN_OK;
}

#ifdef HAVE_LCD_CHARCELLS

void roll_credits(void)
{
    int numnames = sizeof(credits)/sizeof(char*);
    int curr_name = 0;
    int curr_len = rb->utf8length(credits[0]);
    int curr_index = 0;
    int curr_line = 0;
    int name, len, new_len, line, x;

    while (1)
    {
        rb->lcd_clear_display();

        name = curr_name;
        x = -curr_index;
        len = curr_len;
        line = curr_line;

        while (x < 11)
        {
            int x2;

            if (x < 0)
                rb->lcd_puts(0, line, credits[name] + rb->utf8seek(credits[name], -x));
            else
                rb->lcd_puts(x, line, credits[name]);

            if (++name >= numnames)
                break;

            line ^= 1;

            x2 = x + len/2;
            if ((unsigned)x2 < 11)
                rb->lcd_putc(x2, line, '*');

            new_len = rb->utf8length(credits[name]);
            x += MAX(len/2 + 2, len - new_len/2 + 1);
            len = new_len;
        }
        rb->lcd_update();

        /* abort on keypress */
        if (rb->button_get_w_tmo(HZ/8) & QUIT)
            return;

        if (++curr_index >= curr_len)
        {
            if (++curr_name >= numnames)
                break;
            new_len = rb->utf8length(credits[curr_name]);
            curr_index -= MAX(curr_len/2 + 2, curr_len - new_len/2 + 1);
            curr_len = new_len;
            curr_line ^= 1;
        }
    }
}

#else

void roll_credits(void)
{
#if (CONFIG_KEYPAD == RECORDER_PAD)
    #define PAUSE_TIME 1.2
    #define ANIM_SPEED 35
#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
    #define PAUSE_TIME 0
    #define ANIM_SPEED 100
#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
    #define PAUSE_TIME 0
    #define ANIM_SPEED 35
#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
    #define PAUSE_TIME 0
    #define ANIM_SPEED 100
#else
    #define PAUSE_TIME 1
    #define ANIM_SPEED 40
#endif

    #define NUM_VISIBLE_LINES (LCD_HEIGHT/font_h - 1)
    #define CREDITS_TARGETPOS ((LCD_WIDTH/2)-(credits_w/2))

    int i=0, j=0, k=0, namepos=0, offset_dummy, btn;
    int name_w, name_h, name_targetpos=1, font_h;
    int credits_w, credits_pos;
    int numnames = (sizeof(credits)/sizeof(char*));
    char name[40], elapsednames[20];

    rb->lcd_setfont(FONT_UI);
    rb->lcd_clear_display();
    rb->lcd_update();

    rb->lcd_getstringsize("A", NULL, &font_h);

    /* snprintf "credits" text, and save the width and height */
    rb->snprintf(elapsednames, sizeof(elapsednames), "[Credits] %d/%d",
                 j+1, numnames);
    rb->lcd_getstringsize(elapsednames, &credits_w, NULL);

    /* fly in "credits" text from the left */
    for(credits_pos = 0 - credits_w; credits_pos <= CREDITS_TARGETPOS;
        credits_pos += (CREDITS_TARGETPOS-credits_pos + 14) / 7)
    {
        rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        rb->lcd_fillrect(0, 0, LCD_WIDTH, font_h);
        rb->lcd_set_drawmode(DRMODE_SOLID);
        rb->lcd_putsxy(credits_pos, 0, elapsednames);
        rb->lcd_update_rect(0, 0, LCD_WIDTH, font_h);
        rb->sleep(HZ/ANIM_SPEED);
    }

    /* first screen's worth of lines fly in */
    for(i=0; i<NUM_VISIBLE_LINES; i++)
    {
        rb->snprintf(name, sizeof(name), "%s", credits[i]);
        rb->lcd_getstringsize(name, &name_w, &name_h);

        rb->snprintf(elapsednames, sizeof(elapsednames), "[Credits] %d/%d",
                     i+1, numnames);
        rb->lcd_getstringsize(elapsednames, &credits_w, NULL);
        rb->lcd_putsxy(CREDITS_TARGETPOS, 0, elapsednames);

        for(namepos = 0-name_w; namepos <= name_targetpos;
            namepos += (name_targetpos - namepos + 14) / 7)
        {
            rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
            rb->lcd_fillrect(0, font_h*(i+1), LCD_WIDTH, font_h); /* clear any trails left behind */
            rb->lcd_set_drawmode(DRMODE_SOLID);
            rb->lcd_putsxy(namepos, font_h*(i+1), name);
            rb->lcd_update_rect(0, font_h*(i+1), LCD_WIDTH, font_h);
            rb->lcd_update_rect(CREDITS_TARGETPOS, 0, credits_w, font_h);

            /* exit on power key */
            btn = rb->button_get_w_tmo(HZ/ANIM_SPEED);
            if (btn !=  BUTTON_NONE && (btn & QUIT))
                return;
        }
    }
    j+=i;

    /* pause for a bit if needed */
    btn = rb->button_get_w_tmo(HZ*PAUSE_TIME); /* exit on keypress */
    if (btn !=  BUTTON_NONE && (btn & QUIT))
        return;

    /* now begin looping the in-out animation */
    do {
      for(; j < numnames; j+=i)
        {
          /* just a screen's worth at a time */
          for(i=0; i<NUM_VISIBLE_LINES; i++)
            {
              if(j+i >= numnames)
                break;

              offset_dummy=1;

              rb->snprintf(name, sizeof(name), "%s", 
                           credits[(j>=NUM_VISIBLE_LINES)?
                                 j+i-NUM_VISIBLE_LINES:j+i]);
              rb->lcd_getstringsize(name, &name_w, &name_h);

              /* fly out an existing line.. */
              while(namepos<LCD_WIDTH+offset_dummy)
              {
                rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                rb->lcd_fillrect(0, font_h*(i+1), LCD_WIDTH, font_h); /* clear trails */
                rb->lcd_set_drawmode(DRMODE_SOLID);
                rb->lcd_putsxy(namepos, font_h*(i+1), name);
                rb->lcd_update_rect(0, font_h*(i+1), LCD_WIDTH, font_h);

                /* exit on keypress, react to scrolling */
                btn = rb->button_get_w_tmo(HZ/ANIM_SPEED);
                if (btn !=  BUTTON_NONE) 
                {
                  if (btn & QUIT)
                    return;
                  else if ((btn & UP) ^ (btn & DOWN)) 
                  {
                    /* compute the new position */
                    j+=((btn & UP)?-1:1)*(NUM_VISIBLE_LINES/2);
                    if (j+i >= numnames) j=numnames-i-1;
                    if (j < 0) j = 0;
                  
                    /* and refresh the whole screen */
                    rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                    rb->lcd_fillrect(0, 0, LCD_WIDTH, 
                                     font_h * (NUM_VISIBLE_LINES+1));
                    rb->lcd_set_drawmode(DRMODE_SOLID);
                    
                    rb->snprintf(elapsednames, sizeof(elapsednames), 
                                 "[Credits] %d/%d", j+i+1, numnames);
                    rb->lcd_getstringsize(elapsednames, &credits_w, NULL);
                    rb->lcd_putsxy(CREDITS_TARGETPOS, 0, elapsednames);
                    
                    for (k=0; k<NUM_VISIBLE_LINES; k++)
                      if (k!=i)
                      {
                        rb->snprintf(name, sizeof(name), "%s", 
                                     credits[(j>=NUM_VISIBLE_LINES)?
                                             ((k<i)?
                                              (j+k):(j+k-NUM_VISIBLE_LINES)):
                                             j+k]);
                        rb->lcd_putsxy(0, font_h*(k+1), name);
                      }
                    rb->lcd_update_rect(0, font_h, LCD_WIDTH, 
                                        font_h * (NUM_VISIBLE_LINES+1));
                    break;
                  }
                }
                namepos += offset_dummy;
                offset_dummy++;
              }

              rb->snprintf(name, sizeof(name), "%s", credits[j+i]);
              rb->lcd_getstringsize(name, &name_w, &name_h);

              rb->snprintf(elapsednames, sizeof(elapsednames), "[Credits] %d/%d",
                           j+i+1, numnames);
              rb->lcd_getstringsize(elapsednames, &credits_w, NULL);
              rb->lcd_putsxy(CREDITS_TARGETPOS, 0, elapsednames);
              if (j+i < NUM_VISIBLE_LINES) /* takes care of trail on loop */
                rb->lcd_update_rect(0, 0, LCD_WIDTH, font_h);

              for(namepos = 0-name_w; namepos <= name_targetpos;
                  namepos += (name_targetpos - namepos + 14) / 7)
              {
                rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
                rb->lcd_fillrect(0, font_h*(i+1), LCD_WIDTH, font_h);
                rb->lcd_set_drawmode(DRMODE_SOLID);
                rb->lcd_putsxy(namepos, font_h*(i+1), name);
                rb->lcd_update_rect(0, font_h*(i+1), LCD_WIDTH, font_h);
                rb->lcd_update_rect(CREDITS_TARGETPOS, 0, credits_w, font_h);

                /* exit on keypress */
                btn = rb->button_get_w_tmo(HZ/ANIM_SPEED);
                if (btn !=  BUTTON_NONE && (btn & QUIT))
                  return;
              }

              namepos = name_targetpos;

              /* ..and repeat. */
            }

          btn = rb->button_get_w_tmo(HZ*PAUSE_TIME); /* exit on keypress */
          if (btn !=  BUTTON_NONE && (btn & QUIT))
            return;
        }

      j = 0;
      if(k) {
        /* on loop, the new credit line might shorten */
        rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        rb->lcd_fillrect(0, 0, LCD_WIDTH, font_h);
      }
    } while(k); /* repeat in-out animation forever if scrolling occured */

    btn = rb->button_get_w_tmo(HZ*2.5); /* exit on keypress */
    if (btn !=  BUTTON_NONE && (btn & QUIT))
        return;

    offset_dummy = 1;

    /* now make the text exit to the right */
    for(credits_pos = (LCD_WIDTH/2)-(credits_w/2); credits_pos <= LCD_WIDTH+offset_dummy;
        credits_pos += offset_dummy, offset_dummy++)
    {
        rb->lcd_set_drawmode(DRMODE_SOLID|DRMODE_INVERSEVID);
        rb->lcd_fillrect(0, 0, LCD_WIDTH, font_h);
        rb->lcd_set_drawmode(DRMODE_SOLID);
        rb->lcd_putsxy(credits_pos, 0, elapsednames);
        rb->lcd_update();
    }
}

#endif
