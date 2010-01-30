/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2006-2007 Dave Chapman
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
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <errno.h>

#include "ipodio.h"

#if defined(linux) || defined (__linux)
#include <sys/mount.h>
#include <linux/hdreg.h>
#include <scsi/scsi_ioctl.h>
#include <scsi/sg.h>

#define IPOD_SECTORSIZE_IOCTL BLKSSZGET

static void get_geometry(struct ipod_t* ipod)
{
    struct hd_geometry geometry;

    if (!ioctl(ipod->dh, HDIO_GETGEO, &geometry)) {
        /* never use geometry.cylinders - it is truncated */
        ipod->num_heads = geometry.heads;
        ipod->sectors_per_track = geometry.sectors;
    } else {
        ipod->num_heads = 0;
        ipod->sectors_per_track = 0;
    }
}

/* Linux SCSI Inquiry code based on the documentation and example code from
   http://www.ibm.com/developerworks/linux/library/l-scsi-api/index.html
*/

int ipod_scsi_inquiry(struct ipod_t* ipod, int page_code,
                      unsigned char* buf, int bufsize)
{
    unsigned char cdb[6];
    struct sg_io_hdr hdr;
    unsigned char sense_buffer[255];

    memset(&hdr, 0, sizeof(hdr));

    hdr.interface_id = 'S'; /* this is the only choice we have! */
    hdr.flags = SG_FLAG_LUN_INHIBIT; /* this would put the LUN to 2nd byte of cdb*/

    /* Set xfer data */
    hdr.dxferp = buf;
    hdr.dxfer_len = bufsize;

    /* Set sense data */
    hdr.sbp = sense_buffer;
    hdr.mx_sb_len = sizeof(sense_buffer);

    /* Set the cdb format */
    cdb[0] = 0x12;
    cdb[1] = 1;   /* Enable Vital Product Data (EVPD) */
    cdb[2] = page_code & 0xff;
    cdb[3] = 0;
    cdb[4] = 0xff;
    cdb[5] = 0; /* For control filed, just use 0 */

    hdr.dxfer_direction = SG_DXFER_FROM_DEV;
    hdr.cmdp = cdb;
    hdr.cmd_len = 6;

    int ret = ioctl(ipod->dh, SG_IO, &hdr);

    if (ret < 0) {
        return -1;
    } else {
        return 0;
    }
}

#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) \
      || defined(__bsdi__) || defined(__DragonFly__)
#include <sys/disk.h>
#define IPOD_SECTORSIZE_IOCTL DIOCGSECTORSIZE

/* TODO: Implement this function for BSD */
static void get_geometry(struct ipod_t* ipod)
{
    /* Are these universal for all ipods? */
    ipod->num_heads = 255;
    ipod->sectors_per_track = 63;
}

int ipod_scsi_inquiry(struct ipod_t* ipod, int page_code,
                      unsigned char* buf, int bufsize)
{
    /* TODO: Implement for BSD */
    (void)ipod;
    (void)page_code;
    (void)buf;
    (void)bufsize;
    return -1;
}

#elif defined(__APPLE__) && defined(__MACH__)
/* OS X IOKit includes don't like VERSION being defined! */
#undef VERSION
#include <sys/disk.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/scsi-commands/SCSITaskLib.h>
#include <IOKit/scsi-commands/SCSICommandOperationCodes.h>
#define IPOD_SECTORSIZE_IOCTL DKIOCGETBLOCKSIZE

/* TODO: Implement this function for Mac OS X */
static void get_geometry(struct ipod_t* ipod)
{
    /* Are these universal for all ipods? */
    ipod->num_heads = 255;
    ipod->sectors_per_track = 63;
}

