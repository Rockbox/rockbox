/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2007 Jens Arnold
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include "plugin.h"

PLUGIN_HEADER

#define TEST_FILE "/test_disk.tmp"
#define FRND_SEED 0x78C3     /* arbirary */

#ifdef HAVE_MMC
#define TEST_SIZE (20*1024*1024)
#else
#define TEST_SIZE (300*1024*1024)
#endif

static struct plugin_api* rb;
static unsigned char* audiobuf;
static ssize_t audiobufsize;

static unsigned short frnd_buffer;
static int line = 0;
static int max_line = 0;

static void mem_fill_frnd(unsigned char *addr, int len)
{
    unsigned char *end = addr + len;
    unsigned random = frnd_buffer;

    while (addr < end)
    {
        random = 75 * random + 74;
        *addr++ = random >> 8;
    }
    frnd_buffer = random;
}

static bool mem_cmp_frnd(unsigned char *addr, int len)
{
    unsigned char *end = addr + len;
    unsigned random = frnd_buffer;

    while (addr < end)
    {
        random = 75 * random + 74;
        if (*addr++ != ((random >> 8) & 0xff))
            return false;
    }
    frnd_buffer = random;
    return true;
}

static void log_init(void)
{
    int h;

    rb->lcd_getstringsize("A", NULL, &h);
    max_line = LCD_HEIGHT / h;
    line = 0;
    rb->lcd_clear_display();
    rb->lcd_update();
}

static void log_lcd(char *text, bool advance)
{
    rb->lcd_puts(0, line, text);
    rb->lcd_update();
    if (advance)
        if (++line >= max_line)
            line = 0;
}

static bool test_fs(void)
{
    unsigned char text_buf[32];
    unsigned char *buf_start;
    int align, buf_len;
    int total, current;
    int fd;

    align = (-(int)audiobuf) & 3;
    buf_start = audiobuf + align;
    buf_len = (audiobufsize - align - 4) & ~3;

    log_init();
    rb->snprintf(text_buf, sizeof text_buf, "FS stress test: %dKB", (TEST_SIZE>>10));
    log_lcd(text_buf, true);

    fd = rb->creat(TEST_FILE);
    if (fd < 0)
    {
        rb->splash(0, "Couldn't create testfile.");
        goto error;
    }

    frnd_buffer = FRND_SEED;
    total = TEST_SIZE;
    while (total > 0)
    {
        current = rb->rand() % buf_len;
        current = MIN(current, total);
        align = rb->rand() & 3;
        rb->snprintf(text_buf, sizeof text_buf, "Wrt %dKB, %dKB left",
                     current >> 10, total >> 10);
        log_lcd(text_buf, false);

        mem_fill_frnd(buf_start + align, current);
        if (current != rb->write(fd, buf_start + align, current))
        {
            rb->splash(0, "Write error.");
            rb->close(fd);
            goto error;
        }
        total -= current;
    }
    rb->close(fd);

    fd = rb->open(TEST_FILE, O_RDONLY);
    if (fd < 0)
    {
        rb->splash(0, "Couldn't open testfile.");
        goto error;
    }

    frnd_buffer = FRND_SEED;
    total = TEST_SIZE;
    while (total > 0)
    {
        current = rb->rand() % buf_len;
        current = MIN(current, total);
        align = rb->rand() & 3;
        rb->snprintf(text_buf, sizeof text_buf, "Cmp %dKB, %dKB left",
                     current >> 10, total >> 10);
        log_lcd(text_buf, false);

        if (current != rb->read(fd, buf_start + align, current))
        {
            rb->splash(0, "Read error.");
            rb->close(fd);
            goto error;
        }
        if (!mem_cmp_frnd(buf_start + align, current))
        {
            log_lcd(text_buf, true);
            log_lcd("Compare error.", true);
            rb->close(fd);
            goto error;
        }
        total -= current;
    }
    rb->close(fd);
    log_lcd(text_buf, true);
    log_lcd("Test passed.", true);

error:
    rb->remove(TEST_FILE);
    rb->button_clear_queue();
    rb->button_get(true);

    return false;
}

