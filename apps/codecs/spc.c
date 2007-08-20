/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006-2007 Adam Gashlin (hcs)
 * Copyright (C) 2004-2007 Shay Green (blargg)
 * Copyright (C) 2002 Brad Martin
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

/* lovingly ripped off from Game_Music_Emu 0.5.2. http://www.slack.net/~ant/ */
/* DSP Based on Brad Martin's OpenSPC DSP emulator */
/* tag reading from sexyspc by John Brawn (John_Brawn@yahoo.com) and others */
#include "codeclib.h"
#include "spc/spc_codec.h"
#include "spc/spc_profiler.h"

CODEC_HEADER

/**************** ID666 parsing ****************/

struct {
    unsigned char isBinary;
    char song[32];
    char game[32];
    char dumper[16];
    char comments[32];
    int day,month,year;
    unsigned long length;
    unsigned long fade;
    char artist[32];
    unsigned char muted;
    unsigned char emulator;
} ID666;

static int LoadID666(unsigned char *buf) {
    unsigned char *ib=buf;
    int isbinary = 1;
    int i;
  
    memcpy(ID666.song,ib,32);
    ID666.song[31]=0;
    ib+=32;

    memcpy(ID666.game,ib,32);
    ID666.game[31]=0;
    ib+=32;

    memcpy(ID666.dumper,ib,16);
    ID666.dumper[15]=0;
    ib+=16;

    memcpy(ID666.comments,ib,32);
    ID666.comments[31]=0;
    ib+=32;

    /* Ok, now comes the fun part. */
   
    /* Date check */
    if(ib[2] == '/'  && ib[5] == '/' )
        isbinary = 0;
  
    /* Reserved bytes check */
    if(ib[0xD2 - 0x2E - 112] >= '0' &&
       ib[0xD2 - 0x2E - 112] <= '9' &&
       ib[0xD3 - 0x2E - 112] == 0x00)
        isbinary = 0;
    
    /* is length & fade only digits? */
    for (i=0;i<8 && ( 
        (ib[0xA9 - 0x2E - 112+i]>='0'&&ib[0xA9 - 0x2E - 112+i]<='9') ||
        ib[0xA9 - 0x2E - 112+i]=='\0');
        i++);
    if (i==8) isbinary=0;
    
    ID666.isBinary = isbinary;
  
    if(isbinary) {
        DEBUGF("binary tag detected\n");
        ID666.year=*ib;
        ib++;
        ID666.year|=*ib<<8;
        ib++;
        ID666.month=*ib;
        ib++;    
        ID666.day=*ib;
        ib++;

        ib+=7;

        ID666.length=*ib;
        ib++;
    
        ID666.length|=*ib<<8;
        ib++;
    
        ID666.length|=*ib<<16;
        ID666.length*=1000;
        ib++;
    
        ID666.fade=*ib;
        ib++;
        ID666.fade|=*ib<<8; 
        ib++;
        ID666.fade|=*ib<<16;
        ib++;
        ID666.fade|=*ib<<24;
        ib++;

        memcpy(ID666.artist,ib,32);
        ID666.artist[31]=0;
        ib+=32;

        ID666.muted=*ib;
        ib++;

        ID666.emulator=*ib;
        ib++;    
    } else {
        unsigned long tmp;
        char buf[64];
        
        DEBUGF("text tag detected\n");
        
        memcpy(buf, ib, 2);
        buf[2] = 0; 
        tmp = 0;
        for (i=0;i<2 && buf[i]>='0' && buf[i]<='9';i++) tmp=tmp*10+buf[i]-'0';
        ID666.month = tmp;
        ib+=3;
        
        memcpy(buf, ib, 2);
        buf[2] = 0; 
        tmp = 0;
        for (i=0;i<2 && buf[i]>='0' && buf[i]<='9';i++) tmp=tmp*10+buf[i]-'0';
        ID666.day = tmp;
        ib+=3;
        
        memcpy(buf, ib, 4);
        buf[4] = 0; 
        tmp = 0;
        for (i=0;i<4 && buf[i]>='0' && buf[i]<='9';i++) tmp=tmp*10+buf[i]-'0';
        ID666.year = tmp;
        ib+=5;
    
        memcpy(buf, ib, 3);
        buf[3] = 0; 
        tmp = 0;
        for (i=0;i<3 && buf[i]>='0' && buf[i]<='9';i++) tmp=tmp*10+buf[i]-'0';
        ID666.length = tmp * 1000;
        ib+=3;

        memcpy(buf, ib, 5);
        buf[5] = 0;
        tmp = 0;
        for (i=0;i<5 && buf[i]>='0' && buf[i]<='9';i++) tmp=tmp*10+buf[i]-'0';
        ID666.fade = tmp;
        ib+=5;

        memcpy(ID666.artist,ib,32);
        ID666.artist[31]=0;
        ib+=32;

        /*I have no idea if this is right or not.*/
        ID666.muted=*ib;
        ib++;

        memcpy(buf, ib, 1);
        buf[1] = 0;
        tmp = 0;
        ib++;
    }
    return 1;
}

/**************** Codec ****************/

static int32_t samples[WAV_CHUNK_SIZE*2] IBSS_ATTR;

static struct Spc_Emu spc_emu IDATA_ATTR;

enum {SAMPLE_RATE = 32000};

