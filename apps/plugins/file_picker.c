/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2024 William Wilgus
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
/*File Picker Plugin

FPP accepts several arguments to help in your file picking adventure
* NOTE: anything with spaces should be quoted, -f and -l can't be used together *
  -r "<GOTO_PLUGIN_NAME>" doesn't need to be a full path demos/file.rock works
      the file (if choosen, cancel exits) will be passed as a parameter
  -t "<TITLE ( max 64 chars )>"
  -s "<START DIR>"
  -f "<.EXT (max 64 chars) >" the extension of the files you are looking for
              must have the '.' and ".*" accepts any file
  -l "<.EXT.EXT,.EXT .EXT (max 64 chars) >" list of extensions for files you are looking for
              each must have the '.' spaces and commas are ignored
  -a attrib flags eg FILE_ATTR_AUDIO
  -d disallow changing directories (hide directories doesn't allow changing from start dir)
*/
#include "plugin.h"
#include "lang_enum.h"
#include "lib/arg_helper.h"

#if defined(DEBUG) || defined(SIMULATOR)
    #define logf(...) rb->debugf(__VA_ARGS__); rb->debugf("\n")
#elif defined(ROCKBOX_HAS_LOGF)
    #define logf rb->logf
#else
    #define logf(...) do { } while(0)
#endif

#define FIND_NODIRS      0x01
#define FIND_ATTRIB      0x02
#define FIND_WILDCARD    0x04
#define FIND_EXT         0x08
#define FIND_EXT_IN_LIST 0x10

static struct fpp
{
    char return_plugin[MAX_PATH];
    char start_dir[MAX_PATH];
    char file_ext[64];
    char title[64];
    int tree_attr;
    int flags;
}fpp;

static int arg_callback(char argchar, const char **parameter, void *userdata)
{
    struct fpp *pfp = userdata;
    int ret;
    long num;
    const char* start = *parameter;
    while (*parameter[0] > '/' && ispunct(*parameter[0])) (*parameter)++;
    switch (tolower(argchar))
    {
        case 'd' :
            pfp->flags |= FIND_NODIRS;
            logf ("Find no dirs");
            break;
        case 'r' : /*return_plugin*/
            logf ("trying PLUGIN_DIR...");
            size_t l = rb->strlcpy(pfp->return_plugin,
                                   PLUGIN_DIR,
                                   sizeof(pfp->return_plugin));

            ret = string_parse(parameter,
                         pfp->return_plugin + l,
                         sizeof(pfp->return_plugin) - l);

            if (ret && !rb->file_exists(pfp->return_plugin))
            {
                logf("Failed");
                *parameter = start;
                string_parse(parameter, pfp->return_plugin,
                                sizeof(pfp->return_plugin));
            }

            if (ret)
            {
                logf ("Ret plugin: Val: %s\n", pfp->return_plugin);
                logf("ate %d chars\n", ret);
            }
            break;
        case 't' : /* title */
            ret = string_parse(parameter, pfp->title, sizeof(pfp->title));
            if (ret)
            {
                logf ("Title: Val: %s\n", pfp->title);
                logf("ate %d chars\n", ret);
            }
            break;
        case 's' : /* start directory */
            ret = string_parse(parameter, pfp->start_dir, sizeof(pfp->start_dir));
            if (ret)
            {
                if (!rb->dir_exists(pfp->start_dir))
                {
                    rb->strlcpy(pfp->start_dir, PATH_ROOTSTR, sizeof(pfp->start_dir));
                }

                logf ("Start dir: Val: %s\n", pfp->start_dir);
                logf("ate %d chars\n", ret);
            }
            break;
        case 'f' : /* file extension */
            if (pfp->flags & FIND_EXT_IN_LIST)
            {
                rb->splash(HZ*5, "list extensions already active -f ignored");
                break;
            }
            ret = string_parse(parameter, pfp->file_ext, sizeof(pfp->file_ext));
            if (ret)
            {
                if (pfp->file_ext[1] == '*')
                    pfp->flags |= FIND_WILDCARD;
                else
                    pfp->flags |= FIND_EXT;
                logf ("Extension: Val: %s\n", pfp->file_ext);
                logf("ate %d chars\n", ret);
            }
            break;
        case 'l' : /* file extension list */
            if (pfp->flags & FIND_EXT)
            {
                rb->splash(HZ*5, "extension already active -l ignored");
                break;
            }
            ret = string_parse(parameter, pfp->file_ext, sizeof(pfp->file_ext));
            if (ret)
            {
                char *wr = pfp->file_ext;
                char *rd = pfp->file_ext;
                while (*rd != '\0') /* copy the extensions */
                {
                    if (*rd == ' ' || *rd == ',' || *rd == ';')
                    {
                        /* ignore spaces, commas, and semicolons */
                        rd++;
                        continue;
                    }
                    *wr++ = *rd++;
                }
                *wr = '\0';
                pfp->flags |= FIND_EXT_IN_LIST;
                logf ("Extension List: Val: %s\n", pfp->file_ext);
                logf("ate %d chars\n", ret);
            }
            break;
        case 'a' : /* tree attribute */
            ret = longnum_parse(parameter, &num, NULL);
            if (ret)
            {
                pfp->tree_attr = (num)&FILE_ATTR_MASK;
                pfp->flags |= FIND_ATTRIB;
                logf("Attrib: Val: 0x%lx\n", (unsigned long)num);
                logf("ate %d chars\n", ret);
            }
            break;
        default :
            rb->splashf(HZ, "Unknown switch '%c'",argchar);
            logf("Unknown switch '%c'",argchar);
            //return 0;
    }

    return 1;
}

