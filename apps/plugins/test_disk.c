/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
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
#include "lib/xlcd.h"



#define TESTDIRNAME "__TEST__"
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
static int line_height = 0;
static int log_fd;
static char logfilename[MAX_PATH];

/* The disk under test: HOME_DIR, or where there are several volumes the
   one chosen from the menu.  Its test directory, test file and log are all
   on it, so nothing is written anywhere else. */
#ifdef HAVE_MULTIVOLUME
#define MAX_DISKS (NUM_VOLUMES + 1)
#else
#define MAX_DISKS 1
#endif
static char disks[MAX_DISKS][24];
static int ndisks;
static int disk;
static char testbasedir[MAX_PATH];
static char testfile[MAX_PATH];

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
    rb->lcd_getstringsize("A", NULL, &line_height);
    max_line = LCD_HEIGHT / line_height;
    line = 0;
    rb->lcd_clear_display();
    rb->lcd_update();

    rb->create_numbered_filename(logfilename, disks[disk], "test_disk_log_",
                                 ".txt", 2 IF_CNFN_NUM_(, NULL));
    log_fd = rb->open(logfilename, O_RDWR|O_CREAT|O_TRUNC, 0666);
    return log_fd >= 0;
}

static void log_text(char *text, bool advance)
{
    if (line >= max_line)
    {
        /* The screen is full: scroll up to make room for this line */
        xlcd_scroll_up(line_height);
        line = max_line - 1;
    }
    rb->lcd_puts(0, line, text);
    rb->lcd_update();
    if (advance)
    {
        line++;
        rb->fdprintf(log_fd, "%s\n", text);
    }
}

static void log_close(void)
{
    rb->close(log_fd);
}

/* The name of the i'th of the many small files the speed test makes. */
static void tmp_name(char *buf, size_t size, int i)
{
    rb->snprintf(buf, size, "%s/%08x.tmp", testbasedir, i);
}

/* The disks there are to test: HOME_DIR, and each volume the root lists. */
static void find_disks(void)
{
    rb->strlcpy(disks[0], HOME_DIR, sizeof disks[0]);
    ndisks = 1;
#ifdef HAVE_MULTIVOLUME
    {
        DIR *dir = rb->opendir(PATH_ROOTSTR);
        struct dirent *entry;

        if (dir == NULL)
            return;
        while (ndisks < MAX_DISKS && (entry = rb->readdir(dir)) != NULL)
        {
            if (!(rb->dir_get_info(dir, entry).attribute & ATTR_VOLUME))
                continue;
            rb->snprintf(disks[ndisks], sizeof disks[0], PATH_ROOTSTR "%s",
                         entry->d_name);
            ndisks++;
        }
        rb->closedir(dir);
    }
#endif
}

/* Make disk n the one under test.  The test directory on the one before
   is removed and one made here. */
static bool use_disk(int n)
{
    DIR *dir;

    if (testbasedir[0])
        rb->rmdir(testbasedir);
    disk = n;
    rb->snprintf(testbasedir, sizeof testbasedir, "%s/" TESTDIRNAME,
                 disks[disk]);
    rb->snprintf(testfile, sizeof testfile, "%s/test_disk.tmp", testbasedir);

    if ((dir = rb->opendir(testbasedir)) == NULL)
    {
        if (rb->mkdir(testbasedir) < 0)
        {
            rb->splashf(HZ*2, "Can't create %s", testbasedir);
            testbasedir[0] = '\0';
            return false;
        }
    }
    else
    {
        rb->closedir(dir);
    }
    return true;
}

#ifdef HAVE_MULTIVOLUME
/* Ask which disk to test. */
static bool select_disk(void)
{
    struct opt_items names[MAX_DISKS];
    int n = disk;
    int i;

    for (i = 0; i < ndisks; i++)
    {
        names[i].string = disks[i];
        names[i].voice_id = -1;
    }
    rb->set_option("Disk to test", &n, RB_INT, names, ndisks, NULL);
    return use_disk(n);
}
#endif

