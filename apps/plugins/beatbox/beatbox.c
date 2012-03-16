/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2005 Karl Kurbjun based on midi2wav by Stepan Moskovchenko
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
#include "midi/guspat.h"
#include "midi/midiutil.h"
#include "midi/synth.h"
#include "midi/sequencer.h"
#include "midi/midifile.h"


/* variable button definitions */
#if (CONFIG_KEYPAD == IRIVER_H100_PAD) || (CONFIG_KEYPAD == IRIVER_H300_PAD)
#define BTN_QUIT         BUTTON_OFF
#define BTN_RIGHT        BUTTON_RIGHT
#define BTN_UP           BUTTON_UP
#define BTN_DOWN         BUTTON_DOWN

#define BTN_RC_QUIT      BUTTON_RC_STOP

#elif (CONFIG_KEYPAD == IPOD_4G_PAD) || (CONFIG_KEYPAD == IPOD_3G_PAD) || \
      (CONFIG_KEYPAD == IPOD_1G2G_PAD)
#define BTN_QUIT         (BUTTON_SELECT | BUTTON_MENU)
#define BTN_RIGHT        BUTTON_RIGHT
#define BTN_UP           BUTTON_SCROLL_FWD
#define BTN_DOWN         BUTTON_SCROLL_BACK

#elif (CONFIG_KEYPAD == GIGABEAT_PAD)
#define BTN_QUIT         BUTTON_POWER
#define BTN_RIGHT        BUTTON_RIGHT
#define BTN_UP           BUTTON_UP
#define BTN_DOWN         BUTTON_DOWN

#elif (CONFIG_KEYPAD == SANSA_E200_PAD) || \
(CONFIG_KEYPAD == SANSA_C200_PAD)
#define BTN_QUIT         BUTTON_POWER
#define BTN_RIGHT        BUTTON_RIGHT
#define BTN_UP           BUTTON_UP
#define BTN_DOWN         BUTTON_DOWN


#elif CONFIG_KEYPAD == IAUDIO_X5M5_PAD
#define BTN_QUIT         BUTTON_POWER
#define BTN_RIGHT        BUTTON_RIGHT
#define BTN_UP           BUTTON_UP
#define BTN_DOWN         BUTTON_DOWN

#elif CONFIG_KEYPAD == IRIVER_H10_PAD
#define BTN_QUIT         BUTTON_POWER
#define BTN_RIGHT        BUTTON_RIGHT
#define BTN_UP           BUTTON_SCROLL_UP
#define BTN_DOWN         BUTTON_SCROLL_DOWN

#elif CONFIG_KEYPAD == PHILIPS_HDD1630_PAD
#define BTN_QUIT         BUTTON_POWER
#define BTN_RIGHT        BUTTON_RIGHT
#define BTN_UP           BUTTON_UP
#define BTN_DOWN         BUTTON_DOWN

#elif CONFIG_KEYPAD == PHILIPS_HDD6330_PAD
#define BTN_QUIT         BUTTON_POWER
#define BTN_RIGHT        BUTTON_NEXT
#define BTN_UP           BUTTON_UP
#define BTN_DOWN         BUTTON_DOWN

#elif CONFIG_KEYPAD == PHILIPS_SA9200_PAD
#define BTN_QUIT         BUTTON_POWER
#define BTN_RIGHT        BUTTON_NEXT
#define BTN_UP           BUTTON_UP
#define BTN_DOWN         BUTTON_DOWN

#elif (CONFIG_KEYPAD == SAMSUNG_YH820_PAD) || \
      (CONFIG_KEYPAD == SAMSUNG_YH92X_PAD)
#define BTN_QUIT         BUTTON_PLAY
#define BTN_RIGHT        BUTTON_RIGHT
#define BTN_UP           BUTTON_UP
#define BTN_DOWN         BUTTON_DOWN

#elif CONFIG_KEYPAD == PBELL_VIBE500_PAD
#define BTN_QUIT         BUTTON_REC
#define BTN_RIGHT        BUTTON_NEXT
#define BTN_UP           BUTTON_UP
#define BTN_DOWN         BUTTON_DOWN

#elif CONFIG_KEYPAD == CREATIVE_ZENXFI3_PAD
#define BTN_QUIT         BUTTON_PLAY
#define BTN_RIGHT        BUTTON_MENU
#define BTN_UP           BUTTON_UP
#define BTN_DOWN         BUTTON_DOWN

