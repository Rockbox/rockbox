/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2021 Aidan MacDonald
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

#ifndef _XF_ERROR_H_
#define _XF_ERROR_H_

enum {
    XF_E_SUCCESS            = 0,
    XF_E_IO                 = -1,
    XF_E_LINE_TOO_LONG      = -2,
    XF_E_FILENAME_TOO_LONG  = -3,
    XF_E_INT_OVERFLOW       = -4,
    XF_E_BUF_OVERFLOW       = -5,
    XF_E_SYNTAX_ERROR       = -6,
    XF_E_INVALID_PARAMETER  = -7,
    XF_E_NAND               = -8,
    XF_E_OUT_OF_MEMORY      = -9,
    XF_E_OUT_OF_RANGE       = -10,
    XF_E_VERIFY_FAILED      = -11,
    XF_E_CANNOT_OPEN_FILE   = -12,
};

const char* xf_strerror(int err);

#endif /* _XF_ERROR_H_ */