static bool test_fs(void)
{
    unsigned char text_buf[32];
    int total, current, align;
    int fd, ret;

    log_init();
    log_text("test_disk WRITE&VERIFY", true);
    log_text(disks[disk], true);
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
    rb->snprintf(text_buf, sizeof(text_buf), "CPU clock: %ld Hz",
                 *rb->cpu_frequency);
    log_text(text_buf, true);
#endif
    log_text("----------------------", true);
    rb->snprintf(text_buf, sizeof text_buf, "Data size: %dKB", (TEST_SIZE>>10));
    log_text(text_buf, true);

    fd = rb->creat(testfile, 0666);
    if (fd < 0)
    {
        rb->splashf(HZ, "creat() failed: %d", fd);
        goto error;
    }

    frnd_buffer = FRND_SEED;
    total = TEST_SIZE;
    while (total > 0)
    {
        align = rb->rand() & 0xf;
        current = rb->rand() % (audiobuflen - align);
        current = MIN(current, total);
        rb->snprintf(text_buf, sizeof text_buf, "Wrt %dKB, %dKB left",
                     current >> 10, total >> 10);
        log_text(text_buf, false);

        mem_fill_frnd(audiobuf + align, current);
        ret = rb->write(fd, audiobuf + align, current);
        if (current != ret)
        {
            rb->splashf(0, "write() failed: %d/%d", ret, current);
            rb->close(fd);
            goto error;
        }
        total -= current;
    }
    rb->close(fd);

    fd = rb->open(testfile, O_RDONLY);
    if (fd < 0)
    {
        rb->splashf(0, "open() failed: %d", fd);
        goto error;
    }

    frnd_buffer = FRND_SEED;
    total = TEST_SIZE;
    while (total > 0)
    {
        align = rb->rand() & 0xf;
        current = rb->rand() % (audiobuflen - align);
        current = MIN(current, total);
        rb->snprintf(text_buf, sizeof text_buf, "Cmp %dKB, %dKB left",
                     current >> 10, total >> 10);
        log_text(text_buf, false);

        ret = rb->read(fd, audiobuf + align, current);
        if (current != ret)
        {
            rb->splashf(0, "read() failed: %d/%d", ret, current);
            rb->close(fd);
            goto error;
        }
        if (!mem_cmp_frnd(audiobuf + align, current))
        {
            log_text(text_buf, true);
            log_text("Compare error.", true);
            rb->close(fd);
            goto error;
        }
        total -= current;
    }
    rb->close(fd);
    log_text(text_buf, true);
    log_text("Test passed.", true);

error:
    log_close();
    rb->remove(testfile);
    rb->button_clear_queue();
    rb->button_get(true);

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
    fd = rb->creat(testfile, 0666);
    if (fd < 0)
    {
        rb->splashf(HZ, "creat() failed: %d", fd);
        goto error;
    }
    time = *rb->current_tick;
    while (TIME_BEFORE(*rb->current_tick, time + TEST_TIME*HZ))
    {
        ret = rb->write(fd, audiobuf + (align ? 0 : 1), chunksize);
        if (chunksize != ret)
        {
            rb->splashf(HZ, "write() failed: %d/%d", ret, chunksize);
            rb->close(fd);
            goto error;
        }
        filesize += chunksize;
    }
    time = *rb->current_tick - time;
    rb->close(fd);
    rb->snprintf(text_buf, sizeof text_buf, "Create (%d,%c): %ld KB/s",
                 chunksize, align ? 'A' : 'U', (25 * (filesize>>8) / time) );
    log_text(text_buf, true);

    /* Existing file write speed */
    fd = rb->open(testfile, O_WRONLY);
    if (fd < 0)
    {
        rb->splashf(0, "open() failed: %d", fd);
        goto error;
    }
    time = *rb->current_tick;
    for (size = filesize; size > 0; size -= chunksize)
    {
        ret = rb->write(fd, audiobuf + (align ? 0 : 1), chunksize);
        if (chunksize != ret)
        {
            rb->splashf(0, "write() failed: %d/%d", ret, chunksize);
            rb->close(fd);
            goto error;
        }
    }
    time = *rb->current_tick - time;
    rb->close(fd);
    rb->snprintf(text_buf, sizeof text_buf, "Write  (%d,%c): %ld KB/s",
                 chunksize, align ? 'A' : 'U', (25 * (filesize>>8) / time) );
    log_text(text_buf, true);

    /* File read speed */
    fd = rb->open(testfile, O_RDONLY);
    if (fd < 0)
    {
        rb->splashf(0, "open() failed: %d", fd);
        goto error;
    }
    time = *rb->current_tick;
    for (size = filesize; size > 0; size -= chunksize)
    {
        ret = rb->read(fd, audiobuf + (align ? 0 : 1), chunksize);
        if (chunksize != ret)
        {
            rb->splashf(0, "read() failed: %d/%d", ret, chunksize);
            rb->close(fd);
            goto error;
        }
    }
    time = *rb->current_tick - time;
    rb->close(fd);
    rb->snprintf(text_buf, sizeof text_buf, "Read   (%d,%c): %ld KB/s",
                 chunksize, align ? 'A' : 'U', (25 * (filesize>>8) / time) );
    log_text(text_buf, true);
    rb->remove(testfile);
    return true;

  error:
    rb->remove(testfile);
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

    rb->memset(audiobuf, 'T', audiobuflen);
    log_init();
    log_text("test_disk SPEED TEST", true);
    log_text(disks[disk], true);
#if (CONFIG_PLATFORM & PLATFORM_NATIVE)
    rb->snprintf(text_buf, sizeof(text_buf), "CPU clock: %ld Hz",
                 *rb->cpu_frequency);
    log_text(text_buf, true);
#endif
    log_text("--------------------", true);

    /* File creation speed */
    time = *rb->current_tick + TEST_TIME*HZ;
    for (i = 0; TIME_BEFORE(*rb->current_tick, time); i++)
    {
        tmp_name(text_buf, sizeof(text_buf), i);
        fd = rb->creat(text_buf, 0666);
        if (fd < 0)
        {
            last_file = i;
            rb->splashf(HZ, "creat() failed: %d", fd);
            goto error;
        }
        rb->close(fd);
    }
    last_file = i;
    rb->snprintf(text_buf, sizeof(text_buf), "Create:  %d files/s",
                 last_file / TEST_TIME);
    log_text(text_buf, true);

    /* File open speed */
    time = *rb->current_tick + TEST_TIME*HZ;
    for (n = 0, i = 0; TIME_BEFORE(*rb->current_tick, time); n++, i++)
    {
        if (i >= last_file)
            i = 0;
        tmp_name(text_buf, sizeof(text_buf), i);
        fd = rb->open(text_buf, O_RDONLY);
        if (fd < 0)
        {
            rb->splashf(HZ, "open() failed: %d", fd);
            goto error;
        }
        rb->close(fd);
    }
    rb->snprintf(text_buf, sizeof(text_buf), "Open:    %d files/s", n / TEST_TIME);
    log_text(text_buf, true);

    /* Directory scan speed */
    time = *rb->current_tick + TEST_TIME*HZ;
    for (n = 0; TIME_BEFORE(*rb->current_tick, time); n++)
    {
        if (entry == NULL)
        {
            if (dir != NULL)
                rb->closedir(dir);
            dir = rb->opendir(testbasedir);
            if (dir == NULL)
            {
                rb->splash(HZ, "opendir() failed.");
                goto error;
            }
        }
        entry = rb->readdir(dir);
    }
    rb->closedir(dir);
    rb->snprintf(text_buf, sizeof(text_buf), "Dirscan: %d files/s", n / TEST_TIME);
    log_text(text_buf, true);

    dir = NULL;
    entry = NULL;
    /* Directory scan speed 2 */
    time = *rb->current_tick + TEST_TIME*HZ;
    for (n = 0; TIME_BEFORE(*rb->current_tick, time); n++)
    {
        if (entry == NULL)
        {
            if (dir != NULL)
                rb->closedir(dir);
            dir = rb->opendir(testbasedir);
            if (dir == NULL)
            {
                rb->splash(HZ, "opendir() failed.");
                goto error;
            }
        }
        else
            (void) rb->dir_get_info(dir, entry);
        entry = rb->readdir(dir);
    }
    rb->closedir(dir);
    rb->snprintf(text_buf, sizeof(text_buf), "Dirscan w info: %d files/s", n / TEST_TIME);
    log_text(text_buf, true);

    /* File delete speed */
    time = *rb->current_tick;
    for (i = 0; i < last_file; i++)
    {
        tmp_name(text_buf, sizeof(text_buf), i);
        rb->remove(text_buf);
    }
    rb->snprintf(text_buf, sizeof(text_buf), "Delete:  %ld files/s",
                 last_file * HZ / (*rb->current_tick - time));
    log_text(text_buf, true);

    if (file_speed(512, true)
        && file_speed(512, false)
        && file_speed(4096, true)
        && file_speed(4096, false)
        && file_speed(1048576, true))
        file_speed(1048576, false);

    log_text("DONE", false);
    log_close();
    rb->button_clear_queue();
    rb->button_get(true);
    return false;

  error:
    for (i = 0; i < last_file; i++)
    {
        tmp_name(text_buf, sizeof(text_buf), i);
        rb->remove(text_buf);
    }
    log_text("DONE", false);
    log_close();
    rb->button_clear_queue();
    rb->button_get(true);
    return false;
}


