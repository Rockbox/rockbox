/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2002 by Alex Gitelman
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#ifndef __UNICODE__
#define __UNICODE__

unsigned char from_unicode(const unsigned char *uni_char);
void to_unicode(unsigned char c, unsigned char page, unsigned char *uni_char);
/* Unicode -> ASCII */
void install_conversion_table(unsigned char page, unsigned char* conv_table);
/* ASCII -> Unicode */
void install_reverse_conversion_table(unsigned char page, 
                                      unsigned char* rev_conv_table);
void unicode_init(void);

/* Unicode main init point. Here we must read conversion 
   tables and install them */
void install_unicode_tables(void);


#endif