#elif CONFIG_KEYPAD == SANSA_FUZEPLUS_PAD
#define BTN_QUIT         BUTTON_POWER
#define BTN_RIGHT        BUTTON_RIGHT
#define BTN_UP           BUTTON_UP
#define BTN_DOWN         BUTTON_DOWN

#endif

#define FRACTSIZE 10

#ifndef SIMULATOR

#if (HW_SAMPR_CAPS & SAMPR_CAP_22)
#define SAMPLE_RATE SAMPR_22  // 44100 22050 11025
#else
#define SAMPLE_RATE SAMPR_44  // 44100 22050 11025
#endif

#define MAX_VOICES 20   // Note: 24 midi channels is the minimum general midi
                        // spec implementation

#else   // Simulator requires 44100, and we can afford to use more voices

#define SAMPLE_RATE SAMPR_44
#define MAX_VOICES 48

#endif


#define BUF_SIZE 256
#define NBUF   2

#undef SYNC

#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
    #define SYNC
#endif

struct MIDIfile * mf IBSS_ATTR;

int numberOfSamples IBSS_ATTR;
long bpm IBSS_ATTR;

const unsigned char * drumNames[]={
    "Bass Drum 2    ",
    "Bass Drum 1    ",
    "Side Stick     ",
    "Snare Drum 1   ",
    "Hand Clap      ",
    "Snare Drum 2   ",
    "Low Tom 2      ",
    "Closed Hi-hat  ",
    "Low Tom 1      ",
    "Pedal Hi-hat   ",
    "Mid Tom 2      ",
    "Open Hi-hat    ",
    "Mid Tom 1      ",
    "High Tom 2     ",
    "Crash Cymbal 1 ",
    "High Tom 1     ",
    "Ride Cymbal 1  ",
    "Chinese Cymbal ",
    "Ride Bell      ",
    "Tambourine     ",
    "Splash Cymbal  ",
    "Cowbell        ",
    "Crash Cymbal 2 ",
    "Vibra Slap     ",
    "Ride Cymbal 2  ",
    "High Bongo     ",
    "Low Bongo      ",
    "Mute High Conga",
    "Open High Conga",
    "Low Conga      ",
    "High Timbale   ",
    "Low Timbale    ",
    "High Agogo     ",
    "Low Agogo      ",
    "Cabasa         ",
    "Maracas        ",
    "Short Whistle  ",
    "Long Whistle   ",
    "Short Guiro    ",
    "Long Guiro     ",
    "Claves         ",
    "High Wood Block",
    "Low Wood Block ",
    "Mute Cuica     ",
    "Open Cuica     ",
    "Mute Triangle  ",
    "Open Triangle  ",
    "Shaker         ",
    "Jingle Bell    ",
    "Bell Tree      ",
    "Castenets      ",
    "Mute Surdo     ",
    "Open Surdo     "
};

long gmbuf[BUF_SIZE*NBUF];

int quit=0;

#define STATE_STOPPED 0
#define STATE_PAUSED 1
#define STATE_PLAYING 2


#define BEATBOX_UP      BUTTON_UP
#define BEATBOX_DOWN    BUTTON_DOWN
#define BEATBOX_LEFT    BUTTON_LEFT
#define BEATBOX_RIGHT   BUTTON_RIGHT
#define BEATBOX_SELECT  BUTTON_SELECT

#if (CONFIG_KEYPAD == SANSA_FUZEPLUS_PAD)
#define BEATBOX_PLAY BUTTON_PLAYPAUSE
#define BEATBOX_STOP BUTTON_BACK
#else
#define BEATBOX_PLAY BUTTON_ON
#define BEATBOX_STOP BUTTON_OFF
#endif

#define VAL_NONE    0
#define VAL_ENABLED 1
#define VAL_LOOP    2

#define H_NUMCELLS  24
#define V_NUMCELLS  8

#define HILIGHT_NONE 0
#define HILIGHT_PLAY 1
#define HILIGHT_USER 2

#define CELL_XSIZE 9
#define CELL_YSIZE 9

#define GRID_XPOS 2
#define GRID_YPOS 10