static bool cb_show_item(char *name, int attr, struct tree_context *tc)
{
    static int dirlevel = -1;
    if(attr & ATTR_DIRECTORY)
    {
        if (fpp.flags & FIND_NODIRS)
        {
            if (tc->dirlevel == dirlevel)
                return false;
            dirlevel = tc->dirlevel;
            if (rb->strcasestr(tc->currdir, fpp.start_dir) == NULL)
            {
                tc->is_browsing = false; /* exit immediately */
                logf("exiting %d", tc->dirlevel);
            }
        }

        return true;
    }
    if (fpp.flags & FIND_WILDCARD)
    {
        return true;
    }
    if ((fpp.flags & FIND_ATTRIB) && (fpp.tree_attr & attr) != 0)
    {
        return true;
    }
    if (fpp.flags & FIND_EXT)
    {
        const char *p = rb->strrchr(name, '.' );
        if (p != NULL && !rb->strcasecmp( p, fpp.file_ext))
            return true;
    }

    if (fpp.flags & FIND_EXT_IN_LIST)
    {
        const char *p = rb->strrchr(name, '.' );
        if (p != NULL && rb->strcasestr(fpp.file_ext, p) != NULL)
            return true;
    }

    logf("Excluded: %s", name);
    return false;

}

static int browse_file_dir(struct fpp *pfp)
{
    char buf[MAX_PATH];
    struct browse_context browse = {
        .dirfilter = SHOW_ALL,
        .flags = BROWSE_SELECTONLY | BROWSE_DIRFILTER,
        .title = pfp->title,
        .icon = Icon_Playlist,
        .buf = buf,
        .bufsize = sizeof(buf),
        .root = pfp->start_dir,
        .callback_show_item = &cb_show_item,
    };

    if (rb->rockbox_browse(&browse) == GO_TO_PREVIOUS)
    {
        if (rb->file_exists(buf))
        {
            logf("Loading %s", buf);
            return rb->plugin_open(pfp->return_plugin, buf);
        }
        else
        {
            logf("Error opening %s", buf);
            rb->splashf(HZ *2, "Error Opening %s", buf);
            return PLUGIN_ERROR;
        }
    }

    return PLUGIN_OK;
}

enum plugin_status plugin_start(const void* parameter)
{
    if (!parameter)
    {
        rb->splash(HZ *2, "No Args");
        return PLUGIN_ERROR;
    }

    argparse((const char*) parameter, -1, &fpp, &arg_callback);

    if (fpp.title[0] == '\0')
    {
        if (rb->global_settings->talk_menu)
            rb->talk_id(LANG_CHOOSE_FILE, true);

        if ((fpp.flags & FIND_EXT) || (fpp.flags & FIND_EXT_IN_LIST))
        {
            rb->snprintf(fpp.title, sizeof(fpp.title),
                         "%s (%s)", rb->str(LANG_CHOOSE_FILE), fpp.file_ext);
            if (rb->global_settings->talk_menu)
                rb->talk_spell(fpp.file_ext, true);
        }
        else
        {
            rb->snprintf(fpp.title, sizeof(fpp.title), "%s", rb->str(LANG_CHOOSE_FILE));
        }
        rb->talk_force_enqueue_next();
    }
    if (fpp.start_dir[0] == '\0' || !rb->dir_exists(fpp.start_dir))
        rb->strcpy(fpp.start_dir, PATH_ROOTSTR);

    return browse_file_dir(&fpp);
}
