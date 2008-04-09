/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006 Jonathan Gordon
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"
#include "oldmenuapi.h"

PLUGIN_HEADER

static struct plugin_api* rb;
static bool abort;
static int fd;
static int dirs_count;
static int lasttick;
#define RFA_FILE ROCKBOX_DIR "/folder_advance_list.dat"
#define RFADIR_FILE ROCKBOX_DIR "/folder_advance_dir.txt"
#define MAX_REMOVED_DIRS 10

char *buffer = NULL;
ssize_t buffer_size;
int num_replaced_dirs = 0;
char removed_dirs[MAX_REMOVED_DIRS][MAX_PATH];
struct file_format {
    int count;
    char folder[][MAX_PATH];
};
struct file_format *list = NULL;
#if CONFIG_KEYPAD == PLAYER_PAD 

#elif (CONFIG_KEYPAD == RECORDER_PAD) \
   || (CONFIG_KEYPAD == ONDIO_PAD)

#elif (CONFIG_KEYPAD == IRIVER_H100_PAD) \
   || (CONFIG_KEYPAD == IRIVER_H300_PAD)

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || \
      (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)

#elif CONFIG_KEYPAD == IRIVER_IFP7XX_PAD

#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD

#elif CONFIG_KEYPAD == GIGABEAT_PAD

#elif CONFIG_KEYPAD == SANSA_E200_PAD

#elif CONFIG_KEYPAD == IRIVER_H10_PAD

#endif

void update_screen(bool clear)
{
    char buf[15];
    int i;

    rb->snprintf(buf,sizeof(buf),"Folders: %d",dirs_count);
    FOR_NB_SCREENS(i)
    {
        if(clear)
            rb->screens[i]->clear_display();
        rb->screens[i]->putsxy(0,0,buf);
        rb->screens[i]->update();
    }
}

void traversedir(char* location, char* name)
{
    struct dirent *entry;
    DIR* dir;
    char fullpath[MAX_PATH], path[MAX_PATH];
    bool check = false;
    int i;

    rb->snprintf(fullpath, sizeof(fullpath), "%s/%s", location, name);
    dir = rb->opendir(fullpath);
    if (dir) {
        entry = rb->readdir(dir);
        while (entry) {
            if (abort == true)
                break;
            /* Skip .. and . */
            if (entry->d_name[0] == '.')
            {
                if (    !rb->strcmp(entry->d_name,".")
                     || !rb->strcmp(entry->d_name,"..")
                     || !rb->strcmp(entry->d_name,".rockbox"))
                    check = false;
                else check = true;
            }
            else check = true;

        /* check if path is removed directory, if so dont enter it */
        rb->snprintf(path, MAX_PATH, "%s/%s", fullpath, entry->d_name);
        while(path[0] == '/')
            rb->strncpy(path, path + 1, rb->strlen(path));
        for(i = 0; i < num_replaced_dirs; i++)
        {
            if(!rb->strcmp(path, removed_dirs[i]))
            {
                check = false;
                break;
            }
        }

            if (check)
            {
                if (entry->attribute & ATTR_DIRECTORY) {
                    char *start;
                    dirs_count++;
                    rb->snprintf(path,MAX_PATH,"%s/%s",fullpath,entry->d_name);
                    start = &path[rb->strlen(path)];
                    rb->memset(start,0,&path[MAX_PATH-1]-start);
                    rb->write(fd,path,MAX_PATH);
                    traversedir(fullpath, entry->d_name);
                }
            }
            if (*rb->current_tick - lasttick > (HZ/2)) {
                update_screen(false);
                lasttick = *rb->current_tick;
                if (rb->action_userabort(TIMEOUT_NOBLOCK))
                {
                    abort = true;
                    break;
                }
            }

            entry = rb->readdir(dir);
        }
        rb->closedir(dir);
    }
}

