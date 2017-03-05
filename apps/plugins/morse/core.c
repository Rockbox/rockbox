/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2017 Franklin Wei
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ***************************************************************************/

#include "plugin.h"

#include "lib/pluginlib_actions.h"

#include "morse.h"

static struct morse_mapping { char ch; const char *code; } morse_map[256];

static void play_char(char c, int dot, const struct morse_output_api *api)
{
    rb->splashf(0, "%c", c);
    const char *seq = morse_map[tolower(c)].code;
    if(!seq)
        return;
    while(*seq)
    {
        char c = *seq++;
        switch(c)
        {
        case '.':
            api->beep(750, dot);
            //rb->beep_play(750, dot, 1500);
            break;
        case '-':
            api->beep(750, dot * 3);
            //rb->beep_play(750, dot * 3, 1500);
            break;
        }
        api->silence(dot * 3);
        //rb->sleep((dot * 3) / (1000 / HZ));
    }
}

/* returns false if interrupted by user action */
bool morse_play(const char *str, int dot, const struct morse_output_api *api)
{
    while(*str)
    {
        char c = *str++;
        switch(c)
        {
        case ' ':
            api->silence(dot * 7);
            //rb->sleep((dot * 7) / (1000 / HZ));
            break;
        default:
            play_char(c, dot, api);
            api->silence(dot * 3);
            //rb->sleep((dot * 3) / (1000 / HZ));
            break;
        }

        static const struct button_mapping *plugin_contexts[] = { pla_main_ctx };

        if(pluginlib_getaction(0, plugin_contexts, ARRAYLEN(plugin_contexts)) == PLA_CANCEL)
        {
            return;
        }

        rb->yield();
    }
}