/* this is the plugin entry point */
enum plugin_status plugin_start(const void* parameter)
{
#ifdef HAVE_MULTIVOLUME
    MENUITEM_STRINGLIST(menu, "Test Disk", NULL,
                        "Disk speed", "Write & verify", "Select disk");
#else
    MENUITEM_STRINGLIST(menu, "Test Disk", NULL,
                        "Disk speed", "Write & verify");
#endif
    int selected=0;
    bool quit = false;

    (void)parameter;

    find_disks();
#ifdef HAVE_MULTIVOLUME
    if (ndisks > 1 ? !select_disk() : !use_disk(0))
        return PLUGIN_ERROR;
#else
    if (!use_disk(0))
        return PLUGIN_ERROR;
#endif

    audiobuf = rb->plugin_get_audio_buffer(&audiobuflen);
#ifdef STORAGE_WANTS_ALIGN
    /* align start and length for DMA */
    STORAGE_ALIGN_BUFFER(audiobuf, audiobuflen);
#else
    /* align start and length to 32 bit */
    ALIGN_BUFFER(audiobuf, audiobuflen, 4);
#endif

    rb->srand(*rb->current_tick);

    /* Turn off backlight timeout */
    backlight_ignore_timeout();


    while(!quit)
    {
        switch(rb->do_menu(&menu, &selected, NULL, false))
        {
            case 0:
                test_speed();
                break;
            case 1:
                test_fs();
                break;
#ifdef HAVE_MULTIVOLUME
            case 2:
                if (!select_disk())
                    quit = true;
                break;
#endif
            default:
                quit = true;
                break;
        }
    }

    /* Turn on backlight timeout (revert to settings) */
    backlight_use_settings();

    if (testbasedir[0])
        rb->rmdir(testbasedir);

    return PLUGIN_OK;
}
