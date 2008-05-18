/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 David Dent
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"

PLUGIN_HEADER
static const struct plugin_api* rb;
MEM_FUNCTION_WRAPPERS(rb)

/* function return values */
enum tidy_return
{
    TIDY_RETURN_OK = 0,
    TIDY_RETURN_ERROR = 1,
    TIDY_RETURN_USB = 2,
    TIDY_RETURN_ABORT = 3,
};

#define MAX_TYPES 64
struct tidy_type {
    char filestring[64];
    bool directory;
    bool remove;
} tidy_types[MAX_TYPES];
int tidy_type_count;
bool tidy_loaded_and_changed = false;

#define DEFAULT_FILES PLUGIN_APPS_DIR "/disktidy.config"
#define CUSTOM_FILES  PLUGIN_APPS_DIR "/disktidy_custom.config"
void add_item(const char* name, int index)
{
    rb->strcpy(tidy_types[index].filestring, name);
    if (name[rb->strlen(name)-1] == '/')
    {
        tidy_types[index].directory = true;
        tidy_types[index].filestring[rb->strlen(name)-1] = '\0';
    }
    else
        tidy_types[index].directory = false;
}
static int find_file_string(const char *file, char *last_group)
{
    char temp[MAX_PATH];
    int i = 0, idx_last_group = -1;
    bool folder = false;
    rb->strcpy(temp, file);
    if (temp[rb->strlen(temp)-1] == '/')
    {
        folder = true;
        temp[rb->strlen(temp)-1] = '\0';
    }
    while (i<tidy_type_count)
    {
        if (!rb->strcmp(tidy_types[i].filestring, temp) &&
            folder == tidy_types[i].directory)
            return i;
        else if (!rb->strcmp(tidy_types[i].filestring, last_group))
            idx_last_group = i;
        i++;
    }
    /* not found, so insert it into its group */
    if (file[0] != '<' && idx_last_group != -1)
    {
        for (i=idx_last_group; i<tidy_type_count; i++)
        {
            if (tidy_types[i].filestring[0] == '<')
            {
                idx_last_group = i;
                break;
            }
        }
        /* shift items up one */
        for (i=tidy_type_count;i>idx_last_group;i--)
        {
            rb->strcpy(tidy_types[i].filestring, tidy_types[i-1].filestring);
            tidy_types[i].directory = tidy_types[i-1].directory;
            tidy_types[i].remove = tidy_types[i-1].remove;
        }
        tidy_type_count++;
        add_item(file, idx_last_group+1);
        return idx_last_group+1;
    }
    return i;
}

bool tidy_load_file(const char* file)
{
    int fd = rb->open(file, O_RDONLY), i;
    char buf[MAX_PATH], *str, *remove;
    char last_group[MAX_PATH] = "";
    bool new;
    if (fd < 0)
        return false;
    while ((tidy_type_count < MAX_TYPES) && 
            rb->read_line(fd, buf, MAX_PATH))
    {
        if (rb->settings_parseline(buf, &str, &remove))
        {
            i = find_file_string(str, last_group);
            new = (i >= tidy_type_count);
            if (!rb->strcmp(remove, "yes"))
                tidy_types[i].remove = true;
            else tidy_types[i].remove = false;
            if (new)
            {
                i = tidy_type_count;
                add_item(str, i);
                tidy_type_count++;
            }
            if (str[0] == '<')
                rb->strcpy(last_group, str);
        }
    }
    rb->close(fd);
    return true;
}

bool tidy_remove_item(char *item, int attr)
{
    int i;
    char *file;
    bool ret = false, rem = false;
    for (i=0; ret == false && i < tidy_type_count; i++)
    {
        file = tidy_types[i].filestring;
        if (file[rb->strlen(file)-1] == '*')
        {
            if (!rb->strncmp(file, item, rb->strlen(file)-1))
                 rem = true;
        }
        else if (!rb->strcmp(file, item))
            rem = true;
        if (rem)
        {
            if (!tidy_types[i].remove)
                return false;
            if (attr&ATTR_DIRECTORY)
                ret = tidy_types[i].directory;
            else ret = true;
        }
    }
    return ret;
}

void tidy_lcd_status(const char *name, int *removed)
{
    char text[24]; /* "Cleaned up nnnnn items" */

    /* display status text */
    rb->lcd_clear_display();
    rb->lcd_puts(0, 0, "Working ...");
    rb->lcd_puts(0, 1, name);
    rb->snprintf(text, 24, "Cleaned up %d items", *removed);
#ifdef HAVE_LCD_BITMAP
    rb->lcd_puts(0, 2, text);
#endif
    rb->lcd_update();
}

