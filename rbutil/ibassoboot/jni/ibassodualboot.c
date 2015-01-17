/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2014 by Ilia Sergachev
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

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>

#include <linux/fb.h>
#include <linux/input.h>
#include <linux/reboot.h>

#include <signal.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <time.h>

#include <sys/limits.h>
#include <sys/mman.h>
#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/reboot.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <unistd.h>

#include "qdbmp.h"

#define PLAYER_FILE "/data/chosen_player"
#define POLL_MS 10

static const char ROCKBOX_BIN[]   = "/mnt/sdcard/.rockbox/rockbox";
static const char OF_PLAYER_BIN[] = "/system/bin/MangoPlayer_original";

#define CHOOSER_BMP "/system/chooser.bmp"
#define RBMISSING_BMP "/system/rbmissing.bmp"
#define USB_BMP "/system/usb.bmp"

#define KEYCODE_HEADPHONES 114
#define KEYCODE_HOLD 115
#define KEYCODE_PWR 116
#define KEYCODE_PWR_LONG 117
#define KEYCODE_SD 143
#define KEYCODE_VOLPLUS 158
#define KEYCODE_VOLMINUS 159
#define KEYCODE_PREV 160
#define KEYCODE_NEXT 162
#define KEYCODE_PLAY 161
#define KEY_HOLD_OFF 16


/*-----------------------------------------------------------------------------------------------*/


#ifdef DEBUG


#include <android/log.h>


static const char log_tag[] = "Rockbox Boot";


void debugf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    __android_log_vprint(ANDROID_LOG_DEBUG, log_tag, fmt, ap);
    va_end(ap);
}


void ldebugf(const char* file, int line, const char *fmt, ...)
{
    va_list ap;
    /* 13: 5 literal chars and 8 chars for the line number. */
    char buf[strlen(file) + strlen(fmt) + 13];
    snprintf(buf, sizeof(buf), "%s (%d): %s", file, line, fmt);
    va_start(ap, fmt);
    __android_log_vprint(ANDROID_LOG_DEBUG, log_tag, buf, ap);
    va_end(ap);
}


void debug_trace(const char* function)
{
    static const char trace_tag[] = "TRACE: ";
    char msg[strlen(trace_tag) + strlen(function) + 1];
    snprintf(msg, sizeof(msg), "%s%s", trace_tag, function);
    __android_log_write(ANDROID_LOG_DEBUG, log_tag, msg);
}


#define DEBUGF  debugf
#define TRACE debug_trace(__func__)
#else
#define DEBUGF(...)
#define TRACE
#endif


#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>


/*
    Without this socket iBasso Vold will not start.
    iBasso Vold uses this to send status messages about storage devices.
*/
static const char VOLD_MONITOR_SOCKET_NAME[] = "UNIX_domain\0";
static int _vold_monitor_socket_fd           = -1;


static int vold_monitor_open_socket(void)
{
    TRACE;

    unlink(VOLD_MONITOR_SOCKET_NAME);

    _vold_monitor_socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if(_vold_monitor_socket_fd < 0)
    {
        _vold_monitor_socket_fd = -1;
        return -1;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, VOLD_MONITOR_SOCKET_NAME, sizeof(addr.sun_path) -1);

    if(bind(_vold_monitor_socket_fd, (struct sockaddr*) &addr, sizeof(addr)) < 0)
    {
        close(_vold_monitor_socket_fd);
        unlink(VOLD_MONITOR_SOCKET_NAME);
        _vold_monitor_socket_fd = -1;
        return -1;
    }

    if(listen(_vold_monitor_socket_fd, 1) < 0)
    {
        close(_vold_monitor_socket_fd);
        unlink(VOLD_MONITOR_SOCKET_NAME);
        _vold_monitor_socket_fd = -1;
        return -1;
    }

    return _vold_monitor_socket_fd;
}


/*
    bionic does not have pthread_cancel.
    0: Vold monitor thread stopped/ending.
    1: Vold monitor thread started/running.
*/
static volatile sig_atomic_t _vold_monitor_active = 0;


/* true: sdcard not mounted. */
static bool _sdcard_not_mounted = true;


/* Mutex for sdcard mounted flag. */
static pthread_mutex_t _sdcard_mount_mtx = PTHREAD_MUTEX_INITIALIZER;


/* Signal condition for sdcard mounted flag. */
static pthread_cond_t _sdcard_mount_cond = PTHREAD_COND_INITIALIZER;


