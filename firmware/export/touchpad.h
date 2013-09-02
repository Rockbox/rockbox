/***************************************************************************
 * __________ __ ___.
 * Open \______ \ ____ ____ | | _\_ |__ _______ ___
 * Source | _// _ \_/ ___\| |/ /| __ \ / _ \ \/ /
 * Jukebox | | ( <_> ) \___| < | \_\ ( <_> > < <
 * Firmware |____|_ /\____/ \___ >__|_ \|___ /\____/__/\_ \
 * \/ \/ \/ \/ \/
 * $Id$
 *
 * Copyright (C) 2013 by Jean-Louis Biasini
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

#ifndef _TOUCHDEV_H_
#define _TOUCHDEV_H_

void touchpad_enable(bool en);
int touchpad_filter(int button);

#endif /* _TOUCHDEV_H_ */
