/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 Franklin Wei
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 */

/* Tonebox is a very-simplifed, MIDI-like format for representing music */
/* The extension for a Tonebox file is .tbx */
/* The format is as follows:
   Header: <f3> <76> -- a tribute to the Z80 :)
   Data:
   - a tone is represented like: <ff> <32-bit big-endian frequency> <32-bit big-endian duration in microseconds>
   - silence is represented as: <00> <32-bit big-endian duration> <32-bit garbage>
*/
#include "plugin.h"
enum plugin_status plugin_start(const void* param)
{
    if(!param)
    {
        rb->splash(HZ, "No .tbx file given!");
        return PLUGIN_ERROR;
    }
    else
    {
        char* filename=(char*)param;
        if(!rb->file_exists(filename))
        {
            rb->splash(HZ, "File does not exist!");
            return PLUGIN_ERROR;
        }
        int fd=rb->open(filename, O_RDONLY);
        unsigned char header[2];
        int ret=rb->read(fd, header, 2);
        if(header[0]!=0xf3 || header[1]!=0x76 || ret!=2)
        {
            rb->splashf(HZ, "Header missing/corrupted!");
            return PLUGIN_ERROR;
        }
        unsigned char buf[9];
        do {
            ret=rb->read(fd, buf, 9);
            if(ret!=9)
                break;
            switch(buf[0])
            {
            case 0xff: /* tone */
            {
                unsigned int freq=(buf[1]<<24)|(buf[2]<<16)|(buf[3]<<8)|buf[4];
                unsigned int dur= (buf[5]<<24)|(buf[6]<<16)|(buf[7]<<8)|buf[8];
                rb->splashf(0, "Playing %u Hz tone for %u microseconds", freq, dur);
                if(freq)
                    rb->piezo_play(dur, freq, true);
                else
                    rb->sleep(dur/10000.0);
                break;
            }
            case 0:
            {
                unsigned int dur=(buf[1]<<24)|(buf[2]<<16)|(buf[3]<<8)|buf[4];
                rb->splashf(0, "Sleeping %u microseconds", dur);
                rb->sleep(dur/10000.0);
                break;
            }
            default:
                rb->splash(HZ, "Unknown tone/silence byte");
                return PLUGIN_ERROR;
                break;
            }
        } while(ret==9);
        rb->close(fd);
    }
    return PLUGIN_OK;
}