#define COLOR_NAME_TEXT LCD_RGBPACK(0xFF,0xFF,0xFF)
#define COLOR_NORMAL    LCD_RGBPACK(0xFF,0xFF,0xFF)
#define COLOR_PLAY      LCD_RGBPACK(0xFF,0xFF,0x00)
#define COLOR_DISABLED  LCD_RGBPACK(0xA0,0xA0,0xA0)
#define COLOR_LOOPCELL  LCD_RGBPACK(0xC0,0xC0,0xC0)
#define COLOR_EDIT      LCD_RGBPACK(0x30,0x30,0xFF)
#define COLOR_GRID      LCD_RGBPACK(0xD0,0xD0,0xD0)

#define EDITSTATE_PATTERN 0

int xCursor=0, yCursor=0;

int editState=EDITSTATE_PATTERN;

int playState=STATE_STOPPED, stepFlag=0;


enum plugin_status plugin_start(const void* parameter)
{
    int retval = 0;

    rb->lcd_setfont(FONT_SYSFIXED);

#if defined(HAVE_ADJUSTABLE_CPU_FREQ)
    rb->cpu_boost(true);
#endif

#ifdef RB_PROFILE
    rb->profile_thread();
#endif
    if (initSynth(NULL, ROCKBOX_DIR "/patchset/patchset.cfg",
        ROCKBOX_DIR "/patchset/drums.cfg") == -1)
    {
        printf("\nINIT ERROR\n");
        return -1;
    }
//#ifndef SIMULATOR
    rb->pcm_play_stop();
#if INPUT_SRC_CAPS != 0
    /* Select playback */
    rb->audio_set_input_source(AUDIO_SRC_PLAYBACK, SRCF_PLAYBACK);
    rb->audio_set_output_source(AUDIO_SRC_PLAYBACK);
#endif
    rb->pcm_set_frequency(SAMPLE_RATE); // 44100 22050 11025


    retval = beatboxmain();

#ifdef RB_PROFILE
    rb->profstop();
#endif

    rb->pcm_play_stop();
    rb->pcm_set_frequency(HW_SAMPR_DEFAULT);

#if defined(HAVE_ADJUSTABLE_CPU_FREQ)
    rb->cpu_boost(false);
#endif


    if(retval == -1)
        return PLUGIN_ERROR;
    return PLUGIN_OK;
}

bool swap=0;
bool lastswap=1;

inline void synthbuf(void)
{
   long *outptr;
   register int i;
   static int currentSample=0;
   int synthtemp[2];

#ifndef SYNC
   if(lastswap==swap) return;
   lastswap=swap;

   outptr=(swap ? gmbuf : gmbuf+BUF_SIZE);
#else
   outptr=gmbuf;
#endif

   for(i=0; i<BUF_SIZE/2; i++)
   {
      synthSample(&synthtemp[0], &synthtemp[1]);
      currentSample++;
      *outptr=((synthtemp[0]&0xFFFF) << 16) | (synthtemp[1]&0xFFFF);
      outptr++;
      if(currentSample==numberOfSamples)
      {
            if(playState == STATE_PLAYING)
            {
                stepFlag=1;
            }

            currentSample=0;
      }
   }
}





unsigned char trackPos[V_NUMCELLS];
unsigned char trackData[H_NUMCELLS][V_NUMCELLS];
unsigned char trackMap[V_NUMCELLS] = {38, 39, 40, 41, 42, 43, 44, 56};


struct Cell
{
    unsigned char val;
    int color;
};

struct Cell pattern[H_NUMCELLS][V_NUMCELLS];
struct Cell dispPattern[H_NUMCELLS][V_NUMCELLS];


void advancePosition()
{
    int i=0;
    for(i=0; i<V_NUMCELLS; i++)
    {
        trackPos[i]++;
        if(trackPos[i] == H_NUMCELLS || trackData[trackPos[i]][i] == VAL_LOOP)
            trackPos[i]=0;
    }
}


void sendEvents()
{
    int i;
    for(i=0; i<V_NUMCELLS; i++)
    {
        if(trackData[trackPos[i]][i] == VAL_ENABLED)
            pressNote(9, trackMap[i], 127);
    }
}

#define NAME_POSX 10
#define NAME_POSY 100
void showDrumName(int trackNum)
{
    rb->lcd_set_foreground(COLOR_NAME_TEXT);
    rb->lcd_putsxy(NAME_POSX, NAME_POSY, drumNames[trackMap[trackNum]-35]);
}

