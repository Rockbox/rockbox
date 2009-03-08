/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 by Kevin Ferrare
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

#ifndef _GUI_YESNO_H_
#define _GUI_YESNO_H_

enum yesno_res
{
    YESNO_YES,
    YESNO_NO,
    YESNO_USB
};

struct text_message
{
    const char **message_lines;
    int nb_lines;
};

/*
 * Runs the yesno asker :
 * it will display the 'main_message' question, and wait for user keypress
 * PLAY means yes, other keys means no
 *  - main_message : the question the user has to answer
 *  - yes_message : message displayed if answer is 'yes'
 *  - no_message : message displayed if answer is 'no'
 */
extern enum yesno_res gui_syncyesno_run(
                           const struct text_message * main_message,
                           const struct text_message * yes_message,
                           const struct text_message * no_message);
#endif /* _GUI_YESNO_H_ */
