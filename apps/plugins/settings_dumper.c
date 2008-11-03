/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: helloworld.c 17847 2008-06-28 18:10:04Z bagder $
 *
 * Copyright (C) 2002 Björn Stenberg
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

PLUGIN_HEADER

static const struct plugin_api* rb;
#define FILENAME "/settings_dumper.txt"
static int setting_count = 0;

static void write_setting(const struct settings_list *setting, int fd, unsigned int group)
{
    char text[1024];
    if (setting->cfg_name == NULL)
        return;
    if (group && (setting->flags&group) == 0) /* not in the group we are dumping */
        return;
    else if (!group && (setting->flags&(F_THEMESETTING|
                                        F_RECSETTING|
                                        F_EQSETTING|
                                        F_SOUNDSETTING)))
        return;
    setting_count++;
    if (setting_count%10 == 0)
        rb->fdprintf(fd, "\r\n");
    switch (setting->flags&F_T_MASK)
    {
        case F_T_CUSTOM:
            rb->strcpy(text, "No information available. Check the manual for valid values");
            break;
        case F_T_INT:
        case F_T_UINT:
            if (setting->flags&F_RGB)
                rb->strcpy(text, "RGB colour value in the form RRGGBB (in hexadecimal)");
            else if (setting->flags&F_T_SOUND)
                rb->snprintf(text, sizeof(text), "Min: %d, Max %d",
                         rb->sound_min(setting->sound_setting->setting),
                         rb->sound_max(setting->sound_setting->setting)); 
            else if (setting->flags&F_TABLE_SETTING)
            {
                char temp[8];
                int i;
                rb->snprintf(text, sizeof(text), "Setting allows the following values%s: ",
                             setting->flags&F_ALLOW_ARBITRARY_VALS? " (And any value between)":"");
                for(i=0;i<setting->table_setting->count;i++)
                {
                    rb->snprintf(temp, 8, "%d, ", 
                                 setting->table_setting->values[i]);
                    rb->strcat(text, temp);
                }
            }
            else if (setting->flags&F_INT_SETTING)
            {
                int min = setting->int_setting->min,
                    max = setting->int_setting->max, 
                    step = setting->int_setting->step;
                rb->snprintf(text, sizeof(text), "Min: %d, Max: %d, Step size: %d",
                         min, max, step);
            }
            else if (setting->flags&F_CHOICE_SETTING &&
                     (setting->cfg_vals == NULL))
            {
                char temp[64];
                int i;
                temp[0] = '\0';
                for(i=0;i<setting->choice_setting->count;i++)
                {
                    rb->snprintf(temp, 64, "%s, ", 
                                 setting->choice_setting->desc[i]);
                    rb->strcat(text, temp);
                }
            }
            else if (setting->cfg_vals != NULL)
            {
                rb->snprintf(text, sizeof(text), "%s", setting->cfg_vals);
            }
            break; /* F_T_*INT */
        case F_T_BOOL:
            rb->snprintf(text, sizeof(text), "on, off");
            break;
        case F_T_CHARPTR:
        case F_T_UCHARPTR:
            rb->snprintf(text, sizeof(text), "Text.. Up to %d characters. ",
                         setting->filename_setting->max_len-1);
            if (setting->filename_setting->prefix &&
                setting->filename_setting->suffix)
            {
                rb->strcat(text, "Should start with:\"");
                rb->strcat(text, setting->filename_setting->prefix);
                rb->strcat(text, "\" and end with:\"");
                rb->strcat(text, setting->filename_setting->suffix);
                rb->strcat(text, "\"");
            }
            break;
    }
    rb->fdprintf(fd, "%s: %s\r\n", setting->cfg_name, text);
}
                    


/* this is the plugin entry point */
enum plugin_status plugin_start(const struct plugin_api* api,
                                const void* parameter)
{
    const struct settings_list *list;
    int setting_count, i;
    int fd;
    (void)parameter;
    rb = api;
    
    fd = rb->open(FILENAME, O_CREAT|O_TRUNC|O_WRONLY);
    if (fd < 0)
        return PLUGIN_ERROR;
    list = rb->get_settings_list(&setting_count);
    rb->fdprintf(fd, "# .cfg file created by rockbox %s - "
            "http://www.rockbox.org\r\n\r\n", rb->appsversion);
    
    rb->fdprintf(fd, "# -- Sound settings -- #\r\n");
    for(i=0;i<setting_count;i++)
        write_setting(&list[i], fd, F_SOUNDSETTING);
    
    rb->fdprintf(fd, "\r\n\r\n# -- Theme settings -- #\r\n");
    for(i=0;i<setting_count;i++)
        write_setting(&list[i], fd, F_THEMESETTING);
#ifdef HAVE_RECORDING
    rb->fdprintf(fd, "\r\n\r\n# -- Recording settings -- #\r\n");
    for(i=0;i<setting_count;i++)
        write_setting(&list[i], fd, F_RECSETTING);
#endif
    rb->fdprintf(fd, "\r\n\r\n# -- EQ settings -- #\r\n");
    for(i=0;i<setting_count;i++)
        write_setting(&list[i], fd, F_EQSETTING);
    
    rb->fdprintf(fd, "\r\n\r\n# -- Other settings -- #\r\n");
    for(i=0;i<setting_count;i++)
        write_setting(&list[i], fd, 0);
    
    rb->fdprintf(fd, "\r\n\r\n# Total settings count: %d\r\n", setting_count);
    rb->close(fd);
    rb->splash(HZ, "Done!");
    return PLUGIN_OK;
}
