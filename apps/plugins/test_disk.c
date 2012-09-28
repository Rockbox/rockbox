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
#include "lib/helper.h"



#define TESTBASEDIR HOME_DIR "/__TEST__"
#define TEST_FILE   TESTBASEDIR "/test_disk.tmp"
#define FRND_SEED   0x78C3     /* arbirary */

#if (CONFIG_STORAGE & STORAGE_MMC)
#define TEST_SIZE (20*1024*1024)
#else
#define TEST_SIZE (300*1024*1024)
#endif
#define TEST_TIME 10 /* in seconds */

static unsigned char* audiobuf;
static size_t audiobuflen;

static unsigned short frnd_buffer;
static int line = 0;
static int max_line = 0;
static int log_fd;
static char logfilename[MAX_PATH];
static const char testbasedir[] = TESTBASEDIR;

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

static bool log_init(void)
{
    int h;

    lcd_getstringsize("A", NULL, &h);
    max_line = LCD_HEIGHT / h;
    line = 0;
    lcd_clear_display();
    lcd_update();
    
    create_numbered_filename(logfilename, HOME_DIR, "test_disk_log_", ".txt",
                                 2 IF_CNFN_NUM_(, NULL));
    log_fd = open(logfilename, O_RDWR|O_CREAT|O_TRUNC, 0666);
    return log_fd >= 0;
}

static void log_text(char *text, bool advance)
{
    lcd_puts(0, line, text);
    lcd_update();
    if (advance)
    {
        if (++line >= max_line)
            line = 0;
        fdprintf(log_fd, "%s\n", text);
    }
}

static void log_close(void)
{
    close(log_fd);
}

static bool test_fs(void)
{
    unsigned char text_buf[32];
    int total, current, align;
    int fd, ret;

    log_init();
    log_text("test_disk WRITE&VERIFY", true);
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
    snprintf(text_buf, sizeof(text_buf), "CPU clock: %ld Hz",
                 *cpu_frequency);
    log_text(text_buf, true);
#endif
    log_text("----------------------", true);
    snprintf(text_buf, sizeof text_buf, "Data size: %dKB", (TEST_SIZE>>10));
    log_text(text_buf, true);

    fd = creat(TEST_FILE, 0666);
    if (fd < 0)
    {
        splashf(HZ, "creat() failed: %d", fd);
        goto error;
    }

    frnd_buffer = FRND_SEED;
    total = TEST_SIZE;
    while (total > 0)
    {
        align = rand() & 0xf;
        current = rand() % (audiobuflen - align);
        current = MIN(current, total);
        snprintf(text_buf, sizeof text_buf, "Wrt %dKB, %dKB left",
                     current >> 10, total >> 10);
        log_text(text_buf, false);

        mem_fill_frnd(audiobuf + align, current);
        ret = write(fd, audiobuf + align, current);
        if (current != ret)
        {
            splashf(0, "write() failed: %d/%d", ret, current);
            close(fd);
            goto error;
        }
        total -= current;
    }
    close(fd);

    fd = open(TEST_FILE, O_RDONLY);
    if (fd < 0)
    {
        splashf(0, "open() failed: %d", ret);
        goto error;
    }

    frnd_buffer = FRND_SEED;
    total = TEST_SIZE;
    while (total > 0)
    {
        align = rand() & 0xf;
        current = rand() % (audiobuflen - align);
        current = MIN(current, total);
        snprintf(text_buf, sizeof text_buf, "Cmp %dKB, %dKB left",
                     current >> 10, total >> 10);
        log_text(text_buf, false);

        ret = read(fd, audiobuf + align, current);
        if (current != ret)
        {
            splashf(0, "read() failed: %d/%d", ret, current);
            close(fd);
            goto error;
        }
        if (!mem_cmp_frnd(audiobuf + align, current))
        {
            log_text(text_buf, true);
            log_text("Compare error.", true);
            close(fd);
            goto error;
        }
        total -= current;
    }
    close(fd);
    log_text(text_buf, true);
    log_text("Test passed.", true);

error:
    log_close();
    remove(TEST_FILE);
    button_clear_queue();
    button_get(true);

    return false;
}

