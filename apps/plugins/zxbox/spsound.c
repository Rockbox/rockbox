/*
 * Copyright (C) 1996-1998 Szeredi Miklos 2006 Anton Romanov
 * Email: mszeredi@inf.bme.hu
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version. See the file COPYING. 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */

/* #define DEBUG_AUDIO */

#include "spsound.h"

#include "zxconfig.h"
#include "spperif.h"
#include "z80.h"
#include "zxmisc.h"
#include "interf.h"
#include "spmain.h"

#include <stdio.h>

int bufframes = 1;

int sound_avail = 0;
int sound_on = 1;

int sound_to_autoclose = 0;
int doneplay=0;


#ifdef HAVE_SOUND

#include "stdlib.h"
#include "file.h"
#include "time.h"
#include "errno.h"
#include "string.h"



#define SKIPTIME 5000

static int last_not_played=0;

#define SPS_OPENED        0
#define SPS_AUTOCLOSED     -1
#define SPS_BUSY     -2
#define SPS_CLOSED     -3
#define SPS_NONEXIST     -4

static int sndstate = SPS_CLOSED;

static void close_snd(int normal);
unsigned short my_buf[TMNUM*2*3*2];


const byte lin8_ulaw[] = {
    31,   31,   31,   32,   32,   32,   32,   33, 
    33,   33,   33,   34,   34,   34,   34,   35, 
    35,   35,   35,   36,   36,   36,   36,   37, 
    37,   37,   37,   38,   38,   38,   38,   39, 
    39,   39,   39,   40,   40,   40,   40,   41, 
    41,   41,   41,   42,   42,   42,   42,   43, 
    43,   43,   43,   44,   44,   44,   44,   45, 
    45,   45,   45,   46,   46,   46,   46,   47, 
    47,   47,   47,   48,   48,   49,   49,   50, 
    50,   51,   51,   52,   52,   53,   53,   54, 
    54,   55,   55,   56,   56,   57,   57,   58, 
    58,   59,   59,   60,   60,   61,   61,   62, 
    62,   63,   63,   64,   65,   66,   67,   68, 
    69,   70,   71,   72,   73,   74,   75,   76, 
    77,   78,   79,   81,   83,   85,   87,   89, 
    91,   93,   95,   99,  103,  107,  111,  119, 
   255,  247,  239,  235,  231,  227,  223,  221, 
   219,  217,  215,  213,  211,  209,  207,  206, 
   205,  204,  203,  202,  201,  200,  199,  198, 
   197,  196,  195,  194,  193,  192,  191,  191, 
   190,  190,  189,  189,  188,  188,  187,  187, 
   186,  186,  185,  185,  184,  184,  183,  183, 
   182,  182,  181,  181,  180,  180,  179,  179, 
   178,  178,  177,  177,  176,  176,  175,  175, 
   175,  175,  174,  174,  174,  174,  173,  173, 
   173,  173,  172,  172,  172,  172,  171,  171, 
   171,  171,  170,  170,  170,  170,  169,  169, 
   169,  169,  168,  168,  168,  168,  167,  167, 
   167,  167,  166,  166,  166,  166,  165,  165, 
   165,  165,  164,  164,  164,  164,  163,  163, 
   163,  163,  162,  162,  162,  162,  161,  161, 
   161,  161,  160,  160,  160,  160,  159,  159, 
};

static void open_snd(void)
{
    sndstate = SPS_OPENED;
    sound_avail=1;
   rb->pcm_play_stop();
#if INPUT_SRC_CAPS != 0
    /* Select playback */
    rb->audio_set_input_source(AUDIO_SRC_PLAYBACK, SRCF_PLAYBACK);
    rb->audio_set_output_source(AUDIO_SRC_PLAYBACK);
#endif
    rb->pcm_set_frequency(SAMPR_44);
}

static void close_snd(int normal)
{
    (void)normal;
    sound_avail = 0;
    rb->pcm_play_stop();    
    rb->pcm_set_frequency(HW_SAMPR_DEFAULT);
}



void init_spect_sound(void)
{
#if 1 /* TODO: Is this OK? */
  open_snd();
#endif
}


#ifndef VOLREDUCE
#define VOLREDUCE 0
#endif

