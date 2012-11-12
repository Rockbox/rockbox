/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 Amaury Pouly
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
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdarg.h>
#include <ctype.h>
#include "misc.h"
#include "elf.h"
#include <sys/stat.h>
#include <openssl/md5.h>
#include "crypt.h"
#include "fwp.h"
#include "keysig_search.h"

#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

bool g_debug = false;
static char *g_out_prefix = NULL;
static char *g_in_file = NULL;
bool g_force = false;
static const char *g_model = NULL;
static int g_model_index = -1;
static char *g_kas = NULL;
static char *g_key = NULL;
static char *g_sig = NULL;

enum keysig_search_method_t g_keysig_search = KEYSIG_SEARCH_NONE;

#define let_the_force_flow(x) do { if(!g_force) return x; } while(0)
#define continue_the_force(x) if(x) let_the_force_flow(x)

#define check_field(v_exp, v_have, str_ok, str_bad) \
    if((v_exp) != (v_have)) \
    { cprintf(RED, str_bad); let_the_force_flow(__LINE__); } \
    else { cprintf(RED, str_ok); }

static void print_hex(void *p, int size, int unit)
{
    uint8_t *p8 = p;
    uint16_t *p16 = p;
    uint32_t *p32 = p;
    for(int i = 0; i < size; i += unit, p8++, p16++, p32++)
    {
        if(i != 0 && (i % 16) == 0)
            printf("\n");
        if(unit == 1)
            printf(" %02x", *p8);
        else if(unit == 2)
            printf(" %04x", *p16);
        else
            printf(" %08x", *p32);
    }
}

static void usage(void);

/* key and signature */
struct nwz_kas_t
{
    char kas[NWZ_KAS_SIZE];
};

#define HAS_KAS     (1 << 0)
#define HAS_KEY     (1 << 1)
#define HAS_SIG     (1 << 2)
#define CONFIRMED   (1 << 3)

struct nwz_model_t
{
    const char *model;
    unsigned flags;
    struct nwz_kas_t kas;
    char key[8];
    char sig[8];
};

struct upg_md5_t
{
    uint8_t md5[16];
}__attribute__((packed));

struct upg_header_t
{
    char sig[8];
    uint32_t nr_files;
    uint32_t pad; // make sure structure size is a multiple of 8
} __attribute__((packed));

struct upg_entry_t
{
    uint32_t offset;
    uint32_t size;
} __attribute__((packed));

struct nwz_model_t g_model_list[] =
{
    { "nwz-e463", HAS_KAS | HAS_KEY | HAS_SIG | CONFIRMED, {"89d813f8f966efdebd9c9e0ea98156d2"}, "eb4431eb", "4f1d9cac" },
    { "nwz-a86x", HAS_KEY | HAS_SIG, {""}, "c824e4e2", "7c262bb0" },
    { "nw-a82x", HAS_KEY | HAS_SIG, {""}, "4df06482", "07fa0b6e" },
};

static int digit_value(char c)
{
    if(c >= '0' && c <= '9') return c - '0';
    if(c >= 'a' && c <= 'f') return c - 'a' + 10;
    if(c >= 'A' && c <= 'F') return c - 'A' + 10;
    return -1;
}

static char hex_digit(unsigned v)
{
    return (v < 10) ? v + '0' : (v < 16) ? v - 10 + 'a' : 'x';
}

static int decrypt_keysig(char keysig[NWZ_KEYSIG_SIZE])
{
    uint8_t src[16];
    for(int i = 32; i < NWZ_KEYSIG_SIZE; i++)
        keysig[i] = 0;
    for(int index = 0; index < 16; index++)
    {
        int a = digit_value(keysig[index * 2]);
        int b = digit_value(keysig[index * 2 + 1]);
        if(a < 0 || b < 0)
        {
            cprintf(GREY, "Invalid KAS !\n");
            return -1;
        }
        src[index] = a << 4 | b;
    }
    fwp_setkey("ed295076");
    fwp_crypt(src, sizeof(src), 1);
    memcpy(keysig + 33, src, 8);
    memcpy(keysig + 42, src + 8, 8);
    return 0;
}

static bool upg_notify_keysig(void *user, uint8_t key[8], uint8_t sig[8])
{
    memcpy(user + 33, key, 8);
    memcpy(user + 42, sig, 8);
    return true;
}

