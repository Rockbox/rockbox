/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id: main.c 11997 2007-01-13 09:08:18Z miipekk $
 *
 * Copyright (C) 2005 by Linus Nielsen Feltzing
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/
#include "lcd.h"
#include "lcd-remote.h"
#include "font.h"
#include "system.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdbool.h>
#include "cpu.h"
#include "common.h"
#include "power.h"
#include "kernel.h"

/* TODO: Other bootloaders need to be adjusted to set this variable to true
   on a button press - currently only the ipod, H10 and Sansa versions do. */
#if defined(IPOD_ARCH) || defined(IRIVER_H10)  || defined(IRIVER_H10_5GB) \
 || defined(SANSA_E200) || defined(SANSA_C200) || defined(GIGABEAT_F) \
 || defined(PHILIPS_SA9200)
bool verbose = false;
#else
bool verbose = true;
#endif

int line = 0;
#ifdef HAVE_REMOTE_LCD
int remote_line = 0;
#endif

char printfbuf[256];

void reset_screen(void)
{
    lcd_clear_display();
    line = 0;
#ifdef HAVE_REMOTE_LCD
    lcd_remote_clear_display();
    remote_line = 0;
#endif
}

void printf(const char *format, ...)
{
    int len;
    unsigned char *ptr;
    va_list ap;
    va_start(ap, format);

    ptr = printfbuf;
    len = vsnprintf(ptr, sizeof(printfbuf), format, ap);
    va_end(ap);

    lcd_puts(0, line++, ptr);
    if (verbose)
        lcd_update();
    if(line >= LCD_HEIGHT/SYSFONT_HEIGHT)
        line = 0;
#ifdef HAVE_REMOTE_LCD
    lcd_remote_puts(0, remote_line++, ptr);
    if (verbose)
        lcd_remote_update();
    if(remote_line >= LCD_REMOTE_HEIGHT/SYSFONT_HEIGHT)
        remote_line = 0;
#endif
}

char *strerror(int error)
{
    switch(error)
    {
    case EOK:
        return "OK";
    case EFILE_NOT_FOUND:
        return "File not found";
    case EREAD_CHKSUM_FAILED:
        return "Read failed (chksum)";
    case EREAD_MODEL_FAILED:
        return "Read failed (model)";
    case EREAD_IMAGE_FAILED:
        return "Read failed (image)";
    case EBAD_CHKSUM:
        return "Bad checksum";
    case EFILE_TOO_BIG:
        return "File too big";
    case EINVALID_FORMAT:
        return "Invalid file format";
    default:
        return "Unknown";
    }
}

void error(int errortype, int error)
{
    switch(errortype)
    {
    case EATA:
        printf("ATA error: %d", error);
        break;

    case EDISK:
        printf("No partition found");
        break;

    case EBOOTFILE:
        printf(strerror(error));
        break;
    }

    lcd_update();
    sleep(5*HZ);
    power_off();
}

/* Load firmware image in a format created by tools/scramble */
int load_firmware(unsigned char* buf, char* firmware, int buffer_size)
{
    int fd;
    int rc;
    int len;
    unsigned long chksum;
    char model[5];
    unsigned long sum;
    int i;
    char filename[MAX_PATH];

    snprintf(filename,sizeof(filename),"/.rockbox/%s",firmware);
    fd = open(filename, O_RDONLY);
    if(fd < 0)
    {
        snprintf(filename,sizeof(filename),"/%s",firmware);
        fd = open(filename, O_RDONLY);
        if(fd < 0)
            return EFILE_NOT_FOUND;
    }

    len = filesize(fd) - 8;

    printf("Length: %x", len);

    if (len > buffer_size)
        return EFILE_TOO_BIG;

    lseek(fd, FIRMWARE_OFFSET_FILE_CRC, SEEK_SET);

    rc = read(fd, &chksum, 4);
    chksum=betoh32(chksum); /* Rockbox checksums are big-endian */
    if(rc < 4)
        return EREAD_CHKSUM_FAILED;

    printf("Checksum: %x", chksum);

    rc = read(fd, model, 4);
    if(rc < 4)
        return EREAD_MODEL_FAILED;

    model[4] = 0;

    printf("Model name: %s", model);
    printf("Loading %s", firmware);

    lseek(fd, FIRMWARE_OFFSET_FILE_DATA, SEEK_SET);

    rc = read(fd, buf, len);
    if(rc < len)
        return EREAD_IMAGE_FAILED;

    close(fd);

    sum = MODEL_NUMBER;

    for(i = 0;i < len;i++) {
        sum += buf[i];
    }

    printf("Sum: %x", sum);

    if(sum != chksum)
        return EBAD_CHKSUM;

    return EOK;
}

/* Load raw binary image. */
int load_raw_firmware(unsigned char* buf, char* firmware, int buffer_size)
{
    int fd;
    int rc;
    int len;
    char filename[MAX_PATH];

    snprintf(filename,sizeof(filename),"%s",firmware);
    fd = open(filename, O_RDONLY);
    if(fd < 0)
    {
        return EFILE_NOT_FOUND;
    }

    len = filesize(fd);
    
    if (len > buffer_size)
        return EFILE_TOO_BIG;

    rc = read(fd, buf, len);
    if(rc < len)
        return EREAD_IMAGE_FAILED;

    close(fd);
    return len;
}

/* These functions are present in the firmware library, but we reimplement
   them here because the originals do a lot more than we want */
void reset_poweroff_timer(void)
{
}

int dbg_ports(void)
{
   return 0;
}

void mpeg_stop(void)
{
}

void sys_poweroff(void)
{
}
