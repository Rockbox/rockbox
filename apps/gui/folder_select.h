/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
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

#ifndef __FOLDER_SELECT_H__
#define __FOLDER_SELECT_H__

/**
 * A GUI browser to select folders from the file system
 *
 * It reads a list of folders, separated by colons (:) from setting
 * and pre-selects them in the UI. If the user is done  it writes the new
 * list back to setting (again separated by colons), assuming the
 * user confirms the yesno dialog.
 *
 * Returns true if the the folder list has changed, otherwise false */
bool folder_select(char* setting, int setting_len);

#endif /* __FOLDER_SELECT_H__ */
