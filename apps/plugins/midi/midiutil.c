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


#define BYTE unsigned char

//Data chunk ID types, returned by readID()
#define ID_UNKNOWN  -1
#define ID_MTHD     1
#define ID_MTRK     2
#define ID_EOF      3

//MIDI Commands
#define MIDI_NOTE_OFF   128
#define MIDI_NOTE_ON    144
#define MIDI_AFTERTOUCH 160
#define MIDI_CONTROL    176
#define MIDI_PRGM   192
#define MIDI_PITCHW 224


//MIDI Controllers
#define CTRL_VOLUME 7
#define CTRL_BALANCE    8
#define CTRL_PANNING    10
#define CHANNEL     1

//Most of these are deprecated.. rampdown is used, maybe one other one too
#define STATE_ATTACK        1
#define STATE_DECAY         2
#define STATE_SUSTAIN       3
#define STATE_RELEASE       4
#define STATE_RAMPDOWN      5

//Loop states
#define STATE_LOOPING           7
#define STATE_NONLOOPING    8

//Various bits in the GUS mode byte
#define LOOP_ENABLED    4
#define LOOP_PINGPONG   8
#define LOOP_REVERSE    16

#define LOOPDIR_FORWARD 0
#define LOOPDIR_REVERSE 1


extern struct plugin_api * rb;


int chVol[16] IDATA_ATTR;       /* Channel volume                */
int chPanLeft[16] IDATA_ATTR;   /* Channel panning               */
int chPanRight[16] IDATA_ATTR;
int chPat[16];                  /* Channel patch                 */
int chPW[16];                   /* Channel pitch wheel, MSB only */


struct GPatch * gusload(char *);
struct GPatch * patchSet[128];
struct GPatch * drumSet[128];

struct Event
{
    unsigned int delta;
    unsigned char status, d1, d2;
    unsigned int len;
    unsigned char * evData;
};

struct Track
{
    unsigned int size;
    unsigned int numEvents;
    unsigned int delta; /* For sequencing */
    unsigned int pos;   /* For sequencing */
    void * dataBlock;
};


struct MIDIfile
{
    int  Length;
    unsigned short numTracks;
    unsigned short div; /* Time division, X ticks per millisecond */
    struct Track * tracks[48];
    unsigned char patches[128];
    int numPatches;
};

/*
struct SynthObject
{
    struct GWaveform * wf;
    unsigned int delta;
    unsigned int decay;
    unsigned int cp;
    unsigned char state, loopState, loopDir;
    unsigned char note, vol, ch, isUsed;
    int curRate, curOffset, targetOffset;
    unsigned int curPoint;
};
*/

struct SynthObject
{
    struct GWaveform * wf;
    int delta;
    int decay;
    unsigned int cp;
    int state, loopState, loopDir;
    int note, vol, ch, isUsed;
    int curRate, curOffset, targetOffset;
    int curPoint;
};

struct SynthObject voices[MAX_VOICES] IDATA_ATTR;



void sendEvent(struct Event * ev);
int tick(struct MIDIfile * mf);
inline void setPoint(struct SynthObject * so, int pt);
struct Event * getEvent(struct Track * tr, int evNum);
int readTwoBytes(int file);
int readFourBytes(int file);
int readVarData(int file);
int midimain(void * filename);


void *alloc(int size)
{
    static char *offset = NULL;
    static int totalSize = 0;
    char *ret;

    int remainder = size % 4;

    size = size + 4-remainder;

    if (offset == NULL)
    {
        offset = rb->plugin_get_audio_buffer(&totalSize);
    }

    if (size + 4 > totalSize)
    {
        return NULL;
    }

    ret = offset + 4;
    *((unsigned int *)offset) = size;

    offset += size + 4;
    totalSize -= size + 4;
    return ret;
}


/* Rick's code */
/*
void *alloc(int size)
{
    static char *offset = NULL;
    static int totalSize = 0;
    char *ret;


    if (offset == NULL)
    {
        offset = rb->plugin_get_audio_buffer(&totalSize);
    }

    if (size + 4 > totalSize)
    {
        return NULL;
    }

    ret = offset + 4;
    *((unsigned int *)offset) = size;

    offset += size + 4;
    totalSize -= size + 4;
    return ret;
}*/
void * allocate(int size)
{
    return alloc(size);
}

unsigned char readChar(int file)
{
    char buf[2];
    rb->read(file, &buf, 1);
    return buf[0];
}

unsigned char * readData(int file, int len)
{
    unsigned char * dat = allocate(len);
    rb->read(file, dat, len);
    return dat;
}

int eof(int fd)
{
    int curPos = rb->lseek(fd, 0, SEEK_CUR);

    int size = rb->lseek(fd, 0, SEEK_END);

    rb->lseek(fd, curPos, SEEK_SET);
    return size+1 == rb->lseek(fd, 0, SEEK_CUR);
}

void printf(char *fmt, ...) {fmt=fmt; }

void exit(int code)
{
    code = code; /* Stub function, kill warning for now */
}