static void* vold_monitor_run(void* nothing)
{
    _vold_monitor_active = 1;

    (void*) nothing;

    TRACE;

    _vold_monitor_socket_fd = vold_monitor_open_socket();
    if(_vold_monitor_socket_fd < 0)
    {
        goto end;
    }

    struct pollfd fds[1];
    fds[0].fd     = _vold_monitor_socket_fd;
    fds[0].events = POLLIN;

    while(_vold_monitor_active == 1)
    {
        poll(fds, 1, 10);
        if(! (fds[0].revents & POLLIN))
        {
            continue;
        }

        int socket_fd = accept(_vold_monitor_socket_fd, NULL, NULL);

        if(socket_fd < 0)
        {
            goto end;
        }

        while(true)
        {
            char msg[1024];
            memset(msg, 0, sizeof(msg));
            int length = read(socket_fd, msg, sizeof(msg));

            if(length < 0)
            {
                close(socket_fd);
                goto end;
            }
            else if(length == 0)
            {
                close(socket_fd);
                break;
            }

            DEBUGF("%s: msg: %s", __func__, msg);

            if(   (strcmp(msg, "Volume flash /mnt/sdcard state changed from -1 (Initializing) to 0 (No-Media)") == 0)
               || (strcmp(msg, "Volume flash /mnt/sdcard state changed from 0 (No-Media) to 1 (Idle-Unmounted)") == 0)
               || (strcmp(msg, "Volume flash /mnt/sdcard state changed from 1 (Idle-Unmounted) to 3 (Checking)") == 0)
               || (strcmp(msg, "Volume flash /mnt/sdcard state changed from 4 (Mounted) to 5 (Unmounting)") == 0)
               || (strcmp(msg, "Volume flash /mnt/sdcard state changed from 5 (Unmounting) to 1 (Idle-Unmounted)") == 0)
               || (strcmp(msg, "Volume flash /mnt/sdcard state changed from 1 (Idle-Unmounted) to 7 (Shared-Unmounted)") == 0)
               || (strcmp(msg, "Volume flash /mnt/sdcard state changed from 7 (Shared-Unmounted) to 1 (Idle-Unmounted)") == 0))
            {
                pthread_mutex_lock(&_sdcard_mount_mtx);
                _sdcard_not_mounted = true;
                pthread_mutex_unlock(&_sdcard_mount_mtx);
            }
            else if(strcmp(msg, "Volume flash /mnt/sdcard state changed from 3 (Checking) to 4 (Mounted)") == 0)
            {
                pthread_mutex_lock(&_sdcard_mount_mtx);
                _sdcard_not_mounted = false;
                pthread_cond_signal(&_sdcard_mount_cond);
                pthread_mutex_unlock(&_sdcard_mount_mtx);
            }
        }
    }

end:
    if(_vold_monitor_socket_fd > -1)
    {
        close(_vold_monitor_socket_fd);
        unlink(VOLD_MONITOR_SOCKET_NAME);
    }

    _vold_monitor_socket_fd = -1;

    DEBUGF("%s: Thread end.", __func__);

    _vold_monitor_active = 0;
    return 0;
}


/* Thread id. */
static pthread_t _vold_monitor_thread;


static void vold_monitor_start(void)
{
    TRACE;

    if(_vold_monitor_active == 0)
    {
        pthread_create(&_vold_monitor_thread, NULL, vold_monitor_run, NULL);
    }
}


static void vold_monitor_stop(void)
{
    TRACE;

    if(_vold_monitor_active == 1)
    {
        _vold_monitor_active = 0;
        pthread_join(_vold_monitor_thread, NULL);
    }
}


/*-----------------------------------------------------------------------------------------------*/


static struct pollfd *ufds;
static char **device_names;
static int nfds;


static int open_device(const char *device, int print_flags)
{
    int fd;
    struct pollfd *new_ufds;
    char **new_device_names;

    fd = open(device, O_RDWR);
    if(fd < 0)
    {
        fprintf(stderr, "could not open %s, %s\n", device, strerror(errno));
        return -1;
    }

    new_ufds = realloc(ufds, sizeof(ufds[0]) * (nfds + 1));
    if(new_ufds == NULL)
    {
        fprintf(stderr, "out of memory\n");
        return -1;
    }
    ufds = new_ufds;
    new_device_names = realloc(device_names, sizeof(device_names[0]) * (nfds + 1));
    if(new_device_names == NULL)
    {
        fprintf(stderr, "out of memory\n");
        return -1;
    }
    device_names = new_device_names;

    ufds[nfds].fd = fd;
    ufds[nfds].events = POLLIN;
    device_names[nfds] = strdup(device);
    nfds++;

    return 0;
}



