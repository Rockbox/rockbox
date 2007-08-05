/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Bryan Childs
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

static struct plugin_api* rb;

#define SHORTCUTS_FILENAME "/shortcuts.link"
#define MAX_SHORTCUTS 50

MEM_FUNCTION_WRAPPERS(rb);

typedef struct sc_file_s
{
    int readsize;
    char* filebuf;
} sc_file_t;

typedef struct sc_entries_s
{
    char shortcut[MAX_PATH+1];
    int sc_len;
    struct sc_entries_s* next;
} sc_entries_t;

enum shortcut_type {
    SCTYPE_NONE,
    SCTYPE_FILE,
    SCTYPE_DIR,
};

enum sc_list_action_type {
    SCLA_NONE,
    SCLA_SELECT,
    SCLA_DELETE,
};

void sc_alloc_init(void);
void* sc_malloc(unsigned int size);
bool sc_init(void);
enum sc_list_action_type draw_sc_list(struct gui_synclist gui_sc);
char* build_sc_list(int selected_item, void* data, char* buffer);
void delete_sc(int sc_num);
bool load_sc_file(void);
bool load_user_sc_file(char* filename);
bool exists(char* filename);
enum plugin_status list_sc(void);
enum plugin_status write_sc_file(char* directory_name,enum shortcut_type st);

char str_dirname[MAX_PATH];
ssize_t bufleft;
long mem_ptr;
long bufsize;
unsigned char* mallocbuf;
bool its_a_dir = false;
bool user_file = false;
sc_file_t the_file;
sc_entries_t* shortcuts = 0;
sc_entries_t* lastentry = 0;
int total_entries = 0;
int gselected_item = 0;

void sc_alloc_init(void){
    mem_ptr=0;

    mallocbuf = rb->plugin_get_buffer(&bufleft);
    bufsize = (long)bufleft;

    rb->memset(mallocbuf,0,bufsize);

    return;
}

void* sc_malloc(unsigned int size) {
    void* x;

    if(mem_ptr + (long)size > bufsize) {
        rb->splash(HZ*2,"OUT OF MEMORY");
        return NULL;
    }

    x=&mallocbuf[mem_ptr];
    mem_ptr+=(size+3)&~3;  /* Keep memory 32-bit aligned */

    return x;
}

bool exists(char* filename){
    int fd = 0;
    /*strip trailing slashes */
    char* ptr = rb->strrchr((char*)filename, '/') + 1;
    int dirlen = (ptr - (char*)filename);
    rb->strncpy(str_dirname, (char*)filename, dirlen);
    str_dirname[dirlen] = 0;

    fd = rb->open(str_dirname,O_RDONLY);
    if (!fd) {
        return false;
    }
    rb->close(fd);
    return true;
}

bool sc_init(void) {
    return load_sc_file();
}

enum sc_list_action_type draw_sc_list(struct gui_synclist gui_sc) {
    int button;

    rb->gui_synclist_draw(&gui_sc);

    while (true) {
        /* draw the statusbar, should be done often */
        rb->gui_syncstatusbar_draw(rb->statusbars, true);
        /* user input */
        button = rb->get_action(CONTEXT_LIST,TIMEOUT_BLOCK);
        if (rb->gui_synclist_do_button(&gui_sc,button,
                                            LIST_WRAP_UNLESS_HELD)) {
            /* automatic handling of user input.
            * _UNLESS_HELD can be _ON or _OFF also
            * selection changed, so redraw */
            continue;
        }
        switch (button) { /* process the user input */
            case ACTION_STD_OK:
                gselected_item = rb->gui_synclist_get_sel_pos(&gui_sc);
                return SCLA_SELECT;
                break;
            case ACTION_STD_MENU:
                if(!user_file){
                    gselected_item = rb->gui_synclist_get_sel_pos(&gui_sc);
                    rb->splash(HZ,"Deleting item");
                    return SCLA_DELETE;
                } else {
                    return SCLA_NONE;
                }
                break;
            case ACTION_STD_CANCEL:
                return SCLA_NONE;
                break;
        }
    }
}