static bool test_speed(void)
{
    unsigned char text_buf[32];
    unsigned char *buf_start;
    long time;
    int buf_len;
    int fd;

    buf_len = (-(int)audiobuf) & 3;
    buf_start = audiobuf + buf_len;
    buf_len = (audiobufsize - buf_len) & ~3;
    rb->memset(buf_start, 'T', buf_len);
    buf_len -= 2;

    log_init();
    log_lcd("Disk speed test", true);

    fd = rb->creat(TEST_FILE);
    if (fd < 0)
    {
        rb->splash(0, "Couldn't create testfile.");
        goto error;
    }
    time = *rb->current_tick;
    if (buf_len != rb->write(fd, buf_start, buf_len))
    {
        rb->splash(0, "Write error.");
        rb->close(fd);
        goto error;
    }
    time = *rb->current_tick - time;
    rb->snprintf(text_buf, sizeof text_buf, "Create: %ld KByte/s",
                 (25 * buf_len / time) >> 8);
    log_lcd(text_buf, true);
    rb->close(fd);

    fd = rb->open(TEST_FILE, O_WRONLY);
    if (fd < 0)
    {
        rb->splash(0, "Couldn't open testfile.");
        goto error;
    }
    time = *rb->current_tick;
    if (buf_len != rb->write(fd, buf_start, buf_len))
    {
        rb->splash(0, "Write error.");
        rb->close(fd);
        goto error;
    }
    time = *rb->current_tick - time;
    rb->snprintf(text_buf, sizeof text_buf, "Write A: %ld KByte/s",
                 (25 * buf_len / time) >> 8);
    log_lcd(text_buf, true);
    rb->close(fd);
    
    fd = rb->open(TEST_FILE, O_WRONLY);
    if (fd < 0)
    {
        rb->splash(0, "Couldn't open testfile.");
        goto error;
    }
    time = *rb->current_tick;
    if (buf_len != rb->write(fd, buf_start + 1, buf_len))
    {
        rb->splash(0, "Write error.");
        rb->close(fd);
        goto error;
    }
    time = *rb->current_tick - time;
    rb->snprintf(text_buf, sizeof text_buf, "Write U: %ld KByte/s",
                 (25 * buf_len / time) >> 8);
    log_lcd(text_buf, true);
    rb->close(fd);
    
    fd = rb->open(TEST_FILE, O_RDONLY);
    if (fd < 0)
    {
        rb->splash(0, "Couldn't open testfile.");
        goto error;
    }
    time = *rb->current_tick;
    if (buf_len != rb->read(fd, buf_start, buf_len))
    {
        rb->splash(0, "Read error.");
        rb->close(fd);
        goto error;
    }
    time = *rb->current_tick - time;
    rb->snprintf(text_buf, sizeof text_buf, "Read A: %ld KByte/s",
                 (25 * buf_len / time) >> 8);
    log_lcd(text_buf, true);
    rb->close(fd);
    
    fd = rb->open(TEST_FILE, O_RDONLY);
    if (fd < 0)
    {
        rb->splash(0, "Couldn't open testfile.");
        goto error;
    }
    time = *rb->current_tick;
    if (buf_len != rb->read(fd, buf_start + 1, buf_len))
    {
        rb->splash(0, "Read error.");
        rb->close(fd);
        goto error;
    }
    time = *rb->current_tick - time;
    rb->snprintf(text_buf, sizeof text_buf, "Read U: %ld KByte/s",
                 (25 * buf_len / time) >> 8);
    log_lcd(text_buf, true);
    rb->close(fd);

error:
    rb->remove(TEST_FILE);
    rb->button_clear_queue();
    rb->button_get(true);

    return false;
}


/* this is the plugin entry point */
enum plugin_status plugin_start(struct plugin_api* api, void* parameter)
{
    static const struct menu_item items[] = {
        { "Disk speed",     test_speed  },
        { "FS stress test", test_fs     },
    };
    int m;

    (void)parameter;
    rb = api;
    audiobuf = rb->plugin_get_audio_buffer((size_t *)&audiobufsize);
    rb->srand(*rb->current_tick);

    if (rb->global_settings->backlight_timeout > 0)
        rb->backlight_set_timeout(1); /* keep the light on */

    m = rb->menu_init(items, sizeof(items) / sizeof(*items), NULL,
                      NULL, NULL, NULL);
    rb->menu_run(m);
    rb->menu_exit(m);

    /* restore normal backlight setting */
    rb->backlight_set_timeout(rb->global_settings->backlight_timeout);

    return PLUGIN_OK;
}