static int scan_dir(const char *dirname, int print_flags)
{
    char devname[PATH_MAX];
    char *filename;
    DIR *dir;
    struct dirent *de;
    dir = opendir(dirname);
    if(dir == NULL)
        return -1;
    strcpy(devname, dirname);
    filename = devname + strlen(devname);
    *filename++ = '/';
    while((de = readdir(dir)))
    {
        if(de->d_name[0] == '.' &&
                (de->d_name[1] == '\0' ||
                        (de->d_name[1] == '.' && de->d_name[2] == '\0')))
            continue;
        strcpy(filename, de->d_name);
        open_device(devname, print_flags);
    }
    closedir(dir);
    return 0;
}


void button_init_device(void)
{
    int res;
    int print_flags = 0;
    const char *device = NULL;
    const char *device_path = "/dev/input";

    nfds = 1;
    ufds = calloc(1, sizeof(ufds[0]));
    ufds[0].fd = inotify_init();
    ufds[0].events = POLLIN;
    if(device)
    {
        res = open_device(device, print_flags);
        if(res < 0) {
             fprintf(stderr, "open device failed\n");
        }
    }
    else
    {
        res = inotify_add_watch(ufds[0].fd, device_path, IN_DELETE | IN_CREATE);
        if(res < 0)
        {
            fprintf(stderr, "could not add watch for %s, %s\n", device_path, strerror(errno));
        }
        res = scan_dir(device_path, print_flags);
        if(res < 0)
        {
            fprintf(stderr, "scan dir failed for %s\n", device_path);
        }
    }
}


int draw(char * bitmapfile)
{
    int fbfd = 0;
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    long int screensize = 0;
    char *fbp = 0;
    int x = 0, y = 0;
    long int location = 0;

    /* Open the file for reading and writing */
    fbfd = open("/dev/graphics/fb0", O_RDWR);
    if (fbfd == -1)
    {
        perror("Error: cannot open framebuffer device");
        exit(1);
    }
    /* Get fixed screen information */
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1)
    {
        perror("Error reading fixed information");
        exit(2);
    }

    /* Get variable screen information */
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1)
    {
        perror("Error reading variable information");
        exit(3);
    }

    /* Figure out the size of the screen in bytes */
    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

    vinfo.xres = vinfo.xres_virtual = vinfo.width = 320;
    vinfo.yres = vinfo.yres_virtual = vinfo.height = 240;
    vinfo.xoffset = vinfo.yoffset = vinfo.sync = vinfo.vmode = 0;
    vinfo.pixclock = 104377;
    vinfo.left_margin = 20;
    vinfo.right_margin = 50;
    vinfo.upper_margin = 2;
    vinfo.lower_margin = 4;
    vinfo.hsync_len = 10;
    vinfo.vsync_len = 2;
    vinfo.red.offset = 11;
    vinfo.red.length = 5;
    vinfo.red.msb_right = 0;
    vinfo.green.offset = 5;
    vinfo.green.length = 6;
    vinfo.green.msb_right = 0;
    vinfo.blue.offset = 0;
    vinfo.blue.length = 5;
    vinfo.blue.msb_right = 0;
    vinfo.transp.offset = vinfo.transp.length = vinfo.transp.msb_right = 0;
    vinfo.nonstd = 4;

    if (ioctl(fbfd, FBIOPUT_VSCREENINFO, &vinfo))
    {
        perror("fbset(ioctl)");
        exit(4);
    }


    /* Map the device to memory */
    fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED,
            fbfd, 0);
    if ((int)fbp == -1)
    {
        perror("Error: failed to map framebuffer device to memory");
        exit(4);
    }

    BMP* bmp = BMP_ReadFile(bitmapfile);
    BMP_CHECK_ERROR(stderr, -1);

    UCHAR r, g, b;
    unsigned short int t;

    for (y = 0; y < 240; y++)
        for (x = 0; x < 320; x++)
        {
            location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) +
                    (y+vinfo.yoffset) * finfo.line_length;

            BMP_GetPixelRGB(bmp, x, y, &r, &g, &b);
            t = (r>>3)<<11 | (g>>2) << 5 | (b>>3);
            *((unsigned short int*)(fbp + location)) = t;
        }

    BMP_Free(bmp);

    munmap(fbp, screensize);
    close(fbfd);
    return 0;
}


