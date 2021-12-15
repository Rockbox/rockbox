/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id:$
 *
 * Copyright (C) 2012 by Andrew Ryabinin
 *
 * based on firmware/test/fat/main.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <errno.h>
#include "disk.h"
#include "dir.h"
#include "file.h"
#include "ata.h"
#include "storage.h"
#include "mkrk27boot.h"


const char *img_filename;
static char mkrk27_error_msg[256];

static void mkrk27_set_error(const char *msg, ...) {
    va_list ap;
    va_start(ap, msg);
    snprintf(mkrk27_error_msg, sizeof(mkrk27_error_msg), msg, ap);
    va_end(ap);
    return;
}



/* Extracts in_file from FAT image to out_file */
static int extract_file(const char *in_file, const char* out_file) {
    char buf[81920];
    int nread;

    int fd = open(in_file, O_RDONLY);
    if (fd < 0) {
        mkrk27_set_error("Failed to open file %s. ", in_file);
        return -1;
    }

    FILE *file = fopen(out_file, "wb");
    if (!file) {
        mkrk27_set_error("Failed to open file %s. ", out_file);
        close(fd);
        return -1;
    }

    while (nread = read(fd, buf, sizeof buf), nread > 0) {
        char *out_ptr = buf;
        int nwritten;

        do {
            nwritten = fwrite(buf, 1, nread, file);

            if (nwritten >= 0) {
                nread -= nwritten;
                out_ptr += nwritten;
            } else if (errno != EINTR) {
                mkrk27_set_error("Writting to file %s failed.", out_file);
                goto exit;
            }
        } while(nread > 0);
    }

    if (nread == 0) {
        fclose(file);
        close(fd);
        return 0;
    } else {
        mkrk27_set_error("Copy from %s to %s failed.", out_file, in_file);
    }
exit:
    fclose(file);
    close(fd);
    return -1;
}

/* Replace out_file in FAT image with in_file */
static int replace_file(const char *in_file, const char *out_file) {
    char buf[81920];
    int fd;
    int nread;

    fd = creat(out_file, 0666);
    if (fd < 0) {
        mkrk27_set_error("Failed to open file %s. ", out_file);
        return -1;
    }

    FILE *file = fopen(in_file, "rb");
    if (!file) {
        mkrk27_set_error("Failed to open file %s. ", in_file);
        close(fd);
        return -1;
    }

    while (nread = fread(buf, 1, sizeof buf, file), nread > 0) {
        char *out_ptr = buf;
        int nwritten;

        do {
            nwritten = write(fd, buf, nread);
            if (nwritten >= 0) {
                nread -= nwritten;
                out_ptr += nwritten;
            } else {
                mkrk27_set_error("Writting to file %s failed.", out_file);
                goto exit;
            }
        } while(nread > 0);
    }

    if (nread == 0) {
        fclose(file);
        close(fd);
        return 0;
    } else {
        mkrk27_set_error("Replacing %s with %s failed.", out_file, in_file);
    }

exit:
    fclose(file);
    close(fd);
    return -1;
}

static int mkrk27_init(const char *filename) {
    int i;
    int rc;
    srand(clock());

    img_filename = filename;

    if(ata_init()) {
        mkrk27_set_error("Warning! The disk is uninitialized\n");
        return -1;
    }

    struct partinfo *pinfo = disk_init();

    if (!pinfo) {
        mkrk27_set_error("Failed reading partitions\n");
        return -1;
    }

    for ( i=0; i<4; i++ ) {
        if ( pinfo[i].type == PARTITION_TYPE_FAT32
#ifdef HAVE_FAT16SUPPORT
             || pinfo[i].type == PARTITION_TYPE_FAT16
#endif
            ) {
            rc = fat_mount(IF_MV(0,) IF_MD(0,) pinfo[i].start);
            if(rc) {
                mkrk27_set_error("mount: %d",rc);
                return -1;
            }
            break;
        }
    }
    if ( i==4 ) {
        if(fat_mount(IF_MV(0,) IF_MD(0,) 0)) {
            mkrk27_set_error("FAT32 partition!");
            return -1;
        }
    }
    return 0;
}

extern void ata_exit(void);

static void mkrk27_deinit(void) {
    ata_exit();
}

/* copy file */
static int copy(const char *to, const char *from) {
    FILE* fd_to, *fd_from;
    char buf[4096];
    ssize_t nread;

    if (to == from) {
        return 0;
    }

    fd_from = fopen(from, "rb");
    if (!fd_from) {
        mkrk27_set_error("Failed to open file %s.", from);
        return -1;
    }

    fd_to = fopen(to, "wb");
    if (!fd_to) {
        mkrk27_set_error("Failed to open file %s.", to);
        goto out_error;
    }

    while (nread = fread(buf, 1, sizeof buf, fd_from), nread > 0) {
        char *out_ptr = buf;
        ssize_t nwritten;

        do {
            nwritten = fwrite(out_ptr, 1, nread, fd_to);

            if (nwritten >= 0)
            {
                nread -= nwritten;
                out_ptr += nwritten;
            }
            else if (errno != EINTR)
            {
                mkrk27_set_error( "Writing to file %s failed.", to);
                goto out_error;
            }
        } while (nread > 0);
    }

    if (nread == 0) {
        fclose(fd_to);
        fclose(fd_from);
        return 0;
    } else {
        mkrk27_set_error("Copy from %s to %s failed.", from, to);
    }

out_error:

    fclose(fd_from);
    fclose(fd_to);

    return -1;
}

char* mkrk27_get_error(void) {
    return mkrk27_error_msg;
}


/* Patch rk27 firmware.
    - img_file - original FAT image file, containing OF,
    - rkw_file - rkw file which will replace BASE.RKW from OF,
    - out_img_file - patched img file,
    - out_rkw_file - BASE.RKW extracted from OF.
*/
int mkrk27_patch(const char* img_file, const char* rkw_file, const char* out_img_file, const char* out_rkw_file) {
    if (copy(out_img_file, img_file)) {
        return -1;
    }
    if (mkrk27_init(out_img_file)) {
        return -1;
    }
    if (extract_file("/SYSTEM/BASE.RKW", out_rkw_file)) {
        return -1;
    }
    if (replace_file(rkw_file, "/SYSTEM/BASE.RKW") ||
        replace_file(rkw_file, "/SYSTEM00/BASE.RKW")) {
        return -1;
    }
    mkrk27_deinit();
    return 0;
}
