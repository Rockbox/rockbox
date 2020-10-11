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
#define _XOPEN_SOURCE 500 /* for strdup */
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdarg.h>
#include <ctype.h>
#include "misc.h"
#include <sys/stat.h>
#include <zlib.h>
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
#define MAX_NR_FILES    32
bool g_compress[MAX_NR_FILES] = {false};
const char *g_md5name[MAX_NR_FILES] = {NULL};

enum keysig_search_method_t g_keysig_search = KEYSIG_SEARCH_NONE;

#define cprintf(col, ...) do {color(col); printf(__VA_ARGS__); }while(0)

#define cprintf_field(str1, ...) do{ cprintf(GREEN, str1); cprintf(YELLOW, __VA_ARGS__); }while(0)

static void usage(void);

/* user needs to be pointer to a NWZ_KEYSIG_SIZE-byte buffer, on success g_key
 * and g_sig are updated to point to the key and sig in the buffer */
static bool upg_notify_keysig(void *user, uint8_t key[NWZ_KEY_SIZE],
    uint8_t sig[NWZ_SIG_SIZE])
{
    g_key = user;
    g_sig = user + 9;
    memcpy(g_key, key, NWZ_KEY_SIZE);
    g_key[8] = 0;
    memcpy(g_sig, sig, NWZ_SIG_SIZE);
    g_sig[8] = 0;
    return true;
}

static int get_key_and_sig(bool is_extract, void *buf)
{
    /* database lookup */
    if(g_model_index != -1)
        g_kas = strdup(g_model_list[g_model_index].kas);

    /* always prefer KAS because it contains everything */
    if(g_kas)
    {
        if(strlen(g_kas) != 32 && strlen(g_kas) != 64)
        {
            cprintf(GREY, "The KAS has wrong length (must be 32 or 64 hex digits)\n");
            return 4;
        }
        decrypt_keysig(g_kas, &g_key, &g_sig);
    }
    /* Otherwise require key and signature */
    else if(g_key && g_sig)
    {
        /* check key and signature size */
        if(strlen(g_key) != 8 && strlen(g_key) != 16)
        {
            cprintf(GREY, "The specified key has wrong length (must be 8 or 16 hex digits)\n");
            return 4;
        }
        if(strlen(g_sig) != strlen(g_key))
        {
            cprintf(GREY, "The specified sig has wrong length (must match key length)\n");
            return 5;
        }
    }
    /* for extraction, we offer a brute force search method from the MD5 */
    else if(is_extract && g_keysig_search != KEYSIG_SEARCH_NONE)
    {
        static char keysig[18]; /* 8+NUL+8+NULL */
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
        encrypt_keysig(&g_kas, g_key, g_sig);
    }

    cprintf(BLUE, "Keys\n");
    cprintf_field("  KAS: ", "%s\n", g_kas);
    cprintf_field("  Key: ", "%s\n", g_key);
    cprintf_field("  Sig: ", "%s\n", g_sig);

    return 0;
}

static unsigned xdigit2val(char c)
{
    if('0' <= c && c <= '9')
        return c - '0';
    if('a' <= c && c <= 'f')
        return c - 'a' + 10;
    if('A' <= c && c <= 'F')
        return c - 'A' + 10;
    return 0;
}

static bool find_md5_entry(struct upg_file_t *file, const char *name, size_t *out_size, uint8_t *md5)
{
    char *content = file->files[1].data;
    size_t size = file->files[1].size;
    /* we expect the file to have a terminating zero because it is padded with zeroes, if not, add one */
    if(content[size - 1] != 0)
    {
        content = file->files[1].data = realloc(content, size + 1);
        content[size] = 0;
        size++;
    }
    /* now we can parse safely by stopping t the first 0 */
    size_t pos = 0;
    while(true)
    {
        /* format of each line: filesize md5 name */
        char *end;
        if(content[pos] == 0)
            break; /* stop on zero */
        if(!isdigit(content[pos]))
            goto Lskipline;
        /* parse size */
        *out_size = strtoul(content + pos, &end, 0);
        pos = end - content;
        while(content[pos] == ' ')
            pos++;
        /* parse md5 */
        for(int i = 0; i < NWZ_MD5_SIZE; i++)
        {
            if(!isxdigit(content[pos]))
                goto Lskipline;
            if(!isxdigit(content[pos + 1]))
                goto Lskipline;
            md5[i] = xdigit2val(content[pos]) << 4 | xdigit2val(content[pos + 1]);
            pos += 2;
        }
        /* parse name: this is a stupid comparison, no trimming */
        while(content[pos] == ' ')
            pos++;
        size_t name_begin = pos;
        while(content[pos] != 0 && content[pos] != '\n')
            pos++;
        if(strlen(name) == pos - name_begin && !memcmp(content + name_begin, name, pos - name_begin))
            return true;
        /* fallthrough: eat end of line */
        Lskipline:
        while(content[pos] != 0 && content[pos] != '\n')
            pos++;
        if(content[pos] == '\n')
            pos++;
    }
    return false;
}