static int do_upg(void *buf, long size)
{
    struct upg_md5_t *md5 = buf;
    cprintf(BLUE, "Preliminary\n");
    cprintf(GREEN, "  MD5: ");
    for(int i = 0; i < 16; i++)
        cprintf(YELLOW, "%02x", md5->md5[i]);
    printf(" ");

    uint8_t actual_md5[MD5_DIGEST_LENGTH];
    {
        MD5_CTX c;
        MD5_Init(&c);
        MD5_Update(&c, md5 + 1, size - sizeof(struct upg_header_t));
        MD5_Final(actual_md5, &c);
    }
    check_field(memcmp(actual_md5, md5->md5, 16), 0, "Ok\n", "Mismatch\n");

    if(g_model_index == -1 && g_keysig_search == KEYSIG_SEARCH_NONE && g_key == NULL && g_kas == NULL)
    {
        cprintf(GREY, "A KAS or a keysig is needed to decrypt the firmware\n");
        cprintf(GREY, "You have the following options(see help for more details):\n");
        cprintf(GREY, "- select a model with a known KAS\n");
        cprintf(GREY, "- specify an explicit KAS or key(+optional sig)\n");
        cprintf(GREY, "- let me try to find the keysig(slow !)\n");
        return 1;
    }

    struct nwz_kas_t kas;
    char keysig[NWZ_KEYSIG_SIZE];

    memset(kas.kas, '?', NWZ_KAS_SIZE);
    memset(keysig, '?', NWZ_KEYSIG_SIZE);
    keysig[32] = keysig[41] = keysig[50] = 0;

    if(g_kas)
    {
        if(strlen(g_kas) != NWZ_KAS_SIZE)
        {
            cprintf(GREY, "The specified KAS has wrong length (must be %d hex digits)\n", NWZ_KAS_SIZE);
            return 4;
        }
        memcpy(keysig, g_kas, NWZ_KAS_SIZE);
        decrypt_keysig(keysig);
        g_kas = keysig;
        g_key = keysig + 33;
        g_sig = keysig + 42;
    }
    else if(g_key)
    {
        if(strlen(g_key) != 8)
        {
            cprintf(GREY, "The specified key has wrong length (must be 8 hex digits)\n");
            return 4;
        }
        if(g_sig && strlen(g_sig) != 8)
        {
            cprintf(GREY, "The specified sig has wrong length (must be 8 hex digits)\n");
            return 5;
        }
        
        memcpy(keysig + 33, g_key, 8);
        if(!g_sig)
            cprintf(GREY, "Warning: you have specified a key but no sig, I won't be able to do any checks\n");
        else
            memcpy(keysig + 42, g_sig, 8);
        g_key = keysig + 33;
        if(g_sig)
            g_sig = keysig + 42;
    }
    else if(g_model_index == -1)
    {
        cprintf(BLUE, "keysig Search\n");
        cprintf_field("  Method: ", "%s\n", keysig_search_desc[g_keysig_search].name);
        bool ok = keysig_search_desc[g_keysig_search].fn((void *)(md5 + 1), &upg_notify_keysig, keysig);
        cprintf(GREEN, "  Result: ");
        cprintf(ok ? YELLOW : RED, "%s\n", ok ? "Key found" : "No key found");
        if(!ok)
            return 2;
        g_key = keysig + 33;
        g_sig = keysig + 42;
    }
    else
    {
        if(g_model_list[g_model_index].flags & HAS_KAS)
            g_kas = g_model_list[g_model_index].kas.kas;
        if(g_model_list[g_model_index].flags & HAS_KEY)
            g_key = g_model_list[g_model_index].key;
        if(g_model_list[g_model_index].flags & HAS_SIG)
            g_sig = g_model_list[g_model_index].sig;

        if(g_kas)
        {
            memcpy(keysig, g_kas, NWZ_KAS_SIZE);
            decrypt_keysig(keysig);
            g_kas = keysig;
            g_key = keysig + 33;
            g_sig = keysig + 42;
        }
        else
        {
            if(g_key)
            {
                memcpy(keysig + 33, g_key, 8);
                g_key = keysig + 33;
            }
            if(g_sig)
            {
                memcpy(keysig + 42, g_sig, 8);
                g_sig = keysig + 42;
            }
        }
    }

    if(!g_kas)
    {
        g_kas = keysig;
        fwp_setkey("ed295076");
        if(g_key)
        {
            memcpy(kas.kas, g_key, 8);
            fwp_crypt(kas.kas, 8, 0);
            for(int i = 0; i < 8; i++)
            {
                g_kas[2 * i] = hex_digit((kas.kas[i] >> 4) & 0xf);
                g_kas[2 * i + 1] = hex_digit(kas.kas[i] & 0xf);
            }
        }
        if(g_sig)
        {
            memcpy(kas.kas + 8, g_sig, 8);
            fwp_crypt(kas.kas + 8, 8, 0);
            for(int i = 8; i < 16; i++)
            {
                g_kas[2 * i] = hex_digit((kas.kas[i] >> 4) & 0xf);
                g_kas[2 * i + 1] = hex_digit(kas.kas[i] & 0xf);
            }
        }
    }

    cprintf(BLUE, "Keys\n");
    cprintf_field("  KAS: ", "%."STR(NWZ_KAS_SIZE)"s\n", g_kas);
    cprintf_field("  Key: ", "%s\n", g_key);
    if(g_sig)
        cprintf_field("  Sig: ", "%s\n", g_sig);

    struct upg_header_t *hdr = (void *)(md5 + 1);
    int ret = fwp_read(hdr, sizeof(struct upg_header_t), hdr, (void *)g_key);
    if(ret)
        return ret;

    cprintf(BLUE, "Header\n");
    cprintf_field("  Signature:", " ");
    for(int i = 0; i < 8; i++)
        cprintf(YELLOW, "%c", isprint(hdr->sig[i]) ? hdr->sig[i] : '.');
    if(g_sig)
    {
        check_field(memcmp(hdr->sig, g_sig, 8), 0, " OK\n", " Mismatch\n");
    }
    else
        cprintf(RED, " Can't check\n");
    cprintf_field("  Files: ", "%d\n", hdr->nr_files);
    cprintf_field("  Pad: ", "0x%x\n", hdr->pad);

    cprintf(BLUE, "Files\n");
    struct upg_entry_t *entry = (void *)(hdr + 1);
    for(unsigned i = 0; i < hdr->nr_files; i++, entry++)
    {
        int ret = fwp_read(entry, sizeof(struct upg_entry_t), entry, (void *)g_key);
        if(ret)
            return ret;
        cprintf(GREY, "  File");
        cprintf(RED, " %d\n", i);
        cprintf_field("    Offset: ", "0x%x\n", entry->offset);
        cprintf_field("    Size: ", "0x%x\n", entry->size);

        if(g_out_prefix)
        {
            char *str = malloc(strlen(g_out_prefix) + 32);
            sprintf(str, "%s%d.bin", g_out_prefix, i);
            FILE *f = fopen(str, "wb");
            if(f)
            {
                // round up size, there is some padding done with random data
                int crypt_size = ROUND_UP(entry->size, 8);
                int ret = fwp_read(buf + entry->offset, crypt_size,
                    buf + entry->offset, (void *)g_key);
                if(ret)
                    return ret;
                // but write the *good* amount of data
                fwrite(buf + entry->offset, 1, entry->size, f);
                
                fclose(f);
            }
            else
                cprintf(GREY, "Cannot open '%s' for writing\n", str);
        }
    }

    return 0;
}

