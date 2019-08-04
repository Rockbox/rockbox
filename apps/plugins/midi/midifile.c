/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Stepan Moskovchenko
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
#include "plugin.h"
#include "midiutil.h"
#include "midifile.h"

struct Track * readTrack(int file);
int readID(int file);

struct MIDIfile midi_file IBSS_ATTR;

struct MIDIfile * loadFile(const char * filename)
{
    struct MIDIfile * mfload;
    int file = rb->open (filename, O_RDONLY);

    if(file < 0)
    {
        midi_debug("Could not open file");
        return NULL;
    }

    mfload = &midi_file;

    rb->memset(mfload, 0, sizeof(struct MIDIfile));

    int fileID = readID(file);
    if(fileID != ID_MTHD)
    {
        if(fileID == ID_RIFF)
        {
            midi_debug("Detected RMID file");
            midi_debug("Looking for MThd header");
            char dummy[17];
            rb->read(file, &dummy, 16);
            if(readID(file) != ID_MTHD)
            {
                rb->close(file);
                midi_debug("Invalid MIDI header within RIFF.");
                return NULL;
            }

        } else
        {
            rb->close(file);
            midi_debug("Invalid file header chunk.");
            return NULL;
        }
    }

    if(readFourBytes(file)!=6)
    {
        rb->close(file);
        midi_debug("Header chunk size invalid.");
        return NULL;
    }

    if(readTwoBytes(file)==2)
    {
        rb->close(file);
        midi_debug("MIDI file type 2 not supported");
        return NULL;
    }

    mfload->numTracks = readTwoBytes(file);
    mfload->div = readTwoBytes(file);

    int track=0;

    midi_debug("File has %d tracks.", mfload->numTracks);

    while(! eof(file) && track < mfload->numTracks)
    {
        unsigned char id = readID(file);


        if(id == ID_EOF)
        {
            if(mfload->numTracks != track)
            {
                midi_debug("Warning: file claims to have %d tracks. I only see %d here.", mfload->numTracks, track);
                mfload->numTracks = track;
            }
            rb->close(file);
            return mfload;
        }

        if(id == ID_MTRK)
        {
            mfload->tracks[track] = readTrack(file);
            if (!mfload->tracks[track])
            {
                rb->close(file);
                return NULL;
            }
            track++;
        } else
        {
            midi_debug("SKIPPING TRACK");
            int len = readFourBytes(file);
            while(--len)
                readChar(file);
        }
    }

    rb->close(file);
    return mfload;

}

/* Global again. Not static. What if track 1 ends on a running status event
 * and then track 2 starts loading */

int rStatus = 0;
/* Returns 0 if done, 1 if keep going and -1 in case of error */
static int readEvent(int file, void * dest)
{
    struct Event dummy;
    struct Event * ev = (struct Event *) dest;

    if(ev == NULL)
        ev = &dummy; /* If we are just counting events instead of loading them */

    ev->delta = readVarData(file);


    int t=readChar(file);

    if((t&0x80) == 0x80) /* if not a running status event */
    {
        ev->status = t;
        if(t == 0xFF)
        {
            ev->d1 = readChar(file);
            ev->len = readVarData(file);

            /* Allocate and read in the data block */
            if(dest != NULL)
            {
                /* Null-terminate for text events */
                ev->evData = malloc(ev->len+1); /* Extra byte for the null termination */
                if (!ev->evData)
                    return -1;

                rb->read(file, ev->evData, ev->len);
                ev->evData[ev->len] = 0;

                switch(ev->d1)
                {
                    case 0x01:  /* Generic text */
                    {
                        midi_debug("Text: %s", ev->evData);
                        break;
                    }

                    case 0x02:  /* A copyright string within the file */
                    {
                        midi_debug("Copyright: %s", ev->evData);
                        break;
                    }

                    case 0x03:  /* Sequence of track name */
                    {
                        midi_debug("Name: %s", ev->evData);
                        break;
                    }

                    case 0x04:  /* Instrument (synth) name */
                    {
                        midi_debug("Instrument: %s", ev->evData);
                        break;
                    }

                    case 0x05:  /* Lyrics. These appear on the tracks at the right times */
                    {           /* Usually only a small 'piece' of the lyrics.           */
                                /* Maybe the sequencer should print these at play time?  */
                        midi_debug("Lyric: %s", ev->evData);
                        break;
                    }

                    case 0x06:  /* Text marker */
                    {
                        midi_debug("Marker: %s", ev->evData);
                        break;
                    }


                    case 0x07:  /* Cue point */
                    {
                        midi_debug("Cue point: %s", ev->evData);
                        break;
                    }

                    case 0x08:  /* Program name */
                    {
                        midi_debug("Patch: %s", ev->evData);
                        break;
                    }


                    case 0x09:  /* Device name. Very much irrelevant here, though. */
                    {
                        midi_debug("Port: %s", ev->evData);
                        break;
                    }
                }
            }
            else
            {
                /*
                 * Don't allocate anything, just see how much it would take
                 * To make memory usage efficient
                 */
                unsigned int a=0;
                for(a=0; a<ev->len; a++)
                    readChar(file); //Skip skip
            }

            if(ev->d1 == 0x2F)
            {
                return 0;   /* Termination meta-event */
            }
        } else              /* If part of a running status event */
        {
            rStatus = t;
            ev->status = t;
            ev->d1 = readChar(file);

            if (  ((t & 0xF0) != 0xD0) && ((t & 0xF0) != 0xC0) && ((t & 0xF0) > 0x40) )
            {
                ev->d2 = readChar(file);
            } else
                ev->d2 = 127;
        }
    } else  /* Running Status */
    {
        ev->status = rStatus;
        ev->d1 = t;
        if (  ((rStatus & 0xF0) != 0xD0) && ((rStatus & 0xF0) != 0xC0) && ((rStatus & 0xF0) > 0x40) )
        {
            ev->d2 = readChar(file);
        } else
            ev->d2 = 127;
    }
    return 1;
}