char* build_sc_list(int selected_item, void* data, char* buffer) {
    int i;
    sc_entries_t* temp_node = (sc_entries_t*)data;
    char text_buffer[MAX_PATH];

    for (i=0;i<selected_item && temp_node != NULL;i++){
        temp_node = temp_node->next;
    }
    if (temp_node == NULL){
        return NULL;
    }
    rb->snprintf(text_buffer, MAX_PATH, "%s", temp_node->shortcut);

    rb->strcpy(buffer, text_buffer);
    return buffer;
}

void delete_sc(int sc_num){
    /* Note: This function is a nasty hack and I should probably
     * be shot for doing it this way.*/
    int i;
    sc_entries_t* current = shortcuts;
    sc_entries_t* previous = shortcuts;

    if(total_entries==1){
        /* This is the only item in the file
         * so just set the whole shortcuts list
         * to zero */
        shortcuts=0;
    } else {
        if(sc_num!=0){
            for (i=0;i<sc_num && current != NULL;i++){
                /* keep previous pointing at the prior
                * item in the list */
                if(previous!=current){
                    previous = current;
                }
                current=current->next;
            }
            /* current should now be pointing at the item
            * to be deleted, so update the previous item
            * to point to whatever current is pointing to
            * as next item */
            if(current){
                previous->next = current->next;
            }else{
                previous->next = 0;
            }
        }else{
            shortcuts = shortcuts->next;
        }
    }
    return;
}

enum plugin_status list_sc(void) {
    int selected_item = 0;
    char selected_dir[MAX_PATH];
    enum sc_list_action_type action = SCLA_NONE;
    struct gui_synclist gui_sc;

    rb->memset(selected_dir,0,MAX_PATH);

    /* Setup the GUI list object, draw it to the screen,
     * and then handle the user input to it */
    rb->gui_synclist_init(&gui_sc,&build_sc_list,shortcuts,false,1);
    rb->gui_synclist_set_title(&gui_sc,"Shortcuts",NOICON);
    rb->gui_synclist_set_nb_items(&gui_sc,total_entries);
    rb->gui_synclist_limit_scroll(&gui_sc,false);
    rb->gui_synclist_select_item(&gui_sc,0);

    /* Draw the prepared widget to the LCD now */
    action = draw_sc_list(gui_sc);

    /* which item do we action? */
    selected_item = gselected_item;

    /* Find out which option the user selected.
     * Handily, the callback function which is used
     * to populate the list is equally useful here! */
    build_sc_list(selected_item,(void*)shortcuts,selected_dir);

    /* perform the following actions if the user "selected"
     * the item in the list (i.e. they want to go there
     * in the filebrowser tree */
    switch(action) {
        case SCLA_SELECT:
            /* Check to see if the directory referenced still exists */
            if(!exists(selected_dir)){
                rb->splash(HZ*2,"File / Directory no longer exists on disk");
                return PLUGIN_ERROR;
            }

            /* Set the browsers dirfilter to the global setting
             * This is required in case the plugin was launched
             * from the plugins browser, in which case the
             * dirfilter is set to only display .rock files */
            rb->set_dirfilter(rb->global_settings->dirfilter);

            /* Change directory to the entry selected by the user */
            rb->set_current_file(selected_dir);
            break;
        case SCLA_DELETE:
            delete_sc(selected_item);
            return write_sc_file(0,SCTYPE_NONE);
            break;
        case SCLA_NONE:
            return PLUGIN_OK;
            break;
    }
    return PLUGIN_OK;
}

