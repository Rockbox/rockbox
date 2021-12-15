/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2015 by Cástor Muñoz
 *
 * based on:
 *  ipoddfu_c by user890104
 *  xpwn/pwnmetheus2
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
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#ifdef WIN32
#include <windows.h>
#include <setupapi.h>
#endif
#ifdef USE_LIBUSBAPI
#include <libusb-1.0/libusb.h>
#endif
#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOCFPlugIn.h>
#include <IOKit/usb/IOUSBLib.h>
#endif

#include "mks5lboot.h"


#ifdef WIN32
#define sleep_ms(ms) Sleep(ms)
#else
#include <time.h>
static void sleep_ms(unsigned int ms)
{
   struct timespec req;
   req.tv_sec = ms / 1000;
   req.tv_nsec = (ms % 1000) * 1000000;
   nanosleep(&req, NULL);
}
#endif

static void put_uint32le(unsigned char* p, uint32_t x)
{
    p[0] = x & 0xff;
    p[1] = (x >> 8) & 0xff;
    p[2] = (x >> 16) & 0xff;
    p[3] = (x >> 24) & 0xff;
}

/*
 * CRC32 functions
 * Based on public domain implementation by Finn Yannick Jacobs.
 *
 * Written and copyright 1999 by Finn Yannick Jacobs
 * No rights were reserved to this, so feel free to
 * manipulate or do with it, what you want or desire :)
 */

/* crc32table[] built by crc32_init() */
static uint32_t crc32table[256];

/* Calculate crc32 */
static uint32_t crc32(void *data, unsigned int len, uint32_t previousCrc32)
{
    uint32_t crc = ~previousCrc32;
    unsigned char *d = (unsigned char*) data;
    while (len--)
        crc = (crc >> 8) ^ crc32table[(crc & 0xFF) ^ *d++];
    return ~crc;
}

/* Calculate crc32table */
static void crc32_init()
{
    uint32_t poly = 0xEDB88320L;
    uint32_t crc;
    int i, j;
    for (i = 0; i < 256; ++i)
    {
        crc = i;
        for (j = 0; j < 8; ++j)
            crc = (crc >> 1) ^ ((crc & 1) ? poly : 0);
        crc32table[i] = crc;
    }
}

/* USB */
#define APPLE_VID   0x05AC

struct pid_info {
    int pid;
    int mode; /* 0->DFU, 1->WTF */
    char *desc;
};

struct pid_info known_pids[] =
{
    /* DFU */
    { 0x1220, 0, "Nano 2G" },
    { 0x1223, 0, "Nano 3G / Classic" },
    { 0x1224, 0, "Shuffle 3G" },
    { 0x1225, 0, "Nano 4G" },
    { 0x1231, 0, "Nano 5G" },
    { 0x1232, 0, "Nano 6G" },
    { 0x1233, 0, "Shuffle 4G" },
    { 0x1234, 0, "Nano 7G" },
    /* WTF */
    { 0x1240, 1, "Nano 2G" },
    { 0x1241, 1, "Classic 1G" },
    { 0x1242, 1, "Nano 3G" },
    { 0x1243, 1, "Nano 4G" },
    { 0x1245, 1, "Classic 2G" },
    { 0x1246, 1, "Nano 5G" },
    { 0x1247, 1, "Classic 3G" },
    { 0x1248, 1, "Nano 6G" },
    { 0x1249, 1, "Nano 7G" },
    { 0x124a, 1, "Nano 7G" },
    { 0x1250, 1, "Classic 4G" },
};
#define N_KNOWN_PIDS (sizeof(known_pids)/sizeof(struct pid_info))

struct usbControlSetup {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
} __attribute__ ((packed));
#define USB_CS_SZ (sizeof(struct usbControlSetup))

struct usbStatusData {
    uint8_t bStatus;
    uint8_t bwPollTimeout0;
    uint8_t bwPollTimeout1;
    uint8_t bwPollTimeout2;
    uint8_t bState;
    uint8_t iString;
} __attribute__ ((packed));


/*
 * DFU API
 */
#define DFU_PKT_SZ  2048    /* must be pow2 <= wTransferSize (2048) */

/* DFU 1.1 specs */
typedef enum {
    appIDLE = 0,
    appDETACH = 1,
    dfuIDLE = 2,
    dfuDNLOAD_SYNC = 3,
    dfuDNBUSY = 4,
    dfuDNLOAD_IDLE = 5,
    dfuMANIFEST_SYNC = 6,
    dfuMANIFEST = 7,
    dfuMANIFEST_WAIT_RESET = 8,
    dfuUPLOAD_IDLE = 9,
    dfuERROR = 10
} DFUState;