static int extract_upg(int argc, char **argv)
{
    if(argc == 0 || argc > 1)
    {
        if(argc == 0)
            printf("You must specify a firmware file\n");
        else
            printf("Extra arguments after firmware file\n");
        usage();
    }

    g_in_file = argv[0];
    FILE *fin = fopen(g_in_file, "r");
    if(fin == NULL)
    {
        perror("Cannot open boot file");
        return 1;
    }
    fseek(fin, 0, SEEK_END);
    long size = ftell(fin);
    fseek(fin, 0, SEEK_SET);

    void *buf = malloc(size);
    if(buf == NULL)
    {
        perror("Cannot allocate memory");
        return 1;
    }

    if(fread(buf, size, 1, fin) != 1)
    {
        perror("Cannot read file");
        return 1;
    }

    fclose(fin);

    int ret = do_upg(buf, size);
    if(ret != 0)
    {
        cprintf(GREY, "Error: %d", ret);
        if(!g_force)
            cprintf(GREY, " (use --force to force processing)");
        printf("\n");
        ret = 2;
    }
    free(buf);

    return ret;
}

static long filesize(FILE *f)
{
    long pos = ftell(f);
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, pos, SEEK_SET);
    return size;
}

static int create_upg(int argc, char **argv)
{
    if(argc == 0)
    {
        printf("You must specify a firmware filename\n");
        usage();
    }
    
    if(g_model_index == -1 && (g_key == NULL || g_sig == NULL) && g_kas == NULL)
    {
        cprintf(GREY, "A KAS or a keysig is needed to encrypt the firmware\n");
        cprintf(GREY, "You have the following options(see help for more details):\n");
        cprintf(GREY, "- select a model with a known KAS\n");
        cprintf(GREY, "- specify an explicit KAS or key+sig\n");
        return 1;
    }

    struct nwz_kas_t kas;
    char keysig[NWZ_KEYSIG_SIZE];

    memset(kas.kas, '?', NWZ_KAS_SIZE);
    memset(keysig, '?', NWZ_KEYSIG_SIZE);
    keysig[32] = keysig[41] = keysig[50] = 0;

    if(g_kas)
    {
        if(strlen(g_kas) != NWZ_KAS_SIZE)
        {
            cprintf(GREY, "The specified KAS has wrong length (must be %d hex digits)\n", NWZ_KAS_SIZE);
            return 4;
        }
        memcpy(keysig, g_kas, NWZ_KAS_SIZE);
        decrypt_keysig(keysig);
        g_kas = keysig;
        g_key = keysig + 33;
        g_sig = keysig + 42;
    }
    else if(g_key)
    {
        if(strlen(g_key) != 8)
        {
            cprintf(GREY, "The specified key has wrong length (must be 8 hex digits)\n");
            return 4;
        }
        if(strlen(g_sig) != 8)
        {
            cprintf(GREY, "The specified sig has wrong length (must be 8 hex digits)\n");
            return 5;
        }

        memcpy(keysig + 33, g_key, 8);
        if(!g_sig)
            cprintf(GREY, "Warning: you have specified a key but no sig, I won't be able to do any checks\n");
        else
            memcpy(keysig + 42, g_sig, 8);
        g_key = keysig + 33;
        g_sig = keysig + 42;
    }
    else if(g_model_index != -1)
    {
        if(g_model_list[g_model_index].flags & HAS_KAS)
            g_kas = g_model_list[g_model_index].kas.kas;
        if(g_model_list[g_model_index].flags & HAS_KEY)
            g_key = g_model_list[g_model_index].key;
        if(g_model_list[g_model_index].flags & HAS_SIG)
            g_sig = g_model_list[g_model_index].sig;

        if(g_key && g_sig)
        {
            memcpy(keysig + 33, g_key, 8);
            g_key = keysig + 33;
            memcpy(keysig + 42, g_sig, 8);
            g_sig = keysig + 42;
        }
        else if(g_kas)
        {
            memcpy(keysig, g_kas, NWZ_KAS_SIZE);
            decrypt_keysig(keysig);
            g_kas = keysig;
            g_key = keysig + 33;
            g_sig = keysig + 42;
        }
        else
        {
            printf("Target doesn't have enough information to get key and sig\n");
            return 1;
        }
    }
    else
    {
        printf("Kill me\n");
        return 1;
    }

    if(!g_kas)
    {
        g_kas = keysig;
        fwp_setkey("ed295076");
        memcpy(kas.kas, g_key, 8);
        fwp_crypt(kas.kas, 8, 0);
        for(int i = 0; i < 8; i++)
        {
            g_kas[2 * i] = hex_digit((kas.kas[i] >> 4) & 0xf);
            g_kas[2 * i + 1] = hex_digit(kas.kas[i] & 0xf);
        }
        memcpy(kas.kas + 8, g_sig, 8);
        fwp_crypt(kas.kas + 8, 8, 0);
        for(int i = 8; i < 16; i++)
        {
            g_kas[2 * i] = hex_digit((kas.kas[i] >> 4) & 0xf);
            g_kas[2 * i + 1] = hex_digit(kas.kas[i] & 0xf);
        }
    }

    cprintf(BLUE, "Keys\n");
    cprintf_field("  KAS: ", "%."STR(NWZ_KAS_SIZE)"s\n", g_kas);
    cprintf_field("  Key: ", "%s\n", g_key);
    if(g_sig)
        cprintf_field("  Sig: ", "%s\n", g_sig);

    FILE *fout = fopen(argv[0], "wb");
    if(fout == NULL)
    {
        printf("Cannot open output firmware file: %m\n");
        return 1;
    }

    int nr_files = argc - 1;
    FILE **files = malloc(nr_files * sizeof(FILE *));

    for(int i = 0; i < nr_files; i++)
    {
        files[i] = fopen(argv[1 + i], "rb");
        if(files[i] == NULL)
        {
            printf("Cannot open input file '%s': %m\n", argv[i + 1]);
            return 1;
        }
    }

    struct upg_md5_t md5;
    memset(&md5, 0, sizeof(md5));
    MD5_CTX c;
    MD5_Init(&c);
    // output a dummy md5 sum
    fwrite(&md5, 1, sizeof(md5), fout);
    // output the encrypted signature
    struct upg_header_t hdr;
    memcpy(hdr.sig, g_sig, 8);
    hdr.nr_files = nr_files;
    hdr.pad = 0;
    
    int ret = fwp_write(&hdr, sizeof(hdr), &hdr, (void *)g_key);
    if(ret)
        return ret;
    MD5_Update(&c, &hdr, sizeof(hdr));
    fwrite(&hdr, 1, sizeof(hdr), fout);

    // output file headers
    long offset = sizeof(md5) + sizeof(hdr) + nr_files * sizeof(struct upg_entry_t);
    for(int i = 0; i < nr_files; i++)
    {
        struct upg_entry_t entry;
        entry.offset = offset;
        entry.size = filesize(files[i]);
        offset += ROUND_UP(entry.size, 8); // do it before encryption !!
        
        ret = fwp_write(&entry, sizeof(entry), &entry, (void *)g_key);
        if(ret)
            return ret;
        MD5_Update(&c, &entry, sizeof(entry));
        fwrite(&entry, 1, sizeof(entry), fout);
    }

    cprintf(BLUE, "Files\n");
    for(int i = 0; i < nr_files; i++)
    {
        long size = filesize(files[i]);
        long r_size = ROUND_UP(size, 8);
        cprintf(GREY, "  File");
        cprintf(RED, " %d\n", i);
        cprintf_field("    Offset: ", "0x%lx\n", ftell(fout));
        cprintf_field("    Size: ", "0x%lx\n", size);

        void *buf = malloc(r_size);
        memset(buf, 0, r_size);
        fread(buf, 1, size, files[i]);
        fclose(files[i]);

        ret = fwp_write(buf, r_size, buf, (void *)g_key);
        if(ret)
            return ret;
        MD5_Update(&c, buf, r_size);
        fwrite(buf, 1, r_size, fout);

        free(buf);
    }

    fseek(fout, 0, SEEK_SET);
    MD5_Final(md5.md5, &c);
    fwrite(&md5, 1, sizeof(md5), fout);
    fclose(fout);

    return 0;
}

