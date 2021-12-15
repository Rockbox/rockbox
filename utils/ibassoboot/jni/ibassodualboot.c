/***************************************************************************
 *             __________               __   ___
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2014 by Ilia Sergachev: Initial Rockbox port to iBasso DX50
 * Copyright (C) 2014 by Mario Basister: iBasso DX90 port
 * Copyright (C) 2014 by Simon Rothen: Initial Rockbox repository submission, additional features
 * Copyright (C) 2014 by Udo Schläpfer: Code clean up, additional features
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
#include <fcntl.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/fb.h>
#include <linux/input.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/reboot.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>

#include "qdbmp.h"


/*- Android logcat ------------------------------------------------------------------------------*/


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
#endif /* DEBUG */


/*- Vold monitor --------------------------------------------------------------------------------*/


/*
    Without this socket iBasso Vold will not start.
    iBasso Vold uses this to send status messages about storage devices.
*/
static const char VOLD_MONITOR_SOCKET_NAME[] = "UNIX_domain";
static int _vold_monitor_socket_fd           = -1;


static void vold_monitor_open_socket(void)
{
    TRACE;

    unlink(VOLD_MONITOR_SOCKET_NAME);

    _vold_monitor_socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);

    if(_vold_monitor_socket_fd < 0)
    {
        _vold_monitor_socket_fd = -1;
        return;
    }

    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, VOLD_MONITOR_SOCKET_NAME, sizeof(addr.sun_path) - 1);

    if(bind(_vold_monitor_socket_fd, (struct sockaddr*) &addr, sizeof(addr)) < 0)
    {
        close(_vold_monitor_socket_fd);
        unlink(VOLD_MONITOR_SOCKET_NAME);
        _vold_monitor_socket_fd = -1;
        return;
    }

    if(listen(_vold_monitor_socket_fd, 1) < 0)
    {
        close(_vold_monitor_socket_fd);
        unlink(VOLD_MONITOR_SOCKET_NAME);
        _vold_monitor_socket_fd = -1;
        return;
    }
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

    (void) nothing;

    DEBUGF("DEBUG %s: Thread start.", __func__);

    vold_monitor_open_socket();
    if(_vold_monitor_socket_fd < 0)
    {
        DEBUGF("ERROR %s: Thread end: No socket.", __func__);

        _vold_monitor_active = 0;
        return 0;
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
            DEBUGF("ERROR %s: accept failed.", __func__);

            continue;
        }

        while(true)
        {
            char msg[1024];
            memset(msg, 0, sizeof(msg));
            int length = read(socket_fd, msg, sizeof(msg));

            if(length <= 0)
            {
                close(socket_fd);
                break;
            }

            DEBUGF("DEBUG %s: msg: %s", __func__, msg);

            if(strcmp(msg, "Volume flash /mnt/sdcard state changed from 3 (Checking) to 4 (Mounted)") == 0)
            {
                pthread_mutex_lock(&_sdcard_mount_mtx);
                _sdcard_not_mounted = false;
                pthread_cond_signal(&_sdcard_mount_cond);
                pthread_mutex_unlock(&_sdcard_mount_mtx);
            }
        }
    }

    close(_vold_monitor_socket_fd);
    unlink(VOLD_MONITOR_SOCKET_NAME);
    _vold_monitor_socket_fd = -1;

    DEBUGF("DEBUG %s: Thread end.", __func__);

    _vold_monitor_active = 0;
    return 0;
}


/* Vold monitor thread. */
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
        int ret = pthread_join(_vold_monitor_thread, NULL);
        DEBUGF("DEBUG %s: Thread joined: ret: %d.", __func__, ret);
    }
}


/*- Input handler -------------------------------------------------------------------------------*/


/* Input devices monitored with poll API. */
static struct pollfd* _fds = NULL;


/* Number of input devices monitored with poll API. */
static nfds_t _nfds = 0;


/* The names of the devices in _fds. */
static char** _device_names = NULL;


/* Open device device_name and add it to the list of polled devices. */
static void open_device(const char* device_name)
{
    int fd = open(device_name, O_RDONLY);
    if(fd == -1)
    {
        DEBUGF("ERROR %s: open failed on %s.", __func__, device_name);
        exit(-1);
    }

    struct pollfd* new_fds = realloc(_fds, sizeof(struct pollfd) * (_nfds + 1));
    if(new_fds == NULL)
    {
        DEBUGF("ERROR %s: realloc for _fds failed.", __func__);
        exit(-1);
    }

    _fds = new_fds;
    _fds[_nfds].fd = fd;
    _fds[_nfds].events = POLLIN;

    char** new_device_names = realloc(_device_names, sizeof(char*) * (_nfds + 1));
    if(new_device_names == NULL)
    {
        DEBUGF("ERROR %s: realloc for _device_names failed.", __func__);
        exit(-1);
    }

    _device_names = new_device_names;
    _device_names[_nfds] = strdup(device_name);
    if(_device_names[_nfds] == NULL)
    {
        DEBUGF("ERROR %s: strdup failed.", __func__);
        exit(-1);
    }

    ++_nfds;

    DEBUGF("DEBUG %s: Opened device %s.", __func__, device_name);
}


