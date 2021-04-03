/***************************************************************************
*             __________               __   ___.
*   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
*   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
*   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
*   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
*                     \/            \/     \/    \/            \/
* $Id$
*
* Copyright (C) 2021 William Wilgus
*
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
/* action_helper provides a way to turn numeric action/context into strings
* the file action_helper.c is generated at compile time
* ACTION_ and CONTEXT_ are stripped from the strings and replaced when
* action_name and context_name are called,
* NOTE: both share the same static buffer sized as the largest string possible
*/
#ifndef _ACTION_HELPER_H_
#define _ACTION_HELPER_H_
extern const size_t action_helper_maxbuffer;
char* action_name(int action);
char* context_name(int context);

#endif /* _ACTION_HELPER_H_ */
