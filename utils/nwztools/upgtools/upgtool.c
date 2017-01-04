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
#include "crypt.h"
#include "fwp.h"
#include "keysig_search.h"
#include "upg.h"

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
static int g_nr_threads = 1;

enum keysig_search_method_t g_keysig_search = KEYSIG_SEARCH_NONE;

#define let_the_force_flow(x) do { if(!g_force) return x; } while(0)
#define continue_the_force(x) if(x) let_the_force_flow(x)

#define check_field(v_exp, v_have, str_ok, str_bad) \
    if((v_exp) != (v_have)) \
    { cprintf(RED, str_bad); let_the_force_flow(__LINE__); } \
    else { cprintf(RED, str_ok); }

#define cprintf(col, ...) do {color(col); printf(__VA_ARGS__); }while(0)

#define cprintf_field(str1, ...) do{ cprintf(GREEN, str1); cprintf(YELLOW, __VA_ARGS__); }while(0)

static void usage(void);

/* user needs to be pointer to a NWZ_KEYSIG_SIZE-byte buffer, on success g_key
 * and g_sig are updated to point to the key and sig in the buffer */
static bool upg_notify_keysig(void *user, uint8_t key[NWZ_KEY_SIZE],
    uint8_t sig[NWZ_SIG_SIZE])
{
    g_key = user;
    g_sig = user + NWZ_KEY_SIZE;
    memcpy(g_key, key, NWZ_KEY_SIZE);
    memcpy(g_sig, sig, NWZ_SIG_SIZE);
    return true;
}

static int get_key_and_sig(bool is_extract, void *buf)
{
    static char keysig[NWZ_KEYSIG_SIZE];
    static char kas[NWZ_KAS_SIZE];
    /* database lookup */
    if(g_model_index != -1)
        g_kas = g_model_list[g_model_index].kas;

    /* always prefer KAS because it contains everything */
    if(g_kas)
    {
        if(strlen(g_kas) != NWZ_KAS_SIZE)
        {
            cprintf(GREY, "The KAS has wrong length (must be %d hex digits)\n", NWZ_KAS_SIZE);
            return 4;
        }
        g_key = keysig;
        g_sig = keysig + NWZ_KEY_SIZE;
        decrypt_keysig(g_kas, g_key, g_sig);
    }
    /* Otherwise require key and signature */
    else if(g_key && g_sig)
    {
        /* check key and signature size */
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
    }
    /* for extraction, we offer a brute force search method from the MD5 */
    else if(is_extract && g_keysig_search != KEYSIG_SEARCH_NONE)
    {
        struct upg_md5_t *md5 = (void *)buf;
        void *encrypted_hdr = (md5 + 1);
        cprintf(BLUE, "keysig Search\n");
        cprintf_field("  Method: ", "%s\n", keysig_search_desc[g_keysig_search].name);
        bool ok = keysig_search(g_keysig_search, encrypted_hdr, 8,
            &upg_notify_keysig, keysig, g_nr_threads);
        cprintf(GREEN, "  Result: ");
        cprintf(ok ? YELLOW : RED, "%s\n", ok ? "Key found" : "No key found");
        if(!ok)
            return 2;
    }
    else
    {
        cprintf(GREY, "A KAS or a keysig is needed to decrypt the firmware\n");
        cprintf(GREY, "You have the following options(see help for more details):\n");
        cprintf(GREY, "- select a model with a known KAS\n");
        cprintf(GREY, "- specify an explicit KAS or key+sig\n");
        if(is_extract)
            cprintf(GREY, "- let me try to find the keysig by brute force\n");
        return 1;
    }

    /* If we only have the key and signature, we can create a "fake" KAS
     * that decrypts to the same key and signature. Since it is not unique,
     * it will generally not match the "official" one from Sony but will produce
     * valid files anyway */
    if(!g_kas)
    {
        /* This is useful to print the KAS for the user when brute-forcing since
         * the process will produce a key+sig and the database requires a KAS */
        g_kas = kas;
        encrypt_keysig(g_kas, g_key, g_sig);
    }

    cprintf(BLUE, "Keys\n");
    cprintf_field("  KAS: ", "%."STR(NWZ_KAS_SIZE)"s\n", g_kas);
    cprintf_field("  Key: ", "%."STR(NWZ_KEY_SIZE)"s\n", g_key);
    cprintf_field("  Sig: ", "%."STR(NWZ_SIG_SIZE)"s\n", g_sig);

    return 0;
}

