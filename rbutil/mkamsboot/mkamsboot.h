/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * mkamsboot.h - a tool for merging bootloader code into an Sansa V2
 *               (AMS) firmware file
 *
 * Copyright (C) 2008 Dave Chapman
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

#ifndef MKAMSBOOT_H
#define MKAMSBOOT_H

#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif 

/* Supported models */
enum {
    MODEL_UNKNOWN = -1,
    MODEL_FUZE = 0,
    MODEL_CLIP,
    MODEL_CLIPV2,
    MODEL_E200V2,
    MODEL_M200V4,
    MODEL_C200V2,
};


/* Holds info about the OF */
struct md5sums {
    int model;
    char *version;
    char *md5;
};

extern const unsigned short hw_revisions[];
extern const unsigned short fw_revisions[];
extern const char* model_names[];
extern const int bootloader_sizes[];

/* load_rockbox_file()
 *
 * Loads a rockbox bootloader file into memory
 *
 * ARGUMENTS
 *
 * filename         :   bootloader file to load
 * model            :   a 4 characters string representing the Sansa model
 *                      ("fuze", "clip", "e2v2", "m2v4", or "c2v2")
 * bootloader_size  :   set to the uncompressed bootloader size
 * rb_packed_size   :   set to the size of compressed bootloader
 * errstr           :   provided buffer to store an eventual error
 * errstrsize       :   size of provided error buffer
 *
 * RETURN VALUE
 *  pointer to allocated memory containing the content of compressed bootloader
 *  or NULL in case of error (errstr will hold a description of the error)
 */

unsigned char* load_rockbox_file(
        char* filename, int model, int* bootloader_size, int* rb_packedsize,
        char* errstr, int errstrsize);


/* load_of_file()
 *
 * Loads a Sansa AMS Original Firmware file into memory
 *
 * ARGUMENTS
 *
 * filename         :   firmware file to load
 * bufsize          :   set to firmware file size
 * md5sum           :   set to file md5sum, must be at least 33 bytes long
 * model            :   set to firmware model (MODEL_XXX)
 * fw_version       :   set to firmware format version (1 or 2)
 * firmware_size    :   set to firmware block's size
 * of_packed        :   pointer to allocated memory containing the compressed
 *                      original firmware block
 * of_packedsize    :   size of compressed original firmware block
 * errstr           :   provided buffer to store an eventual error
 * errstrsize       :   size of provided error buffer
 *
 * RETURN VALUE
 *  pointer to allocated memory containing the content of Original Firmware
 *  or NULL in case of error (errstr will hold a description of the error)
 */

unsigned char* load_of_file(
        char* filename, off_t* bufsize, struct md5sums *sum,
        int* firmware_size, unsigned char** of_packed,
        int* of_packedsize, char* errstr, int errstrsize);


/* patch_firmware()
 *
 * Patches a Sansa AMS Original Firmware file
 *
 * ARGUMENTS
 *
 * model            :   firmware model (MODEL_XXX)
 * fw_version       :   firmware format version (1 or 2)
 * firmware_size    :   size of uncompressed original firmware block
 * buf              :   pointer to original firmware file
 * len              :   size of original firmware file
 * of_packed        :   pointer to compressed original firmware block
 * of_packedsize    :   size of compressed original firmware block
 * rb_packed        :   pointer to compressed rockbox bootloader
 * rb_packed_size   :   size of compressed rockbox bootloader
 */

void patch_firmware(
        int model, int fw_version, int firmware_size, unsigned char* buf,
        int len, unsigned char* of_packed, int of_packedsize,
        unsigned char* rb_packed, int rb_packedsize);


/* total_size()
 *
 * Calculates the size of the new firmware block
 *
 * ARGUMENTS
 *
 * model            :   firmware model (MODEL_XXX)
 * rb_packed_size   :   size of compressed rockbox bootloader
 * of_packedsize    :   size of compressed original firmware block
 *
 * RETURN VALUE
 *  Size of new firmware block
*/

int total_size(int model, int rb_packedsize, int of_packedsize);

/* firmware_revision()
 *
 * returns the firmware revision for a particular model
 *
 * ARGUMENTS
 *
 * model            :   firmware model (MODEL_XXX)
 *
 * RETURN VALUE
 *  firmware version
*/
int firmware_revision(int model);

#ifdef __cplusplus
};
#endif 

#endif
