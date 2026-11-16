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

/* Raw NAND dump of ATJ213x devices through hwstub. Uploads helper.bin to
 * SDRAM and calls it repeatedly to read batches of raw pages (main + spare,
 * no ECC processing) which are appended to the output file.
 * Only NAND read commands are ever issued.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <time.h>
#include <sys/stat.h>
#include <iostream>
#include <vector>
#include "hwstub.hpp"
#include "hwstub_uri.hpp"
#include "hwstub_usb.hpp"
#include <chrono>
#include "helper.h"

using namespace hwstub;

static std::shared_ptr<handle> g_dev;

static void usage(void)
{
    fprintf(stderr, "usage: atjnanddump [options] <output file>\n");
    fprintf(stderr, "options:\n");
    fprintf(stderr, "  -d <uri>          hwstub device URI\n");
    fprintf(stderr, "  -H <file>         helper binary (default: helper.bin)\n");
    fprintf(stderr, "  -c <ce>           chip enable (default 0)\n");
    fprintf(stderr, "  -s <page>         first page (default 0)\n");
    fprintf(stderr, "  -n <pages>        number of pages (default 8192*128)\n");
    fprintf(stderr, "  -p <bytes>        bytes per page incl. spare (default 4096+224)\n");
    fprintf(stderr, "  -b <pages>        max pages per helper call (default: buffer limit),\n");
    fprintf(stderr, "                    actual batch adapts to keep each call short\n");
    fprintf(stderr, "  -t <clk>:<fifo>   CMU_NANDCLK:NAND_FIFOTIM (default 8:4)\n");
    fprintf(stderr, "  -C <bytes>        bytes per DMA transfer (default 512)\n");
    fprintf(stderr, "  -e <val>          NAND_ECCMODE value during reads (default 1)\n");
    fprintf(stderr, "  -w <us>           delay after page load command (default 0)\n");
    fprintf(stderr, "  -r                resume: continue after data already in output\n");
    fprintf(stderr, "  -i                only print NAND ID\n");
    exit(1);
}

static bool xfer_params(struct helper_params_t& p, bool to_dev)
{
    size_t sz = sizeof(p);
    error ret = to_dev ? g_dev->write(HELPER_PARAMS, &p, sz, false)
                       : g_dev->read(HELPER_PARAMS, &p, sz, false);
    if (ret != error::SUCCESS || sz != sizeof(p))
    {
        fprintf(stderr, "parameter %s failed: %s\n", to_dev ? "write" : "read",
                error_string(ret).c_str());
        return false;
    }
    return true;
}

static bool call_helper(struct helper_params_t& p)
{
    p.magic = HELPER_MAGIC;
    p.status = 0xffffffff;
    if (!xfer_params(p, true))
        return false;
    error ret = g_dev->exec(HELPER_ENTRY, HWSTUB_EXEC_CALL);
    if (ret != error::SUCCESS)
    {
        fprintf(stderr, "helper call failed: %s\n", error_string(ret).c_str());
        return false;
    }
    if (!xfer_params(p, false))
        return false;
    if (p.status != ST_OK)
    {
        fprintf(stderr, "helper op %u failed: status %u, done %u, dbg %#x\n",
                p.op, p.status, p.done, p.dbg);
        return false;
    }
    return true;
}

static bool load_helper(const char *path)
{
    FILE *f = fopen(path, "rb");
    if (f == NULL)
    {
        fprintf(stderr, "cannot open helper %s\n", path);
        return false;
    }
    std::vector<uint8_t> buf(HELPER_MAX_SIZE);
    size_t sz = fread(buf.data(), 1, buf.size(), f);
    fclose(f);
    if (sz == 0 || sz == buf.size())
    {
        fprintf(stderr, "bad helper size\n");
        return false;
    }
    sz = (sz + 3) & ~3;
    size_t wsz = sz;
    error ret = g_dev->write(HELPER_BASE, buf.data(), wsz, false);
    if (ret != error::SUCCESS || wsz != sz)
    {
        fprintf(stderr, "helper upload failed: %s\n", error_string(ret).c_str());
        return false;
    }
    /* read back to make sure upload is fine */
    std::vector<uint8_t> chk(sz);
    size_t rsz = sz;
    ret = g_dev->read(HELPER_BASE, chk.data(), rsz, false);
    if (ret != error::SUCCESS || rsz != sz || memcmp(chk.data(), buf.data(), sz))
    {
        fprintf(stderr, "helper verify failed\n");
        return false;
    }
    return true;
}

