/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alan Korr
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <memory.h>

/* NOT VERY OPTIMIZED AT ALL BUT WE WILL DO IT WHEN PRIORITY COMES */
void memory_copy (void *target,void const *source,unsigned int count)
  {
    while (count--)
      *((char *)target)++ = *((char const *)source)++;
  }
  
/* NOT VERY OPTIMIZED AT ALL BUT WE WILL DO IT WHEN PRIORITY COMES */
void memory_set (void *target,int byte,unsigned int count)
  {
    while (count--)
      *((char *)target)++ = (char)byte;
  } 

void memory_setup (void)
  {
#if 1
    memory_set (free_page,0,MEMORY_TOTAL_BYTES);
    memory_set (free_page_bin,0,MEMORY_TOTAL_ORDERS *sizeof (struct memory_free_page *));
    memory_set (free_page_order + 1,0,MEMORY_TOTAL_PAGES);
#endif
    free_page_order[0] = MEMORY_TOTAL_ORDERS - 1;
    free_page_bin[MEMORY_TOTAL_ORDERS - 1] = free_page;
  }
