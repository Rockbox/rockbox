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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "playlist.h"
#include "filetree.h"
#include "dir.h"
#include "tree.h"
#include "lang.h"
#include "fileutils.h"
#include "settings.h"
#include "gui/splash.h"
#include "action.h"
#include "debug.h"

#define MAX_DEPTH 32

/**
 RETURNS: DIRWALKER_RETURN_SUCCESS if it successfully went through the directory tree
          DIRWALKER_RETURN_ERROR if it stopped for an error
          DIRWALKER_RETURN_FORCED if it was told to stop by one of the callbacks, or user aborted
**/
int dirwalker(const char *start_dir, bool recurse, bool is_forward,
           int (*folder_callback)(const char* dir, void *param),void* folder_param,
           int (*file_callback)(const char* folder,const char* file,
                        const int file_attr,void *param),void* file_param)
{
    char folder[MAX_PATH+1];
    int  current_file_stack[MAX_DEPTH];
    int  stack_top = 0;
    
    int result = DIRWALKER_RETURN_SUCCESS;
    int num_files = 0;
    int i;
    char *s;
    struct entry *files;
    struct tree_context* tc = tree_get_context();
    int dirfilter = *(tc->dirfilter);
    int sort_dir = global_settings.sort_dir;
    
    /* sort in another direction if previous dir is requested */
    if(!is_forward){
        int reverse_sort_dir[5] = {4,2,1,4,0};
        global_settings.sort_dir = reverse_sort_dir[sort_dir];
    }

    /* use the tree browser dircache to load files */
    *(tc->dirfilter) = SHOW_ALL;    
    
    /* setup stuff */
    strcpy(folder, start_dir);
    current_file_stack[0] = -1;
    
    while (!result && (stack_top>=0) )
    {        
        if (ft_load(tc, folder) < 0)
        {
            gui_syncsplash(HZ*2, true,"%s %s",folder, 
                           str(LANG_PLAYLIST_DIRECTORY_ACCESS_ERROR));
            result = DIRWALKER_RETURN_ERROR;
        }

        files = (struct entry*) tc->dircache;
        num_files = tc->filesindir;
    
        i = current_file_stack[stack_top--]+1;
        while ( i<num_files )
        {
            /* user abort */
            if (action_userabort(TIMEOUT_NOBLOCK))
            {
                result = DIRWALKER_RETURN_FORCED;
                break;
            }

            if ((files[i].attr & ATTR_DIRECTORY) && recurse)
            {
                bool enter_dir = true;
                strcat(folder,"/");
                strcat(folder,files[i].name);
                if (folder_callback)
                {
                    switch (folder_callback(folder,folder_param))
                    {
                        case DIRWALKER_ERROR:
                            result = DIRWALKER_RETURN_ERROR;
                            enter_dir = false;
                            break;
                        case DIRWALKER_OK:
                            enter_dir = true;
                            break;
                        case DIRWALKER_IGNORE:
                            enter_dir = false;
                            break;
                        case DIRWALKER_QUIT:
                            result = DIRWALKER_RETURN_FORCED;
                            enter_dir = false;
                            break;
                    }
                }
                if (enter_dir)
                {
                    current_file_stack[++stack_top] = i;

                    if(ft_load(tc, folder) < 0)
                    {
                        gui_syncsplash(HZ*2, true,"%s %s",folder, 
                                       str(LANG_PLAYLIST_DIRECTORY_ACCESS_ERROR));
                        result = DIRWALKER_RETURN_ERROR;
                        break;
                    }

                    files = (struct entry*) tc->dircache;
                    num_files = tc->filesindir;
                    i=0;
                    continue;
                } 
                else 
                {
                    s = strrchr(folder,'/');
                    if (s) *s ='\0';
                }
            }
            else if (((files[i].attr & ATTR_DIRECTORY) == 0) && file_callback)
            {
                int ret = file_callback(folder,files[i].name,
                                        files[i].attr,file_param);
                if (ret == DIRWALKER_ERROR)
                    result = DIRWALKER_RETURN_ERROR;
                else if (ret == DIRWALKER_LEAVEDIR)
                    break; /* break out of inner while() */
                else if (ret == DIRWALKER_QUIT)
                    result = DIRWALKER_RETURN_FORCED;
            }
            i++;
        } /* while ( i<num_files ) */
        s = strrchr(folder,'/');
        if (s) *s ='\0';
    } /* while (!result && (stack_top>=0) ) */
    
    /* we've overwritten the dircache so tree browser 
       will need to be reloaded */
    reload_directory();
    /* restore dirfilter */
    *(tc->dirfilter) = dirfilter;
    global_settings.sort_dir = sort_dir;

    return result;
}