int main(int argc, char **argv)
{
    std::string uri_str = uri::default_uri().full_uri();
    const char *helper = "helper.bin";
    uint32_t ce = 0, start = 0, count = 8192 * 128, page_bytes = 4096 + 224;
    uint32_t batch = 0, nandclk = 8, fifotim = 4, chunk = 0x200, eccmode = 1;
    uint32_t load_delay = 0;
    bool resume = false, id_only = false;

    int c;
    while ((c = getopt(argc, argv, "d:H:c:s:n:p:b:t:C:e:w:ri")) != -1)
    {
        switch (c)
        {
            case 'd': uri_str = optarg; break;
            case 'H': helper = optarg; break;
            case 'c': ce = strtoul(optarg, NULL, 0); break;
            case 's': start = strtoul(optarg, NULL, 0); break;
            case 'n': count = strtoul(optarg, NULL, 0); break;
            case 'p': page_bytes = strtoul(optarg, NULL, 0); break;
            case 'b': batch = strtoul(optarg, NULL, 0); break;
            case 't':
            {
                char *end;
                nandclk = strtoul(optarg, &end, 0);
                if (*end != ':')
                    usage();
                fifotim = strtoul(end + 1, NULL, 0);
                break;
            }
            case 'C': chunk = strtoul(optarg, NULL, 0); break;
            case 'e': eccmode = strtoul(optarg, NULL, 0); break;
            case 'w': load_delay = strtoul(optarg, NULL, 0); break;
            case 'r': resume = true; break;
            case 'i': id_only = true; break;
            default: usage();
        }
    }

    if (!id_only && optind + 1 != argc)
        usage();
    if (batch == 0)
        batch = HELPER_BUF_SIZE / page_bytes;
    if (page_bytes & 3 || batch == 0 || (uint64_t)batch * page_bytes > HELPER_BUF_SIZE)
    {
        fprintf(stderr, "bad page size / batch combination\n");
        return 1;
    }

    std::string errstr;
    std::shared_ptr<context> ctx = uri::create_context(uri::uri(uri_str), &errstr);
    if (!ctx)
    {
        fprintf(stderr, "cannot create context: %s\n", errstr.c_str());
        return 1;
    }
    std::vector<std::shared_ptr<device>> list;
    if (ctx->get_device_list(list) != error::SUCCESS || list.empty())
    {
        fprintf(stderr, "no hwstub device found\n");
        return 1;
    }
    if (list[0]->open(g_dev) != error::SUCCESS)
    {
        fprintf(stderr, "cannot open device\n");
        return 1;
    }

    /* helper calls are synchronous control transfers, allow long ones */
    usb::handle *uh = dynamic_cast<usb::handle *>(g_dev.get());
    if (uh)
        uh->set_timeout(std::chrono::milliseconds(10000));

    if (!load_helper(helper))
        return 2;

    struct helper_params_t p;
    memset(&p, 0, sizeof(p));
    p.op = OP_INIT;
    p.ce = ce;
    p.nandclk = nandclk;
    p.fifotim = fifotim;
    if (!call_helper(p))
        return 3;

    p.op = OP_READID;
    p.ce = ce;
    if (!call_helper(p))
        return 3;
    uint8_t id[4];
    size_t sz = sizeof(id);
    g_dev->read(HELPER_BUF, id, sz, false);
    printf("CE%u ID: %02x %02x %02x %02x\n", ce, id[0], id[1], id[2], id[3]);
    if (id_only)
        return 0;

    const char *out = argv[optind];
    if (resume)
    {
        struct stat st;
        if (stat(out, &st) == 0)
        {
            uint32_t have = st.st_size / page_bytes;
            if ((uint64_t)have * page_bytes != (uint64_t)st.st_size)
            {
                fprintf(stderr, "output size is not a multiple of page size\n");
                return 1;
            }
            if (have >= count)
            {
                printf("nothing to do\n");
                return 0;
            }
            start += have;
            count -= have;
            printf("resuming at page %u\n", start);
        }
    }

    FILE *f = fopen(out, resume ? "ab" : "wb");
    if (f == NULL)
    {
        fprintf(stderr, "cannot open %s\n", out);
        return 1;
    }

    std::vector<uint8_t> buf((size_t)batch * page_bytes);
    time_t t0 = time(NULL);
    uint64_t total = 0;

    /* start small and adapt batch so one call takes about TARGET_MS */
    const double TARGET_MS = 400;
    uint32_t cur_batch = std::min(batch, 8u);

    for (uint32_t done = 0; done < count; )
    {
        uint32_t n = std::min(cur_batch, count - done);
        auto c0 = std::chrono::steady_clock::now();

        p.op = OP_READ;
        p.ce = ce;
        p.page = start + done;
        p.npages = n;
        p.page_bytes = page_bytes;
        p.chunk = chunk;
        p.eccmode = eccmode;
        p.load_delay = load_delay;
        if (!call_helper(p))
        {
            fprintf(stderr, "stopped at page %u, rerun with -r to resume\n", start + done);
            fclose(f);
            return 4;
        }
        double ms = std::chrono::duration<double, std::milli>(
            std::chrono::steady_clock::now() - c0).count();
        if (ms > 0)
        {
            double want = n * TARGET_MS / ms;
            uint32_t nb = want < 1 ? 1 : (want > batch ? batch : (uint32_t)want);
            /* grow at most 2x per step to stay well within USB timeout */
            cur_batch = std::min(nb, std::max(cur_batch * 2, 1u));
        }

        size_t len = (size_t)n * page_bytes;
        sz = len;
        error ret = g_dev->read(HELPER_BUF, buf.data(), sz, false);
        if (ret != error::SUCCESS || sz != len)
        {
            fprintf(stderr, "buffer read failed at page %u: %s\n", start + done,
                    error_string(ret).c_str());
            fclose(f);
            return 4;
        }
        if (fwrite(buf.data(), 1, len, f) != len)
        {
            fprintf(stderr, "write to %s failed\n", out);
            fclose(f);
            return 5;
        }
        fflush(f);

        done += n;
        total += len;
        double el = difftime(time(NULL), t0);
        double rate = el > 0 ? total / el : 0;
        double eta = rate > 0 ? (double)(count - done) * page_bytes / rate : 0;
        printf("\rpage %u/%u  %.2f MB/s  batch %u  ETA %.0f min   ", start + done,
               start + count, rate / 1e6, cur_batch, eta / 60);
        fflush(stdout);
    }

    printf("\ndone\n");
    fclose(f);
    return 0;
}
