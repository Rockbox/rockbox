/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * Module: rbutil
 * File: bootloaders.h
 *
 * Copyright (C) 2007 Dominik Wenger
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/



#ifndef BOOTLOADERS_H_INCLUDED
#define BOOTLOADERS_H_INCLUDED

#include <wx/string.h>
#include "irivertools.h"
#include "md5sum.h"

#include "rbutil.h"
#include "installlog.h"

extern "C" {
    // Ipodpatcher
    #include "ipodpatcher/ipodpatcher.h"
    // Sansapatcher
    #include "sansapatcher/sansapatcher.h"
};


bool initIpodpatcher();
bool initSansaPatcher();
bool ipodpatcher(int mode,wxString bootloadername);
bool sansapatcher(int mode,wxString bootloadername);
bool gigabeatf(int mode,wxString bootloadername,wxString deviceDir);
bool iaudiox5(int mode,wxString bootloadername,wxString deviceDir);
bool fwpatcher(int mode,wxString bootloadername,wxString deviceDir,wxString firmware);
bool h10(int mode,wxString bootloadername,wxString deviceDir);


#endif // BOOTLOADERS_H_INCLUDED