bool load_sc_file(void){
    int fd = 0;
    int amountread = 0;
    char sc_content[MAX_PATH];
    sc_entries_t* entry = 0;

    fd = rb->open(SHORTCUTS_FILENAME,O_RDONLY);
    if(fd<0){
        /* The shortcuts.link file didn't exist on disk
         * so create an empty one.
         */
        fd = rb->creat(SHORTCUTS_FILENAME);
        if(fd<0){
            /* For some reason we couldn't create a new shortcuts.link
             * file, so return an error message and exit
             */
            rb->splash(HZ*2,"Couldn't create the shortcuts file");
            return false;
        }
        /* File created, but there's nothing in it
         * so just exit */
        rb->close(fd);
        return true;
    }

    /* if we get to here, the file already exists, and has been opened
     * successfully, so we can start reading it
     */
    while((amountread=rb->read_line(fd,sc_content,MAX_PATH))){
        if(!(entry = (sc_entries_t*)sc_malloc(sizeof(sc_entries_t)))){
            rb->splash(HZ*2,"Couldn't get memory for a new entry");
            rb->close(fd);
            return false;
        }
        if(shortcuts==NULL) {
            /* This is the first entry created, so set
                * shortcuts to point to it
                */
            shortcuts=entry;
        }
        if(lastentry!=NULL) {
            /* This isn't the first item in the list
                * so update the previous item in the list
                * to point to this new item.
                */
            lastentry->next = entry;
        }

        total_entries++;
        rb->snprintf(entry->shortcut,amountread,"%s",sc_content);
        entry->sc_len = amountread-1;

        /* Make sure the 'next' pointer is null */
        entry->next=0;

        /* Now we can make last look at this entry,
            * ready for the next one
            */
        lastentry = entry;
    }
    rb->close(fd);
    return true;
}

bool load_user_sc_file(char* filename){
    int fd = 0;
    int amountread = 0;
    char sc_content[MAX_PATH];
    sc_entries_t* entry = 0;

    /* user has chosen to open a non-default .link file
     * so overwrite current memory contents */
    shortcuts = 0;
    lastentry = 0;
    total_entries = 0;

    fd = rb->open(filename,O_RDONLY);
    if(fd<0){
        /* The shortcuts.link file didn't exist on disk
        * so create an empty one.
        */
        rb->splash(HZ,"Couldn't open %s",filename);
        return false;
    }

    while((amountread=rb->read_line(fd,sc_content,MAX_PATH))){
        if(!(entry = (sc_entries_t*)sc_malloc(sizeof(sc_entries_t)))){
            rb->splash(HZ*2,"Couldn't get memory for a new entry");
            rb->close(fd);
            return false;
        }
        if(shortcuts==NULL) {
            /* This is the first entry created, so set
            * shortcuts to point to it
            */
            shortcuts=entry;
        }
        if(lastentry!=NULL) {
            /* This isn't the first item in the list
            * so update the previous item in the list
            * to point to this new item.
            */
            lastentry->next = entry;
        }

        total_entries++;
        rb->snprintf(entry->shortcut,amountread,"%s",sc_content);
        entry->sc_len = amountread-1;

        /* Make sure the 'next' pointer is null */
        entry->next=0;

        /* Now we can make last look at this entry,
        * ready for the next one
        */
        lastentry = entry;
    }
    rb->close(fd);
    return true;
}

enum plugin_status write_sc_file(char* directory_name, enum shortcut_type st) {
    int fd;
    int i;
    sc_entries_t *temp_node = shortcuts;
    char text_buffer[MAX_PATH];

    if(total_entries>=MAX_SHORTCUTS) {
        /* too many entries in the file already
         * so don't add this one, and give the
         * user an error */
        rb->splash(HZ*2,"Shortcuts file is full");
        return PLUGIN_ERROR;
    }

    /* ideally, we should just write a new
        * entry to the file, but I'm going to
        * be lazy, and just re-write the whole
        * thing. */
    fd = rb->open(SHORTCUTS_FILENAME,O_RDWR);
    if(fd<0){
        rb->splash(HZ*2,"Error writing to shortcuts file");
        return PLUGIN_ERROR;
    }

    /* truncate the current file, since we're writing it
        * all over again */
    rb->ftruncate(fd,0);

