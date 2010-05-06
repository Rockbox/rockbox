/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Linus Nielsen Feltzing
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
#include "highscore.h"

static bool highscore_updated = false;

int highscore_save(char *filename, struct highscore *scores, int num_scores)
{
    int i;
    int fd;
    int rc;
    char buf[80];

    if(!highscore_updated)
        return 1;

    fd = rb->open(filename, O_WRONLY|O_CREAT, 0666);
    if(fd < 0)
        return -1;
    
    for(i = 0;i < num_scores;i++)
    {
        rb->snprintf(buf, sizeof(buf), "%d:%d:%s\n",
                     scores[i].score, scores[i].level, scores[i].name);
        rc = rb->write(fd, buf, rb->strlen(buf));
        if(rc < 0)
        {
            rb->close(fd);
            return -2;
        }
    }
    rb->close(fd);
    highscore_updated = false;
    return 0;
}

int highscore_load(char *filename, struct highscore *scores, int num_scores)
{
    int i;
    int fd;
    char buf[80];
    char *score, *level, *name;

    rb->memset(scores, 0, sizeof(struct highscore)*num_scores);

    fd = rb->open(filename, O_RDONLY);
    if(fd < 0)
        return -1;

    i = 0;
    while(rb->read_line(fd, buf, sizeof(buf)) > 0 && i < num_scores)
    {
        DEBUGF("%s\n", buf);

        if ( !rb->settings_parseline(buf, &score, &level) )
            continue;
        if ( !rb->settings_parseline(level, &level, &name) )
            continue;

        scores[i].score = rb->atoi(score);
        scores[i].level = rb->atoi(level);
        rb->strlcpy(scores[i].name, name, sizeof(scores[i].name));
        i++;
    }
    rb->close(fd);
    highscore_updated = false;
    return 0;
}

int highscore_update(int score, int level, const char *name,
                     struct highscore *scores, int num_scores)
{
    int pos;
    struct highscore *entry;
    
    if (!highscore_would_update(score, scores, num_scores))
        return -1;

    pos = num_scores-1;
    while (pos > 0 && score > scores[pos-1].score)
    {
        /* move down one */
        rb->memcpy((void *)&scores[pos], (void *)&scores[pos-1],
                   sizeof(struct highscore));
        pos--;
    }
    
    entry = scores + pos;
    entry->score = score;
    entry->level = level;
    rb->strlcpy(entry->name, name, sizeof(entry->name));

    highscore_updated = true;
    return pos;
}

bool highscore_would_update(int score, struct highscore *scores,
                            int num_scores)
{
    return (num_scores > 0) && (score > scores[num_scores-1].score);
}

#ifdef HAVE_LCD_BITMAP
#define MARGIN 5
void highscore_show(int position, struct highscore *scores, int num_scores,
                    bool show_level)
{
    int i, w, h;
    char str[30];
#ifdef HAVE_LCD_COLOR
    unsigned bgcolor = rb->lcd_get_background();
    unsigned fgcolor = rb->lcd_get_foreground();
    rb->lcd_set_background(LCD_BLACK);
    rb->lcd_set_foreground(LCD_WHITE);
#endif
    rb->lcd_clear_display();

    rb->lcd_setfont(FONT_UI);
    rb->lcd_getstringsize("High Scores", &w, &h);
    /* check wether it fits on screen */
    if ((4*h + h*(num_scores-1) + MARGIN) > LCD_HEIGHT) {
        rb->lcd_setfont(FONT_SYSFIXED);
        rb->lcd_getstringsize("High Scores", &w, &h);
    }
    rb->lcd_putsxy(LCD_WIDTH/2-w/2, MARGIN, "High Scores");
    rb->lcd_putsxy(LCD_WIDTH/4-w/4,2*h, "Score");
    
    if(show_level) {
        rb->lcd_putsxy(LCD_WIDTH*3/4-w/4,2*h, "Level");
    }

    for (i = 0; i<num_scores; i++)
    {
#ifdef HAVE_LCD_COLOR
        if (i == position) {
            rb->lcd_set_foreground(LCD_RGBPACK(245,0,0));
        }
#endif
        rb->snprintf (str, sizeof (str), "%d)", i+1);
        rb->lcd_putsxy (MARGIN,3*h + h*i, str);
        rb->snprintf (str, sizeof (str), "%d", scores[i].score);
        rb->lcd_putsxy (LCD_WIDTH/4-w/4,3*h + h*i, str);
        
        if(show_level) {
            rb->snprintf (str, sizeof (str), "%d", scores[i].level);
            rb->lcd_putsxy (LCD_WIDTH*3/4-w/4,3*h + h*i, str);
        }
        
        if(i == position) {
#ifdef HAVE_LCD_COLOR
            rb->lcd_set_foreground(LCD_WHITE);
#else
            rb->lcd_hline(MARGIN, LCD_WIDTH-MARGIN*2, 3*h + h*(i+1) - 1);
#endif
        }
    }
    rb->lcd_update();
    rb->sleep(HZ/2);
    rb->button_clear_queue();
    rb->button_get(true);
    rb->lcd_setfont(FONT_SYSFIXED);
#ifdef HAVE_LCD_COLOR
    rb->lcd_set_background(bgcolor);
    rb->lcd_set_foreground(fgcolor);
#endif
}
#else
struct scoreinfo {
    struct highscore *scores;
    int position;
    bool show_level;
};
static const char* get_score(int selected, void * data,
                                  char * buffer, size_t buffer_len)
{
    struct scoreinfo *scoreinfo = (struct scoreinfo *) data;
    int len;
    len = rb->snprintf(buffer, buffer_len, "%c%d) %4d",
                        (scoreinfo->position == selected?'*':' '),
                        selected+1, scoreinfo->scores[selected].score);

    if (scoreinfo->show_level)
        rb->snprintf(buffer + len, buffer_len - len, " %d",
                     scoreinfo->scores[selected].level);
    return buffer;
}

void highscore_show(int position, struct highscore *scores, int num_scores,
                    bool show_level)
{
    struct simplelist_info info;
    struct scoreinfo scoreinfo = {scores, position, show_level};
    rb->simplelist_info_init(&info, "High Scores", num_scores, &scoreinfo);
    if (position >= 0)
        info.selection = position;
    info.hide_selection = true;
    info.get_name = get_score;
    rb->simplelist_show_list(&info);
}
#endif /* HAVE_LCD_BITMAP */