static void usage(void)
{
    color(OFF);
    printf("Usage: upgtool [options] firmware [files...]\n");
    printf("Options:\n");
    printf("  -o <prefix>\t\tSet output prefix\n");
    printf("  -f/--force\t\tForce to continue on errors\n");
    printf("  -?/--help\t\tDisplay this message\n");
    printf("  -d/--debug\t\tDisplay debug messages\n");
    printf("  -c/--no-color\t\tDisable color output\n");
    printf("  -m/--model <model>\tSelect model (or ? to list them)\n");
    printf("  -l/--search <method>\tTry to find the keysig (implies -e)\n");
    printf("  -a/--kas <kas>\tForce KAS\n");
    printf("  -k/--key <key>\tForce key\n");
    printf("  -s/--sig <sig>\tForce sig\n");
    printf("  -e/--extract\t\tExtract a UPG archive\n");
    printf("  -c/--create\t\tCreate a UPG archive\n");
    printf("keysig search method:\n");
    for(int i = KEYSIG_SEARCH_FIRST; i < KEYSIG_SEARCH_LAST; i++)
        printf("  %s\t%s\n", keysig_search_desc[i].name, keysig_search_desc[i].comment);
    exit(1);
}

int main(int argc, char **argv)
{
    bool extract = false;
    bool create = false;

    if(argc <= 1)
        usage();
    
    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, '?'},
            {"debug", no_argument, 0, 'd'},
            {"no-color", no_argument, 0, 'n'},
            {"force", no_argument, 0, 'f'},
            {"model", required_argument, 0, 'm'},
            {"search", required_argument, 0, 'l'},
            {"kas", required_argument, 0, 'a'},
            {"key", required_argument, 0, 'k'},
            {"sig", required_argument, 0, 's'},
            {"extract", no_argument, 0, 'e'},
            {"create", no_argument, 0 ,'c'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "?dnfo:m:l:a:k:s:ec", long_options, NULL);
        if(c == -1)
            break;
        switch(c)
        {
            case -1:
                break;
            case 'n':
                enable_color(false);
                break;
            case 'd':
                g_debug = true;
                break;
            case 'f':
                g_force = true;
                break;
            case '?':
                usage();
                break;
            case 'o':
                g_out_prefix = optarg;
                break;
            case 'm':
                g_model = optarg;
                break;
            case 'l':
                g_keysig_search = KEYSIG_SEARCH_NONE;
                for(int i = KEYSIG_SEARCH_FIRST; i < KEYSIG_SEARCH_LAST; i++)
                    if(strcmp(keysig_search_desc[i].name, optarg) == 0)
                        g_keysig_search = i;
                if(g_keysig_search == KEYSIG_SEARCH_NONE)
                {
                    cprintf(GREY, "Unknown keysig search method '%s'\n", optarg);
                    return 1;
                }
                extract = true;
                break;
            case 'a':
                g_kas = optarg;
                break;
            case 'k':
                g_key = optarg;
                break;
            case 's':
                g_sig = optarg;
                break;
            case 'e':
                extract = true;
                break;
            case 'c':
                create = true;
                break;
            default:
                abort();
        }
    }

    if(g_model && strcmp(g_model, "?") == 0)
    {
        cprintf(BLUE, "Model list:\n");
        for(unsigned i = 0; i < sizeof(g_model_list) / sizeof(g_model_list[0]); i++)
        {
            cprintf(GREEN, "  %s:", g_model_list[i].model);
            if(g_model_list[i].flags & HAS_KAS)
            {
                cprintf(RED, " kas=");
                cprintf(YELLOW, "%."STR(NWZ_KAS_SIZE)"s", g_model_list[i].kas.kas);
            }
            if(g_model_list[i].flags & HAS_KEY)
            {
                cprintf(RED, " key=");
                cprintf(YELLOW, "%.8s", g_model_list[i].key);
            }
            if(g_model_list[i].flags & HAS_SIG)
            {
                cprintf(RED, " sig=");
                cprintf(YELLOW, "%.8s", g_model_list[i].sig);
            }
            if(g_model_list[i].flags & CONFIRMED)
                cprintf(RED, " confirmed");
            else
                cprintf(RED, " guessed");
            printf("\n");
        }
        return 1;
    }

    if(g_model)
    {
        for(unsigned i = 0; i < sizeof(g_model_list) / sizeof(g_model_list[0]); i++)
            if(strcmp(g_model, g_model_list[i].model) == 0)
                g_model_index = i;
        if(g_model_index == -1)
            cprintf(GREY, "Warning: unknown model %s\n", g_model);
    }

    if(!create && !extract)
    {
        printf("You must specify an action (extract or create)\n");
        return 1;
    }

    if(create && extract)
    {
        printf("You cannot specify both create and extract\n");
        return 1;
    }

    int ret = 0;
    if(create)
        ret = create_upg(argc - optind, argv + optind);
    else if(extract)
        ret = extract_upg(argc - optind, argv + optind);
    else
    {
        printf("Die from lack of action\n");
        ret = 1;
    }

    color(OFF);

    return ret;
}