static void compare_md5(struct upg_file_t *file, int idx, size_t filesize, uint8_t *md5)
{
    if(g_md5name[idx] == NULL)
        return;
    size_t expected_size;
    uint8_t expected_md5[NWZ_MD5_SIZE * 2];
    bool found = find_md5_entry(file, g_md5name[idx], &expected_size, expected_md5);
    cprintf(BLUE, "File %d\n", idx);
    cprintf_field("  Name: ", "%s ", g_md5name[idx]);
    cprintf(RED, found ? "Found" : " Not found");
    printf("\n");
    cprintf_field("  Size: ", "%lu", (unsigned long)filesize);
    cprintf(RED, " %s", !found ? "Cannot check" : filesize == expected_size ? "Ok" : "Mismatch");
    printf("\n");
    cprintf_field("  MD5:", " ");
    for(int i = 0; i < NWZ_MD5_SIZE; i++)
        printf("%02x", md5[i]);
    bool ok_md5 = !memcmp(md5, expected_md5, NWZ_MD5_SIZE);
    cprintf(RED, " %s", !found ? "Cannot check" : ok_md5 ? "Ok" : "Mismatch");
    printf("\n");
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
        /* we will compute the MD5 during writing/decompress */
        if(g_compress[i])
        {
            void *md5_obj = md5_start();
            uint8_t md5[NWZ_MD5_SIZE];
            void *buf = file->files[i].data;
            int size = file->files[i].size;
            int pos = 0;
            /* the fwpup tool seems to assume that every block decompresses to less than 4096 bytes,
             * so I guess the encoder splits the input in chunks */
            int max_chunk_size = 4096;
            void *chunk = malloc(max_chunk_size);
            if(g_debug)
                cprintf(GREY, "decompressing file %d with chunk size %d...\n", i, max_chunk_size);
            size_t total_size = 0;
            while(pos + 4 <= size)
            {
                int compressed_chunk_size = *(uint32_t *)(buf + pos);
                if(g_debug)
                    cprintf(GREY, "%d ", compressed_chunk_size);
                if(compressed_chunk_size < 0)
                {
                    cprintf(RED, "invalid block size when decompressing, something is wrong\n");
                    break;
                }
                if(compressed_chunk_size == 0)
                    break;
                uLongf chunk_size = max_chunk_size;
                int zres = uncompress(chunk, &chunk_size, buf + pos + 4, compressed_chunk_size);
                if(zres == Z_BUF_ERROR)
                    cprintf(RED, "the encoder produced a block greater than %d, I can't handle that\n", max_chunk_size);
                if(zres == Z_DATA_ERROR)
                    cprintf(RED, "the compressed data seems corrupted\n");
                if(zres != Z_OK)
                {
                    cprintf(RED, "the compressed suffered an error %d\n", zres);
                    break;
                }
                pos += 4 + compressed_chunk_size;
                md5_update(md5_obj, chunk, chunk_size);
                fwrite(chunk, 1, chunk_size, f);
                total_size += chunk_size;
            }
            free(chunk);
            if(g_debug)
                cprintf(GREY, "done.");
            md5_final(md5_obj, md5);
            compare_md5(file, i, total_size, md5);
        }
        else
        {
            fwrite(file->files[i].data, 1, file->files[i].size, f);
        }
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
    FILE *fin = fopen(g_in_file, "rb");
    if(fin == NULL)
    {
        perror("Cannot open UPG file");
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

    char *md5_prepend = malloc(1);
    md5_prepend[0] = 0;
    size_t md5_prepend_sz = 0;
    for(int i = 0; i < nr_files; i++)
    {
        FILE *f = fopen(argv[1 + i], "rb");
        if(f == NULL)
        {
            upg_free(upg);
            cprintf(GREY, "Cannot open input file '%s': %m\n", argv[i + 1]);
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
        /* add the MD5 of files *before* any kind of treatment. Does nothing if not resquested,
        * which is important on v1 where the second file might not be the md5 file */
        if(g_md5name[i])
        {
            uint8_t md5[NWZ_MD5_SIZE];
            MD5_CalculateDigest(md5, buf, size);
            size_t inc_sz = 16 + NWZ_MD5_SIZE * 2 + strlen(g_md5name[i]);
            md5_prepend = realloc(md5_prepend, md5_prepend_sz + inc_sz);
            md5_prepend_sz += sprintf(md5_prepend + md5_prepend_sz, "%lu ", (unsigned long)size);
            for(int i = 0; i < NWZ_MD5_SIZE; i++)
                md5_prepend_sz += sprintf(md5_prepend + md5_prepend_sz, "%02x", md5[i]);
            md5_prepend_sz += sprintf(md5_prepend + md5_prepend_sz, " %s\n", g_md5name[i]);
        }
        if(g_compress[i])
        {
            /* in the worst case, maybe the compressor will double the size, also we always need
             * at least 4 bytes to write the size of a block */
            int out_buf_max_sz = 4 + 2 * size;
            void *out_buf = malloc(out_buf_max_sz);
            int out_buf_pos = 0, in_buf_pos = 0;
            int max_chunk_size = 4096; /* the OF encoder/decoder expect that */
            while(in_buf_pos < size)
            {
                int chunk_size = MIN(size - in_buf_pos, max_chunk_size);
                uLongf dest_len = out_buf_max_sz - out_buf_pos - 4; /* we reserve 4 for the size */
                int zres = compress(out_buf + out_buf_pos + 4, &dest_len, buf + in_buf_pos, chunk_size);
                if(zres == Z_BUF_ERROR)
                {
                    cprintf(RED, "the compresser produced a file much greater than its input, I can't handle that\n");
                    return 1;
                }
                else if(zres != Z_OK)
                {
                    cprintf(RED, "the compresser suffered an error %d\n", zres);
                    return 1;
                }
                /* write output size in the output buffer */
                *(uint32_t *)(out_buf + out_buf_pos) = dest_len;
                out_buf_pos += 4 + dest_len;
                in_buf_pos += chunk_size;
            }
            /* add an extra zero-length chunk */
            *(uint32_t *)(out_buf + out_buf_pos) = 0;
            out_buf_pos += 4;
            upg_append(upg, out_buf, out_buf_pos);
            free(buf);
        }
        else
            upg_append(upg, buf, size);
    }

    /* modify md5 file (if any) */
    upg->files[1].data = realloc(upg->files[1].data, upg->files[1].size + md5_prepend_sz);
    memmove(upg->files[1].data + md5_prepend_sz, upg->files[1].data, upg->files[1].size);
    memcpy(upg->files[1].data, md5_prepend, md5_prepend_sz);
    upg->files[1].size += md5_prepend_sz;

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
    printf("  -z/--compress <idx>\t\t(De)compress file <idx> (starts at 0)\n");
    printf("  -z/--compress <idx>,<md5name>\t\t(De)compress file <idx> and add it to the MD5 file\n");
    printf("When using -z <idx>,<md5name>, the file file size and MD5 prior to compression will\n");
    printf("be prepended to the contect of the second file (index 1). The name can be arbitrary and\n");
    printf("has meaning only the script, e.g. \"-z 6,system.img\".\n");
    printf("Keysig search method:\n");
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
            {"compress", required_argument, 0, 'z'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "?dnfo:m:l:a:k:s:ect:z:", long_options, NULL);
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
            case 'z':
            {
                char *end;
                int idx = strtol(optarg, &end, 0);
                if(idx < 0 || idx >= MAX_NR_FILES)
                {
                    cprintf(GREY, "Invalid file index\n");
                    return 1;
                }
                g_compress[idx] = true;
                /* distinguish betwen -z <idx> and -z <idx>,<md5name> */
                if(*end == 0)
                    break;
                if(*end != ',')
                {
                    cprintf(GREY, "Invalid file index\n");
                    return 1;
                }
                g_md5name[idx] = end + 1;
                break;
            }
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
            cprintf(YELLOW, "%s", g_model_list[i].kas);
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
    if(extract && !g_out_prefix)
    {
        printf("You need to  specify output prefix (-o) to extract\n");
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