void updateDisplay()
{
    int i, j;
    int grayOut=0;

    for(j=0; j<V_NUMCELLS; j++)
    {
        grayOut=0;
        for(i=0; i<H_NUMCELLS; i++)
        {
            pattern[i][j].color = COLOR_NORMAL;
            pattern[i][j].val = trackData[i][j];

            if(trackPos[j] == i)
                pattern[i][j].color = COLOR_PLAY;

            if(grayOut)
                pattern[i][j].color = COLOR_DISABLED;

            if(trackData[i][j] == VAL_LOOP)
            {
                pattern[i][j].color = COLOR_LOOPCELL;
                grayOut=1;
            }

            if(xCursor == i && yCursor == j && editState == EDITSTATE_PATTERN)
                pattern[i][j].color = COLOR_EDIT;
        }
    }

}

void resetPosition()
{
    int i;
    for(i=0; i<V_NUMCELLS; i++)
        trackPos[i]=0;
}

void clearCells()
{
    int i,j;
    for(i=0; i<H_NUMCELLS; i++)
        for(j=0; j<V_NUMCELLS; j++)
        {
            pattern[i][j].val=VAL_NONE;
            dispPattern[i][j].val=VAL_NONE;
            pattern[i][j].color = 0;
            dispPattern[i][j].color = 0;
        }
}




void drawGrid()
{
    int i, j;

    rb->lcd_set_foreground(COLOR_GRID);

    for(i=0; i<H_NUMCELLS+1; i++)
        rb->lcd_vline(i*CELL_XSIZE+GRID_XPOS, GRID_YPOS, GRID_YPOS+CELL_YSIZE*V_NUMCELLS);

    for(i=0; i<V_NUMCELLS+1; i++)
        rb->lcd_hline(GRID_XPOS, GRID_XPOS+CELL_XSIZE*H_NUMCELLS, GRID_YPOS+i*CELL_YSIZE);


    rb->lcd_update();
}

void drawCell(int i, int j)
{
    int cellX, cellY;

    cellX = GRID_XPOS + CELL_XSIZE*i+1;
    cellY = GRID_YPOS + CELL_YSIZE*j+1;

    rb->lcd_set_foreground(pattern[i][j].color);
    rb->lcd_fillrect(cellX, cellY, CELL_XSIZE-1, CELL_YSIZE-1);

    rb->lcd_set_foreground(0);

    if(pattern[i][j].val == VAL_LOOP)
    {
        rb->lcd_drawline(cellX, cellY, cellX+CELL_XSIZE-2, cellY+CELL_YSIZE-2);
    }

    if(pattern[i][j].val == VAL_ENABLED)
    {
        rb->lcd_fillrect(cellX+1, cellY+1, CELL_XSIZE-3, CELL_YSIZE-3);
    }

}

void redrawScreen(unsigned char force)
{
    int i, j;

    for(i=0; i<H_NUMCELLS; i++)
    {
        for(j=0; j<V_NUMCELLS; j++)
        {
            if(force || (pattern[i][j].val != dispPattern[i][j].val || pattern[i][j].color != dispPattern[i][j].color))
            {
                drawCell(i, j);
                dispPattern[i][j].val = pattern[i][j].val;
                dispPattern[i][j].color = pattern[i][j].color;
            }
        }
    }
    rb->lcd_update();
}

void get_more(const void** start, size_t* size)
{
#ifndef SYNC
    if(lastswap!=swap)
    {
//        printf("Buffer miss!"); // Comment out the printf to make missses less noticable.
    }

#else
    synthbuf();  // For some reason midiplayer crashes when an update is forced
#endif

    *size = BUF_SIZE*sizeof(short);
#ifndef SYNC
    *start = swap ? gmbuf : gmbuf + BUF_SIZE;
    swap=!swap;
#else
    *start = gmbuf;
#endif
}

