/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2004 Björn Stenberg
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "plugin.h"

/* This plugin is designed to measure your battery performance in real-time.
   It will create a big file, read it every ~90 seconds and log the
   battery level in /battery.log

   When battery level goes below 5% the plugin exits, to avoid writing to
   disk in very low battery situations.
   
   Note that this test will run for 10-15 hours or more and is very boring
   to watch.
*/

/* variable button definitions */
#if CONFIG_KEYPAD == RECORDER_PAD
#define BATTERY_TEST_QUIT BUTTON_OFF
#elif CONFIG_KEYPAD == ONDIO_PAD
#define BATTERY_TEST_QUIT BUTTON_OFF
#elif CONFIG_KEYPAD == PLAYER_PAD
#define BATTERY_TEST_QUIT BUTTON_STOP
#elif CONFIG_KEYPAD == IRIVER_H100_PAD
#define BATTERY_TEST_QUIT BUTTON_OFF
#endif

static struct plugin_api* rb;

void* buffer;
int buffersize;

int init(void)
{
    int f;
    buffer = rb->plugin_get_mp3_buffer(&buffersize);
    
#ifdef HAVE_MMC
    /* present info what's going on. MMC is slow. */
    rb->splash(0, true, "Creating dummy file.");
#endif

    /* create a big dummy file */
    f = rb->creat("/battery.dummy", 0);
    if (f<0) {
        rb->splash(HZ, true, "Can't create /battery.dummy");
        return -1;
    }
    rb->write(f, buffer, buffersize);
    rb->close(f);

    return 0;
}

enum plugin_status loop(void)
{
    while (true) {
        struct tm* t;
        char buf[80];
        int f;
        int batt = rb->battery_level();
        int button;

        /* stop measuring when <5% battery left */
        if ((batt > 0) && (batt < 5))
            break;
        
        /* log current time */
        f = rb->open("/battery.log", O_WRONLY | O_APPEND | O_CREAT);
        if (f<0) {
            rb->splash(HZ, true, "Failed creating /battery.log");
            break;
        }
#ifdef HAVE_RTC
        t = rb->get_time();
#else
        {
            static struct tm temp;
            long t2 = *rb->current_tick/HZ;
            temp.tm_hour=t2/3600;
            temp.tm_min=(t2/60)%60;
            temp.tm_sec=t2%60;
            t=&temp;
        }
#endif
        rb->snprintf(buf, sizeof buf, "%02d:%02d:%02d Battery %d%%\n",
                     t->tm_hour, t->tm_min, t->tm_sec, batt);
        rb->write(f, buf, rb->strlen(buf));
        rb->close(f);

        rb->snprintf(buf, sizeof buf, "%02d:%02d:%02d Battery %d%%%%",
                     t->tm_hour, t->tm_min, t->tm_sec, batt);
        rb->splash(0, true, buf);
        
        /* simulate 128 kbit/s (16000 byte/s) playback duration */
        rb->button_clear_queue();
        button = rb->button_get_w_tmo(HZ * buffersize / 16000 - HZ*10);

        if (button == BATTERY_TEST_QUIT)
            return PLUGIN_OK;

        if (rb->default_event_handler(button) == SYS_USB_CONNECTED)
            return PLUGIN_USB_CONNECTED;

        /* simulate filling the mp3 buffer */
        f = rb->open("/battery.dummy", O_RDONLY);
        if (f<0) {
            rb->splash(HZ, true, "Failed opening /battery.dummy");
            break;
        }
        rb->read(f, buffer, buffersize);
        rb->close(f);
    }
    return PLUGIN_OK;
}

enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    TEST_PLUGIN_API(api);
    (void)parameter;
    rb = api;

    if (init() < 0)
        return PLUGIN_OK;

    return loop();
}