int curr_track = 0;
struct Track tracks[48] IBSS_ATTR;

struct Track * readTrack(int file)
{
    struct Track * trk = &tracks[curr_track++];
    rb->memset(trk, 0, sizeof(struct Track));

    trk->size = readFourBytes(file);
    trk->pos = 0;
    trk->delta = 0;

    int numEvents=0;

    int pos = rb->lseek(file, 0, SEEK_CUR);

    int evstat;

    while ((evstat = readEvent(file, NULL)) > 0) /* Memory saving technique                   */
        numEvents++;                /* Attempt to read in events, count how many */
                                    /* THEN allocate memory and read them in     */
    if (evstat < 0)
        return NULL;
    rb->lseek(file, pos, SEEK_SET);

    int trackSize = (numEvents+1) * sizeof(struct Event);
    void * dataPtr = malloc(trackSize);
    if (!dataPtr)
        return NULL;
    trk->dataBlock = dataPtr;

    numEvents=0;

    while ((evstat = readEvent(file, dataPtr)) > 0)
    {
        if(trackSize < dataPtr-trk->dataBlock)
        {
            midi_debug("Track parser memory out of bounds");
            return NULL;
        }
        dataPtr+=sizeof(struct Event);
        numEvents++;
    }
    if (evstat < 0)
        return NULL;
    trk->numEvents = numEvents;

    return trk;
}

int readID(int file)
{
    char id[5];
    id[4]=0;
    BYTE a;

    for(a=0; a<4; a++)
        id[a]=readChar(file);
    if(eof(file))
    {
        midi_debug("End of file reached.");
        return ID_EOF;
    }
    if(rb->strcmp(id, "MThd")==0)
        return ID_MTHD;
    if(rb->strcmp(id, "MTrk")==0)
        return ID_MTRK;
    if(rb->strcmp(id, "RIFF")==0)
        return ID_RIFF;
    return ID_UNKNOWN;
}


int readFourBytes(int file)
{
    int data=0;
    BYTE a=0;
    for(a=0; a<4; a++)
        data=(data<<8)+readChar(file);
    return data;
}

int readTwoBytes(int file)
{
    int data=(readChar(file)<<8)+readChar(file);
    return data;
}

/* This came from the MIDI file format guide */
int readVarData(int file)
{
    unsigned int value;
    char c;
    if ( (value = readChar(file)) & 0x80 )
    {
       value &= 0x7F;
       do
       {
         value = (value << 7) + ((c = readChar(file)) & 0x7F);
       } while (c & 0x80);
    }
    return(value);
}


/*
void unloadFile(struct MIDIfile * mf)
{
    if(mf == NULL)
        return;
    int a=0;
    //Unload each track
    for(a=0; a<mf->numTracks; a++)
    {
        int b=0;

        if(mf->tracks[a] != NULL)
            for(b=0; b<mf->tracks[a]->numEvents; b++)
            {
                if(((struct Event*)((mf->tracks[a]->dataBlock)+b*sizeof(struct Event)))->evData!=NULL)
                    free(((struct Event*)((mf->tracks[a]->dataBlock)+b*sizeof(struct Event)))->evData);
            }

        if(mf->tracks[a]!=NULL && mf->tracks[a]->dataBlock != NULL)
            free(mf->tracks[a]->dataBlock); //Unload the event block

        if(mf->tracks[a]!=NULL)
            free(mf->tracks[a]);    //Unload the track structure itself
    }
    free(mf);   //Unload the main struct
}
*/

