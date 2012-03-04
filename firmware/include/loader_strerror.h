/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2013 Marcin Bukat
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

enum error_t {
    EFILE_EMPTY = 0,
    EFILE_NOT_FOUND = -1,
    EREAD_CHKSUM_FAILED = -2,
    EREAD_MODEL_FAILED = -3,
    EREAD_IMAGE_FAILED = -4,
    EBAD_CHKSUM = -5,
    EFILE_TOO_BIG = -6,
    EINVALID_FORMAT = -7,
    EKEY_NOT_FOUND = -8,
    EDECRYPT_FAILED = -9,
    EREAD_HEADER_FAILED = -10,
    EBAD_HEADER_CHKSUM = -11,
    EINVALID_LOAD_ADDR = -12,
    EBAD_MODEL = -13
};

char *loader_strerror(enum error_t error);