void tidy_get_absolute_path(struct dirent *entry, char *fullname, 
                            const char* name)
{
    /* gets absolute path using dirent and name */
    rb->strcpy(fullname, name);
    if (rb->strlen(name) > 1)
    {
        rb->strcat(fullname, "/");
    }
    rb->strcat(fullname, entry->d_name);
}

enum tidy_return tidy_removedir(const char *name, int *removed)
{
    /* delete directory */
    struct dirent *entry;
    enum tidy_return status = TIDY_RETURN_OK;
    int button;
    DIR *dir;
    char fullname[MAX_PATH];
    
    /* display status text */
    tidy_lcd_status(name, removed);
    
    rb->yield();
    
    dir = rb->opendir(name);
    if (dir)
    {
        while((status == TIDY_RETURN_OK) && ((entry = rb->readdir(dir)) != 0))
        /* walk directory */
        {
            /* check for user input and usb connect */
            button = rb->get_action(CONTEXT_STD, TIMEOUT_NOBLOCK);
            if (button == ACTION_STD_CANCEL)
            {
                rb->closedir(dir);
                return TIDY_RETURN_ABORT;
            }
            if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
            {
                rb->closedir(dir);
                return TIDY_RETURN_USB;
            }
            
            rb->yield();
            
            /* get absolute path */
            tidy_get_absolute_path(entry, fullname, name);
            
            if (entry->attribute & ATTR_DIRECTORY)
            {
                /* dir ignore "." and ".." */
                if ((rb->strcmp(entry->d_name, ".") != 0) && \
                    (rb->strcmp(entry->d_name, "..") != 0))
                {
                    tidy_removedir(fullname, removed);
                }
            }
            else
            {
                /* file */
                *removed += 1;
                rb->remove(fullname);
            }
        }
        rb->closedir(dir);
        /* rmdir */
        *removed += 1;
        rb->rmdir(name);
    }
    else
    {
        status = TIDY_RETURN_ERROR;
    }
    return status;
}

enum tidy_return tidy_clean(const char *name, int *removed)
{
    /* deletes junk files and dirs left by system */
    struct dirent *entry;
    enum tidy_return status = TIDY_RETURN_OK;
    int button;
    int del; /* has the item been deleted */
    DIR *dir;
    char fullname[MAX_PATH];
        
    /* display status text */
    tidy_lcd_status(name, removed);
    
    rb->yield();
    
    dir = rb->opendir(name);
    if (dir)
    {
        while((status == TIDY_RETURN_OK) && ((entry = rb->readdir(dir)) != 0))
        /* walk directory */
        {
            /* check for user input and usb connect */
            button = rb->get_action(CONTEXT_STD, TIMEOUT_NOBLOCK);
            if (button == ACTION_STD_CANCEL)
            {
                rb->closedir(dir);
                return TIDY_RETURN_ABORT;
            }
            if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
            {
                rb->closedir(dir);
                return TIDY_RETURN_USB;
            }
            
            rb->yield();
            
            if (entry->attribute & ATTR_DIRECTORY)
            {
                /* directory ignore "." and ".." */
                if ((rb->strcmp(entry->d_name, ".") != 0) && \
                    (rb->strcmp(entry->d_name, "..") != 0))
                {
                    del = 0;
                    
                    /* get absolute path */
                    tidy_get_absolute_path(entry, fullname, name);
                    
                    if (tidy_remove_item(entry->d_name, entry->attribute))
                    {
                        /* delete dir */
                        tidy_removedir(fullname, removed);
                        del = 1;
                    }
                    
                    if (del == 0)
                    {
                        /* dir not deleted so clean it */
                        status = tidy_clean(fullname, removed);
                    }
                }
            }
            else
            {
                /* file */
                del = 0;
                if (tidy_remove_item(entry->d_name, entry->attribute))
                {
                    *removed += 1; /* increment removed files counter */
                    
                    /* get absolute path */
                    char fullname[MAX_PATH];
                    tidy_get_absolute_path(entry, fullname, name);
                    
                    /* delete file */
                    rb->remove(fullname);
                    del = 1;
                }
            }
        }
        rb->closedir(dir);
        return status;
    }
    else
    {
        return TIDY_RETURN_ERROR;
    }
}

enum plugin_status tidy_do(void)
{
    /* clean disk and display num of items removed */
    int removed = 0;
    enum tidy_return status;
    char text[24]; /* "Cleaned up nnnnn items" */
    
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(true);
#endif
    
    status = tidy_clean("/", &removed);
    
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
    rb->cpu_boost(false);
#endif
    