int choose_player()
{
    int i;
    int res;
    struct input_event event;

    while(true)
    {
        poll(ufds, nfds, POLL_MS);
        for(i = 1; i < nfds; i++)
        {
            if(ufds[i].revents & POLLIN)
            {
                res = read(ufds[i].fd, &event, sizeof(event));
                if(res < (int)sizeof(event))
                {
                    fprintf(stderr, "could not get event\n");
                }
                if(event.type==1)
                {
                    if(event.code==KEYCODE_NEXT)
                    {
                        puts("rockbox!");
                        return 1;
                    }
                    else if(event.code==KEYCODE_PREV)
                    {
                        puts("mango!");
                        return 0;
                    }
                    else if(event.code==KEYCODE_PWR || event.code==KEYCODE_PWR_LONG)
                    {
                        reboot(LINUX_REBOOT_CMD_POWER_OFF);
                    }
                }
                else if(event.type==3)
                {
                    if(event.code==53) /* x coord */
                    {
                        if(event.value<160)
                        {
                            puts("mango!");
                            return 0;
                        }
                        else
                        {
                            puts("rockbox!");
                            return 1;
                        }
                    }
                }
            }
        }
    }
    return true;
}

bool check_for_hold()
{
    FILE *f = fopen("/sys/class/axppower/holdkey", "r");
    char x;
    fscanf(f, "%c", &x);
    fclose(f);

    if(x & KEY_HOLD_OFF)
      return false;
    else
      return true;
}

int main(int argc, char **argv)
{
    TRACE;

    FILE *f;
    int last_chosen_player = -1;

    f = fopen(PLAYER_FILE, "r");
    if(f!=NULL)
    {
        fscanf(f, "%d", &last_chosen_player);
        fclose(f);
    }
    bool ask = check_for_hold() || (last_chosen_player==-1);

    if(ask)
    {
        draw(CHOOSER_BMP);
        button_init_device();
        int player_chosen_now = choose_player();

        if(last_chosen_player!=player_chosen_now)
        {
            f = fopen(PLAYER_FILE, "w");
            fprintf(f, "%d", player_chosen_now);
            fclose(f);
        }

        last_chosen_player = player_chosen_now;
    }

    /* true, Rockbox was started at least once. */
    bool rockboxStarted = false;

    while(1)
    {
        /* Excecute OF MangoPlayer or Rockbox and restart it if it crashes. */

        if(last_chosen_player)
        {
            /* Start Rockbox */

            if(rockboxStarted)
            {
                /*
                    This is a work around until we have better USB detection.
                    At this point it is assumed, that Rockbox was forced closed due to a USB
                    connection remounting sdcard for mass storage access.
                    It will eventually restart, when the SDCard becomes available again.
                */
                draw(USB_BMP);
            }

            pthread_mutex_lock(&_sdcard_mount_mtx);

            /*
                Create the iBasso Vold socket and monitor it.
                Do this for Rockbox only.
            */
            vold_monitor_start();

            DEBUGF("%s: Waitung on /mnt/sdcard/.", __func__);
            while(_sdcard_not_mounted)
            {
                pthread_cond_wait(&_sdcard_mount_cond, &_sdcard_mount_mtx);
            }
            pthread_mutex_unlock(&_sdcard_mount_mtx);

            /* To be able to execute rockbox. */
            system("mount -o remount,exec /mnt/sdcard");

            /* This symlink is needed mainly to keep themes functional. */
            system("ln -s /mnt/sdcard/.rockbox /.rockbox");

            if(access(ROCKBOX_BIN, X_OK) != -1)
            {
                /*
                    Rockbox executable present.
                    Excecute Rockbox and wait for its termination.
                */
                DEBUGF("%s: Excecuting %s", __func__, ROCKBOX_BIN);
                system(ROCKBOX_BIN);
                DEBUGF("%s: Exit %s", __func__, ROCKBOX_BIN);
                rockboxStarted = true;
            }
            else
            {
                vold_monitor_stop();

                /*
                    Rockbox executable missing.
                    Show info screen for 30 seconds then excecute OF MangoPlayer.
                */
                draw(RBMISSING_BMP);
                sleep(30);

                DEBUGF("%s: Rockbox missing, excecuting %s", __func__, OF_PLAYER_BIN);
                system(OF_PLAYER_BIN);
                DEBUGF("%s: Exit %s", __func__, OF_PLAYER_BIN);
            }
        }
        else
        {
            /* Excecute OF MangoPlayer and wait for its termination. */
            DEBUGF("%s: Excecuting %s", __func__, OF_PLAYER_BIN);
            system(OF_PLAYER_BIN);
            DEBUGF("%s: Exit %s", __func__, OF_PLAYER_BIN);
        }

        sleep(1);
    }

    return 0;
}
