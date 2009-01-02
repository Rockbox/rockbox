/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: iap.c 17400 2008-05-07 14:30:29Z xxxxxx $
 *
 * Copyright (C) 2002 by Alan Korr & Nick Robinson
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <memory.h>
#include <string.h>

#include "iap.h"
#include "button.h"
#include "config.h"
#include "cpu.h"
#include "system.h"
#include "kernel.h"
#include "serial.h"

#include "playlist.h"
#include "playback.h"
#include "audio.h"
#include "settings.h"
#include "metadata.h"
#include "gwps.h"

#include "button.h"
#include "action.h"

#define RX_BUFLEN 260
#define TX_BUFLEN 128

static volatile int iap_pollspeed = 0;
static volatile bool iap_remotetick = true;
static bool iap_setupflag = false, iap_updateflag = false;
static int iap_changedctr = 0;

static unsigned long iap_remotebtn = 0;
static int iap_repeatbtn = 0;
static bool iap_btnrepeat = false, iap_btnshuffle = false;

unsigned char serbuf[RX_BUFLEN];
static int serbuf_i = 0;

static unsigned char response[TX_BUFLEN];
static int responselen;

static void iap_task(void)
{
    static int count = 0;

    count += iap_pollspeed;
    if (count < (500/10)) return;

    /* exec every 500ms if pollspeed == 1 */
    count = 0;
    queue_post(&button_queue, SYS_IAP_PERIODIC, 0);
}

void iap_setup(int ratenum)
{
    iap_bitrate_set(ratenum);
    iap_pollspeed = 0;
    iap_remotetick = true;
    iap_updateflag = false;
    iap_changedctr = 0;
    iap_setupflag = true;
    iap_remotebtn = BUTTON_NONE;
    tick_add_task(iap_task);
}

void iap_bitrate_set(int ratenum)
{
    switch(ratenum)
    {
        case 0:
            serial_bitrate(0);
            break;
        case 1:
            serial_bitrate(9600);
            break;
        case 2:
            serial_bitrate(19200);
            break;
        case 3:
            serial_bitrate(38400);
            break;
        case 4:
            serial_bitrate(57600);
            break;
    }
}

/* Message format:
   0xff
   0x55
   length
   mode
   command (2 bytes)
   parameters (0-n bytes)
   checksum (length+mode+parameters+checksum == 0)
*/

static void iap_send_pkt(const unsigned char * data, int len)
{
    int i, chksum;
    
    if(len > TX_BUFLEN-4) len = TX_BUFLEN-4;
    responselen = len + 4;
    
    response[0] = 0xFF;
    response[1] = 0x55;
    
    chksum = response[2] = len;
    for(i = 0; i < len; i ++)
    {
        chksum += data[i];
        response[i+3] = data[i];
    }

    response[i+3] = 0x100 - (chksum & 0xFF);
    
    for(i = 0; i < responselen; i ++)
    {
        while (!tx_rdy()) ;
        tx_writec(response[i]);
    }
}    

int iap_getc(unsigned char x)
{
    static unsigned char last_x = 0;
    static bool newpkt = true;
    static unsigned char chksum = 0;
            
    /* Restart if the sync word is seen */
    if(x == 0x55 && last_x == 0xff/* && newpkt*/)
    {
        serbuf[0] = 0;
        serbuf_i = 0;
        chksum = 0;
        newpkt = false;
    }
    else
    {
        if(serbuf_i >= RX_BUFLEN)
            serbuf_i = 0;

        serbuf[serbuf_i++] = x;
        chksum += x;
    }
    last_x = x;

    /* Broadcast to queue if we have a complete message */
    if(serbuf_i && (serbuf_i == serbuf[0]+2))
    {
        serbuf_i = 0;
        newpkt = true;
        if(chksum == 0)
            queue_post(&button_queue, SYS_IAP_HANDLEPKT, 0);
    }
    return newpkt;
}

void iap_track_changed(void)
{
    iap_changedctr = 1;
}

void iap_periodic(void)
{
    if(!iap_setupflag) return;
    if(!iap_pollspeed) return;
    
    unsigned char data[] = {0x04, 0x00, 0x27, 0x04, 0x00, 0x00, 0x00, 0x00};
    unsigned long time_elapsed = audio_current_track()->elapsed;

    time_elapsed += wps_state.ff_rewind_count;
    
    data[3] = 0x04; // playing

    /* If info has changed, don't flag it right away */
    if(iap_changedctr && iap_changedctr++ >= iap_pollspeed * 2)
    {        
        /* track info has changed */
        iap_changedctr = 0;
        data[3] = 0x01; // 0x02 has same effect? 
        iap_updateflag = true;
    }

    data[4] = time_elapsed >> 24;
    data[5] = time_elapsed >> 16;
    data[6] = time_elapsed >> 8;
    data[7] = time_elapsed;
    iap_send_pkt(data, sizeof(data));
}

