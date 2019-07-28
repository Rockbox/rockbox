/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2012 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/

/** @file SDL_platform.h
 *  Try to get a standard set of platform defines
 */

#ifndef _SDL_platform_h
#define _SDL_platform_h

#if defined(ROCKBOX)
#undef __ROCKBOX__
#undef __LINUX__ /* maybe sim */
#undef __WIN32__ /* maybe sim */
#define __ROCKBOX__ 1
#else
#error This SDL supports Rockbox only!
#endif

#endif /* _SDL_platform_h */
