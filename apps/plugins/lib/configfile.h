/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef CONFIGFILE_H
#define CONFIGFILE_H

#define TYPE_INT    1
#define TYPE_ENUM   2
#define TYPE_STRING 3

struct configdata
{
    int type;      /* See TYPE_ macros above */
    int min;       /* Min value for integers, should be 0 for enums */
    int max;       /* Max value for enums and integers,
                      buffer size for strings  */
    int *val;      /* Pointer to integer/enum value,
                      NULL if the item is a string */
    char *name;    /* Pointer to the name of the item */
    char **values; /* List of strings for enums, NULL if not enum */
    char *string;  /* Pointer to a string buffer if the item is a string,
                      NULL otherwise */
};

void configfile_init(const struct plugin_api* newrb);

/* configfile_save - Given configdata entries this function will
   create a config file with these entries, destroying any
   previous config file of the same name */
int configfile_save(const char *filename, struct configdata *cfg,
                    int num_items, int version);

int configfile_load(const char *filename, struct configdata *cfg,
                    int num_items, int min_version);

/* configfile_get_value - Given a key name, this function will
   return the integer value for that key.

   Input:
     filename = config file filename
     name = (name/value) pair name entry
   Return:
     value if (name/value) pair is found
     -1    if entry is not found
*/
int configfile_get_value(const char* filename, const char* name);

/* configure_update_entry - Given a key name and integer value
   this function will update the entry if found, or add it if
   not found.

   Input:
     filename = config file filename
     name = (name/value) pair name entry
     val = new value for (name/value) pair
   Return:
     1  if the (name/value) pair was found and updated with the new value
     0  if the (name/value) pair was added as a new entry
     -1 if error
*/
int configfile_update_entry(const char* filename, const char* name, int val);

#endif