void iap_handlepkt(void)
{
    
    if(!iap_setupflag) return;
    if(serbuf[0] == 0) return;
    
    /* if we are waiting for a remote button to go out,
       delay the handling of the new packet */
    if(!iap_remotetick)
    {
        queue_post(&button_queue, SYS_IAP_HANDLEPKT, 0);
        return;
    }
    
    /* Handle Mode 0 */
    if (serbuf[1] == 0x00)
    {
        switch (serbuf[2])
        {
            /* get model info */
            case 0x0D:
            {
                unsigned char data[] = {0x00, 0x0E, 0x00, 0x0B, 0x00, 0x10,
                                       'R', 'O', 'C', 'K', 'B', 'O', 'X', 0x00};
                iap_send_pkt(data, sizeof(data));
                break;
            }
            /* No idea ??? */
            case 0x0F:
            {
                unsigned char data[] = {0x00, 0x10, 0x00, 0x01, 0x05};
                iap_send_pkt(data, sizeof(data));
                break;
            }
            /* FM transmitter sends this: FF 55 06 00 01 05 00 02 01 F1 (mode switch) */
            case 0x01:
            {
                if(serbuf[3] == 0x05) 
                {
                    sleep(HZ/3);
                    unsigned char data[] = {0x05, 0x02};
                    iap_send_pkt(data, sizeof(data));
                }
                break;
            }
            /* FM transmitter sends this: FF 55 0E 00 13 00 00 00 35 00 00 00 04 00 00 00 00 A6 (???)*/
            case 0x13:
            {
                unsigned char data[] = {0x00, 0x02, 0x00, 0x13};
                iap_send_pkt(data, sizeof(data));
                unsigned char data2[] = {0x00, 0x27, 0x00};
                iap_send_pkt(data2, sizeof(data2));
                unsigned char data3[] = {0x05, 0x02};
                iap_send_pkt(data3, sizeof(data3));
                break;
            }
            /* FM transmitter sends this: FF 55 02 00 05 F9 (mode switch: AiR mode) */
            case 0x05:
            {
                unsigned char data[] = {0x00, 0x02, 0x06, 0x05, 0x00, 0x00, 0x0B, 0xB8, 0x28};
                iap_send_pkt(data, sizeof(data));
                unsigned char data2[] = {0x00, 0x02, 0x00, 0x05};
                iap_send_pkt(data2, sizeof(data2));
                break;
            }            
            /* default response is with cmd ok packet */
            default:
            {
                unsigned char data[] = {0x00, 0x02, 0x00, 0x00};
                data[3] = serbuf[2]; //respond with cmd
                iap_send_pkt(data, sizeof(data));
                break;
            }
        }
    }
    /* Handle Mode 2 */
    else if (serbuf[1] == 0x02)
    {
        if(serbuf[2] != 0) return;
        iap_remotebtn = BUTTON_NONE;
        iap_remotetick = false;
        
        if(serbuf[0] >= 3 && serbuf[3] != 0)
        {
            if(serbuf[3] & 1)
                iap_remotebtn |= BUTTON_RC_PLAY;
            if(serbuf[3] & 2)
                iap_remotebtn |= BUTTON_RC_VOL_UP;
            if(serbuf[3] & 4)
                iap_remotebtn |= BUTTON_RC_VOL_DOWN;
            if(serbuf[3] & 8)
                iap_remotebtn |= BUTTON_RC_RIGHT;
            if(serbuf[3] & 16)
                iap_remotebtn |= BUTTON_RC_LEFT;
        }
        else if(serbuf[0] >= 4 && serbuf[4] != 0)
        {
            if(serbuf[4] & 1) /* play */
            {
                if (audio_status() != AUDIO_STATUS_PLAY)
                {
                    iap_remotebtn |= BUTTON_RC_PLAY;
                    iap_repeatbtn = 2;
                    iap_remotetick = false;
                    iap_changedctr = 1;
                }
            }
            if(serbuf[4] & 2) /* pause */
            {
                if (audio_status() == AUDIO_STATUS_PLAY)
                {
                    iap_remotebtn |= BUTTON_RC_PLAY;
                    iap_repeatbtn = 2;
                    iap_remotetick = false;
                    iap_changedctr = 1;
                }
            }
            if((serbuf[4] & 128) && !iap_btnshuffle) /* shuffle */
            {
                iap_btnshuffle = true;
                if(!global_settings.playlist_shuffle)
                {
                    global_settings.playlist_shuffle = 1;
                    settings_save();
                    settings_apply(false);
                    if (audio_status() & AUDIO_STATUS_PLAY)
                        playlist_randomise(NULL, current_tick, true);
                }
                else if(global_settings.playlist_shuffle)
                {
                    global_settings.playlist_shuffle = 0;
                    settings_save();
                    settings_apply(false);
                    if (audio_status() & AUDIO_STATUS_PLAY)
                        playlist_sort(NULL, true);
                }
            }
            else
                iap_btnshuffle = false;
        }
        else if(serbuf[0] >= 5 && serbuf[5] != 0)
        {
            if((serbuf[5] & 1) && !iap_btnrepeat) /* repeat */
            {
                int oldmode = global_settings.repeat_mode;
                iap_btnrepeat = true;
            
                if (oldmode == REPEAT_ONE)
                        global_settings.repeat_mode = REPEAT_OFF;
                else if (oldmode == REPEAT_ALL)
                        global_settings.repeat_mode = REPEAT_ONE;
                else if (oldmode == REPEAT_OFF)
                        global_settings.repeat_mode = REPEAT_ALL;

                settings_save();
                settings_apply(false);
                if (audio_status() & AUDIO_STATUS_PLAY)
                audio_flush_and_reload_tracks();
            }
            else
                iap_btnrepeat = false;

            if(serbuf[5] & 16) /* ffwd */
            {
                iap_remotebtn |= BUTTON_RC_RIGHT;
            }
            if(serbuf[5] & 32) /* frwd */
            {
                iap_remotebtn |= BUTTON_RC_LEFT;
            }
        }
    }
    /* Handle Mode 3 */
    else if (serbuf[1] == 0x03)
    {
        switch(serbuf[2])
        {
            /* some kind of status packet? */
            case 0x01:
            {
                unsigned char data[] = {0x03, 0x02, 0x00, 0x00, 0x00, 0x00};
                iap_send_pkt(data, sizeof(data));
                break;
            }
        }
    }
    /* Handle Mode 4 */
    else if (serbuf[1] == 0x04)
    {
        switch (((unsigned long)serbuf[2] << 8) | serbuf[3])
        {
            /* Get data updated??? flag */
            case 0x0009:
            {
                unsigned char data[] = {0x04, 0x00, 0x0A, 0x00};
                data[3] = iap_updateflag ? 0 : 1;
                iap_send_pkt(data, sizeof(data));
                break;
            }
            /* Set data updated??? flag */
            case 0x000B:
            {
                iap_updateflag = serbuf[4] ? 0 : 1;
                /* respond with cmd ok packet */
                unsigned char data[] = {0x04, 0x00, 0x01, 0x00, 0x00, 0x0B};
                iap_send_pkt(data, sizeof(data));
                break;
            }
            /* Get iPod size? */
            case 0x0012:
            {
                unsigned char data[] = {0x04, 0x00, 0x13, 0x01, 0x0B};
                iap_send_pkt(data, sizeof(data));
                break;
            }
            /* Get count of given types */
            case 0x0018:
            {
                unsigned char data[] = {0x04, 0x00, 0x19, 0x00, 0x00, 0x00, 0x00};
                unsigned long num = 0;
                switch(serbuf[4]) /* type number */
                {
                    case 0x01: /* total number of playlists */
                        num = 1;
                        break;
                    case 0x05: /* total number of songs */
                        num = 1;
                }
                data[3] = num >> 24;
                data[4] = num >> 16;
                data[5] = num >> 8;
                data[6] = num;
                iap_send_pkt(data, sizeof(data));
                break;
            }
            /* Get time and status */
            case 0x001C:
            {
                unsigned char data[] = {0x04, 0x00, 0x1D, 0x00, 0x00, 0x00, 
                                        0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
                struct mp3entry *id3 = audio_current_track();
                unsigned long time_total = id3->length;
                unsigned long time_elapsed = id3->elapsed;
                int status = audio_status();
                data[3] = time_total >> 24;
                data[4] = time_total >> 16;
                data[5] = time_total >> 8;
                data[6] = time_total;
                data[7] = time_elapsed >> 24;
                data[8] = time_elapsed >> 16;
                data[9] = time_elapsed >> 8;
                data[10] = time_elapsed;
                if (status == AUDIO_STATUS_PLAY)
                    data[11] = 0x01; /* play */
                else if (status & AUDIO_STATUS_PAUSE)
                    data[11] = 0x02; /* pause */ 
                iap_send_pkt(data, sizeof(data));
                break;
            }
            /* Get current pos in playlist */
            case 0x001E:
            {
                unsigned char data[] = {0x04, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x00};
                long playlist_pos = playlist_next(0);
                playlist_pos -= playlist_get_first_index(NULL);
                if(playlist_pos < 0)
                    playlist_pos += playlist_amount();
                data[3] = playlist_pos >> 24;
                data[4] = playlist_pos >> 16;
                data[5] = playlist_pos >> 8;
                data[6] = playlist_pos;
                iap_send_pkt(data, sizeof(data));
                break;
            }
            /* Get title of a song number */
            case 0x0020:
            /* Get artist of a song number */
            case 0x0022:
            /* Get album of a song number */
            case 0x0024:
            {
                unsigned char data[70] = {0x04, 0x00, 0xFF};
                struct mp3entry id3;
                int fd;
                long tracknum = (signed long)serbuf[4] << 24 |
                                (signed long)serbuf[5] << 16 |
                                (signed long)serbuf[6] << 8 | serbuf[7];
                data[2] = serbuf[3] + 1;
                memcpy(&id3, audio_current_track(), sizeof(id3));
                tracknum += playlist_get_first_index(NULL);
                if(tracknum >= playlist_amount())
                    tracknum -= playlist_amount();

                /* If the tracknumber is not the current one,
                   read id3 from disk */
                if(playlist_next(0) != tracknum)
                {
                    struct playlist_track_info info;
                    playlist_get_track_info(NULL, tracknum, &info);
                    fd = open(info.filename, O_RDONLY);
                    memset(&id3, 0, sizeof(struct mp3entry));
                    get_metadata(&id3, fd, info.filename);
                    close(fd);
                }

                /* Return the requested track data */
                switch(serbuf[3])
                {
                    case 0x20:
                        strncpy((char *)&data[3], id3.title, 64);
                            iap_send_pkt(data, 4+strlen(id3.title));
                        break;
                    case 0x22:
                        strncpy((char *)&data[3], id3.artist, 64);
                            iap_send_pkt(data, 4+strlen(id3.artist));
                        break;
                    case 0x24:
                        strncpy((char *)&data[3], id3.album, 64);
                            iap_send_pkt(data, 4+strlen(id3.album));
                        break;
                }
                break;
            }
            /* Set polling mode */
            case 0x0026:
            {
                iap_pollspeed = serbuf[4] ? 1 : 0;
                /*responsed with cmd ok packet */
                unsigned char data[] = {0x04, 0x00, 0x01, 0x00, 0x00, 0x26};
                iap_send_pkt(data, sizeof(data));
                break;
            }
            /* AiR playback control */
            case 0x0029:
            {
                /* respond with cmd ok packet */
                unsigned char data[] = {0x04, 0x00, 0x01, 0x00, 0x00, 0x29};
                iap_send_pkt(data, sizeof(data));
                switch(serbuf[4])
                {
                    case 0x01: /* play/pause */
                        iap_remotebtn = BUTTON_RC_PLAY;
                        iap_repeatbtn = 2;
                        iap_remotetick = false;
                        iap_changedctr = 1;
                        break;
                    case 0x02: /* stop */
                        iap_remotebtn = BUTTON_RC_PLAY|BUTTON_REPEAT;
                        iap_repeatbtn = 2;
                        iap_remotetick = false;
                        iap_changedctr = 1;
                        break;
                    case 0x03: /* skip++ */
                        iap_remotebtn = BUTTON_RC_RIGHT;
                        iap_repeatbtn = 2;
                        iap_remotetick = false;
                        break;
                    case 0x04: /* skip-- */
                        iap_remotebtn = BUTTON_RC_LEFT;
                        iap_repeatbtn = 2;
                        iap_remotetick = false;
                        break;
                    case 0x05: /* ffwd */
                        iap_remotebtn = BUTTON_RC_RIGHT;
                        iap_remotetick = false;
                        if(iap_pollspeed) iap_pollspeed = 5;
                        break;
                    case 0x06: /* frwd */
                        iap_remotebtn = BUTTON_RC_LEFT;
                        iap_remotetick = false;
                        if(iap_pollspeed) iap_pollspeed = 5;
                        break;
                    case 0x07: /* end ffwd/frwd */
                        iap_remotebtn = BUTTON_NONE;
                        iap_remotetick = false;
                        if(iap_pollspeed) iap_pollspeed = 1;
                        break;
                }
                break;
            }
            /* Get shuffle mode */
            case 0x002C:
            {
                unsigned char data[] = {0x04, 0x00, 0x2D, 0x00};
                data[3] = global_settings.playlist_shuffle ? 1 : 0;
                iap_send_pkt(data, sizeof(data));
                break;
            }
            /* Set shuffle mode */
            case 0x002E:
            {
                if(serbuf[4] && !global_settings.playlist_shuffle)
                {
                    global_settings.playlist_shuffle = 1;
                    settings_save();
                    settings_apply(false);
                    if (audio_status() & AUDIO_STATUS_PLAY)
                        playlist_randomise(NULL, current_tick, true);
                }
                else if(!serbuf[4] && global_settings.playlist_shuffle)
                {
                    global_settings.playlist_shuffle = 0;
                    settings_save();
                    settings_apply(false);
                    if (audio_status() & AUDIO_STATUS_PLAY)
                        playlist_sort(NULL, true);
                }

                
                /* respond with cmd ok packet */
                unsigned char data[] = {0x04, 0x00, 0x01, 0x00, 0x00, 0x2E};
                iap_send_pkt(data, sizeof(data));
                break;
            }
            /* Get repeat mode */
            case 0x002F:
            {
                unsigned char data[] = {0x04, 0x00, 0x30, 0x00};
                if(global_settings.repeat_mode == REPEAT_OFF)
                    data[3] = 0;
                else if(global_settings.repeat_mode == REPEAT_ONE)
                    data[3] = 1;
                else
                    data[3] = 2;
                iap_send_pkt(data, sizeof(data));
                break;
            }
            /* Set repeat mode */
            case 0x0031:
            {
                int oldmode = global_settings.repeat_mode;
                if (serbuf[4] == 0)
                    global_settings.repeat_mode = REPEAT_OFF;
                else if (serbuf[4] == 1)
                    global_settings.repeat_mode = REPEAT_ONE;
                else if (serbuf[4] == 2)
                    global_settings.repeat_mode = REPEAT_ALL;

                if (oldmode != global_settings.repeat_mode)
                {
                    settings_save();
                    settings_apply(false);
                    if (audio_status() & AUDIO_STATUS_PLAY)
                        audio_flush_and_reload_tracks();
                }

                /* respond with cmd ok packet */
                unsigned char data[] = {0x04, 0x00, 0x01, 0x00, 0x00, 0x31};
                iap_send_pkt(data, sizeof(data));
                break;
            }        
            /* Get Max Screen Size for Picture Upload??? */
            case 0x0033:
            {
                unsigned char data[] = {0x04, 0x00, 0x34, 0x01, 0x36, 0x00, 0xA8, 0x01};
                iap_send_pkt(data, sizeof(data));
                break;
            }
            /* Get number songs in current playlist */
            case 0x0035:
            {
                unsigned char data[] = {0x04, 0x00, 0x36, 0x00, 0x00, 0x00, 0x00};
                unsigned long playlist_amt = playlist_amount();
                data[3] = playlist_amt >> 24;
                data[4] = playlist_amt >> 16;
                data[5] = playlist_amt >> 8;
                data[6] = playlist_amt;
                iap_send_pkt(data, sizeof(data));
                break;
            }                
            /* Jump to track number in current playlist */
            case 0x0037:
            {
                long tracknum = (signed long)serbuf[4] << 24 |
                                (signed long)serbuf[5] << 16 |
                                (signed long)serbuf[6] << 8 | serbuf[7];
                if (!wps_state.paused)
                    audio_pause();
                audio_skip(tracknum - playlist_next(0));
                if (!wps_state.paused)
                    audio_resume();
                
                /* respond with cmd ok packet */
                unsigned char data[] = {0x04, 0x00, 0x01, 0x00, 0x00, 0x00};
                data[4] = serbuf[2];
                data[5] = serbuf[3];
                iap_send_pkt(data, sizeof(data));
                break;
            }
            default:
            {
                /* default response is with cmd ok packet */
                unsigned char data[] = {0x04, 0x00, 0x01, 0x00, 0x00, 0x00};
                data[4] = serbuf[2];
                data[5] = serbuf[3];
                iap_send_pkt(data, sizeof(data));
                break;
            }
        }
    }
    serbuf[0] = 0;
}

int remote_control_rx(void)
{
    int btn = iap_remotebtn;
    if(iap_repeatbtn)
    {
        iap_repeatbtn--;
        if(!iap_repeatbtn)
        {
            iap_remotebtn = BUTTON_NONE;
            iap_remotetick = true;
        }
    }
    else
        iap_remotetick = true;

    return btn;
}