int ipod_scsi_inquiry(struct ipod_t* ipod, int page_code,
                      unsigned char* buf, int bufsize)
{
    /* OS X doesn't allow to simply send out a SCSI inquiry request but
     * requires registering an interface handler first.
     * Currently this is done on each inquiry request which is somewhat
     * inefficient but the current ipodpatcher API doesn't really fit here.
     * Based on the documentation in Apple's document
     * "SCSI Architecture Model Device Interface Guide".
     *
     * WARNING: this code currently doesn't take the selected device into
     *          account. It simply looks for an Ipod on the system and uses
     *          the first match.
     */
    (void)ipod;
    int result = 0;
    /* first, create a dictionary to match the device. This is needed to get the
     * service. */
    CFMutableDictionaryRef match_dict;
    match_dict = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, NULL, NULL);
    if(match_dict == NULL)
        return -1;

    /* set value to match. In case of the Ipod this is "iPodUserClientDevice". */
    CFMutableDictionaryRef sub_dict;
    sub_dict = CFDictionaryCreateMutable(kCFAllocatorDefault, 0, NULL, NULL);
    CFDictionarySetValue(sub_dict, CFSTR(kIOPropertySCSITaskDeviceCategory),
                         CFSTR("iPodUserClientDevice"));
    CFDictionarySetValue(match_dict, CFSTR(kIOPropertyMatchKey), sub_dict);

    if(sub_dict == NULL)
        return -1;

    /* get an iterator for searching for the service. */
    kern_return_t kr;
    io_iterator_t iterator = IO_OBJECT_NULL;
    /* get matching services from IO registry. Consumes one reference to
     * the dictionary, so no need to release that. */
    kr = IOServiceGetMatchingServices(kIOMasterPortDefault, match_dict, &iterator);

    if(!iterator | (kr != kIOReturnSuccess))
        return -1;

    /* get interface and obtain exclusive access */
    SInt32 score;
    HRESULT herr;
    kern_return_t err;
    IOCFPlugInInterface **plugin_interface = NULL;
    SCSITaskDeviceInterface **interface = NULL;
    io_service_t device = IO_OBJECT_NULL;
    device = IOIteratorNext(iterator);

    err = IOCreatePlugInInterfaceForService(device, kIOSCSITaskDeviceUserClientTypeID,
                                            kIOCFPlugInInterfaceID, &plugin_interface,
                                            &score);

    if(err != noErr) {
        return -1;
    }
    /* query the plugin interface for task interface */
    herr = (*plugin_interface)->QueryInterface(plugin_interface,
                            CFUUIDGetUUIDBytes(kIOSCSITaskDeviceInterfaceID), (LPVOID*)&interface);
    if(herr != S_OK) {
        IODestroyPlugInInterface(plugin_interface);
        return -1;
    }

    err = (*interface)->ObtainExclusiveAccess(interface);
    if(err != noErr) {
        (*interface)->Release(interface);
        IODestroyPlugInInterface(plugin_interface);
        return -1;
    }

    /* do the inquiry */
    SCSITaskInterface **task = NULL;

    task = (*interface)->CreateSCSITask(interface);
    if(task != NULL) {
        kern_return_t err;
        SCSITaskStatus task_status;
        IOVirtualRange* range;
        SCSI_Sense_Data sense_data;
        SCSICommandDescriptorBlock cdb;
        UInt64 transfer_count = 0;
        memset(buf, 0, bufsize);
        /* allocate virtual range for buffer. */
        range = (IOVirtualRange*) malloc(sizeof(IOVirtualRange));
        memset(&sense_data, 0, sizeof(sense_data));
        memset(cdb, 0, sizeof(cdb));
        /* set up range. address is buffer address, length is request size. */
        range->address = (IOVirtualAddress)buf;
        range->length = bufsize;
        /* setup CDB */
        cdb[0] = 0x12; /* inquiry */
        cdb[1] = 1;
        cdb[2] = page_code;
        cdb[4] = bufsize;

        /* set cdb in task */
        err = (*task)->SetCommandDescriptorBlock(task, cdb, kSCSICDBSize_6Byte);
        if(err != kIOReturnSuccess) {
            result = -1;
            goto cleanup;
        }
        err = (*task)->SetScatterGatherEntries(task, range, 1, bufsize,
                kSCSIDataTransfer_FromTargetToInitiator);
        if(err != kIOReturnSuccess) {
            result = -1;
            goto cleanup;
        }
        /* set timeout */
        err = (*task)->SetTimeoutDuration(task, 10000);
        if(err != kIOReturnSuccess) {
            result = -1;
            goto cleanup;
        }

        /* request data */
        err = (*task)->ExecuteTaskSync(task, &sense_data, &task_status, &transfer_count);
        if(err != kIOReturnSuccess) {
            result = -1;
            goto cleanup;
        }
        /* cleanup */
        free(range);

        /* release task interface */
        (*task)->Release(task);
    }
    else {
        result = -1;
    }
cleanup:
    /* cleanup interface */
    (*interface)->ReleaseExclusiveAccess(interface);
    (*interface)->Release(interface);
    IODestroyPlugInInterface(plugin_interface);

    return result;
}

#else
    #error No sector-size detection implemented for this platform
#endif

#if defined(__APPLE__) && defined(__MACH__)
static int ipod_unmount(struct ipod_t* ipod)
{
    char cmd[4096];
    int res;

    sprintf(cmd, "/usr/sbin/diskutil unmount \"%ss2\"",ipod->diskname);
    fprintf(stderr,"[INFO] ");
    res = system(cmd);

    if (res==0) {
        return 0;
    } else {
        perror("Unmount failed");
        return -1;
    }
}
#endif

void ipod_print_error(char* msg)
{
    perror(msg);
}

int ipod_open(struct ipod_t* ipod, int silent)
{
    ipod->dh=open(ipod->diskname,O_RDONLY);
    if (ipod->dh < 0) {
        if (!silent) perror(ipod->diskname);
        if(errno == EACCES) return -2;
        else return -1;
    }

    /* Read information about the disk */

    if(ioctl(ipod->dh,IPOD_SECTORSIZE_IOCTL,&ipod->sector_size) < 0) {
        ipod->sector_size=512;
        if (!silent) {
            fprintf(stderr,"[ERR] ioctl() call to get sector size failed, defaulting to %d\n"
                   ,ipod->sector_size);
        }
    }

    get_geometry(ipod);

    return 0;
}


int ipod_reopen_rw(struct ipod_t* ipod)
{
#if defined(__APPLE__) && defined(__MACH__)
    if (ipod_unmount(ipod) < 0)
        return -1;
#endif

    close(ipod->dh);
    ipod->dh=open(ipod->diskname,O_RDWR);
    if (ipod->dh < 0) {
        perror(ipod->diskname);
        return -1;
    }
    return 0;
}

int ipod_close(struct ipod_t* ipod)
{
    close(ipod->dh);
    return 0;
}

int ipod_alloc_buffer(unsigned char** sectorbuf, int bufsize)
{
    *sectorbuf=malloc(bufsize);
    if (*sectorbuf == NULL) {
        return -1;
    }
    return 0;
}

int ipod_seek(struct ipod_t* ipod, unsigned long pos)
{
    off_t res;

    res = lseek(ipod->dh, pos, SEEK_SET);

    if (res == -1) {
       return -1;
    }
    return 0;
}

ssize_t ipod_read(struct ipod_t* ipod, unsigned char* buf, int nbytes)
{
    return read(ipod->dh, buf, nbytes);
}

ssize_t ipod_write(struct ipod_t* ipod, unsigned char* buf, int nbytes)
{
    return write(ipod->dh, buf, nbytes);
}