int beatboxmain()
{
    int vol=0;


    numberOfSamples=44100/10;
    synthbuf();
    rb->pcm_play_data(&get_more, NULL, NULL, 0);

    rb->lcd_set_background(0x000000);
    rb->lcd_clear_display();

    resetPosition();

    int i, j;

    /* Start at 16 cells/loop for now. User can un-loop if more are needed */
    for(i=0; i<V_NUMCELLS; i++)
        trackData[16][i] = VAL_LOOP;


/*  Very very rough beat to 'Goodbye Horses'
    trackData[16][3] = VAL_LOOP;
    trackData[16][2] = VAL_LOOP;

    trackData[0][3] = 1;
    trackData[4][3] = 1;
    trackData[8][3] = 1;
    trackData[9][3] = 1;
    trackData[12][3] = 1;
    trackData[13][3] = 1;

    trackData[2][2] = 1;
    trackData[6][2] = 1;
    trackData[10][2] = 1;
    trackData[14][2] = 1;
*/

    drawGrid();
    showDrumName(yCursor);
    updateDisplay();
    redrawScreen(1);


    while(!quit)
    {
    #ifndef SYNC
        synthbuf();
    #endif
        rb->yield();

        if(stepFlag)
        {
            advancePosition();
            sendEvents();
            updateDisplay();
            redrawScreen(0);
            stepFlag=0;
        }

        /* Prevent idle poweroff */
        rb->reset_poweroff_timer();

        /* Code taken from Oscilloscope plugin */
        switch(rb->button_get(false))
        {
        /*
                case BTN_UP:
                case BTN_UP | BUTTON_REPEAT:
                    vol = rb->global_settings->volume;
                    if (vol < rb->sound_max(SOUND_VOLUME))
                    {
                        vol++;
                        rb->sound_set(SOUND_VOLUME, vol);
                        rb->global_settings->volume = vol;
                    }
                    break;

                case BTN_DOWN:
                case BTN_DOWN | BUTTON_REPEAT:
                    vol = rb->global_settings->volume;
                    if (vol > rb->sound_min(SOUND_VOLUME))
                    {
                        vol--;
                        rb->sound_set(SOUND_VOLUME, vol);
                        rb->global_settings->volume = vol;
                    }
                    break;

                case BTN_RIGHT:
                {
                    //pressNote(9, 40, 127);
                 //   resetPosition();
                    advancePosition();
                    sendEvents();
                    updateDisplay();
                    redrawScreen(0);
                    break;
                }

                case BUTTON_LEFT:
                {

//                    isPlaying=1;
                    resetPosition();
                    updateDisplay();
                    redrawScreen(0);
                    //pressNote(9, 39, 127);
                    break;
                }
*/

                case BEATBOX_UP:
                case BEATBOX_UP | BUTTON_REPEAT:
                {
                    if(editState == EDITSTATE_PATTERN)
                    {
                        if(yCursor > 0)
                        {
                            yCursor--;
                            showDrumName(yCursor);
                            updateDisplay();
                            redrawScreen(0);
                        }
                    }
                    break;
                }

                case BEATBOX_DOWN:
                case BEATBOX_DOWN | BUTTON_REPEAT:
                {
                    if(editState == EDITSTATE_PATTERN)
                    {
                        if(yCursor < V_NUMCELLS-1)
                        {
                            yCursor++;
                            showDrumName(yCursor);
                            updateDisplay();
                            redrawScreen(0);
                        }
                    }
                    break;
                }

                case BEATBOX_LEFT:
                case BEATBOX_LEFT | BUTTON_REPEAT:
                {
                    if(editState == EDITSTATE_PATTERN)
                    {
                        if(xCursor > 0)
                        {
                            xCursor--;
                            updateDisplay();
                            redrawScreen(0);
                        }
                    }
                    break;
                }

                case BEATBOX_RIGHT:
                case BEATBOX_RIGHT | BUTTON_REPEAT:
                {
                    if(editState == EDITSTATE_PATTERN)
                    {
                        if(xCursor < H_NUMCELLS-1)
                        {
                            xCursor++;
                            updateDisplay();
                            redrawScreen(0);
                        }
                    }
                    break;
                }

                case BEATBOX_SELECT:
                {
                    if(editState == EDITSTATE_PATTERN)
                    {
                        int cv = trackData[xCursor][yCursor];
                        cv++;
                        if(cv > VAL_LOOP)
                            cv = VAL_NONE;

                        trackData[xCursor][yCursor] = cv;

                        updateDisplay();
                        redrawScreen(0);
                    }
                    break;
                }


                case BEATBOX_PLAY:
                {
                    if(playState == STATE_PLAYING)
                        playState = STATE_PAUSED;
                    else
                    {
                        updateDisplay();
                        redrawScreen(0);
                        sendEvents();
                        playState = STATE_PLAYING;
                    }
                    break;
                }

                case BEATBOX_STOP:
                {
                    if(playState == STATE_STOPPED)
                    {
                        quit=1;
                    } else
                    {
                        playState =STATE_STOPPED;
                        resetPosition();
                        updateDisplay();
                        redrawScreen(0);
                    }
                    break;
                }
        }


    }

    return 0;
}