typedef enum {
    errNONE = 0,
    errTARGET = 1,
    errFILE = 2,
    errWRITE = 3,
    errERASE = 4,
    errCHECK_ERASED = 5,
    errPROG = 6,
    errVERIFY = 7,
    errADDRESS = 8,
    errNOTDONE = 9,
    errFIRMWARE = 10,
    errVENDOR = 11,
    errUSBR = 12,
    errPOR = 13,
    errUNKNOWN = 14,
    errSTALLEDPKT = 15
} DFUStatus;

typedef enum {
    DFU_DETACH = 0,
    DFU_DNLOAD = 1,
    DFU_UPLOAD = 2,
    DFU_GETSTATUS = 3,
    DFU_CLRSTATUS = 4,
    DFU_GETSTATE = 5,
    DFU_ABORT = 6
} DFURequest;

typedef enum {
    DFUAPIFail = 0,
    DFUAPISuccess,
} dfuAPIResult;

struct dfuDev {
    struct dfuAPI *api;
    int found_pid;
    int detached;
    char descr[256];
    dfuAPIResult res;
    char err[256];
    /* API private */
#ifdef WIN32
    HANDLE fh;
    HANDLE ph;
    DWORD ec; /* winapi error code */
#endif
#ifdef USE_LIBUSBAPI
    libusb_context* ctx;
    libusb_device_handle* devh;
    int rc; /* libusb return code */
#endif
#ifdef __APPLE__
    IOUSBDeviceInterface** dev;
    kern_return_t kr;
#endif
};

struct dfuAPI {
    char *name;
    dfuAPIResult (*open_fn)(struct dfuDev*, int*);
    dfuAPIResult (*dfureq_fn)(struct dfuDev*, struct usbControlSetup*, void*);
    dfuAPIResult (*reset_fn)(struct dfuDev*);
    void (*close_fn)(struct dfuDev*);
};


/*
 * DFU API low-level (specific) functions
 */
static bool dfu_check_id(int vid, int pid, int *pid_list)
{
    int *p;
    if (vid != APPLE_VID)
        return 0;
    for (p = pid_list; *p; p++)
        if (*p == pid)
            return 1;
    return 0;
}

/* adds extra DFU request error info */
static void dfu_add_reqerrstr(struct dfuDev *dfuh, struct usbControlSetup *cs)
{
    snprintf(dfuh->err + strlen(dfuh->err),
        sizeof(dfuh->err) - strlen(dfuh->err), " (cs=%02x/%d/%d/%d/%d)",
        cs->bmRequestType, cs->bRequest, cs->wValue, cs->wIndex, cs->wLength);
}

#ifdef WIN32
static bool dfu_winapi_chkrc(struct dfuDev *dfuh, char *str, bool success)
{
    dfuh->res = (success) ? DFUAPISuccess : DFUAPIFail;
    if (!success) {
        dfuh->ec = GetLastError();
        snprintf(dfuh->err, sizeof(dfuh->err), "%s error %ld", str, dfuh->ec);
    }
    return success;
}

static dfuAPIResult dfu_winapi_request(struct dfuDev *dfuh,
                            struct usbControlSetup* cs, void* data)
{
    unsigned char buf[USB_CS_SZ + DFU_PKT_SZ];
    DWORD rdwr;
    bool rc;

    memcpy(buf, cs, USB_CS_SZ);

    if (cs->bmRequestType & 0x80)
    {
        rc = ReadFile(dfuh->ph, buf, USB_CS_SZ + cs->wLength, &rdwr, NULL);
        memcpy(data, buf+USB_CS_SZ, cs->wLength);
        dfu_winapi_chkrc(dfuh, "DFU request failed: ReadFile()", rc);
    }
    else
    {
        memcpy(buf+USB_CS_SZ, data, cs->wLength);
        rc = WriteFile(dfuh->ph, buf, USB_CS_SZ + cs->wLength, &rdwr, NULL);
        dfu_winapi_chkrc(dfuh, "DFU request failed: WriteFile()", rc);
    }
    if (!rc)
        dfu_add_reqerrstr(dfuh, cs);

    return dfuh->res;
}