#define CONVU8(x) ((byte) (((x) >> VOLREDUCE) + 128))

#ifdef CONVERT_TO_ULAW
#  define CONV(x) lin8_ulaw[(int) CONVU8(x)]
#else
#  define CONV(x) CONVU8(x)
#endif

#define HIGH_PASS(hp, sv) (((hp) * 15 + (sv)) >> 4)
#define TAPESOUND(tsp)    ((*tsp) >> 4)

static void process_sound(void)
{
  static int soundhp; 
  int i;
  byte *sb;
  register int sv;

  sb = sp_sound_buf;
  if(last_not_played) {
    soundhp = *sb;
    last_not_played = 0;
  }

  if(!sp_playing_tape) {
    for(i = TMNUM; i; sb++,i--) {
      sv = *sb;
      soundhp = HIGH_PASS(soundhp, sv);
      *sb = CONV(sv - soundhp);
    }
  }
  else {
    signed char *tsp;
    
    tsp = sp_tape_sound;
    for(i = TMNUM; i; sb++,tsp++,i--) {
      sv = *sb + TAPESOUND(tsp);
      soundhp = (soundhp * 15 + sv)>>4;
      *sb = CONV(sv - soundhp);
    }
  }
}

void autoclose_sound(void)
{
    /* do we have any reason to autoclose sound? */
#if 0
  if(sound_on && sound_to_autoclose && sndstate >= SPS_CLOSED) {
    close_snd(1);
    sndstate = SPS_AUTOCLOSED;
  }
#endif
}
void get_more(unsigned char** start, size_t* size)
{
    doneplay = 1;
    rb->pcm_play_stop();
   (void)*start;
   (void)*size;
}

/* sp_sound_buf is Unsigned 8 bit, Rate 8000 Hz, Mono */

void write_buf(void){
    int i,j;

    /* still not sure what is the best way to do this */
#if 0
    my_buf[i] = /*(sp_sound_buf[i]<<8)-0x8000*/sp_sound_buf[i]<<8;


    for (i = 0, j = 0; i<TMNUM; i++, j+=6)
        my_buf[j] = my_buf[j+1] = my_buf[j+2] = my_buf[j+3] = my_buf[j+4] = my_buf[j+5] = (((byte)sp_sound_buf[i])<<8) >> settings.volume;
#endif


    for (i = 0, j = 0; i<TMNUM; i++, j+=12)
        my_buf[j] = my_buf[j+1] = my_buf[j+2] = my_buf[j+3]\
                    = my_buf[j+4] = my_buf[j+5] = my_buf[j+6]\
                    = my_buf[j+7] = my_buf[j+8] = my_buf[j+9] \
                    = my_buf[j+10] = my_buf[j+11] \
                    = (((byte)sp_sound_buf[i])<<8) >> settings.volume;

    rb->pcm_play_data(&get_more,(unsigned char*)(my_buf),TMNUM*4*3*2);

#if 0
    /* can use to save and later analyze what we produce */
    i = rb->open ( "/sound.raw" , O_WRONLY | O_APPEND | O_CREAT, 0666);
    rb->write ( i  , sp_sound_buf , TMNUM );
    rb->close (i);


    i = rb->open ( "/sound2.raw" , O_WRONLY | O_APPEND |O_CREAT, 0666);
    rb->write ( i  , (unsigned char *)my_buf , TMNUM*4*3 );
    rb->close (i);
#endif


    while(!doneplay)
        rb->yield();
 
}
void play_sound(int evenframe)
{
  if(evenframe) return;
  if(!sound_on) {
    if(sndstate <= SPS_CLOSED) return;
    if(sndstate < SPS_OPENED) {
      sndstate = SPS_CLOSED;
      return;
    }
    close_snd(1);
    return;
  }
  
  z80_proc.sound_change = 0;

  process_sound();

  write_buf();
}


void setbufsize(void)
{
    
}


#else /* HAVE_SOUND */

/* Dummy functions */

void setbufsize(void)
{
}

void init_spect_sound(void)
{
}

void play_sound(int evenframe)
{
  (void)evenframe;
}

void autoclose_sound(void)
{
}

#endif /* NO_SOUND */


