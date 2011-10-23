/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
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

#include "iap-core.h"
#include "iap-lingo4.h"
#include "dir.h"
#include "settings.h"
#include "filetree.h"
#include "wps.h"
#include "playback.h"

/*
 * This macro is meant to be used inside an IAP mode message handler.
 * It is passed the expected minimum length of the message buffer.
 * If the buffer does not have the required lenght an ACK
 * packet with a Bad Parameter error is generated.
 */
#define CHECKLEN(x) do { \
        if (len < (x)) { \
            cmd_ack(cmd, IAP_ACK_BAD_PARAM); \
            return; \
        }} while(0)

/* Check for authenticated state, and return an ACK Not
 * Authenticated on failure.
 */
#define CHECKAUTH do { \
        if (!DEVICE_AUTHENTICATED) { \
            cmd_ack(cmd, IAP_ACK_NO_AUTHEN); \
            return; \
        }} while(0)

static char cur_dbrecord[5] = {0};

static void cmd_ack(const unsigned int cmd, const unsigned char status)
{
    IAP_TX_INIT4(0x04, 0x0001);
    IAP_TX_PUT(status);
    IAP_TX_PUT_U16(cmd);

    iap_send_tx();
}

#define cmd_ok(cmd) cmd_ack((cmd), IAP_ACK_OK)

static void get_playlist_name(unsigned char *dest,
                              unsigned long item_offset,
                              size_t max_length)
{
    if (item_offset == 0) return;
    DIR* dp;
    struct dirent* playlist_file = NULL;

    dp = opendir(global_settings.playlist_catalog_dir);

    char *extension;
    unsigned long nbr = 0;
    while ((nbr < item_offset) && ((playlist_file = readdir(dp)) != NULL))
    {
        /*Increment only if there is a playlist extension*/
        if ((extension=strrchr(playlist_file->d_name, '.')) != NULL){
            if ((strcmp(extension, ".m3u") == 0 || 
                strcmp(extension, ".m3u8") == 0))
                nbr++;
        }
    }
    if (playlist_file != NULL) {
        strlcpy(dest, playlist_file->d_name, max_length);
    }
    closedir(dp);
}

