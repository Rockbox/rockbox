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

#include "xf_error.h"

const char* xf_strerror(int err)
{
    switch(err) {
    case XF_E_SUCCESS:              return "Success";
    case XF_E_IO:                   return "I/O error";
    case XF_E_LINE_TOO_LONG:        return "Line too long";
    case XF_E_FILENAME_TOO_LONG:    return "Filename too long";
    case XF_E_INT_OVERFLOW:         return "Numeric overflow";
    case XF_E_BUF_OVERFLOW:         return "Buffer overflowed";
    case XF_E_SYNTAX_ERROR:         return "Syntax error";
    case XF_E_INVALID_PARAMETER:    return "Invalid parameter";
    case XF_E_NAND:                 return "NAND flash error";
    case XF_E_OUT_OF_MEMORY:        return "Out of memory";
    case XF_E_OUT_OF_RANGE:         return "Out of range";
    case XF_E_VERIFY_FAILED:        return "Verification failed";
    case XF_E_CANNOT_OPEN_FILE:     return "Cannot open file";
    default:                        return "Unknown error";
    }
}