    if ((status == TIDY_RETURN_OK) || (status == TIDY_RETURN_ABORT))
    {
        rb->lcd_clear_display();
        rb->snprintf(text, 24, "Cleaned up %d items", removed);
        if (status == TIDY_RETURN_ABORT)
        {
            rb->splash(HZ, "User aborted");
            rb->lcd_clear_display();
        }
        rb->splash(HZ*2, text);
    }
    return status;
}

enum themable_icons get_icon(int item, void * data)
{
    (void)data;
    if (tidy_types[item].filestring[0] == '<') /* special type */
        return Icon_Folder;
    else if (tidy_types[item].remove)
        return Icon_Cursor;
    else
        return Icon_NOICON;
}

char * get_name(int selected_item, void * data,
                     char * buffer, size_t buffer_len)
{
    (void)data;
    if (tidy_types[selected_item].directory)
    {
        rb->snprintf(buffer, buffer_len, "%s/",
                     tidy_types[selected_item].filestring);
        return buffer;
    }
    return tidy_types[selected_item].filestring;
}

int list_action_callback(int action, struct gui_synclist *lists)
{
    if (action == ACTION_STD_OK)
    {
        int selection = rb->gui_synclist_get_sel_pos(lists);
        if (tidy_types[selection].filestring[0] == '<')
        {
            int i;
            if (!rb->strcmp(tidy_types[selection].filestring, "< ALL >"))
            {
                for (i=0; i<tidy_type_count; i++)
                {
                    if (tidy_types[i].filestring[0] != '<')
                        tidy_types[i].remove = true;
                }
            }
            else if (!rb->strcmp(tidy_types[selection].filestring, "< NONE >"))
            {
                for (i=0; i<tidy_type_count; i++)
                {
                    if (tidy_types[i].filestring[0] != '<')
                        tidy_types[i].remove = false;
                }
            }
            else /* toggle all untill the next <> */
            {
                selection++;
                while (selection < tidy_type_count &&
                       tidy_types[selection].filestring[0] != '<')
                {
                    tidy_types[selection].remove = !tidy_types[selection].remove;
                    selection++;
                }
            }
        }
        else
            tidy_types[selection].remove = !tidy_types[selection].remove;
        tidy_loaded_and_changed = true;
        return ACTION_REDRAW;
    }
    return action;
}

int tidy_lcd_menu(void)
{
    int selection, ret = 3;
    bool menu_quit = false;
    
    MENUITEM_STRINGLIST(menu,"Disktidy Menu",NULL,"Start Cleaning",
                        "Files to Clean","Quit");
    
    while (!menu_quit)
    {    
        switch(rb->do_menu(&menu, &selection, NULL, false))
        {
         
            case 0:
                menu_quit = true;   /* start cleaning */
                break;
                
            case 1:
            {
                struct simplelist_info list;
                rb->simplelist_info_init(&list, "Files to Clean", tidy_type_count, NULL);
                list.get_icon = get_icon;
                list.get_name = get_name;
                list.action_callback = list_action_callback;
                rb->simplelist_show_list(&list);
            }
            break;
        
            default:
                ret = 99;    /* exit plugin */
                menu_quit = true;
                break;
        }
    }
    return ret;
}

/* this is the plugin entry point */
enum plugin_status plugin_start(const struct plugin_api* api, const void* parameter)
{
    enum tidy_return status;
    int ret;
    (void)parameter;

    rb = api;
    tidy_type_count = 0;
    tidy_load_file(DEFAULT_FILES);
    tidy_load_file(CUSTOM_FILES);
    if (tidy_type_count == 0)
    {
        rb->splash(3*HZ, "Missing disktidy.config file");
        return PLUGIN_ERROR;
    }
    ret = tidy_lcd_menu();
    if (tidy_loaded_and_changed)
    {
        int fd = rb->creat(CUSTOM_FILES);
        int i;
        if (fd >= 0)
        {
            for(i=0;i<tidy_type_count;i++)
            {
                rb->fdprintf(fd, "%s%c: %s\n", tidy_types[i].filestring,
                             tidy_types[i].directory?'/':'\0',
                             tidy_types[i].remove?"yes":"no");
            }
            rb->close(fd);
        }
    }
    if (ret == 99)
        return PLUGIN_OK;
    while (true)
    {
            status = tidy_do();

            switch (status)
            {
                case TIDY_RETURN_OK:
                    return PLUGIN_OK;
                case TIDY_RETURN_ERROR:
                    return PLUGIN_ERROR;
                case TIDY_RETURN_USB:
                   return PLUGIN_USB_CONNECTED;
                case TIDY_RETURN_ABORT:
                    return PLUGIN_OK;                
            }
    }
        
    if (rb->default_event_handler(rb->button_get(false)) == SYS_USB_CONNECTED)
        return PLUGIN_USB_CONNECTED;
        
    rb->yield();

    
    return PLUGIN_OK;
}