static void button_init_device(void)
{
    TRACE;

    if((_fds != NULL) || (_nfds != 0) || (_device_names != NULL))
    {
        DEBUGF("ERROR %s: Allready initialized.", __func__);
        return;
    }

    /* The input device directory. */
    static const char device_path[] = "/dev/input";

    /* Path delimeter. */
    static const char delimeter[] = "/";

    /* Open all devices in device_path. */
    DIR* dir = opendir(device_path);
    if(dir == NULL)
    {
        DEBUGF("ERROR %s: opendir failed: errno: %d.", __func__, errno);
        exit(errno);
    }

    char device_name[PATH_MAX];
    strcpy(device_name, device_path);
    strcat(device_name, delimeter);
    char* device_name_idx = device_name + strlen(device_name);

    struct dirent* dir_entry;
    while((dir_entry = readdir(dir)))
    {
        if(   ((dir_entry->d_name[0] == '.') && (dir_entry->d_name[1] == '\0'))
           || ((dir_entry->d_name[0] == '.') && (dir_entry->d_name[1] == '.') && (dir_entry->d_name[2] == '\0')))
        {
            continue;
        }

        strcpy(device_name_idx, dir_entry->d_name);

        /* Open and add device to _fds. */
        open_device(device_name);
    }

    closedir(dir);

    /* Sanity check. */
    if(_nfds < 2)
    {
        DEBUGF("ERROR %s: No input devices.", __func__);
        exit(-1);
    }
}


#define EVENT_TYPE_BUTTON 1


#define EVENT_CODE_BUTTON_PWR_LONG 117
#define EVENT_CODE_BUTTON_REV      160
#define EVENT_CODE_BUTTON_NEXT     162


#define EVENT_TYPE_TOUCHSCREEN 3


#define EVENT_CODE_TOUCHSCREEN_X 53


enum user_choice
{
    CHOICE_NONE = -1,
    CHOICE_MANGO,
    CHOICE_ROCKBOX,
    CHOICE_POWEROFF
};


static int get_user_choice(void)
{
    TRACE;

    button_init_device();

    enum user_choice choice = CHOICE_NONE;

    while(choice == CHOICE_NONE)
    {
        /* Poll all input devices. */
        poll(_fds, _nfds, 0);

        nfds_t fds_idx = 0;
        for( ; fds_idx < _nfds; ++fds_idx)
        {
            if(! (_fds[fds_idx].revents & POLLIN))
            {
                continue;
            }

            struct input_event event;
            if(read(_fds[fds_idx].fd, &event, sizeof(event)) < (int) sizeof(event))
            {
                DEBUGF("ERROR %s: Read of input devices failed.", __func__);
                continue;
            }

            DEBUGF("DEBUG %s: device: %s, event.type: %d, event.code: %d, event.value: %d", __func__, _device_names[fds_idx], event.type, event.code, event.value);

            if(event.type == EVENT_TYPE_BUTTON)
            {
                switch(event.code)
                {
                    case EVENT_CODE_BUTTON_REV:
                    {
                        choice = CHOICE_MANGO;
                        break;
                    }

                    case EVENT_CODE_BUTTON_NEXT:
                    {
                        choice = CHOICE_ROCKBOX;
                        break;
                    }

                    case EVENT_CODE_BUTTON_PWR_LONG:
                    {
                        choice = CHOICE_POWEROFF;
                        break;
                    }
                }
            }
            else if((event.type == EVENT_TYPE_TOUCHSCREEN) && (event.code == EVENT_CODE_TOUCHSCREEN_X))
            {
                if(event.value < 160)
                {
                    choice = CHOICE_MANGO;
                }
                else
                {
                    choice = CHOICE_ROCKBOX;
                }
            }
        }
    }

    if(_fds)
    {
        nfds_t fds_idx = 0;
        for( ; fds_idx < _nfds; ++fds_idx)
        {
            close(_fds[fds_idx].fd);
        }
        free(_fds);
        _fds = NULL;
    }

    if(_device_names)
    {
        nfds_t fds_idx = 0;
        for( ; fds_idx < _nfds; ++fds_idx)
        {
            free(_device_names[fds_idx]);
        }
        free(_device_names);
        _device_names = NULL;
    }

    _nfds = 0;

    return choice;
}