    /* Check to see that the list is not empty */
    if(temp_node){
        for (i=0;i<MAX_SHORTCUTS && temp_node != NULL;i++){
            rb->snprintf(text_buffer,temp_node->sc_len+2,
                        "%s\n",temp_node->shortcut);
            rb->write(fd,text_buffer,temp_node->sc_len+1);
            temp_node = temp_node->next;
        }
    }
    /* Reached the end of the existing entries, so check to
    * see if we need to add one more for the new entry
    */
    if(st!=SCTYPE_NONE){
        if(st==SCTYPE_FILE) {
            rb->snprintf(text_buffer,rb->strlen(directory_name)+2, /*+2 is \n and 0x00 */
                    "%s\n",directory_name);
            rb->write(fd,text_buffer,rb->strlen(directory_name)+1);
        } else if(st==SCTYPE_DIR){
            rb->snprintf(text_buffer,rb->strlen(directory_name)+3, /*+3 is /, \n and 0x00 */
                        "%s/\n",directory_name);
            rb->write(fd,text_buffer,rb->strlen(directory_name)+2);
        }
    }
    rb->close(fd);

    return PLUGIN_OK;
}

enum plugin_status plugin_start(struct plugin_api* api, void* parameter) {
    rb = api;
    bool found = false;

    DIR* dir;
    struct dirent* entry;

    /* Initialise the plugin buffer */
    sc_alloc_init();

    if(!sc_init())
        return PLUGIN_ERROR;

    /* Were we passed a parameter at startup? */
    if(parameter) {
        /* determine if it's a file or a directory */
        char* ptr = rb->strrchr((char*)parameter, '/') + 1;
        int dirlen = (ptr - (char*)parameter);
        rb->strncpy(str_dirname, (char*)parameter, dirlen);
        str_dirname[dirlen] = 0;

        dir = rb->opendir(str_dirname);
        if (dir) {
            while(0 != (entry = rb->readdir(dir))) {
                if(!rb->strcmp(entry->d_name, parameter+dirlen)) {
                    its_a_dir = entry->attribute & ATTR_DIRECTORY ?
                                                        true : false;
                    found = true;
                    break;
                }
            }
            rb->closedir(dir);
        }
        /* now we know if it's a file or a directory
         * (or something went wrong) */

        if(!found) {
            /* Something's gone properly pear shaped -
             * we couldn't even find the entry */
            rb->splash(HZ*2,"File / Directory not found : %s",
                       (char*)parameter);
            return PLUGIN_ERROR;
        }

        if(!its_a_dir) {
            /* Don't add the shortcuts.link file to itself */
            if(rb->strcmp((char*)parameter,SHORTCUTS_FILENAME)==0){
                return list_sc();
            }
            /* this section handles user created .link files */
            if(rb->strcasestr((char*)parameter,".link")){
                if(!load_user_sc_file((char*)parameter)){
                    return PLUGIN_ERROR;
                }
                user_file = true;
                if(total_entries==1){
                    /* if there's only one entry in the user .link file,
                     * go straight to it without displaying the menu */
                    char selected_dir[MAX_PATH];
                    /* go to entry immediately */
                    build_sc_list(0,(void*)shortcuts,selected_dir);

                    /* Check to see if the directory referenced still exists */
                    if(!exists(selected_dir)){
                        rb->splash(HZ*2,"File / Directory no longer exists on disk");
                        return PLUGIN_ERROR;
                    }

                    /* Set the browsers dirfilter to the global setting */
                    rb->set_dirfilter(rb->global_settings->dirfilter);

                    /* Change directory to the entry selected by the user */
                    rb->set_current_file(selected_dir);
                    return PLUGIN_OK;
                }else{
                    /* This user created link file has multiple entries in it
                        * so display a menu to choose between them */
                    return list_sc();
                }
            } else {
                /* This isn't a .link file so add it to the shortcuts.link
                    * file as a new entry */
                return write_sc_file((char*)parameter,SCTYPE_FILE);
            }
        }else{
            /* This is a directory and should be added to
                * the shortcuts.link file */
            return write_sc_file((char*)parameter,SCTYPE_DIR);
        }
    }
    else { /* We weren't passed a parameter, so open the
           * shortcuts file directly */
        return list_sc();
    }
    return PLUGIN_OK;
}

