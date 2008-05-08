/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 by Jörg Hohensohn
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 * A tool to generate the Rockbox "voicefont", a collection of all the UI
 * strings. 
 * 
 * Details at http://www.rockbox.org/twiki/bin/view/Main/VoiceBuilding
 *
 ****************************************************************************/

#ifndef VOICEFONT_H
#define VOICEFONT_H

#include <stdio.h>
#include <stdlib.h>
 
int voicefont(FILE* voicefontids,int targetnum,char* filedir, FILE* output);

#endif
