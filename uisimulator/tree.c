/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 Daniel Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <dir.h>
#include <types.h>

#define TREE_MAX_LEN 15

bool dirbrowse(char *root)
{
  DIR *dir = opendir(root);
  int i;
  struct dirent *entry;
  char buffer[20];

  if(!dir)
    return TRUE; /* failure */

  i=0;
  while((entry = readdir(dir))) {
    strncpy(buffer, entry->d_name, TREE_MAX_LEN);
    buffer[TREE_MAX_LEN]=0;
    lcd_puts(0, i*8, buffer, 0);

    if(++i > 8)
      break;
  }

  closedir(dir);

  return FALSE;
}