void iap_handlepkt_mode4(const unsigned int len, const unsigned char *buf)
{
    unsigned int cmd = (buf[1] << 8) | buf[2];

    /* Lingo 0x04 commands are at least 3 bytes in length */
    CHECKLEN(3);

    /* Lingo 0x04 must have been negotiated */
    if (!DEVICE_LINGO_SUPPORTED(0x04)) {
        cmd_ack(cmd, IAP_ACK_BAD_PARAM);
        return;
    }

    /* All these commands require extended interface mode */
    if (interface_state != IST_EXTENDED) {
        cmd_ack(cmd, IAP_ACK_BAD_PARAM);
        return;
    }

    switch (cmd)
    {
        /* GetAudioBookSpeed */
        case 0x0009:
        {
            /* ReturnAudioBookSpeed */
            unsigned char data[] = {0x04, 0x00, 0x0A, 0x00};
            iap_send_pkt(data, sizeof(data));
            break;
        }
        
        /* SetAudioBookSpeed */
        case 0x000B:
        {
            /* respond with cmd ok packet */
            cmd_ok(cmd);
            break;
        }
        
        /* RequestProtocolVersion (0x0012)
         *
         * This command is deprecated.
         *
         * Return the Extended Interface Protocol version
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: Extended Interface Protocol Lingo, always 0x04
         * 0x01-0x02: Command, always 0x0012
         *
         * Returns:
         * ReturnProtocolVersion
         *
         * Packet format (offset in data[]: Description)
         * 0x00: Lingo ID: Extended Interface Protocol Lingo, always 0x04
         * 0x01-0x02: Command, always 0x0013
         * 0x03: Major protocol version
         * 0x04: Minor protocol version
         */
        case 0x0012:
        {
            IAP_TX_INIT4(0x04, 0x0013);
            IAP_TX_PUT(LINGO_MAJOR(0x04));
            IAP_TX_PUT(LINGO_MINOR(0x04));

            iap_send_tx();
            break;
        }

        /* ReturnProtocolVersion (0x0013)
         *
         * This command is deprecated.
         *
         * Sent from the iPod to the device
         */

        /* RequestiPodName (0x0014)
         *
         * This command is deprecated.
         *
         * Retrieves the name of the iPod
         *
         * Packet format (offset in buf[]: Description)
         * 0x00: Lingo ID: Extended Interface Protocol Lingo, always 0x04
         * 0x01-0x02: Command, always 0x0014
         *
         * Returns:
         * ReturniPodName
         *
         * Packet format (offset in data[]: Description)
         * 0x00: Lingo ID: Extended Interface Protocol Lingo, always 0x04
         * 0x01-0x02: Command, always 0x0015
         * 0x03-0xNN: iPod name as NULL-terminated UTF8 string
         */
        case 0x0014:
        {
            IAP_TX_INIT4(0x04, 0x0015);
            IAP_TX_PUT_STRING("ROCKBOX");

            iap_send_tx();
            break;
        }

        /* ReturniPodName (0x0015)
         *
         * This command is deprecated.
         *
         * Sent from the iPod to the device
         */

        /* SelectDBRecord */
        case 0x0017:
        {
            memcpy(cur_dbrecord, buf + 3, 5);
            cmd_ok(cmd);
            break;
        }

        /* GetNumberCategorizedDBRecords */
        case 0x0018:
        {
            /* ReturnNumberCategorizedDBRecords */
            unsigned char data[] = {0x04, 0x00, 0x19, 0x00, 0x00, 0x00, 0x00};
            unsigned long num = 0;
            
            DIR* dp;
            unsigned long nbr_total_playlists = 0;
            struct dirent* playlist_file = NULL;
            char *extension;

            switch(buf[3]) /* type number */
            {
                case 0x01: /* total number of playlists */
                    dp = opendir(global_settings.playlist_catalog_dir);
                    while ((playlist_file = readdir(dp)) != NULL)
                    {
                    /*Increment only if there is a playlist extension*/
                        if ((extension=strrchr(playlist_file->d_name, '.'))
                        != NULL) {
                            if ((strcmp(extension, ".m3u") == 0 || 
                                strcmp(extension, ".m3u8") == 0))
                                nbr_total_playlists++;
                        }
                    }
                    closedir(dp);
                    /*Add 1 for the main playlist*/
                    num = nbr_total_playlists + 1;
                    break;
                case 0x05: /* total number of songs */
                    num = 1;
                    break;
            }
            put_u32(&data[3], num);
            iap_send_pkt(data, sizeof(data));
            break;
        }
        
        /* RetrieveCategorizedDatabaseRecords */
        case 0x001A:
        {
            /* ReturnCategorizedDatabaseRecord */
            unsigned char data[7 + MAX_PATH] = 
                            {0x04, 0x00, 0x1B, 0x00, 0x00, 0x00, 0x00,
                             'R', 'O', 'C', 'K', 'B', 'O', 'X', '\0'};
            
            unsigned long item_offset = get_u32(&buf[4]);

            get_playlist_name(data + 7, item_offset, MAX_PATH);
            /*Remove file extension*/
            char *dot=NULL;
            dot = (strrchr(data+7, '.'));
            if (dot != NULL)
                *dot = '\0';
            iap_send_pkt(data, 7 + strlen(data+7) + 1);
            break;
        }
        
        /* GetPlayStatus */
        case 0x001C:
        {
            /* ReturnPlayStatus */
            unsigned char data[] = {0x04, 0x00, 0x1D, 0x00, 0x00, 0x00, 
                                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
            struct mp3entry *id3 = audio_current_track();
            unsigned long time_total = id3->length;
            unsigned long time_elapsed = id3->elapsed;
            int status = audio_status();
            put_u32(&data[3], time_total);
            put_u32(&data[7], time_elapsed);
            if (status == AUDIO_STATUS_PLAY)
                data[11] = 0x01; /* play */
            else if (status & AUDIO_STATUS_PAUSE)
                data[11] = 0x02; /* pause */ 
            iap_send_pkt(data, sizeof(data));
            break;
        }
        
        /* GetCurrentPlayingTrackIndex */
        case 0x001E:
        {
            /* ReturnCurrentPlayingTrackIndex */
            unsigned char data[] = {0x04, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x00};
            long playlist_pos = playlist_next(0);
            playlist_pos -= playlist_get_first_index(NULL);
            if(playlist_pos < 0)
                playlist_pos += playlist_amount();
            put_u32(&data[3], playlist_pos);
            iap_send_pkt(data, sizeof(data));
            break;
        }
        
        /* GetIndexedPlayingTrackTitle */
        case 0x0020:
        /* GetIndexedPlayingTrackArtistName */
        case 0x0022:
        /* GetIndexedPlayingTrackAlbumName */
        case 0x0024:
        {
            unsigned char data[70] = {0x04, 0x00, 0xFF};
            struct mp3entry id3;
            int fd;
            size_t len;
            long tracknum = get_u32(&buf[3]);
            
            data[2] = cmd + 1;
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
            switch(cmd)
            {
                case 0x20:
                    len = strlcpy((char *)&data[3], id3.title, 64);
                    iap_send_pkt(data, 4+len);
                    break;
                case 0x22:
                    len = strlcpy((char *)&data[3], id3.artist, 64);
                    iap_send_pkt(data, 4+len);
                    break;
                case 0x24:
                    len = strlcpy((char *)&data[3], id3.album, 64);
                    iap_send_pkt(data, 4+len);
                    break;
            }
            break;
        }
        
        /* SetPlayStatusChangeNotification */
        case 0x0026:
        {
            device.do_notify = buf[3] ? true : false;
            /* respond with cmd ok packet */
            cmd_ok(cmd);
            break;
        }
        
        /* PlayCurrentSelection */
        case 0x0028:
        {
            switch (cur_dbrecord[0])
            {
                case 0x01:
                {/*Playlist*/
                    unsigned long item_offset = get_u32(&cur_dbrecord[1]);
                    
                    unsigned char selected_playlist
                    [sizeof(global_settings.playlist_catalog_dir)
                    + 1
                    + MAX_PATH] = {0};

                    strcpy(selected_playlist,
                            global_settings.playlist_catalog_dir);
                    int len = strlen(selected_playlist);
                    selected_playlist[len] = '/';
                    get_playlist_name (selected_playlist + len + 1,
                                       item_offset,
                                       MAX_PATH);
                    ft_play_playlist(selected_playlist,
                                     global_settings.playlist_catalog_dir,
                                     strrchr(selected_playlist, '/') + 1);
                    break;
                }
            }
            cmd_ok(cmd);
            break;
        }
        
        /* PlayControl */
        case 0x0029:
        {
            switch(buf[3])
            {
                case 0x01: /* play/pause */
                    iap_remotebtn = BUTTON_RC_PLAY;
                    iap_repeatbtn = 2;
                    break;
                case 0x02: /* stop */
                    iap_remotebtn = BUTTON_RC_PLAY|BUTTON_REPEAT;
                    iap_repeatbtn = 2;
                    break;
                case 0x03: /* skip++ */
                    iap_remotebtn = BUTTON_RC_RIGHT;
                    iap_repeatbtn = 2;
                    break;
                case 0x04: /* skip-- */
                    iap_remotebtn = BUTTON_RC_LEFT;
                    iap_repeatbtn = 2;
                    break;
                case 0x05: /* ffwd */
                    iap_remotebtn = BUTTON_RC_RIGHT;
                    break;
                case 0x06: /* frwd */
                    iap_remotebtn = BUTTON_RC_LEFT;
                    break;
                case 0x07: /* end ffwd/frwd */
                    iap_remotebtn = BUTTON_NONE;
                    break;
            }
            /* respond with cmd ok packet */
            cmd_ok(cmd);
            break;
        }
        
        /* GetShuffle */
        case 0x002C:
        {
            /* ReturnShuffle */
            unsigned char data[] = {0x04, 0x00, 0x2D, 0x00};
            data[3] = global_settings.playlist_shuffle ? 1 : 0;
            iap_send_pkt(data, sizeof(data));
            break;
        }
        
        /* SetShuffle */
        case 0x002E:
        {
            if(buf[3] && !global_settings.playlist_shuffle)
            {
                global_settings.playlist_shuffle = 1;
                settings_save();
                if (audio_status() & AUDIO_STATUS_PLAY)
                    playlist_randomise(NULL, current_tick, true);
            }
            else if(!buf[3] && global_settings.playlist_shuffle)
            {
                global_settings.playlist_shuffle = 0;
                settings_save();
                if (audio_status() & AUDIO_STATUS_PLAY)
                    playlist_sort(NULL, true);
            }

            /* respond with cmd ok packet */
            cmd_ok(cmd);
            break;
        }
        
        /* GetRepeat */
        case 0x002F:
        {
            /* ReturnRepeat */
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
        
        /* SetRepeat */
        case 0x0031:
        {
            int oldmode = global_settings.repeat_mode;
            if (buf[3] == 0)
                global_settings.repeat_mode = REPEAT_OFF;
            else if (buf[3] == 1)
                global_settings.repeat_mode = REPEAT_ONE;
            else if (buf[3] == 2)
                global_settings.repeat_mode = REPEAT_ALL;

            if (oldmode != global_settings.repeat_mode)
            {
                settings_save();
                if (audio_status() & AUDIO_STATUS_PLAY)
                    audio_flush_and_reload_tracks();
            }

            /* respond with cmd ok packet */
            cmd_ok(cmd);
            break;
        }
        
        /* GetMonoDisplayImageLimits */
        case 0x0033:
        {
            /* ReturnMonoDisplayImageLimits */
            unsigned char data[] = {0x04, 0x00, 0x34,
                                    LCD_WIDTH >> 8, LCD_WIDTH & 0xff,
                                    LCD_HEIGHT >> 8, LCD_HEIGHT & 0xff,
                                    0x01};
            iap_send_pkt(data, sizeof(data));
            break;
        }
        
        /* GetNumPlayingTracks */
        case 0x0035:
        {
            /* ReturnNumPlayingTracks */
            unsigned char data[] = {0x04, 0x00, 0x36, 0x00, 0x00, 0x00, 0x00};
            unsigned long playlist_amt = playlist_amount();
            put_u32(&data[3], playlist_amt);
            iap_send_pkt(data, sizeof(data));
            break;
        }
        
        /* SetCurrentPlayingTrack */
        case 0x0037:
        {
            int paused = (is_wps_fading() || (audio_status() & AUDIO_STATUS_PAUSE));
            long tracknum = get_u32(&buf[3]);
            
            audio_pause();
            audio_skip(tracknum - playlist_next(0));
            if (!paused)
                audio_resume();
            
            /* respond with cmd ok packet */
            cmd_ok(cmd);
            break;
        }
        
        default:
        {
            /* The default response is IAP_ACK_BAD_PARAM */
            cmd_ack(cmd, IAP_ACK_BAD_PARAM);
            break;
        }
    }
}