/* The main decoder loop */
static int play_track( void )
{
    int sampleswritten=0;
    
    unsigned long fadestartsample = ID666.length*(long long) SAMPLE_RATE/1000;
    unsigned long fadeendsample = (ID666.length+ID666.fade)*(long long) SAMPLE_RATE/1000;
    int fadedec = 0;
    int fadevol = 0x7fffffffl;
    
    if (fadeendsample>fadestartsample)
        fadedec=0x7fffffffl/(fadeendsample-fadestartsample)+1;
        
    ENTER_TIMER(total);
    
    while ( 1 )
    {
        ci->yield();
        if (ci->stop_codec || ci->new_track) {
            break;
        }

        if (ci->seek_time) {
            int curtime = sampleswritten*1000LL/SAMPLE_RATE;
            DEBUGF("seek to %ld\ncurrently at %d\n",ci->seek_time,curtime);
            if (ci->seek_time < curtime) {
                DEBUGF("seek backwards = reset\n");
                ci->seek_complete();
                return 1;
            }
            ci->seek_complete();
        }
        
        ENTER_TIMER(render);
        /* fill samples buffer */
        if ( SPC_play(&spc_emu,WAV_CHUNK_SIZE*2,samples) )
            assert( false );
        EXIT_TIMER(render);
        
        sampleswritten+=WAV_CHUNK_SIZE;

        /* is track timed? */
        if (ci->global_settings->repeat_mode!=REPEAT_ONE && ci->id3->length) {
            unsigned long curtime = sampleswritten*1000LL/SAMPLE_RATE;
            unsigned long lasttimesample = (sampleswritten-WAV_CHUNK_SIZE);

            /* fade? */
            if (curtime>ID666.length)
            {
#ifdef CPU_COLDFIRE
                /* Have to switch modes to do this */
                long macsr = coldfire_get_macsr();
                coldfire_set_macsr(EMAC_SATURATE | EMAC_FRACTIONAL | EMAC_ROUND);
#endif
                int i;
                for (i=0;i<WAV_CHUNK_SIZE;i++) {
                    if (lasttimesample+i>fadestartsample) {
                        if (fadevol>0) {
                            samples[i] = FRACMUL(samples[i], fadevol);
                            samples[i+WAV_CHUNK_SIZE] = FRACMUL(samples[i+WAV_CHUNK_SIZE], fadevol);
                        } else samples[i]=samples[i+WAV_CHUNK_SIZE]=0;
                        fadevol-=fadedec;
                    }
                }
#ifdef CPU_COLDFIRE
               coldfire_set_macsr(macsr);
#endif
            }
            /* end? */
            if (lasttimesample>=fadeendsample)
                break;
        }

        ci->pcmbuf_insert(samples, samples+WAV_CHUNK_SIZE, WAV_CHUNK_SIZE);

        if (ci->global_settings->repeat_mode!=REPEAT_ONE)
        ci->set_elapsed(sampleswritten*1000LL/SAMPLE_RATE);
        else
            ci->set_elapsed(0);
    }
    
    EXIT_TIMER(total);
    
    return 0;
}

/* this is the codec entry point */
enum codec_status codec_main(void)
{
#ifdef CPU_COLDFIRE
    /* signed integer mode with saturation */
    coldfire_set_macsr(EMAC_SATURATE);
#endif
    CPU_Init(&spc_emu);

    do
    {
        DEBUGF("SPC: next_track\n");
        if (codec_init()) {
            return CODEC_ERROR;
        }
        DEBUGF("SPC: after init\n");

        ci->configure(DSP_SET_SAMPLE_DEPTH, 24);
        ci->configure(DSP_SET_FREQUENCY, SAMPLE_RATE);
        ci->configure(DSP_SET_STEREO_MODE, STEREO_NONINTERLEAVED);

        /* wait for track info to load */
        while (!*ci->taginfo_ready && !ci->stop_codec)
            ci->sleep(1);

        codec_set_replaygain(ci->id3);

        /* Read the entire file */
        DEBUGF("SPC: request initial buffer\n");
        ci->configure(CODEC_SET_FILEBUF_WATERMARK, ci->filesize);

        ci->seek_buffer(0);
        size_t buffersize;
        uint8_t* buffer = ci->request_buffer(&buffersize, ci->filesize);
        if (!buffer) {
            return CODEC_ERROR;
        }

        DEBUGF("SPC: read size = 0x%lx\n",(unsigned long)buffersize);
        do
        {
            SPC_Init(&spc_emu);
            if (SPC_load_spc(&spc_emu,buffer,buffersize)) {
                DEBUGF("SPC load failure\n");
                return CODEC_ERROR;
            }

            LoadID666(buffer+0x2e);
            
            if (ci->global_settings->repeat_mode!=REPEAT_ONE && ID666.length==0) {
                ID666.length=3*60*1000; /* 3 minutes */
                ID666.fade=5*1000; /* 5 seconds */
            }
            ci->id3->length = ID666.length+ID666.fade;
            ci->id3->title = ID666.song;
            ci->id3->album = ID666.game;
            ci->id3->artist = ID666.artist;
            ci->id3->year = ID666.year;
            ci->id3->comment = ID666.comments;

            reset_profile_timers();
        }
        
        while ( play_track() );

        print_timers(ci->id3->path);
    }
    while ( ci->request_next_track() );
    
    return CODEC_OK;
}
