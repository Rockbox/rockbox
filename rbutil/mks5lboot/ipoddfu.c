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
#ifndef NO_LIBUSBAPI
#define USE_LIBUSBAPI
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#ifdef WIN32
#include <windows.h>
#include <setupapi.h>
#include <stdbool.h>
#endif

#ifdef USE_LIBUSBAPI
#include <libusb-1.0/libusb.h>
#endif

#include "mks5lboot.h"


static void sleep_ms(unsigned int ms)
{
   struct timespec req;
   req.tv_sec = ms / 1000;
   req.tv_nsec = (ms % 1000) * 1000000;
   nanosleep(&req, NULL);
}

/*
 * CRC32 functions
 * Based on public domain implementation by Finn Yannick Jacobs.
 */

/* Written and copyright 1999 by Finn Yannick Jacobs
 * No rights were reserved to this, so feel free to
 * manipulate or do with it, what you want or desire :)
 */

#define CRC32_DEFAULT_SEED 0xffffffff

/* crc32table[] built by crc32_init() */
static unsigned long crc32table[256];

/* Calculate crc32. Little endian.
 * Standard seed is 0xffffffff or 0.
 * Some implementations xor result with 0xffffffff after calculation.
 */
static uint32_t crc32(void *data, unsigned int len, uint32_t seed)
{
    uint8_t *d = data;

    while (len--)
    {
        seed = ((seed >> 8) & 0x00FFFFFF) ^ crc32table [(seed ^ *d++) & 0xFF];
    }

    return seed;
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

        for (j = 8; j > 0; --j)
        {
            crc = (crc >> 1) ^ ((crc & 1) ? poly : 0);
        }

        crc32table[i] = crc;
    }
}


/*
 * DFU
 */

/* must be pow2 <= wTransferSize (2048) */
#define DFU_PKT_SZ  2048

#define APPLE_VID   0x05AC

static int KNOWN_PIDS[] =
{
    /* DFU */
    0x1220,     /* Nano 2G */
    0x1223,     /* Nano 3G and Classic 1G/2G/3G/4G */
    0x1224,     /* Shuffle 3G */
    0x1225,     /* Nano 4G */
    0x1231,     /* Nano 5G */
    0x1232,     /* Nano 6G */
    0x1233,     /* Shuffle 4G */
    0x1234,     /* Nano 7G */
    /* WTF */
    0x1240,     /* Nano 2G */
    0x1241,     /* Classic 1G */
    0x1242,     /* Nano 3G */
    0x1243,     /* Nano 4G */
    0x1245,     /* Classic 2G */
    0x1246,     /* Nano 5G */
    0x1247,     /* Classic 3G */
    0x1248,     /* Nano 6G */
    0x1249,     /* Nano 7G */
    0x124a,     /* Nano 7G */
    0x1250,     /* Classic 4G */
    0
};

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