static dfuAPIResult dfu_winapi_reset(struct dfuDev *dfuh)
{
    DWORD bytesReturned;
    bool rc = DeviceIoControl(dfuh->fh,
                0x22000c, NULL, 0, NULL, 0, &bytesReturned, NULL);
    dfu_winapi_chkrc(dfuh,
                "Could not reset USB device: DeviceIoControl()", rc);
    return dfuh->res;
}

static void dfu_winapi_close(struct dfuDev *dfuh)
{
    if (dfuh->fh != INVALID_HANDLE_VALUE) {
        CloseHandle(dfuh->fh);
        dfuh->fh = INVALID_HANDLE_VALUE;
    }
    if (dfuh->ph != INVALID_HANDLE_VALUE) {
        CloseHandle(dfuh->ph);
        dfuh->ph = INVALID_HANDLE_VALUE;
    }
}

static const GUID GUID_AAPLDFU =
    { 0xB8085869L, 0xFEB9, 0x404B, {0x8C, 0xB1, 0x1E, 0x5C, 0x14, 0xFA, 0x8C, 0x54}};

static dfuAPIResult dfu_winapi_open(struct dfuDev *dfuh, int *pid_list)
{
    const GUID *guid = &GUID_AAPLDFU;
    HDEVINFO devinfo = NULL;
    SP_DEVICE_INTERFACE_DETAIL_DATA_A* details = NULL;
    SP_DEVICE_INTERFACE_DATA iface;
    char *path = NULL;
    DWORD i, size;
    bool rc;

    dfuh->fh =
    dfuh->ph = INVALID_HANDLE_VALUE;
    dfuh->found_pid = 0;
    dfuh->res = DFUAPISuccess;
    dfuh->ec = 0;

    /* Get DFU path */
    devinfo = SetupDiGetClassDevsA(guid, NULL, NULL,
                            DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (!dfu_winapi_chkrc(dfuh, "SetupDiGetClassDevsA()",
                            (devinfo != INVALID_HANDLE_VALUE)))
        goto error;

    iface.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    for (i = 0; SetupDiEnumDeviceInterfaces(devinfo, NULL, guid, i, &iface); i++)
    {
        int vid, pid;

        SetupDiGetDeviceInterfaceDetailA(devinfo, &iface, NULL, 0, &size, NULL);

        if (details) free(details);
        details = (SP_DEVICE_INTERFACE_DETAIL_DATA_A*) malloc(size);
        details->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
        rc = SetupDiGetDeviceInterfaceDetailA(devinfo, &iface, details, size, NULL, NULL);
        if (!dfu_winapi_chkrc(dfuh, "SetupDiGetDeviceInterfaceDetailA()", rc))
            goto error;

        CharUpperA(details->DevicePath);
        if (sscanf(details->DevicePath, "%*4cUSB#VID_%04x&PID_%04x%*s", &vid, &pid) != 2)
            continue;
        if (!dfu_check_id(vid, pid, pid_list))
            continue;

        if (path) free(path);
        path = malloc(size - sizeof(DWORD) + 16);
        memcpy(path, details->DevicePath, size - sizeof(DWORD));

        dfuh->fh = CreateFileA(path, GENERIC_READ|GENERIC_WRITE,
                    FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (!dfu_winapi_chkrc(dfuh, "CreateFileA(fh)", (dfuh->fh != INVALID_HANDLE_VALUE)))
            goto error;

        strcat(path, "\\PIPE0");
        dfuh->ph = CreateFileA(path, GENERIC_READ|GENERIC_WRITE,
                    FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (!dfu_winapi_chkrc(dfuh, "CreateFileA(ph)", (dfuh->ph != INVALID_HANDLE_VALUE)))
            goto error;

        /* ok */
        snprintf(dfuh->descr, sizeof(dfuh->descr), "%s", details->DevicePath);
        dfuh->found_pid = pid;
        goto bye;
    }

    if (!dfu_winapi_chkrc(dfuh, "SetupDiEnumDeviceInterfaces()",
                            (GetLastError() == ERROR_NO_MORE_ITEMS)))
       goto error;

    /* no devices found */

bye:
    if (path) free(path);
    if (details) free(details);
    if (devinfo) SetupDiDestroyDeviceInfoList(devinfo);
    return dfuh->res;

error:
    dfu_winapi_close(dfuh);
    goto bye;
}
#endif /* WIN32 */

#ifdef USE_LIBUSBAPI
static bool dfu_libusb_chkrc(struct dfuDev *dfuh, char *str)
{
    dfuh->res = (dfuh->rc < LIBUSB_SUCCESS) ? DFUAPIFail : DFUAPISuccess;
    if (dfuh->res == DFUAPIFail)
        snprintf(dfuh->err, sizeof(dfuh->err),
                    "%s: %s", str, libusb_error_name(dfuh->rc));
    return (dfuh->res == DFUAPISuccess);
}

static dfuAPIResult dfu_libusb_request(struct dfuDev *dfuh,
                            struct usbControlSetup *cs, void *data)
{
    dfuh->rc = libusb_control_transfer(dfuh->devh, cs->bmRequestType,
            cs->bRequest, cs->wValue, cs->wIndex, data, cs->wLength, 500);
    if (!dfu_libusb_chkrc(dfuh, "DFU request failed"))
        dfu_add_reqerrstr(dfuh, cs);
    return dfuh->res;
}

static dfuAPIResult dfu_libusb_reset(struct dfuDev *dfuh)
{
    dfuh->rc = libusb_reset_device(dfuh->devh);
    dfu_libusb_chkrc(dfuh, "Could not reset USB device");
    return dfuh->res;
}

static void dfu_libusb_close(struct dfuDev *dfuh)
{
    if (dfuh->devh) {
        libusb_release_interface(dfuh->devh, 0);
        if (dfuh->detached)
            libusb_attach_kernel_driver(dfuh->devh, 0);
        libusb_close(dfuh->devh);
        dfuh->devh = NULL;
    }
    if (dfuh->ctx) {
        libusb_exit(dfuh->ctx);
        dfuh->ctx = NULL;
    }
}

static dfuAPIResult dfu_libusb_open(struct dfuDev *dfuh, int *pid_list)
{
    struct libusb_device_descriptor desc;
    libusb_device **devs = NULL, *dev;
    int n_devs, i;

    dfuh->devh = NULL;
    dfuh->found_pid = 0;
    dfuh->detached = 0;
    dfuh->res = DFUAPISuccess;

    dfuh->rc = libusb_init(&(dfuh->ctx));
    if (!dfu_libusb_chkrc(dfuh, "Could not init USB library")) {
        dfuh->ctx = NULL; /* invalidate ctx (if any) */
        goto error;
    }

    n_devs =
    dfuh->rc = libusb_get_device_list(dfuh->ctx, &devs);
    if (!dfu_libusb_chkrc(dfuh, "Could not get USB device list"))
        goto error;

    for (i = 0; i < n_devs; ++i)
    {
        dev = devs[i];

        /* Note: since libusb-1.0.16 (LIBUSB_API_VERSION >= 0x01000102)
           this function always succeeds. */
        if (libusb_get_device_descriptor(dev, &desc) != LIBUSB_SUCCESS)
            continue; /* Unable to get device descriptor */

        if (!dfu_check_id(desc.idVendor, desc.idProduct, pid_list))
            continue;

        dfuh->rc = libusb_open(dev, &(dfuh->devh));
        if (!dfu_libusb_chkrc(dfuh, "Could not open USB device"))
            goto error;

        dfuh->rc = libusb_set_configuration(dfuh->devh, 1);
        if (!dfu_libusb_chkrc(dfuh, "Could not set USB configuration"))
            goto error;

        dfuh->rc = libusb_kernel_driver_active(dfuh->devh, 0);
        if (dfuh->rc != LIBUSB_ERROR_NOT_SUPPORTED) {
            if (!dfu_libusb_chkrc(dfuh, "Could not get USB driver status"))
                goto error;
            if (dfuh->rc == 1) {
                dfuh->rc = libusb_detach_kernel_driver(dfuh->devh, 0);
                if (!dfu_libusb_chkrc(dfuh, "Could not detach USB driver"))
                    goto error;
                dfuh->detached = 1;
            }
        }

        dfuh->rc = libusb_claim_interface(dfuh->devh, 0);
        if (!dfu_libusb_chkrc(dfuh, "Could not claim USB interface"))
            goto error;

        /* ok */
        snprintf(dfuh->descr, sizeof(dfuh->descr),
                "[%04x:%04x] at bus %d, device %d, USB ver. %04x",
                desc.idVendor, desc.idProduct, libusb_get_bus_number(dev),
                libusb_get_device_address(dev), desc.bcdUSB);
        dfuh->found_pid = desc.idProduct;
        break;
    }

bye:
    if (devs)
        libusb_free_device_list(devs, 1);
    if (!dfuh->found_pid)
        dfu_libusb_close(dfuh);
    return dfuh->res;

error:
    goto bye;
}
#endif /* USE_LIBUSBAPI */

#ifdef __APPLE__
static bool dfu_iokit_chkrc(struct dfuDev *dfuh, char *str)
{
    dfuh->res = (dfuh->kr == kIOReturnSuccess) ? DFUAPISuccess : DFUAPIFail;
    if (dfuh->res == DFUAPIFail)
        snprintf(dfuh->err, sizeof(dfuh->err),
                    "%s: error %08x", str, dfuh->kr);
    return (dfuh->res == DFUAPISuccess);
}

static dfuAPIResult dfu_iokit_request(struct dfuDev *dfuh,
                            struct usbControlSetup *cs, void *data)
{
    IOUSBDevRequest req;
    req.bmRequestType = cs->bmRequestType;
    req.bRequest = cs->bRequest;
    req.wValue = cs->wValue;
    req.wIndex = cs->wIndex;
    req.wLength = cs->wLength;
    req.pData = data;

    dfuh->kr = (*(dfuh->dev))->DeviceRequest(dfuh->dev, &req);
    if (!dfu_iokit_chkrc(dfuh, "DFU request failed"))
        dfu_add_reqerrstr(dfuh, cs);

    return dfuh->res;
}

static dfuAPIResult dfu_iokit_reset(struct dfuDev *dfuh)
{
    dfuh->kr = (*(dfuh->dev))->ResetDevice(dfuh->dev);
#if 0
    /* On 10.11+ ResetDevice() returns no error but does not perform
     * any reset, just a kernel log message.
     * USBDeviceReEnumerate() could be used as a workaround.
     */
    dfuh->kr = (*(dfuh->dev))->USBDeviceReEnumerate(dfuh->dev, 0);
#endif
    dfu_iokit_chkrc(dfuh, "Could not reset USB device");
    return dfuh->res;
}

static void dfu_iokit_close(struct dfuDev *dfuh)
{
    if (dfuh->dev) {
        (*(dfuh->dev))->USBDeviceClose(dfuh->dev);
        (*(dfuh->dev))->Release(dfuh->dev);
        dfuh->dev = NULL;
    }
}

static dfuAPIResult dfu_iokit_open(struct dfuDev *dfuh, int *pid_list)
{
    kern_return_t kr;
    CFMutableDictionaryRef usb_matching_dict = 0;
    io_object_t usbDevice;
    io_iterator_t usb_iterator = IO_OBJECT_NULL;
    IOCFPlugInInterface **plugInInterface = NULL;
    IOUSBDeviceInterface **dev = NULL;
    HRESULT result;
    SInt32 score;
    UInt16 vendor;
    UInt16 product;
    UInt16 release;

    dfuh->dev = NULL;
    dfuh->found_pid = 0;
    dfuh->res = DFUAPISuccess;

    usb_matching_dict = IOServiceMatching(kIOUSBDeviceClassName);
    dfuh->kr = IOServiceGetMatchingServices(
                kIOMasterPortDefault, usb_matching_dict, &usb_iterator);
    if (!dfu_iokit_chkrc(dfuh, "Could not get matching services"))
        goto error;

    while ((usbDevice = IOIteratorNext(usb_iterator)))
    {
        /* Create an intermediate plug-in */
        kr = IOCreatePlugInInterfaceForService(usbDevice,
                    kIOUSBDeviceUserClientTypeID,
                    kIOCFPlugInInterfaceID,
                    &plugInInterface,
                    &score);
        IOObjectRelease(usbDevice);

        if ((kIOReturnSuccess != kr) || !plugInInterface)
            continue; /* Unable to create a plugin */

        /* Now create the device interface */
        result = (*plugInInterface)->QueryInterface(plugInInterface,
                    CFUUIDGetUUIDBytes(kIOUSBDeviceInterfaceID),
                    (LPVOID*)&dev);
        (*plugInInterface)->Release(plugInInterface);

        if (result || !dev)
            continue; /* Couldn't create a device interface */

        kr = (*dev)->GetDeviceVendor(dev, &vendor);
        kr = (*dev)->GetDeviceProduct(dev, &product);
        kr = (*dev)->GetDeviceReleaseNumber(dev, &release);

        if (!dfu_check_id(vendor, product, pid_list)) {
            (*dev)->Release(dev);
            continue;
        }

        /* Device found, open it */
        dfuh->kr = (*dev)->USBDeviceOpen(dev);
        if (!dfu_iokit_chkrc(dfuh, "Could not open USB device")) {
            (*dev)->Release(dev);
            goto error;
        }

        /* ok */
        dfuh->found_pid = product;
        dfuh->dev = dev;
        snprintf(dfuh->descr, sizeof(dfuh->descr),
                "[%04x:%04x] release: %d", vendor, product, release);
        break;
    }

bye:
    if (usb_iterator != IO_OBJECT_NULL)
        IOObjectRelease(usb_iterator);
    return dfuh->res;

error:
    goto bye;
}
#endif /* __APPLE__ */

/* list of suported APIs */
static struct dfuAPI api_list[] =
{
#ifdef WIN32
    { "winapi",
      dfu_winapi_open,
      dfu_winapi_request,
      dfu_winapi_reset,
      dfu_winapi_close },
#endif
#ifdef USE_LIBUSBAPI
    { "libusb",
      dfu_libusb_open,
      dfu_libusb_request,
      dfu_libusb_reset,
      dfu_libusb_close },
#endif
#ifdef __APPLE__
    { "IOKit",
      dfu_iokit_open,
      dfu_iokit_request,
      dfu_iokit_reset,
      dfu_iokit_close },
#endif
};
#define N_DFU_APIS (sizeof(api_list)/sizeof(struct dfuAPI))


/*
 * DFU API common functions
 */
static int DEBUG_DFUREQ = 0;

static dfuAPIResult dfuapi_request(struct dfuDev *dfuh,
                            struct usbControlSetup *cs, void *data)
{
    if (!DEBUG_DFUREQ)
        return dfuh->api->dfureq_fn(dfuh, cs, data);

    /* DEBUG */

    /* previous state */
    unsigned char ste = 0;
    struct usbControlSetup css = { 0xA1, DFU_GETSTATE, 0, 0, sizeof(ste) };
    if (dfuh->api->dfureq_fn(dfuh, &css, &ste) != DFUAPISuccess) {
        snprintf(dfuh->err + strlen(dfuh->err), sizeof(dfuh->err) -
                strlen(dfuh->err), " [DEBUG_DFUREQ ERROR: state=%d]", ste);
        goto error;
    }

    dfuh->api->dfureq_fn(dfuh, cs, data);
    fprintf(stderr, "[DEBUG]: REQ: ste=%d, cs=%2x/%d/%d/%d/%d -> %s",
                ste, cs->bmRequestType, cs->bRequest, cs->wValue,
                cs->wIndex, cs->wLength,
                (dfuh->res == DFUAPISuccess) ? "ok" : "ERROR");
    if (cs->bRequest == DFU_GETSTATE)
        fprintf(stderr, " (state=%d)", *((unsigned char*)(data)));
    if (cs->bRequest == DFU_GETSTATUS) {
        struct usbStatusData *sd = (struct usbStatusData*)data;
        fprintf(stderr, " (status=%d, polltmo=%d, state=%d)", sd->bStatus,
                (sd->bwPollTimeout2 << 16) | (sd->bwPollTimeout1 << 8) |
                (sd->bwPollTimeout0), sd->bState);
    }
    fputc('\n', stderr);
    fflush(stderr);

bye:
    return dfuh->res;
error:
    goto bye;
}

static dfuAPIResult dfuapi_req_getstatus(struct dfuDev *dfuh,
                            DFUStatus *status, int *poll_tmo /*ms*/,
                            DFUState *state)
{
    struct usbStatusData sd = { 0, 0, 0, 0, 0, 0 };
    struct usbControlSetup cs = { 0xA1, DFU_GETSTATUS, 0, 0, sizeof(sd) };
    dfuapi_request(dfuh, &cs, &sd);
    if (status) *status = sd.bStatus;
    if (state) *state = sd.bState;
    if (poll_tmo) *poll_tmo = (sd.bwPollTimeout2 << 16) |
                (sd.bwPollTimeout1 << 8) | (sd.bwPollTimeout0);
    return dfuh->res;
}

static dfuAPIResult dfuapi_req_getstate(struct dfuDev *dfuh, DFUState *state)
{
    unsigned char sts = 0;
    struct usbControlSetup cs = { 0xA1, DFU_GETSTATE, 0, 0, sizeof(sts) };
    dfuapi_request(dfuh, &cs, &sts);
    if (state) *state = sts;
    return dfuh->res;
}

static dfuAPIResult dfuapi_req_dnload(struct dfuDev* dfuh, uint16_t blknum,
                            uint16_t len, unsigned char *data)
{
    struct usbControlSetup cs = { 0x21, DFU_DNLOAD, blknum, 0, len };
    return dfuapi_request(dfuh, &cs, data);
}

/* not used */
#if 0
static dfuAPIResult dfuapi_req_upload(struct dfuDev* dfuh,
                uint16_t blknum, uint16_t len, unsigned char *data)
{
    struct usbControlSetup cs = { 0xA1, DFU_UPLOAD, blknum, 0, len };
    return dfuapi_request(dfuh, &cs, data);
}

static dfuAPIResult dfuapi_req_clrstatus(struct dfuDev* dfuh)
{
    struct usbControlSetup cs = { 0x21, DFU_CLRSTATUS, 0, 0, 0 };
    return dfuapi_request(dfuh, &cs, NULL);
}

static dfuAPIResult dfuapi_req_abort(struct dfuDev* dfuh)
{
    struct usbControlSetup cs = { 0x21, DFU_ABORT, 0, 0, 0 };
    return dfuapi_request(dfuh, &cs, NULL);
}

/* not implemented on DFU8702 */
static dfuAPIResult dfuapi_req_detach(struct dfuDev* dfuh, int tmo)
{
    struct usbControlSetup cs = { 0x21, DFU_DETACH, tmo, 0, 0 };
    return dfuapi_request(dfuh, &cs, NULL);
}
#endif

static dfuAPIResult dfuapi_reset(struct dfuDev *dfuh)
{
    return dfuh->api->reset_fn(dfuh);
}

static dfuAPIResult dfuapi_send_packet(struct dfuDev* dfuh, uint16_t blknum,
                            uint16_t len, unsigned char *data, DFUStatus *status,
                            int *poll_tmo, DFUState *state, DFUState *pre_state)
{
    if (dfuapi_req_dnload(dfuh, blknum, len, data) != DFUAPISuccess)
        goto error;

    /* device is in dfuDLSYNC state, waiting for a GETSTATUS request
     * to enter the next state, if she respond with dfuDLBUSY then
     * we must wait to resend the GETSTATUS request */

    if (dfuapi_req_getstatus(dfuh, status, poll_tmo, state) != DFUAPISuccess)
        goto error;

    if (*state == dfuDNBUSY) {
        if (*poll_tmo)
            sleep_ms(*poll_tmo);
        if (pre_state)
            if (dfuapi_req_getstate(dfuh, pre_state) != DFUAPISuccess)
                goto error;
        if (dfuapi_req_getstatus(dfuh, status, poll_tmo, state) != DFUAPISuccess)
            goto error;
    }

bye:
    return dfuh->res;
error:
    goto bye;
}

static void dfuapi_set_err(struct dfuDev *dfuh, char *str)
{
    dfuh->res = DFUAPIFail;
    strncpy(dfuh->err, str, sizeof(dfuh->err));
}

static dfuAPIResult dfuapi_open(struct dfuDev *dfuh, int pid)
{
    int pid_l[N_KNOWN_PIDS+1] = { 0 };
    struct dfuAPI *api;
    unsigned i, p;

    /* fill pid list */
    if (pid)
        pid_l[0] = pid;
    else
        for (p = 0; p < N_KNOWN_PIDS; p++)
            pid_l[p] = known_pids[p].pid;

    for (i = 0; i < N_DFU_APIS; i++)
    {
        api = &api_list[i];
        if (api->open_fn(dfuh, pid_l) != DFUAPISuccess)
            goto error;
        if (dfuh->found_pid) {
            /* ok */
            dfuh->api = api;
            printf("[INFO] %s: found %s\n", api->name, dfuh->descr);
            for (p = 0; p < N_KNOWN_PIDS; p++) {
                if (known_pids[p].pid == dfuh->found_pid) {
                    printf("[INFO] iPod %s, mode: %s\n", known_pids[p].desc,
                                known_pids[p].mode ? "WTF" : "DFU");
                    break;
                }
            }
            fflush(stdout);
            goto bye;
        }
        printf("[INFO] %s: no DFU devices found\n", api->name);
        fflush(stdout);
    }

    /* error */
    dfuapi_set_err(dfuh, "DFU device not found");

bye:
    return dfuh->res;
error:
    goto bye;
}

static void dfuapi_destroy(struct dfuDev *dfuh)
{
    if (dfuh) {
        if (dfuh->api)
            dfuh->api->close_fn(dfuh);
        free(dfuh);
    }
}

static struct dfuDev *dfuapi_create(void)
{
    return calloc(sizeof(struct dfuDev), 1);
}


/*
 * app level functions
 */
static int ipoddfu_download_file(struct dfuDev* dfuh,
                            unsigned char *data, unsigned long size)
{
    unsigned int blknum, len, remaining;
    int poll_tmo;
    DFUStatus status;
    DFUState state;

    if (dfuapi_req_getstate(dfuh, &state) != DFUAPISuccess)
        goto error;

    if (state != dfuIDLE) {
        dfuapi_set_err(dfuh, "Could not start DFU download: not idle");
        goto error;
    }

    blknum = 0;
    remaining = size;
    while (remaining)
    {
        len = (remaining < DFU_PKT_SZ) ? remaining : DFU_PKT_SZ;

        if (dfuapi_send_packet(dfuh, blknum, len, data + blknum*DFU_PKT_SZ,
                    &status, &poll_tmo, &state, NULL) != DFUAPISuccess)
            goto error;

        if (state != dfuDNLOAD_IDLE) {
            dfuapi_set_err(dfuh, "DFU download aborted: unexpected state");
            goto error;
        }

        remaining -= len;
        blknum++;
    }

    /* send ZLP */
    DFUState pre_state = appIDLE; /* dummy state */
    if (dfuapi_send_packet(dfuh, blknum, 0, NULL,
                &status, &poll_tmo, &state, &pre_state) != DFUAPISuccess) {
        if (pre_state == dfuMANIFEST_SYNC)
            goto ok; /* pwnaged .dfu file */
        goto error;
    }

    if (state != dfuMANIFEST) {
        if (status == errFIRMWARE)
            dfuapi_set_err(dfuh, "DFU download failed: corrupt firmware");
        else
            dfuapi_set_err(dfuh, "DFU download failed: unexpected state");
        goto error;
    }

    /* wait for manifest stage */
    if (poll_tmo)
        sleep_ms(poll_tmo);

    if (dfuapi_req_getstatus(dfuh, &status, NULL, &state) != DFUAPISuccess)
        goto ok; /* 1223 .dfu file */

    /* XXX: next code never tested */

    if (state != dfuMANIFEST_WAIT_RESET) {
        if (status == errVERIFY)
            dfuapi_set_err(dfuh, "DFU manifest failed: wrong FW verification");
        else
            dfuapi_set_err(dfuh, "DFU manifest failed: unexpected state");
        goto error;
    }

    if (dfuapi_reset(dfuh) != DFUAPISuccess)
        goto error;

ok:
    return 1;
error:
    return 0;
}

/* exported functions */
int ipoddfu_send(int pid, unsigned char *data, int size,
                            char* errstr, int errstrsize)
{
    struct dfuDev *dfuh;
    unsigned char *buf;
    uint32_t checksum;
    int ret = 1; /* ok */

    dfuh = dfuapi_create();

    buf = malloc(size+4);
    if (!buf) {
        dfuapi_set_err(dfuh, "Could not allocate memory for DFU buffer");
        goto error;
    }

    if (memcmp(data, IM3_IDENT, 4)) {
        dfuapi_set_err(dfuh, "Bad DFU image data");
        goto error;
    }

    crc32_init();
    checksum = crc32(data, size, 0);
    memcpy(buf, data, size);
    put_uint32le(buf+size, ~checksum);

    if (dfuapi_open(dfuh, pid) != DFUAPISuccess)
        goto error;

    if (!ipoddfu_download_file(dfuh, buf, size+4))
        goto error;

bye:
    if (buf) free(buf);
    dfuapi_destroy(dfuh);
    return ret;

error:
    ret = 0;
    if (errstr)
        snprintf(errstr, errstrsize, "[ERR] %s", dfuh->err);
    goto bye;
}

/* search for the DFU device and gets its DFUState */
int ipoddfu_scan(int pid, int *state, int reset,
                            char* errstr, int errstrsize)
{
    struct dfuDev *dfuh;
    int ret = 1; /* ok */

    dfuh = dfuapi_create();

    if (dfuapi_open(dfuh, pid) != DFUAPISuccess)
        goto error;

    if (reset)
        if (dfuapi_reset(dfuh) != DFUAPISuccess)
            goto error;

    if (state) {
        DFUState sts;
        if (dfuapi_req_getstate(dfuh, &sts) != DFUAPISuccess)
            goto error;
        *state = (int)sts;
    }

bye:
    dfuapi_destroy(dfuh);
    return ret;

error:
    ret = 0;
    if (errstr)
        snprintf(errstr, errstrsize, "[ERR] %s", dfuh->err);
    goto bye;
}

void ipoddfu_debug(int debug)
{
    DEBUG_DFUREQ = debug;
}
