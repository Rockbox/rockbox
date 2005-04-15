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
#define ID_UNKNOWN 	-1
#define ID_MTHD		1
#define ID_MTRK 	2
#define ID_EOF  	3

//MIDI Commands
#define MIDI_NOTE_OFF	128
#define MIDI_NOTE_ON	144
#define MIDI_AFTERTOUCH	160
#define MIDI_CONTROL	176
#define MIDI_PRGM	192
#define MIDI_PITCHW	224


//MIDI Controllers
#define CTRL_VOLUME	7
#define CTRL_BALANCE	8
#define CTRL_PANNING	10
#define CHANNEL 	1


#define STATE_ATTACK  		1
#define STATE_DECAY  		2
#define STATE_SUSTAIN 		3
#define STATE_RELEASE 		4
#define STATE_RAMPDOWN		5

#define STATE_LOOPING           7
#define STATE_NONLOOPING	8

#define LOOP_ENABLED	4
#define LOOP_PINGPONG   8
#define LOOP_REVERSE    16

#define LOOPDIR_FORWARD 0
#define LOOPDIR_REVERSE 1


extern struct plugin_api * rb;


unsigned char chVol[16];	//Channel volume
unsigned char chPanLeft[16];	//Channel panning
unsigned char chPanRight[16];
unsigned char chPat[16];        //Channel patch
unsigned char chPW[16];        //Channel pitch wheel, MSB

struct GPatch * gusload(char *);
struct GPatch * patchSet[128];
struct GPatch * drumSet[128];
struct SynthObject voices[MAX_VOICES];



struct SynthObject
{
	int tmp;
	struct GWaveform * wf;
	unsigned int delta;
	unsigned int decay;
	unsigned int cp;
	unsigned char state, pstate, loopState, loopDir;
	unsigned char note, vol, ch, isUsed;
	int curRate, curOffset, targetOffset;
	int curPoint;
};


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
	unsigned int delta;	//For sequencing
	unsigned int pos;	//For sequencing
	void * dataBlock;
};


struct MIDIfile
{
	int  Length;

	//int Format; //We don't really care what type it is
	unsigned short numTracks;
	unsigned short div;	//Time division, X ticks per millisecond
	struct Track * tracks[48];
	unsigned char patches[128];
	int numPatches;
};

void *my_malloc(int size);

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
}

void *ralloc(char *offset, int len)
{
	int size;
	char *ret;

	if (offset == NULL)
	{
		return alloc(len);
	}

	size = *((unsigned int *)offset - 4);

	if (size >= 0x02000000)
	{
		return NULL;
	}

	ret = alloc(len);

	if (len < size)
	{
		rb->memcpy(ret, offset, len);
	}
	else
	{
		rb->memcpy(ret, offset, size);
		rb->memset(ret, 0, len - size);
	}

	return ret;
}


void * allocate(int size)
{
	return alloc(size);
}

void sendEvent(struct Event * ev);
int tick(struct MIDIfile * mf);
inline void setPoint(struct SynthObject * so, int pt);
struct Event * getEvent(struct Track * tr, int evNum);

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

void printf(char *fmt, ...) {}

//#define my_malloc(a) malloc(a)


void *audio_bufferbase;
void *audio_bufferpointer;
unsigned int audio_buffer_free;




void *my_malloc(int size)
{

    void *alloc;

    if (!audio_bufferbase)
    {
        audio_bufferbase = audio_bufferpointer
            = rb->plugin_get_audio_buffer(&audio_buffer_free);
#if MEM <= 8 && !defined(SIMULATOR)

        if ((unsigned)(ovl_start_addr - (unsigned char *)audio_bufferbase)
            < audio_buffer_free)
            audio_buffer_free = ovl_start_addr - (unsigned char *)audio_bufferbase;
#endif
    }
    if (size + 4 > audio_buffer_free)
        return 0;
    alloc = audio_bufferpointer;
    audio_bufferpointer += size + 4;
    audio_buffer_free -= size + 4;
    return alloc;
}

void setmallocpos(void *pointer)
{
    audio_bufferpointer = pointer;
    audio_buffer_free = audio_bufferpointer - audio_bufferbase;
}

void exit(int code)
{
}

void free(void * ptr)
{
}