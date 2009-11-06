/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2009 Dave Chapman
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 *
 * Based on the getCapsUsingSCSIPassThrough() function from "cddrv.cpp":
 *    - http://www.farmanager.com/svn/trunk/unicode_far/cddrv.cpp
 *
 * Copyright (c) 1996 Eugene Roshal
 * Copyright (c) 2000 Far Group
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the authors may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#include <windows.h>
#include <stddef.h>
#include <stdio.h>
#include <ddk/ntddscsi.h>

#include "ipodio.h"

typedef struct _SCSI_PASS_THROUGH_WITH_BUFFERS {
    SCSI_PASS_THROUGH Spt;
    ULONG Filler;   /* realign buffers to double word boundary */
    UCHAR SenseBuf[32];
    UCHAR DataBuf[512];
} SCSI_PASS_THROUGH_WITH_BUFFERS, *PSCSI_PASS_THROUGH_WITH_BUFFERS;

int ipod_scsi_inquiry(struct ipod_t* ipod, int page_code,
                      unsigned char* buf, int bufsize)
{
    SCSI_PASS_THROUGH_WITH_BUFFERS sptwb;
    ULONG length;
    DWORD returned;
    BOOL  status;

    if (bufsize > 255) {
        fprintf(stderr,"[ERR]  Invalid bufsize in ipod_scsi_inquiry\n");
        return -1;
    }

    memset(&sptwb, 0, sizeof(sptwb));

    sptwb.Spt.Length = sizeof(SCSI_PASS_THROUGH);
    sptwb.Spt.PathId = 0;
    sptwb.Spt.TargetId = 1;
    sptwb.Spt.Lun = 0;
    sptwb.Spt.CdbLength = 6;
    sptwb.Spt.SenseInfoLength = 32; /* sbuf size */;
    sptwb.Spt.DataIn = SCSI_IOCTL_DATA_IN;
    sptwb.Spt.DataTransferLength = bufsize;
    sptwb.Spt.TimeOutValue = 2;  /* 2 seconds */
    sptwb.Spt.DataBufferOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS, DataBuf);
    sptwb.Spt.SenseInfoOffset = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS, SenseBuf);
    length = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS, DataBuf) + 
             sptwb.Spt.DataTransferLength;

    /* Set cdb info */
    sptwb.Spt.Cdb[0] = 0x12;  /* SCSI Inquiry */
    sptwb.Spt.Cdb[1] = 1;
    sptwb.Spt.Cdb[2] = page_code;
    sptwb.Spt.Cdb[3] = 0;
    sptwb.Spt.Cdb[4] = bufsize;
    sptwb.Spt.Cdb[5] = 0;

    status = DeviceIoControl(ipod->dh,
                             IOCTL_SCSI_PASS_THROUGH,
                             &sptwb,
                             sizeof(SCSI_PASS_THROUGH),
                             &sptwb,
                             length,
                             &returned,
                             FALSE);

    if (status) {
        /* W32 sometimes returns more bytes with additional garbage.
         * Make sure to not copy that garbage. */
        memcpy(buf, sptwb.DataBuf,
               (DWORD)bufsize >= returned ? returned : (DWORD)bufsize);
        return 0;
    } else {
        return -1;
    }
}