/*
    Changing bit, when hold switch is toggled.
    Bit is off when hold switch is engaged.
*/
#define HOLD_SWITCH_BIT 16


static bool check_for_hold(void)
{
    TRACE;

    char hold_state;

    FILE* f = fopen("/sys/class/axppower/holdkey", "r");
    fscanf(f, "%c", &hold_state);
    fclose(f);

    return(! (hold_state & HOLD_SWITCH_BIT));
}


/*- Display -------------------------------------------------------------------------------------*/


static void draw(const char* file)
{
    DEBUGF("DEBUG %s: file: %s.", __func__, file);

    int dev_fd = open("/dev/graphics/fb0", O_RDWR);
    if(dev_fd == -1)
    {
        DEBUGF("ERROR %s: open failed on /dev/graphics/fb0, errno: %d.", __func__, errno);
        exit(errno);
    }

    /* Get fixed screen information. */
    struct fb_fix_screeninfo finfo;
    if(ioctl(dev_fd, FBIOGET_FSCREENINFO, &finfo) < 0)
    {
        DEBUGF("ERROR %s: ioctl FBIOGET_FSCREENINFO failed on /dev/graphics/fb0, errno: %d.", __func__, errno);
        exit(errno);
    }

    /* Get the changeable information. */
    struct fb_var_screeninfo vinfo;
    if(ioctl(dev_fd, FBIOGET_VSCREENINFO, &vinfo) < 0)
    {
        DEBUGF("ERROR %s: ioctl FBIOGET_VSCREENINFO failed on /dev/graphics/fb0, errno: %d.", __func__, errno);
        exit(errno);
    }

    DEBUGF("DEBUG %s: bits_per_pixel: %u, width: %u, height: %u.", __func__, vinfo.bits_per_pixel, vinfo.width, vinfo.height);

    size_t screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

    /* ToDo: Is this needed? */
    vinfo.xres             = 320;
    vinfo.xres_virtual     = 320;
    vinfo.width            = 320;
    vinfo.yres             = 240;
    vinfo.yres_virtual     = 240;
    vinfo.height           = 240;
    vinfo.xoffset          = 0;
    vinfo.yoffset          = 0;
    vinfo.sync             = 0;
    vinfo.vmode            = 0;
    vinfo.pixclock         = 104377;
    vinfo.left_margin      = 20;
    vinfo.right_margin     = 50;
    vinfo.upper_margin     = 2;
    vinfo.lower_margin     = 4;
    vinfo.hsync_len        = 10;
    vinfo.vsync_len        = 2;
    vinfo.red.offset       = 11;
    vinfo.red.length       = 5;
    vinfo.red.msb_right    = 0;
    vinfo.green.offset     = 5;
    vinfo.green.length     = 6;
    vinfo.green.msb_right  = 0;
    vinfo.blue.offset      = 0;
    vinfo.blue.length      = 5;
    vinfo.blue.msb_right   = 0;
    vinfo.transp.offset    = 0;
    vinfo.transp.length    = 0;
    vinfo.transp.msb_right = 0;
    vinfo.nonstd           = 4;
    if(ioctl(dev_fd, FBIOPUT_VSCREENINFO, &vinfo) < 0)
    {
        DEBUGF("ERROR %s: ioctl FBIOPUT_VSCREENINFO failed on /dev/graphics/fb0, errno: %d.", __func__, errno);
        exit(errno);
    }

    /* Map the device to memory. */
    char* dev_fb = (char*) mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, dev_fd, 0);
    if(dev_fb == MAP_FAILED)
    {
        DEBUGF("ERROR %s: mmap failed on /dev/graphics/fb0, errno: %d.", __func__, errno);
        exit(errno);
    }

    BMP* bmp = BMP_ReadFile(file);
    if(BMP_GetError() != BMP_OK )
    {
        DEBUGF("ERROR %s: BMP_ReadFile failed on %s: %d.", __func__, file, BMP_GetError());
        exit(BMP_GetError());
    }

    int y = 0;
    for( ; y < 240; ++y)
    {
        int x = 0;
        for( ; x < 320; ++x)
        {
            long int position =   (x + vinfo.xoffset) * (vinfo.bits_per_pixel / 8 )
                                + (y + vinfo.yoffset) * finfo.line_length;
            UCHAR r, g, b;
            BMP_GetPixelRGB(bmp, x, y, &r, &g, &b);
            unsigned short int pixel = (r >> 3) << 11 | (g >> 2) << 5 | (b >> 3);
            *((unsigned short int*)(dev_fb + position)) = pixel;
        }
    }

    BMP_Free(bmp);
    munmap(dev_fb, screensize);
    close(dev_fd);
}


