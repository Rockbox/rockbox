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
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
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

#ifdef __cplusplus
extern "C" {
#endif

int voicefont(FILE* voicefontids,int targetnum,char* filedir, FILE* output);

#ifdef __cplusplus
}
#endif
#endif