/* DFU 1.1 specs */
typedef enum DFUState {
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

typedef enum DFUStatus {
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

typedef enum DFURequest {
    DFU_DETACH = 0,
    DFU_DNLOAD = 1,
    DFU_UPLOAD = 2,
    DFU_GETSTATUS = 3,
    DFU_CLRSTATUS = 4,
    DFU_GETSTATE = 5,
    DFU_ABORT = 6
} DFURequest;

struct dfuDev {
    struct dfuAPI *api;
    int found_pid;
    int detached;
    char descr[256];
    int res; /* API result: 1->ok, 0->failure */
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
};

struct dfuAPI {
    char *name;
    int (*open_fn)(struct dfuDev*, int*);
    int (*dfureq_fn)(struct dfuDev*, struct usbControlSetup*, void*);
    int (*reset_fn)(struct dfuDev*);
    void (*close_fn)(struct dfuDev*);
};


/*
 * low-level (API specific) functions
 */
static int dfu_check_id(int vid, int pid, int *pid_list)
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
static int dfu_winapi_chkrc(struct dfuDev *dfuh, char *str, bool success)
{
    dfuh->res = (int)success;
    if (!success) {
        dfuh->ec = GetLastError();
        snprintf(dfuh->err, sizeof(dfuh->err), "%s error %ld", str, dfuh->ec);
    }
    return dfuh->res;
}

static int dfu_winapi_request(struct dfuDev *dfuh,
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

    if (!dfuh->res)
        dfu_add_reqerrstr(dfuh, cs);
    return dfuh->res;
}

static int dfu_winapi_reset(struct dfuDev *dfuh)
{
    DWORD bytesReturned;
    bool rc = DeviceIoControl(dfuh->fh, 0x22000c,
                            NULL, 0, NULL, 0, &bytesReturned, NULL);
    return dfu_winapi_chkrc(dfuh,
                "Could not reset USB device: DeviceIoControl()", rc);
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

static int dfu_winapi_open(struct dfuDev *dfuh, int *pid_list)
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
    dfuh->res = 1; /* ok */
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
static int dfu_libusb_chkrc(struct dfuDev *dfuh, char *str)
{
    dfuh->res = (dfuh->rc < LIBUSB_SUCCESS) ? 0 : 1;
    if (dfuh->res == 0)
        snprintf(dfuh->err, sizeof(dfuh->err),
                    "%s: %s", str, libusb_error_name(dfuh->rc));
    return dfuh->res;
}

static int dfu_libusb_request(struct dfuDev *dfuh,
                            struct usbControlSetup *cs, void *data)
{
    dfuh->rc = libusb_control_transfer(dfuh->devh, cs->bmRequestType,
            cs->bRequest, cs->wValue, cs->wIndex, data, cs->wLength, 500);
    if (!dfu_libusb_chkrc(dfuh, "DFU request failed"))
        dfu_add_reqerrstr(dfuh, cs);
    return dfuh->res;
}

static int dfu_libusb_reset(struct dfuDev *dfuh)
{
    dfuh->rc = libusb_reset_device(dfuh->devh);
    return dfu_libusb_chkrc(dfuh, "Could not reset USB device");
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

static int dfu_libusb_open(struct dfuDev *dfuh, int *pid_list)
{
    struct libusb_device_descriptor desc;
    libusb_device **devs = NULL, *dev;
    int n_devs, i;

    dfuh->devh = NULL;
    dfuh->found_pid = 0;
    dfuh->detached = 0;
    dfuh->res = 1; /* ok */

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

/* list of suported APIs:
 *  Windows: winapi and libusb (optional)
 *  Linux and OSX: libusb
 */
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
    /* TODO: implement API for OS X < 10.6 ??? */
#endif
};
#define DFU_N_APIS (sizeof(api_list)/sizeof(struct dfuAPI))

/*
 * mid-layer (common) functions
 */
static void dfu_set_errstr(struct dfuDev *dfuh, char *str)
{
    strncpy(dfuh->err, str, sizeof(dfuh->err));
}

static int DEBUG_DFUREQ = 0;

static int dfu_request(struct dfuDev *dfuh,
                            struct usbControlSetup *cs, void *data)
{
    if (!DEBUG_DFUREQ)
        return dfuh->api->dfureq_fn(dfuh, cs, data);

    /* DEBUG */

    /* previous state */
    unsigned char ste = 0;
    struct usbControlSetup css = { 0xA1, DFU_GETSTATE, 0, 0, sizeof(ste) };
    if (!dfuh->api->dfureq_fn(dfuh, &css, &ste)) {
        snprintf(dfuh->err + strlen(dfuh->err), sizeof(dfuh->err) -
                strlen(dfuh->err), " [DEBUG_DFUREQ ERROR: state=%d]", ste);
        return 0;
    }

    int ret = dfuh->api->dfureq_fn(dfuh, cs, data);
    fprintf(stderr, "[DEBUG]: REQ: ste=%d, cs=%2x/%d/%d/%d/%d -> %s",
                ste, cs->bmRequestType, cs->bRequest, cs->wValue,
                cs->wIndex, cs->wLength, ret ? "ok" : "ERROR");
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
    return ret;
}

static int dfureq_getstatus(struct dfuDev *dfuh, int *status,
                                int *poll_tmo /*ms*/, int *state)
{
    struct usbStatusData sd = { 0, 0, 0, 0, 0, 0 };
    struct usbControlSetup cs = { 0xA1, DFU_GETSTATUS, 0, 0, sizeof(sd) };
    int ret = dfu_request(dfuh, &cs, &sd);
    if (status) *status = sd.bStatus;
    if (state) *state = sd.bState;
    if (poll_tmo) *poll_tmo = (sd.bwPollTimeout2 << 16) |
                (sd.bwPollTimeout1 << 8) | (sd.bwPollTimeout0);
    return ret;
}

static int dfureq_getstate(struct dfuDev *dfuh, int *state)
{
    if (!state)
        return 1; /* nothing to do */
    unsigned char sts = 0;
    struct usbControlSetup cs = { 0xA1, DFU_GETSTATE, 0, 0, sizeof(sts) };
    int ret = dfu_request(dfuh, &cs, &sts);
    *state = sts;
    return ret;
}

static int dfureq_dnload(struct dfuDev* dfuh, uint16_t blknum,
                            uint16_t len, unsigned char *data)
{
    struct usbControlSetup cs = { 0x21, DFU_DNLOAD, blknum, 0, len };
    return dfu_request(dfuh, &cs, data);
}

/* not used */
#if 0
static int dfureq_upload(struct dfuDev* dfuh,
                uint16_t blknum, uint16_t len, unsigned char *data)
{
    struct usbControlSetup cs = { 0xA1, DFU_UPLOAD, blknum, 0, len };
    return dfu_request(dfuh, &cs, data);
}

static int dfureq_clrstatus(struct dfuDev* dfuh)
{
    struct usbControlSetup cs = { 0x21, DFU_CLRSTATUS, 0, 0, 0 };
    return dfu_request(dfuh, &cs, NULL);
}

static int dfureq_abort(struct dfuDev* dfuh)
{
    struct usbControlSetup cs = { 0x21, DFU_ABORT, 0, 0, 0 };
    return dfu_request(dfuh, &cs, NULL);
}

/* not implemented on DFU8702 */
static int dfureq_detach(struct dfuDev* dfuh, int tmo)
{
    struct usbControlSetup cs = { 0x21, DFU_DETACH, tmo, 0, 0 };
    return dfu_request(dfuh, &cs, NULL);
}
#endif

static int dfu_send_packet(struct dfuDev* dfuh, uint16_t blknum,
                            uint16_t len, unsigned char *data, int *status,
                            int *poll_tmo, int *state, int *pre_state)
{
    if (!dfureq_dnload(dfuh, blknum, len, data))
        return 0;

    /* device is in dfuDLSYNC state, waiting for a GETSTATUS request
       to enter the next state, if she respond with dfuDLBUSY then
       we must wait to resend the GETSTATUS request */

    if (!dfureq_getstatus(dfuh, status, poll_tmo, state))
        return 0;

    if (*state == dfuDNBUSY) {
        if (*poll_tmo)
            sleep_ms(*poll_tmo);
        if (!dfureq_getstate(dfuh, pre_state))
            return 0;
        if (!dfureq_getstatus(dfuh, status, poll_tmo, state))
            return 0;
    }

    return 1;
}

static int dfu_download_file(struct dfuDev* dfuh,
                            unsigned char *data, unsigned long size)
{
    unsigned int blknum, len, remaining;
    int status, poll_tmo, state;

    if (!dfureq_getstate(dfuh, &state))
        goto error;

    if (state != dfuIDLE) {
        dfu_set_errstr(dfuh, "Could not start DFU download: not idle");
        goto error;
    }

    blknum = 0;
    remaining = size;
    while (remaining)
    {
        len = (remaining < DFU_PKT_SZ) ? remaining : DFU_PKT_SZ;

        if (!dfu_send_packet(dfuh, blknum, len, data +
                    blknum*DFU_PKT_SZ, &status, &poll_tmo, &state, NULL))
            goto error;

        if (state != dfuDNLOAD_IDLE) {
            dfu_set_errstr(dfuh, "DFU download aborted: unexpected state");
            goto error;
        }

        remaining -= len;
        blknum++;
    }

    /* send ZLP */
    int pre_state = 0;
    if (!dfu_send_packet(dfuh, blknum, 0, NULL,
                &status, &poll_tmo, &state, &pre_state)) {
        if (pre_state == dfuMANIFEST_SYNC)
            goto ok; /* pwnaged .dfu file */
        goto error;
    }

    if (state != dfuMANIFEST) {
        if (status == errFIRMWARE)
            dfu_set_errstr(dfuh, "DFU download failed: corrupt firmware");
        else
            dfu_set_errstr(dfuh, "DFU download failed: unexpected state");
        goto error;
    }

    /* wait for manifest stage */
    if (poll_tmo)
        sleep_ms(poll_tmo);

    if (!dfureq_getstatus(dfuh, &status, NULL, &state))
        goto ok; /* 1223 .dfu file */


    /* TODO: next code never tested */

    if (state != dfuMANIFEST_WAIT_RESET) {
        if (status == errVERIFY)
            dfu_set_errstr(dfuh, "DFU manifest failed: wrong FW verification");
        else
            dfu_set_errstr(dfuh, "DFU manifest failed: unexpected state");
        goto error;
    }

    if (!dfuh->api->reset_fn(dfuh))
        goto error;

ok:
    return 1;
error:
    return 0;
}

static int dfu_open(struct dfuDev *dfuh, int pid)
{
    int pid_l[2] = {0};
    struct dfuAPI *api;
    unsigned i;

    pid_l[0] = pid;

    for (i = 0; i < DFU_N_APIS; i++)
    {
        api = &api_list[i];
        if (!(api->open_fn(dfuh, pid ? pid_l : KNOWN_PIDS)))
            return 0; /* error */
        if (dfuh->found_pid) {
            dfuh->api = api;
            printf("[INFO] %s: found %s\n", api->name, dfuh->descr);
            fflush(stdout);
            return 1; /* ok */
        }
        printf("[INFO] %s: no DFU devices found\n", api->name);
        fflush(stdout);
    }

    dfu_set_errstr(dfuh, "DFU device not found");
    return 0;
}

static void dfu_destroy(struct dfuDev *dfuh)
{
    if (dfuh) {
        if (dfuh->api)
            dfuh->api->close_fn(dfuh);
        free(dfuh);
    }
}

static struct dfuDev *dfu_create()
{
    return calloc(sizeof(struct dfuDev), 1);
}

/*
 * exported functions
 */
int ipoddfu_send(int pid, unsigned char *data, int size,
                            char* errstr, int errstrsize)
{
    struct dfuDev *dfuh;
    unsigned char *buf;
    unsigned int checksum;
    int ret = 1; /* ok */

    dfuh = dfu_create();

    buf = malloc(size+4);
    if (!buf) {
        dfu_set_errstr(dfuh, "Could not allocate memory for DFU buffer");
        goto error;
    }

    if (memcmp(data, IM3_IDENT, 4)) {
        dfu_set_errstr(dfuh, "Bad DFU image data");
        goto error;
    }

    /* FIXME: big endian */
    crc32_init();
    checksum = crc32(data, size, CRC32_DEFAULT_SEED);
    memcpy(buf, data, size);
    memcpy(buf+size, &checksum, 4);

    if (!dfu_open(dfuh, pid))
        goto error;

    if (!dfu_download_file(dfuh, buf, size+4))
        goto error;

bye:
    if (buf) free(buf);
    dfu_destroy(dfuh);
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

    dfuh = dfu_create();

    if (!dfu_open(dfuh, pid))
        goto error;

    if (reset)
        if (!dfuh->api->reset_fn(dfuh))
            goto error;

    if (!dfureq_getstate(dfuh, state))
        goto error;

bye:
    dfu_destroy(dfuh);
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
