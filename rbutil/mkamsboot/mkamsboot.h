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
    MODEL_CLIPPLUS,
    MODEL_FUZEV2,
    /* new models go here */

    NUM_MODELS
};


/* Holds info about the OF */
struct md5sums {
    int model;
    char *version;
    char *md5;
};

struct ams_models {
    unsigned short hw_revision;
    unsigned short fw_revision;
    /* Descriptive name of this model */
    const char* model_name;
    /* Dualboot functions for this model */
    const unsigned char* bootloader;
    /* Size of dualboot functions for this model */
    int bootloader_size;
    /* Model name used in the Rockbox header in ".sansa" files - these match the
       -add parameter to the "scramble" tool */
    const char* rb_model_name;
    /* Model number used to initialise the checksum in the Rockbox header in
       ".sansa" files - these are the same as MODEL_NUMBER in config-target.h */
    const int rb_model_num;
};
extern const struct ams_models ams_identity[];

/* load_rockbox_file()
 *
 * Loads a rockbox bootloader file into memory
 *
 * ARGUMENTS
 *
 * filename         :   bootloader file to load
 * model            :   will be set to this bootloader's model
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
        char* filename, int *model, int* bootloader_size, int* rb_packedsize,
        char* errstr, int errstrsize);


/* load_of_file()
 *
 * Loads a Sansa AMS Original Firmware file into memory
 *
 * ARGUMENTS
 *
 * filename         :   firmware file to load
 * model            :   desired player's model
 * bufsize          :   set to firmware file size
 * md5sum           :   set to file md5sum, must be at least 33 bytes long
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
        char* filename, int model, off_t* bufsize, struct md5sums *sum,
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


/* check_sizes()
 *
 * Verify if the given bootloader can be embedded in the OF file, while still
 * allowing both the bootloader and the OF to be unpacked at runtime
 *
 * ARGUMENTS
 *
 * model            :   firmware model (MODEL_XXX)
 * rb_packed_size   :   size of compressed rockbox bootloader
 * rb_unpacked_size :   size of compressed rockbox bootloader
 * of_packed_size   :   size of compressed original firmware block
 * of_unpacked_size :   size of compressed original firmware block
 * total_size       :   will contain the size of useful data that would be
 *                       written to the firmware block, even in case of an
 *                       error
 * errstr           :   provided buffer to store an eventual error
 * errstrsize       :   size of provided error buffer
 *
 * RETURN VALUE
 *  0 if the conditions aren't met, 1 if we can go and patch the firmware
*/

int check_sizes(int model, int rb_packed_size, int rb_unpacked_size,
        int of_packed_size, int of_unpacked_size, int *total_size,
        char *errstr, int errstrsize);

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
