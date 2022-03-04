/***************************************************************************
 *             __________               __   ___.                  
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___  
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /  
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <   
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \  
 *                     \/            \/     \/    \/            \/ 
 * $Id$
 *
 * Copyright (C) 2014 by Michael Sevakis
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
#ifndef _FILESYSTEM_SDL_H_
#define _FILESYSTEM_SDL_H_

#ifdef HAVE_SDL_THREADS
#undef os_read
#undef os_write

ssize_t os_sdl_readwrite(int osfd, void *buf, size_t nbyte, bool dowrite);

#define os_read(osfd, buf, nbyte) \
    os_sdl_readwrite((osfd), (buf), (nbyte), false)
#define os_write(osfd, buf, nbyte) \
    os_sdl_readwrite((osfd), (void *)(buf), (nbyte), true)

#endif /* HAVE_SDL_THREADS */

#endif /* _FILESYSTEM_SDL_H_ */
