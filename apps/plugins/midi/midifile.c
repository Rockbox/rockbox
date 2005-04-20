/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2005 Stepan Moskovchenko
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/



extern struct plugin_api * rb;

struct Track * readTrack(int file);
int readID(int file);
void bail(const char *);

struct MIDIfile * loadFile(char * filename)
{
    struct MIDIfile * mf;
    int file = rb->open (filename, O_RDONLY);

    if(file==-1)
    {
        bail("Could not open file\n");
    }

    mf = (struct MIDIfile*)allocate(sizeof(struct MIDIfile));

    if(mf==NULL)
    {
        rb->close(file);
        bail("Could not allocate memory for MIDIfile struct\n");
    }

    rb->memset(mf, 0, sizeof(struct MIDIfile));

    if(readID(file) != ID_MTHD)
    {
        rb->close(file);
        bail("Invalid file header chunk.");
    }

    if(readFourBytes(file)!=6)
    {
        rb->close(file);
        bail("Header chunk size invalid.");
    }

    if(readTwoBytes(file)==2)
    {
        rb->close(file);
        bail("MIDI file type not supported");
    }

    mf->numTracks = readTwoBytes(file);
    mf->div = readTwoBytes(file);

    int track=0;

    printf("\nnumTracks=%d  div=%d\nBegin reading track data\n", mf->numTracks, mf->div);


    while(! eof(file) && track < mf->numTracks)
    {
        unsigned char id = readID(file);


        if(id == ID_EOF)
        {
            if(mf->numTracks != track)
            {
                printf("\nError: file claims to have %d tracks.\n       I only see %d here.\n",     mf->numTracks, track);
                mf->numTracks = track;
            }
            return mf;
        }

        if(id == ID_MTRK)
        {
            mf->tracks[track] = readTrack(file);
            //exit(0);
            track++;
        } else
        {
            printf("\n SKIPPING TRACK");
            int len = readFourBytes(file);
            while(--len)
                readChar(file);
        }
    }
    return mf;

}


int rStatus = 0;

/* Returns 0 if done, 1 if keep going */
int readEvent(int file, void * dest)
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
                ev->evData = readData(file, ev->len);
                printf("\nDATA: <%s>", ev->evData);
            }
            else
            {
                /*
                 * Don't allocate anything, just see how much it would tale
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



struct Track * readTrack(int file)
{
    struct Track * trk = (struct Track *)allocate(sizeof(struct Track));
    rb->memset(trk, 0, sizeof(struct Track));

    trk->size = readFourBytes(file);
    trk->pos = 0;
    trk->delta = 0;

    int numEvents=0;

    int pos = rb->lseek(file, 0, SEEK_CUR);

    while(readEvent(file, NULL))    /* Memory saving technique                   */
        numEvents++;                /* Attempt to read in events, count how many */
                                    /* THEN allocate memory and read them in     */
    rb->lseek(file, pos, SEEK_SET);

    int trackSize = (numEvents+1) * sizeof(struct Event);
    void * dataPtr = allocate(trackSize);
    trk->dataBlock = dataPtr;

    numEvents=0;

    while(readEvent(file, dataPtr))
    {
        if(trackSize < dataPtr-trk->dataBlock)
        {
            printf("\nTrack parser memory out of  bounds");
            exit(1);
        }
        dataPtr+=sizeof(struct Event);
        numEvents++;
    }
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
        printf("\End of file reached.");
        return ID_EOF;
    }
    if(rb->strcmp(id, "MThd")==0)
        return ID_MTHD;
    if(rb->strcmp(id, "MTrk")==0)
        return ID_MTRK;
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
void bail(const char * err)
{
    rb->splash(HZ*3, true, err);
    exit(0);
}

