/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2026 by Marcin Bukat
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

/* Replays an ADFU session recorded from the Actions ProductTool
 * (converted with usbmon_adfu.py). Every device response and CSW status
 * is compared against the recording and replay stops on first mismatch.
 */

#include <stdlib.h>
#include <libusb.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#define VERSION "v0.1"

#define USB_TIMEOUT 30000
#define VENDORID  0x10d6
#define PRODUCTID 0xff96

#define OUT_EP  0x01
#define IN_EP   0x82

#define DIR_NONE 0
#define DIR_OUT  1
#define DIR_IN   2

struct rec_t
{
    uint8_t cbw[31];
    uint8_t dir;
    uint8_t csw;
    uint16_t reserved;
    uint32_t chunk;
    uint32_t len;
    uint8_t *data;
    uint8_t *volatile_mask;     /* IN data: 1 = byte may differ */
};

static uint32_t get32(const uint8_t *p)
{
    return p[0] | p[1] << 8 | p[2] << 16 | (uint32_t)p[3] << 24;
}

static void mark_volatile(struct rec_t *recs, uint32_t count);

static struct rec_t *load_script(const char *path, uint32_t *count)
{
    FILE *f = fopen(path, "rb");
    uint8_t hdr[12], rh[43];
    struct rec_t *recs;
    uint32_t i;

    if (f == NULL)
    {
        fprintf(stderr, "[error]: can't open %s\n", path);
        return NULL;
    }

    if (fread(hdr, 1, 12, f) != 12 || memcmp(hdr, "ATJRPL01", 8))
    {
        fprintf(stderr, "[error]: %s is not a replay script\n", path);
        fclose(f);
        return NULL;
    }

    *count = get32(hdr + 8);
    recs = calloc(*count, sizeof(struct rec_t));

    for (i = 0; recs && i < *count; i++)
    {
        struct rec_t *r = &recs[i];

        if (fread(rh, 1, sizeof(rh), f) != sizeof(rh))
        {
            goto err;
        }

        memcpy(r->cbw, rh, 31);
        r->dir = rh[31];
        r->csw = rh[32];
        r->chunk = get32(rh + 35);
        r->len = get32(rh + 39);

        if (r->len)
        {
            r->data = malloc(r->len);
            if (r->data == NULL || fread(r->data, 1, r->len, f) != r->len)
            {
                goto err;
            }
        }
    }

    fclose(f);
    mark_volatile(recs, *count);
    return recs;

err:
    fprintf(stderr, "[error]: truncated replay script\n");
    fclose(f);
    return NULL;
}

/* Responses to identical commands that differ within the recording itself
 * (a counter or timestamp in the FWSC status, e.g. bytes 0x5a-0x5d of the
 * 144 byte reply) can not be expected to match on replay: such bytes are
 * not compared, everything else is. */
static void mark_volatile(struct rec_t *recs, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++)
    {
        struct rec_t *a = &recs[i];
        if (a->dir != DIR_IN || a->len == 0)
        {
            continue;
        }
        for (uint32_t j = 0; j < count; j++)
        {
            struct rec_t *b = &recs[j];
            if (j == i || b->dir != DIR_IN || b->len != a->len ||
                memcmp(a->cbw + 15, b->cbw + 15, 16))
            {
                continue;
            }
            for (uint32_t k = 0; k < a->len; k++)
            {
                if (a->data[k] != b->data[k])
                {
                    if (a->volatile_mask == NULL)
                    {
                        a->volatile_mask = calloc(a->len, 1);
                    }
                    if (a->volatile_mask)
                    {
                        a->volatile_mask[k] = 1;
                    }
                }
            }
        }
    }
}

static void describe(int i, struct rec_t *r)
{
    const uint8_t *cb = r->cbw + 15;

    printf("%3d: op=%02x a=%08x b=%08x c=%08x", i, cb[0],
           get32(cb + 1), get32(cb + 5), get32(cb + 9));
    if (r->dir == DIR_OUT)
    {
        printf(" out %u", r->len);
    }
    else if (r->dir == DIR_IN)
    {
        printf(" in %u", r->len);
    }
    printf("\n");
}

static int bulk(libusb_device_handle *h, uint8_t ep, uint8_t *buf, int len, int *xfered)
{
    int ret = libusb_bulk_transfer(h, ep, buf, len, xfered, USB_TIMEOUT);

    if (ret == LIBUSB_ERROR_PIPE)
    {
        libusb_clear_halt(h, ep);
        ret = libusb_bulk_transfer(h, ep, buf, len, xfered, USB_TIMEOUT);
    }

    return ret;
}