/*-----------------------------------------------------------------------------------------------*/


static const char ROCKBOX_BIN[]   = "/mnt/sdcard/.rockbox/rockbox";
static const char OF_PLAYER_BIN[] = "/system/bin/MangoPlayer_original";
static const char PLAYER_FILE[]   = "/data/chosen_player";


int main(int argc, char **argv)
{
    TRACE;

    /*
        Create the iBasso Vold socket and monitor it. 
        Do this early to not block Vold.
    */
    vold_monitor_start();

    int last_chosen_player = CHOICE_NONE;

    FILE* f = fopen(PLAYER_FILE, "r");
    if(f != NULL)
    {
        fscanf(f, "%d", &last_chosen_player);
        fclose(f);
    }

    DEBUGF("DEBUG %s: Current player choice: %d.", __func__, last_chosen_player);

    if(check_for_hold() || (last_chosen_player == CHOICE_NONE))
    {
        draw("/system/chooser.bmp");

        enum user_choice choice = get_user_choice();

        if(choice == CHOICE_POWEROFF)
        {
            reboot(RB_POWER_OFF);
            while(true)
            {
                sleep(1);
            }
        }

        if(choice != last_chosen_player)
        {
            last_chosen_player = choice;

            f = fopen(PLAYER_FILE, "w");
            fprintf(f, "%d", last_chosen_player);
            fclose(f);
        }

        DEBUGF("DEBUG %s: New player choice: %d.", __func__, last_chosen_player);
    }

    /* true, Rockbox was started at least once. */
    bool rockboxStarted = false;

    while(true)
    {
        /* Excecute OF MangoPlayer or Rockbox and restart it if it crashes. */

        if(last_chosen_player == CHOICE_ROCKBOX)
        {
            if(rockboxStarted)
            {
                /*
                    At this point it is assumed, that Rockbox has exited due to a USB connection
                    triggering a remount of the internal storage for mass storage access.
                    Rockbox will eventually restart, when /mnt/sdcard becomes available again.
                */
                draw("/system/usb.bmp");
            }

            pthread_mutex_lock(&_sdcard_mount_mtx);
            while(_sdcard_not_mounted)
            {
                DEBUGF("DEBUG %s: Waiting on /mnt/sdcard/.", __func__);

                pthread_cond_wait(&_sdcard_mount_cond, &_sdcard_mount_mtx);

                DEBUGF("DEBUG %s: /mnt/sdcard/ available.", __func__);
            }
            pthread_mutex_unlock(&_sdcard_mount_mtx);

            /* To be able to execute rockbox. */
            system("mount -o remount,exec /mnt/sdcard");

            /* This symlink is needed mainly to keep themes functional. */
            system("ln -s /mnt/sdcard/.rockbox /.rockbox");

            if(access(ROCKBOX_BIN, X_OK) != -1)
            {
                /* Start Rockbox. */

                /* Rockbox has its own vold monitor. */
                vold_monitor_stop();

                DEBUGF("DEBUG %s: Excecuting %s.", __func__, ROCKBOX_BIN);

                int ret_code = system(ROCKBOX_BIN);
                rockboxStarted = true;

                DEBUGF("DEBUG %s: ret_code: %d.", __func__, ret_code);

                if(WIFEXITED(ret_code) && (WEXITSTATUS(ret_code) == 42))
                {
                    /*
                        Rockbox terminated to prevent a froced shutdown due to a USB connection
                        triggering a remount of the internal storage for mass storage access.
                    */
                    _sdcard_not_mounted = true;
                }
                /* else Rockbox crashed ... */

                vold_monitor_start();
            }
            else
            {
                /* Rockbox executable missing. Show info screen for 30 seconds. */
                draw("/system/rbmissing.bmp");
                sleep(30);

                /* Do not block Vold, so stop after sleep. */
                vold_monitor_stop();

#ifdef DEBUG
                system("setprop persist.sys.usb.config adb");
                system("setprop persist.usb.debug 1");
#endif

                DEBUGF("DEBUG %s: Rockbox missing, excecuting %s.", __func__, OF_PLAYER_BIN);

                /* Start OF MangoPlayer. */
                int ret_code = system(OF_PLAYER_BIN);

                DEBUGF("DEBUG %s: ret_code: %d.", __func__, ret_code);
            }
        }
        else /* if(last_chosen_player == CHOICE_MANGO) */
        {
            vold_monitor_stop();

            DEBUGF("DEBUG %s: Excecuting %s.", __func__, OF_PLAYER_BIN);

            int ret_code = system(OF_PLAYER_BIN);

            DEBUGF("DEBUG %s: ret_code: %d.", __func__, ret_code);
        }
    }

    return 0;
}