bool custom_dir(void)
{
    DIR* dir_check;
    char *starts, line[MAX_PATH], formatted_line[MAX_PATH];
    static int fd2;
    char buf[11];
    int i, errors = 0;

    /* populate removed dirs array */
    if((fd2 = rb->open(RFADIR_FILE,O_RDONLY)) > 0)
    {
        while ((rb->read_line(fd2, line, MAX_PATH - 1)) > 0)
        {
            if ((line[0] == '-') && (line[1] == '/') &&
                     (num_replaced_dirs < MAX_REMOVED_DIRS))
            {
                num_replaced_dirs ++;
                rb->strncpy(removed_dirs[num_replaced_dirs - 1], line + 2,
                                rb->strlen(line));
            }
        }
        rb->close(fd2);
    }

    if((fd2 = rb->open(RFADIR_FILE,O_RDONLY)) > 0)
    {
        while ((rb->read_line(fd2, line, MAX_PATH - 1)) > 0)
        {
            /* blank lines and removed dirs ignored */
            if (rb->strlen(line) && ((line[0] != '-') || (line[1] != '/')))
            {
                /* remove preceeding '/'s from the line */
                while(line[0] == '/')
                    rb->strncpy(line, line + 1, rb->strlen(line));

                rb->snprintf(formatted_line, MAX_PATH, "/%s", line);

                dir_check = rb->opendir(formatted_line);

                if (dir_check)
                {
                    rb->closedir(dir_check);
                    starts = &formatted_line[rb->strlen(formatted_line)];
                    rb->memset(starts, 0, &formatted_line[MAX_PATH-1]-starts);
                    bool write_line = true;

                    for(i = 0; i < num_replaced_dirs; i++)
                    {
                        if(!rb->strcmp(line, removed_dirs[i]))
                        {
                             write_line = false;
                             break;
                        }
                    }

                    if(write_line)
                    {
                        dirs_count++;
                        rb->write(fd, formatted_line, MAX_PATH);
                    }

                    traversedir("", line);
                }
                else
                {
                     errors ++;
                     rb->snprintf(buf,sizeof(buf),"Not found:");
                     FOR_NB_SCREENS(i)
                     {
                         rb->screens[i]->puts(0,0,buf);
                         rb->screens[i]->puts(0, errors, line);
                     }
                     update_screen(false);
                }
            }
        }
        rb->close(fd2);
        if(errors)
            /* Press button to continue */
            rb->get_action(CONTEXT_STD, TIMEOUT_BLOCK); 
    }
    else
        return false;
    return true;
}

void generate(void)
{
    dirs_count = 0;
    abort = false;
    fd = rb->open(RFA_FILE,O_CREAT|O_WRONLY);
    rb->write(fd,&dirs_count,sizeof(int));
    if (fd < 0)
    {
        rb->splash(HZ, "Couldnt open %s", RFA_FILE);
        return;
    }
#ifndef HAVE_LCD_CHARCELLS
    update_screen(true);
#endif
    lasttick = *rb->current_tick;

    if(!custom_dir())
        traversedir("", "");

    rb->lseek(fd,0,SEEK_SET);
    rb->write(fd,&dirs_count,sizeof(int));
    rb->close(fd);
}
char *list_get_name_cb(int selected_item, void* data, char* buf, size_t buf_len)
{
    (void)data;
    rb->strncpy(buf, list->folder[selected_item], buf_len);
    return buf;
}

