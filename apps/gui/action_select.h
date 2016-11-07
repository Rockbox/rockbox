/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2016 William Wilgus
 * Derivative of folder_select.h by:
 * Copyright (C) 2011 Jonathan Gordon
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

#ifndef __ACTION_SELECT_H__
#define __ACTION_SELECT_H__

/**
 * A GUI browser to select action masks on a target
 *
 * It reads an original action mask supplied to function
 * and pre-selects the corresponding actions in the UI. If the user is done  it 
 * returns the new actionmask, assuming the user confirms the yesno dialog.
 *
 * Returns new actionmask if the selected actions have changed, otherwise 
 * returns the actionmask originally supplied */

int action_select(int actmask, char* headermsg);

#endif /* __ACTION_SELECT_H__ */