static bool file_speed(int chunksize, bool align)
{
    unsigned char text_buf[64];
    int fd, ret;
    long filesize = 0;
    long size, time;
    
    if ((unsigned)chunksize >= audiobuflen)
        return false;

    log_text("--------------------", true);

    /* File creation write speed */
    fd = creat(TEST_FILE, 0666);
    if (fd < 0)
    {
        splashf(HZ, "creat() failed: %d", fd);
        goto error;
    }
    time = current_tick;
    while (TIME_BEFORE(current_tick, time + TEST_TIME*HZ))
    {
        ret = write(fd, audiobuf + (align ? 0 : 1), chunksize);
        if (chunksize != ret)
        {
            splashf(HZ, "write() failed: %d/%d", ret, chunksize);
            close(fd);
            goto error;
        }
        filesize += chunksize;
    }
    time = current_tick - time;
    close(fd);
    snprintf(text_buf, sizeof text_buf, "Create (%d,%c): %ld KB/s",
                 chunksize, align ? 'A' : 'U', (25 * (filesize>>8) / time) );
    log_text(text_buf, true);

    /* Existing file write speed */
    fd = open(TEST_FILE, O_WRONLY);
    if (fd < 0)
    {
        splashf(0, "open() failed: %d", fd);
        goto error;
    }
    time = current_tick;
    for (size = filesize; size > 0; size -= chunksize)
    {
        ret = write(fd, audiobuf + (align ? 0 : 1), chunksize);
        if (chunksize != ret)
        {
            splashf(0, "write() failed: %d/%d", ret, chunksize);
            close(fd);
            goto error;
        }
    }
    time = current_tick - time;
    close(fd);
    snprintf(text_buf, sizeof text_buf, "Write  (%d,%c): %ld KB/s",
                 chunksize, align ? 'A' : 'U', (25 * (filesize>>8) / time) );
    log_text(text_buf, true);
    
    /* File read speed */
    fd = open(TEST_FILE, O_RDONLY);
    if (fd < 0)
    {
        splashf(0, "open() failed: %d", fd);
        goto error;
    }
    time = current_tick;
    for (size = filesize; size > 0; size -= chunksize)
    {
        ret = read(fd, audiobuf + (align ? 0 : 1), chunksize);
        if (chunksize != ret)
        {
            splashf(0, "read() failed: %d/%d", ret, chunksize);
            close(fd);
            goto error;
        }
    }
    time = current_tick - time;
    close(fd);
    snprintf(text_buf, sizeof text_buf, "Read   (%d,%c): %ld KB/s",
                 chunksize, align ? 'A' : 'U', (25 * (filesize>>8) / time) );
    log_text(text_buf, true);
    remove(TEST_FILE);
    return true;

  error:
    remove(TEST_FILE);
    return false;
}