static struct morse_mapping morse_map[] = {
    { 0, NULL },
    { 1, NULL },
    { 2, NULL },
    { 3, NULL },
    { 4, NULL },
    { 5, NULL },
    { 6, NULL },
    { 7, NULL },
    { 8, NULL },
    { 9, NULL },
    { 10, NULL },
    { 11, NULL },
    { 12, NULL },
    { 13, NULL },
    { 14, NULL },
    { 15, NULL },
    { 16, NULL },
    { 17, NULL },
    { 18, NULL },
    { 19, NULL },
    { 20, NULL },
    { 21, NULL },
    { 22, NULL },
    { 23, NULL },
    { 24, NULL },
    { 25, NULL },
    { 26, NULL },
    { 27, NULL },
    { 28, NULL },
    { 29, NULL },
    { 30, NULL },
    { 31, NULL },
    { 32, NULL },
    { 33, NULL },
    { 34, NULL },
    { 35, NULL },
    { 36, NULL },
    { 37, NULL },
    { 38, NULL },
    { 39, NULL },
    { 40, NULL },
    { 41, NULL },
    { 42, NULL },
    { 43, NULL },
    { 44, NULL },
    { 45, NULL },
    { 46, NULL },
    { 47, NULL },
    { 48, "-----" },
    { 49, ".----" },
    { 50, "..---" },
    { 51, "...--" },
    { 52, "....-" },
    { 53, "....." },
    { 54, "-...." },
    { 55, "--..." },
    { 56, "---.." },
    { 57, "----." },
    { 58, NULL },
    { 59, NULL },
    { 60, NULL },
    { 61, NULL },
    { 62, NULL },
    { 63, NULL },
    { 64, NULL },
    { 65, NULL },
    { 66, NULL },
    { 67, NULL },
    { 68, NULL },
    { 69, NULL },
    { 70, NULL },
    { 71, NULL },
    { 72, NULL },
    { 73, NULL },
    { 74, NULL },
    { 75, NULL },
    { 76, NULL },
    { 77, NULL },
    { 78, NULL },
    { 79, NULL },
    { 80, NULL },
    { 81, NULL },
    { 82, NULL },
    { 83, NULL },
    { 84, NULL },
    { 85, NULL },
    { 86, NULL },
    { 87, NULL },
    { 88, NULL },
    { 89, NULL },
    { 90, NULL },
    { 91, NULL },
    { 92, NULL },
    { 93, NULL },
    { 94, NULL },
    { 95, NULL },
    { 96, NULL },
    { 97, ".-" },
    { 98, "-..." },
    { 99, "-.-." },
    { 100, "-.." },
    { 101, "." },
    { 102, "..-." },
    { 103, "--." },
    { 104, "...." },
    { 105, ".." },
    { 106, ".---" },
    { 107, "-.-" },
    { 108, ".-.." },
    { 109, "--" },
    { 110, "-." },
    { 111, "---" },
    { 112, ".--." },
    { 113, "--.-" },
    { 114, ".-." },
    { 115, "..." },
    { 116, "-" },
    { 117, "..-" },
    { 118, "...-" },
    { 119, ".--" },
    { 120, "-..-" },
    { 121, "-.--" },
    { 122, "--.." },
    { 123, NULL },
    { 124, NULL },
    { 125, NULL },
    { 126, NULL },
    { 127, NULL },
    { 128, NULL },
    { 129, NULL },
    { 130, NULL },
    { 131, NULL },
    { 132, NULL },
    { 133, NULL },
    { 134, NULL },
    { 135, NULL },
    { 136, NULL },
    { 137, NULL },
    { 138, NULL },
    { 139, NULL },
    { 140, NULL },
    { 141, NULL },
    { 142, NULL },
    { 143, NULL },
    { 144, NULL },
    { 145, NULL },
    { 146, NULL },
    { 147, NULL },
    { 148, NULL },
    { 149, NULL },
    { 150, NULL },
    { 151, NULL },
    { 152, NULL },
    { 153, NULL },
    { 154, NULL },
    { 155, NULL },
    { 156, NULL },
    { 157, NULL },
    { 158, NULL },
    { 159, NULL },
    { 160, NULL },
    { 161, NULL },
    { 162, NULL },
    { 163, NULL },
    { 164, NULL },
    { 165, NULL },
    { 166, NULL },
    { 167, NULL },
    { 168, NULL },
    { 169, NULL },
    { 170, NULL },
    { 171, NULL },
    { 172, NULL },
    { 173, NULL },
    { 174, NULL },
    { 175, NULL },
    { 176, NULL },
    { 177, NULL },
    { 178, NULL },
    { 179, NULL },
    { 180, NULL },
    { 181, NULL },
    { 182, NULL },
    { 183, NULL },
    { 184, NULL },
    { 185, NULL },
    { 186, NULL },
    { 187, NULL },
    { 188, NULL },
    { 189, NULL },
    { 190, NULL },
    { 191, NULL },
    { 192, NULL },
    { 193, NULL },
    { 194, NULL },
    { 195, NULL },
    { 196, NULL },
    { 197, NULL },
    { 198, NULL },
    { 199, NULL },
    { 200, NULL },
    { 201, NULL },
    { 202, NULL },
    { 203, NULL },
    { 204, NULL },
    { 205, NULL },
    { 206, NULL },
    { 207, NULL },
    { 208, NULL },
    { 209, NULL },
    { 210, NULL },
    { 211, NULL },
    { 212, NULL },
    { 213, NULL },
    { 214, NULL },
    { 215, NULL },
    { 216, NULL },
    { 217, NULL },
    { 218, NULL },
    { 219, NULL },
    { 220, NULL },
    { 221, NULL },
    { 222, NULL },
    { 223, NULL },
    { 224, NULL },
    { 225, NULL },
    { 226, NULL },
    { 227, NULL },
    { 228, NULL },
    { 229, NULL },
    { 230, NULL },
    { 231, NULL },
    { 232, NULL },
    { 233, NULL },
    { 234, NULL },
    { 235, NULL },
    { 236, NULL },
    { 237, NULL },
    { 238, NULL },
    { 239, NULL },
    { 240, NULL },
    { 241, NULL },
    { 242, NULL },
    { 243, NULL },
    { 244, NULL },
    { 245, NULL },
    { 246, NULL },
    { 247, NULL },
    { 248, NULL },
    { 249, NULL },
    { 250, NULL },
    { 251, NULL },
    { 252, NULL },
    { 253, NULL },
    { 254, NULL },
    { 255, NULL },
};
