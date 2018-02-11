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
// snd_mix.c -- portable code to mix sounds for snd_dma.c

#include "quakedef.h"

#define DWORD	unsigned long

// why not write straight to SHM buffer?!?
//portable_samplepair_t paintbuffer[PAINTBUFFER_SIZE];
int16_t *paintbuffer; // stereo interleaved samples

/*
===============================================================================

CHANNEL MIXING

===============================================================================
*/

// in snd_mix_*

void SND_PaintChannelFrom8 (int true_lvol, int true_rvol, signed char *sfx, int count);
void SND_PaintChannelFrom16 (int true_lvol, int true_rvol, signed short *sfx, int count);

void S_PaintChannels(int endtime)
{
    int 	i;
    int 	end;
    channel_t *ch;
    sfxcache_t	*sc;
    int		ltime, count;

    paintbuffer = shm->buffer;
        
    //while (paintedtime < endtime)
    //{
    // if paintbuffer is smaller than DMA buffer
        
    end = endtime;
    //if (endtime - paintedtime > PAINTBUFFER_SIZE)
    //end = paintedtime + PAINTBUFFER_SIZE;
        
    // clear the paint buffer
    memset(paintbuffer, 0, (end - paintedtime) * sizeof(int16_t));

    // 0-255
    int volume_fp = volume.value * 255;
    
    // paint in the channels.
    ch = channels;
    for (i=0; i<total_channels ; i++, ch++)
    {
        if (!ch->sfx)
            continue;
        if (!ch->leftvol && !ch->rightvol)
            continue;
        sc = S_LoadSound (ch->sfx); // sound fx cache
        if (!sc)
            continue;

        ltime = paintedtime;

        while (ltime < end)
        {	// paint up to end of sound channel, or end of buffer, whichever is greatest
            if (ch->end < end)
                count = ch->end - ltime;
            else
                count = end - ltime;

            //if(count == 1)
            //    rb->splashf(HZ, "Potential problem");
            
            if (count > 0)
            {
                int true_lvol, true_rvol;
                true_lvol = (volume_fp * ch->leftvol) / 256;
                true_rvol = (volume_fp * ch->rightvol) / 256;

                if (sc->width == 1)
                {
                    signed char *sfx = (char*)sc->data + ch->pos;
                    SND_PaintChannelFrom8(true_lvol, true_rvol, sfx, count);
                }
                else
                {
                    signed short *sfx = (short*)sc->data + ch->pos;
                    SND_PaintChannelFrom16(true_lvol, true_rvol, sfx, count);
                }

                ch->pos += count;
                ltime += count;
            }

            // if at end of loop, restart
            if (ltime >= ch->end)
            {
                if (sc->loopstart >= 0)
                {
                    ch->pos = sc->loopstart;
                    ch->end = ltime + sc->length - ch->pos;
                }
                else				
                {	// channel just stopped
                    ch->sfx = NULL;
                    break;
                }
            }
        }
    }

    // transfer out according to DMA format
    //S_TransferPaintBuffer(end);
    paintedtime = end;
}