static bool test_speed(void)
{
    unsigned char text_buf[64];
    DIR *dir = NULL;
    struct dirent *entry = NULL;
    int fd, last_file;
    int i, n;
    long time;

    memset(audiobuf, 'T', audiobuflen);
    log_init();
    log_text("test_disk SPEED TEST", true);
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
    snprintf(text_buf, sizeof(text_buf), "CPU clock: %ld Hz",
                 *cpu_frequency);
    log_text(text_buf, true);
#endif
    log_text("--------------------", true);

    /* File creation speed */
    time = current_tick + TEST_TIME*HZ;
    for (i = 0; TIME_BEFORE(current_tick, time); i++)
    {
        snprintf(text_buf, sizeof(text_buf), TESTBASEDIR "/%08x.tmp", i);
        fd = creat(text_buf, 0666);
        if (fd < 0)
        {
            last_file = i;
            splashf(HZ, "creat() failed: %d", fd);
            goto error;
        }
        close(fd);
    }
    last_file = i;
    snprintf(text_buf, sizeof(text_buf), "Create:  %d files/s",
                 last_file / TEST_TIME);
    log_text(text_buf, true);
    
    /* File open speed */
    time = current_tick + TEST_TIME*HZ;
    for (n = 0, i = 0; TIME_BEFORE(current_tick, time); n++, i++)
    {
        if (i >= last_file)
            i = 0;
        snprintf(text_buf, sizeof(text_buf), TESTBASEDIR "/%08x.tmp", i);
        fd = open(text_buf, O_RDONLY);
        if (fd < 0)
        {
            splashf(HZ, "open() failed: %d", fd);
            goto error;
        }
        close(fd);
    }
    snprintf(text_buf, sizeof(text_buf), "Open:    %d files/s", n / TEST_TIME);
    log_text(text_buf, true);

    /* Directory scan speed */
    time = current_tick + TEST_TIME*HZ;
    for (n = 0; TIME_BEFORE(current_tick, time); n++)
    {
        if (entry == NULL)
        {
            if (dir != NULL)
                closedir(dir);
            dir = opendir(testbasedir);
            if (dir == NULL)
            {
                splash(HZ, "opendir() failed.");
                goto error;
            }
        }
        entry = readdir(dir);
    }
    closedir(dir);
    snprintf(text_buf, sizeof(text_buf), "Dirscan: %d files/s", n / TEST_TIME);
    log_text(text_buf, true);

    /* File delete speed */
    time = current_tick;
    for (i = 0; i < last_file; i++)
    {
        snprintf(text_buf, sizeof(text_buf), TESTBASEDIR "/%08x.tmp", i);
        remove(text_buf);
    }
    snprintf(text_buf, sizeof(text_buf), "Delete:  %ld files/s",
                 last_file * HZ / (current_tick - time));
    log_text(text_buf, true);
    
    if (file_speed(512, true)
        && file_speed(512, false)
        && file_speed(4096, true)
        && file_speed(4096, false)
        && file_speed(1048576, true))
        file_speed(1048576, false);

    log_text("DONE", false);
    log_close();
    button_clear_queue();
    button_get(true);
    return false;

  error:
    for (i = 0; i < last_file; i++)
    {
        snprintf(text_buf, sizeof(text_buf), TESTBASEDIR "/%08x.tmp", i);
        remove(text_buf);
    }
    log_text("DONE", false);
    log_close();
    button_clear_queue();
    button_get(true);
    return false;
}


/* this is the plugin entry point */
enum plugin_status plugin_start(const void* parameter)
{
    MENUITEM_STRINGLIST(menu, "Test Disk Menu", NULL,
                        "Disk speed", "Write & verify");
    int selected=0;
    bool quit = false;
    int align;
    DIR *dir;

    (void)parameter;

    if ((dir = opendir(testbasedir)) == NULL)
    {
        if (mkdir(testbasedir) < 0)
        {
            splash(HZ*2, "Can't create test directory.");
            return PLUGIN_ERROR;
        }
    }
    else
    {
        closedir(dir);
    }

    audiobuf = plugin_get_audio_buffer(&audiobuflen);
    /* align start and length to 32 bit */
    align = (-(intptr_t)audiobuf) & 3;
    audiobuf += align;
    audiobuflen = (audiobuflen - align) & ~3;

    srand(current_tick);

    /* Turn off backlight timeout */
    backlight_ignore_timeout();

    while(!quit)
    {
        switch(do_menu(&menu, &selected, NULL, false))
        {
            case 0:
                test_speed();
                break;
            case 1:
                test_fs();
                break;
            default:
                quit = true;
                break;
        }
    }

    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings();
    
    rmdir(testbasedir);

    return PLUGIN_OK;
}
