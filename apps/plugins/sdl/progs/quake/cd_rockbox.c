/*
Copyright (C) 1996-1997 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

/* Background music for Rockbox. Uses SDL_mixer and plays from
 * /.rockbox/quake/%02d.wav */

#include "quakedef.h"
#include "SDL_mixer.h"

#define MUSIC_DIR ROCKBOX_DIR "/quake"

#if 1
static Mix_Music *mus = NULL;
#else
static int musfile = -1;
static int musfile_len, musfile_pos;

#define MP3_CHUNK 1024*512

static char mp3buf[MP3_CHUNK];

static void get_mp3(const void **start, size_t *size)
{
    if(musfile >= 0)
    {
        int len = MIN(musfile_len - musfile_pos, MP3_CHUNK);

        Sys_FileRead(musfile, mp3buf, len);
        *start = mp3buf;
        *size = len;

        musfile_pos += len;
    }
}
#endif

void CDAudio_Play(byte track, qboolean looping)
{
    char path[MAX_PATH];
    snprintf(path, sizeof(path), "%s/%02d.wav", MUSIC_DIR, track - 1);

    rb->splashf(HZ, "playing %s", path);

#if 1
    mus = Mix_LoadMUS(path);

    if(!mus)
    {
        rb->splashf(HZ, "Failed: %s", Mix_GetError());
        return;
    }

    Mix_PlayMusic(mus, looping ? -1 : 0);
#else

    musfile_len = Sys_FileOpenRead(path, &musfile);
    musfile_pos = 0;

    rb->mp3_play_data(NULL, 0, get_mp3);
#endif
}


void CDAudio_Stop(void)
{
    if ( (Mix_PlayingMusic()) || (Mix_PausedMusic()) )
        Mix_HaltMusic();
}


void CDAudio_Pause(void)
{
    Mix_PauseMusic();
}


void CDAudio_Resume(void)
{
    Mix_ResumeMusic();
}


void CDAudio_Update(void)
{
    //Mix_VolumeMusic(bgmvolume.value * SDL_MIX_MAXVOLUME);
}


int CDAudio_Init(void)
{
    rb->splashf(HZ, "CD Init");

    if(Mix_OpenAudio(MIX_DEFAULT_FREQUENCY, MIX_DEFAULT_FORMAT, 2, 1024)==-1) {
        rb->splashf(HZ, "Mix_OpenAudio: %s\n", Mix_GetError());
        return -1;
    }
    return 0;
}


void CDAudio_Shutdown(void)
{
    if(mus)
        Mix_FreeMusic(mus);
    CDAudio_Stop();
}
