/***************************************************************************
 *             __________               __   ___.                  
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___  
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /  
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <   
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \  
 *                     \/            \/     \/    \/            \/ 
 * $Id$
 *
 * Copyright (C) 2002 by Daniel Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "sprintf.h"
#define ONE_KILOBYTE 1024
#define ONE_MEGABYTE (1024*1024)

/* The point of this function would be to return a string of the input data,
   but never longer than 5 columns. Add suffix k and M when suitable...
   Make sure to have space for 6 bytes in the buffer. 5 letters plus the
   terminating zero byte. */
char *num2max5(unsigned int bytes, char *max5)
{
    if(bytes < 100000) {
        snprintf(max5, 6, "%5d", bytes);
        return max5;
    }
    if(bytes < (9999*ONE_KILOBYTE)) {
        snprintf(max5, 6, "%4dk", bytes/ONE_KILOBYTE);
        return max5;
    }
    if(bytes < (100*ONE_MEGABYTE)) {
        /* 'XX.XM' is good as long as we're less than 100 megs */
        snprintf(max5, 6, "%4d.%0dM",
                 bytes/ONE_MEGABYTE,
                 (bytes%ONE_MEGABYTE)/(ONE_MEGABYTE/10) );
        return max5;
    }
    snprintf(max5, 6, "%4dM", bytes/ONE_MEGABYTE);
    return max5;
}

#ifdef TEST_MAX5
int main(int argc, char **argv)
{
    char buffer[32];
    if(argc>1) {
        printf("%d => %s\n",
               atoi(argv[1]),
               num2max5(atoi(argv[1]), buffer));
    }
    return 0;
}

#endif

/* -----------------------------------------------------------------
 * local variables:
 * eval: (load-file "../firmware/rockbox-mode.el")
 * end:
 */