static int replay_one(libusb_device_handle *h, struct rec_t *r)
{
    uint8_t csw[13];
    int xfered, ret;
    uint32_t done, n;

    ret = bulk(h, OUT_EP, r->cbw, 31, &xfered);
    if (ret || xfered != 31)
    {
        fprintf(stderr, "[error]: CBW transfer failed (%s)\n", libusb_error_name(ret));
        return -1;
    }

    if (r->dir == DIR_OUT)
    {
        uint32_t chunk = r->chunk ? r->chunk : 16384;

        for (done = 0; done < r->len; done += n)
        {
            n = r->len - done < chunk ? r->len - done : chunk;
            ret = bulk(h, OUT_EP, r->data + done, n, &xfered);
            if (ret || (uint32_t)xfered != n)
            {
                fprintf(stderr, "[error]: data out failed at %u (%s)\n",
                        done, libusb_error_name(ret));
                return -1;
            }
        }
    }
    else if (r->dir == DIR_IN)
    {
        uint8_t *buf = malloc(r->len);

        if (buf == NULL)
        {
            return -1;
        }

        ret = bulk(h, IN_EP, buf, r->len, &xfered);
        if (ret || (uint32_t)xfered != r->len)
        {
            fprintf(stderr, "[error]: data in failed (%s), got %d bytes\n",
                    libusb_error_name(ret), xfered);
            free(buf);
            return -1;
        }

        uint32_t ndiff = 0, nvol = 0;
        for (uint32_t i = 0; i < r->len; i++)
        {
            if (buf[i] == r->data[i])
            {
                continue;
            }
            if (r->volatile_mask && r->volatile_mask[i])
            {
                nvol++;
                continue;
            }
            if (ndiff++ == 0)
            {
                fprintf(stderr, "[error]: device response differs from recording\n");
            }
            if (ndiff <= 32)
            {
                fprintf(stderr, "  +0x%03x: got %02x expected %02x\n", i, buf[i],
                        r->data[i]);
            }
        }
        if (nvol)
        {
            printf("     %u volatile byte(s) differ, ignored\n", nvol);
        }
        if (ndiff)
        {
            fprintf(stderr, "  %u byte(s) differ\n", ndiff);
            free(buf);
            return -2;
        }
        free(buf);
    }

    ret = bulk(h, IN_EP, csw, sizeof(csw), &xfered);
    if (ret || xfered != 13 || get32(csw) != 0x53425355)
    {
        fprintf(stderr, "[error]: bad CSW (%s)\n", libusb_error_name(ret));
        return -1;
    }

    if (csw[12] != r->csw)
    {
        fprintf(stderr, "[error]: CSW status %d, expected %d\n", csw[12], r->csw);
        return -3;
    }

    return 0;
}

static void usage(const char *name)
{
    printf("usage: (sudo) %s [-u vid:pid] [-l] [-s N] script.rpl\n", name);
    printf("-l          only list commands of the script\n");
    printf("-s N        stop after command N (inclusive)\n");
    printf("-u vid:pid  override device VID:PID (default 0x%04x:0x%04x)\n",
           VENDORID, PRODUCTID);
}

int main(int argc, char **argv)
{
    uint16_t vid = VENDORID, pid = PRODUCTID;
    const char *script = NULL;
    bool list = false;
    long stop = -1;
    struct rec_t *recs;
    uint32_t count, i;
    libusb_device_handle *h;
    int ret = 0, a;

    for (a = 1; a < argc; a++)
    {
        if (!strcmp(argv[a], "-l"))
        {
            list = true;
        }
        else if (!strcmp(argv[a], "-s") && a + 1 < argc)
        {
            stop = strtol(argv[++a], NULL, 0);
        }
        else if (!strcmp(argv[a], "-u") && a + 1 < argc)
        {
            char *end;
            vid = strtoul(argv[++a], &end, 0);
            if (*end != ':')
            {
                usage(argv[0]);
                return 1;
            }
            pid = strtoul(end + 1, NULL, 0);
        }
        else if (argv[a][0] != '-' && script == NULL)
        {
            script = argv[a];
        }
        else
        {
            usage(argv[0]);
            return 1;
        }
    }

    if (script == NULL)
    {
        usage(argv[0]);
        return 1;
    }

    fprintf(stderr, "atjflash " VERSION "\n");

    recs = load_script(script, &count);
    if (recs == NULL)
    {
        return 2;
    }

    if (stop < 0 || (uint32_t)stop >= count)
    {
        stop = count - 1;
    }

    if (list)
    {
        for (i = 0; i < count; i++)
        {
            describe(i, &recs[i]);
        }
        return 0;
    }

    libusb_init(NULL);
    h = libusb_open_device_with_vid_pid(NULL, vid, pid);
    if (h == NULL)
    {
        fprintf(stderr, "[error]: can't open device %04x:%04x\n", vid, pid);
        libusb_exit(NULL);
        return 3;
    }

    if (libusb_kernel_driver_active(h, 0) == 1)
    {
        libusb_detach_kernel_driver(h, 0);
    }

    if (libusb_set_configuration(h, 1) < 0 || libusb_claim_interface(h, 0) < 0)
    {
        fprintf(stderr, "[error]: can't claim interface\n");
        ret = 4;
        goto end;
    }

    for (i = 0; i <= (uint32_t)stop; i++)
    {
        describe(i, &recs[i]);
        fflush(stdout);
        ret = replay_one(h, &recs[i]);
        if (ret)
        {
            fprintf(stderr, "[error]: replay stopped at command %u\n", i);
            ret = 10;
            goto end;
        }
    }

    printf("replayed commands 0..%ld successfully\n", stop);

end:
    libusb_close(h);
    libusb_exit(NULL);
    return ret;
}