static int do_upg(void *buf, long size)
{
    int ret = get_key_and_sig(true, buf);
    if(ret != 0)
        return ret;
    struct upg_file_t *file = upg_read_memory(buf, size, g_key, g_sig, NULL,
        generic_std_printf);
    if(file == NULL)
        return ret;
    for(int i = 0; i < file->nr_files; i++)
    {
        if(!g_out_prefix)
            continue;
        char *str = malloc(strlen(g_out_prefix) + 32);
        sprintf(str, "%s%d.bin", g_out_prefix, i);
        FILE *f = fopen(str, "wb");
        if(!f)
        {
            cprintf(GREY, "Cannot open '%s' for writing\n", str);
            free(str);
            continue;
        }
        free(str);
        fwrite(file->files[i].data, 1, file->files[i].size, f);
        fclose(f);
    }
    upg_free(file);
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
        cprintf(GREY, "You must specify a firmware filename\n");
        usage();
    }

    int ret = get_key_and_sig(false, NULL);
    if(ret != 0)
        return ret;

    struct upg_file_t *upg = upg_new();
    int nr_files = argc - 1;

    for(int i = 0; i < nr_files; i++)
    {
        FILE *f = fopen(argv[1 + i], "rb");
        if(f == NULL)
        {
            upg_free(upg);
            printf(GREY, "Cannot open input file '%s': %m\n", argv[i + 1]);
            return 1;
        }
        size_t size = filesize(f);
        void *buf = malloc(size);
        if(fread(buf, 1, size, f) != size)
        {
            cprintf(GREY, "Cannot read input file '%s': %m\n", argv[i + 1]);
            fclose(f);
            upg_free(upg);
            return 1;
        }
        fclose(f);
        upg_append(upg, buf, size);
    }

    size_t size = 0;
    void *buf = upg_write_memory(upg, g_key, g_sig, &size, NULL, generic_std_printf);
    upg_free(upg);
    if(buf == NULL)
    {
        cprintf(GREY, "Error creating UPG file\n");
        return 1;
    }
    FILE *fout = fopen(argv[0], "wb");
    if(fout == NULL)
    {
        cprintf(GREY, "Cannot open output firmware file: %m\n");
        free(buf);
        return 1;
    }
    if(fwrite(buf, 1, size, fout) != size)
    {
        cprintf(GREY, "Cannot write output file: %m\n");
        fclose(fout);
        free(buf);
        return 1;
    }
    fclose(fout);
    free(buf);
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
    printf("  -t/--threads <nr>\tSpecify number of threads to find the keysig\n");
    printf("  -a/--kas <kas>\tForce KAS\n");
    printf("  -k/--key <key>\tForce key\n");
    printf("  -s/--sig <sig>\tForce sig\n");
    printf("  -e/--extract\t\tExtract a UPG archive\n");
    printf("  -c/--create\t\tCreate a UPG archive\n");
    printf("keysig search method:\n");
    for(int i = KEYSIG_SEARCH_FIRST; i < KEYSIG_SEARCH_LAST; i++)
        printf("  %-10s\t%s\n", keysig_search_desc[i].name, keysig_search_desc[i].comment);
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
            {"threads", required_argument, 0, 't'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "?dnfo:m:l:a:k:s:ect:", long_options, NULL);
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
            case 't':
                g_nr_threads = strtol(optarg, NULL, 0);
                if(g_nr_threads < 1 || g_nr_threads > 128)
                {
                    cprintf(GREY, "Invalid number of threads\n");
                    return 1;
                }
                break;
            default:
                abort();
        }
    }

    if(g_model && strcmp(g_model, "?") == 0)
    {
        cprintf(BLUE, "Model list:\n");
        for(unsigned i = 0; g_model_list[i].model; i++)
        {
            cprintf(GREEN, "  %s:", g_model_list[i].model);

            cprintf(RED, " kas=");
            cprintf(YELLOW, "%."STR(NWZ_KAS_SIZE)"s", g_model_list[i].kas);
            if(g_model_list[i].confirmed)
                cprintf(RED, " confirmed");
            else
                cprintf(RED, " guessed");
            printf("\n");
        }
        return 1;
    }

    if(g_model)
    {
        for(unsigned i = 0; g_model_list[i].model; i++)
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
