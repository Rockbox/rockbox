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
#include "SDL_config.h"

#ifndef _SDL_rockboxaudio_h
#define _SDL_rockboxaudio_h

#include "SDL_rwops.h"
#include "../SDL_sysaudio.h"

/* Hidden "this" pointer for the video functions */
#define _THIS   SDL_AudioDevice *this

#define MAX_BUFFERS 10

struct SDL_PrivateAudioData {
    /* SDL puts `mixlen' samples in this buffer */
    Uint8 *mixbuf;
    Uint32 mixlen;

    int16_t *rb_buf[MAX_BUFFERS];
    int status[MAX_BUFFERS];

    int n_buffers;

    /* Which buffer was last passed to rockbox; the driver will try to
     * fill the next empty buffer. */
    int current_playing;
};

#endif /* _SDL_rockboxaudio_h */