void edit_list(void)
{
    struct gui_synclist lists;
    bool exit = false;
    int button,i;
    int selection;
    fd = rb->open(RFA_FILE,O_RDONLY);
    if (fd < 0)
        return;
    buffer = rb->plugin_get_audio_buffer((size_t *)&buffer_size);
    if (!buffer)
        return;
    rb->read(fd,buffer,buffer_size);
    rb->close(fd);
    list = (struct file_format *)buffer;
    dirs_count = list->count;
    
    rb->gui_synclist_init(&lists,list_get_name_cb,0, false, 1, NULL);
    rb->gui_synclist_set_icon_callback(&lists,NULL);
    rb->gui_synclist_set_nb_items(&lists,list->count);
    rb->gui_synclist_limit_scroll(&lists,true);
    rb->gui_synclist_select_item(&lists, 0);
    
    while (!exit)
    {
        rb->gui_synclist_draw(&lists);
        button = rb->get_action(CONTEXT_LIST,TIMEOUT_BLOCK);
        if (rb->gui_synclist_do_button(&lists,&button,LIST_WRAP_UNLESS_HELD))
            continue;
        selection = rb->gui_synclist_get_sel_pos(&lists);
        switch (button)
        {
            case ACTION_STD_OK:
                list->folder[selection][0] = ' ';
                list->folder[selection][1] = '\0';
                break;
            case ACTION_STD_CONTEXT:
            {
                int m, len;
                static const struct menu_item items[] = {
                    { "Remove Folder", NULL },
                    { "Remove Folder Tree", NULL },
                };
                m = menu_init(rb, items, sizeof(items) / sizeof(*items),
                                  NULL, NULL, NULL, NULL);

                switch (menu_show(m))
                {
                    case 0:
                        list->folder[selection][0] = ' ';
                        list->folder[selection][1] = '\0';
                        break;
                    case 1:
                    {
                        char temp[MAX_PATH];
                        rb->strcpy(temp,list->folder[selection]);
                        len = rb->strlen(temp);
                        for (i=0;i<list->count;i++)
                        {
                            if (!rb->strncmp(list->folder[i],temp,len))
                            {
                                list->folder[i][0] = ' ';
                                list->folder[i][1] = '\0';
                            }
                        }
                    }
                        break;
                }
                menu_exit(m);
            }
            break;
            case ACTION_STD_CANCEL:
            {
                int m;
                static const struct menu_item items[] = {
                    { "Save and Exit", NULL },
                    { "Ignore Changes and Exit", NULL },
                };
                m = menu_init(rb, items, sizeof(items) / sizeof(*items),
                                  NULL, NULL, NULL, NULL);

                switch (menu_show(m))
                {
                    case 0:
                        exit = true;
                        rb->splash(HZ*2, "Saving " RFA_FILE);
                        fd = rb->open(RFA_FILE, O_CREAT|O_WRONLY);
                        if (fd < 0)
                        {
                            rb->splash(HZ, "Could Not Open " RFA_FILE);
                            break;
                        }
                        dirs_count = 0;
                        rb->write(fd,&dirs_count,sizeof(int));
                        for (i=0;i<list->count;i++)
                        {
                            if (list->folder[i][0] != ' ')
                            {
                                dirs_count++;
                                rb->write(fd,list->folder[i],MAX_PATH);
                            }
                        }
                        rb->lseek(fd,0,SEEK_SET);
                        rb->write(fd,&dirs_count,sizeof(int));
                        rb->close(fd);
                    case 1:
                        exit = true;
                }
                menu_exit(m);
            }
                break;
        }
    }
}
int main_menu(void)
{
    int m;
    static const struct menu_item items[] = {
        { "Generate Folder List", NULL },
        { "Edit Folder List", NULL },
        { "Quit", NULL },
    };
    m = menu_init(rb, items, sizeof(items) / sizeof(*items),
                      NULL, NULL, NULL, NULL);

    switch (menu_show(m))
    {
        case 0: /* generate */
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
            rb->cpu_boost(true);
#endif
            generate();
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
            rb->cpu_boost(false);
#endif
#ifdef HAVE_REMOTE_LCD
            rb->remote_backlight_on();
#endif
            rb->backlight_on();
            break;
        case 1:
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
            rb->cpu_boost(true);
#endif
            edit_list();
#ifdef HAVE_ADJUSTABLE_CPU_FREQ
            rb->cpu_boost(false);
#endif
#ifdef HAVE_REMOTE_LCD
            rb->remote_backlight_on();
#endif
            rb->backlight_on();
            break;
        case 2:
            menu_exit(m);
            return 1;
    }
    menu_exit(m);
    return 0;
}

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    (void)parameter;

    rb = api;
    abort = false;
    
    while (!main_menu())
        ;
    return PLUGIN_OK;
}
