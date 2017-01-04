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
static int g_nr_threads = 1;

enum keysig_search_method_t g_keysig_search = KEYSIG_SEARCH_NONE;

#define let_the_force_flow(x) do { if(!g_force) return x; } while(0)
#define continue_the_force(x) if(x) let_the_force_flow(x)

#define check_field(v_exp, v_have, str_ok, str_bad) \
    if((v_exp) != (v_have)) \
    { cprintf(RED, str_bad); let_the_force_flow(__LINE__); } \
    else { cprintf(RED, str_ok); }

static void usage(void);

#define HAS_KAS     (1 << 0)
#define HAS_KEY     (1 << 1)
#define HAS_SIG     (1 << 2)
#define CONFIRMED   (1 << 3)

struct nwz_model_t
{
    const char *model;
    unsigned flags;
    char *kas;
    char *key;
    char *sig;
};

/** Firmware format
 *
 * The firmware starts with the MD5 hash of the entire file (except the MD5 hash
 * itself of course). This is used to check that the file was not corrupted.
 * The remaining of the file is encrypted (using DES) with the model key. The
 * encrypted part starts with a header containing the model signature and the
 * number of files. Since the header is encrypted, decrypting the header with
 * the key and finding the right signature serves to authenticate the firmware.
 * The header is followed by N entries (where N is the number of files) giving
 * the offset, within the file, and size of each file. Note that the files in
 * the firmware have no name. */

struct upg_md5_t
{
    uint8_t md5[16];
}__attribute__((packed));

struct upg_header_t
{
    uint8_t sig[NWZ_SIG_SIZE];
    uint32_t nr_files;
    uint32_t pad; // make sure structure size is a multiple of 8
} __attribute__((packed));

struct upg_entry_t
{
    uint32_t offset;
    uint32_t size;
} __attribute__((packed));

/** KAS / Key / Signature
 *
 * Since this is all very confusing, we need some terminology and notations:
 * - [X, Y, Z] is a sequence of bytes, for example:
 *     [8, 0x89, 42]
 *   is a sequence of three bytes.
 * - "abcdef" is a string: it is a sequences of bytes where each byte happens to
 *   be the ASCII encoding of a letter. So for example:
 *     "abc" = [97, 98, 99]
 *   because 'a' has ASCII encoding 97 and so one
 * - HexString(Seq) refers to the string where each byte of the original sequence
 *   is represented in hexadecimal by two ASCII characters. For example:
 *     HexString([8, 0x89, 42]) = "08892a"
 *   because 8 = 0x08 so it represented by "08" and 42 = 0x2a. Note that the length
 *   of HexString(Seq) is always exactly twice the length of Seq.
 * - DES(Seq,Pass) is the result of encrypting Seq with Pass using the DES cipher.
 *   Seq must be a sequence of 8 bytes (known as a block) and Pass must be a
 *   sequence of 8 bytes. The result is also a 8-byte sequence.
 * - ECB_DES([Block0, Block1, ..., BlockN], Pass)
 *     = [DES(Block0,Pass), DES(Block1,Pass), ..., DES(BlockN,Pass)]
 *   where Blocki is a block (8 byte).
 *
 *
 * A firmware upgrade file is always encrypted using a Key. To authenticate it,
 * the upgrade file (before encryption) contains a Sig(nature). The pair (Key,Sig)
 * is refered to as KeySig and is specific to each series. For example all
 * NWZ-E46x use the same KeySig but the NWZ-E46x and NWZ-A86x use different KeySig.
 * In the details, a Key is a sequence of 8 bytes and a Sig is also a sequence
 * of 8 bytes. A KeySig is a simply the concatenation of the Key followed by
 * the Sig, so it is a sequence of 16 bytes. Probably in an attempt to obfuscate
 * things a little further, Sony never provides the KeySig directly but instead
 * encrypts it using DES in ECB mode using a hardcoded password and provides
 * the hexadecimal string of the result, known as the KAS, which is thus a string
 * of 32 ASCII characters.
 * Note that since DES works on blocks of 8 bytes and ECB encrypts blocks
 * independently, it is the same to encrypt the KeySig as once or encrypt the Key
 * and Sig separately.
 *
 * To summarize:
 *   Key = [K0, K1, K2, ..., K7] (8 bytes) (model specific)
 *   Sig = [S0, S1, S2, ..., S7] (8 bytes) (model specific)
 *   KeySig = [Key, Sig] = [K0, ... K7, S0, ..., S7] (16 bytes)
 *   FwpPass = "ed295076" (8 bytes) (never changes)
 *   EncKeySig = ECB_DES(KeySig, FwpPass) = [DES(Key, FwpPass), DES(Sig, FwpPass)]
 *   KAS = HexString(EncKeySig) (32 characters)
 *
 * In theory, the Key and Sig can be any 8-byte sequence. In practice, they always
 * are strings, probably to make it easier to write them down. In many cases, the
 * Key and Sig are even the hexadecimal string of 4-byte sequences but it is
 * unclear if this is the result of pure luck, confused engineers, lazyness on
 * Sony's part or by design. The following code assumes that Key and Sig are
 * strings (though it could easily be fixed to work with anything if this is
 * really needed).
 *
 *
 * Here is a real example, from the NWZ-E46x Series:
 *   Key = "6173819e" (note that this is a string and even a hex string in this case)
 *   Sig = "30b82e5c"
 *   KeySig = [Key, Sig] = "6173819e30b82e5c"
 *   FwpPass = "ed295076" (never changes)
 *   EncKeySig = ECB_DES(KeySig, FwpPass)
 *             = [0x8a, 0x01, 0xb6, ..., 0xc5] (16 bytes)
 *   KAS = HexString(EncKeySig) = "8a01b624bfbfde4a1662a1772220e3c5"
 *
 */

struct nwz_model_t g_model_list[] =
{
    { "nwz-e450", HAS_KAS | CONFIRMED, "8a01b624bfbfde4a1662a1772220e3c5", "", "" },
    { "nwz-e460", HAS_KAS | CONFIRMED, "89d813f8f966efdebd9c9e0ea98156d2", "", "" },
    { "nwz-a860", HAS_KAS | CONFIRMED, "a7c4af6c28b8900a783f307c1ba538c5", "", "" },
    { "nwz-a850", HAS_KAS | CONFIRMED, "a2efb9168616c2e84d78291295c1aa5d", "", "" },
    { "nwz-e470", HAS_KAS | CONFIRMED, "e4144baaa2707913f17b5634034262c4", "", "" },
    /* The following keys were obtained by brute forcing firmware upgrades,
     * someone with a device needs to confirm that they work */
    { "nw-a820", HAS_KEY | HAS_SIG, "", "4df06482", "07fa0b6e" },
    { "nwz-a10", HAS_KEY | HAS_SIG, "", "ec2888e2", "f62ced8a" },
    { "nwz-a20", HAS_KEY | HAS_SIG, "", "e8e204ee", "577614df" },
    { "nwz-zx100", HAS_KEY | HAS_SIG, "", "22e44606", "a9f95e90" },
    { "nwz-e580", HAS_KEY | HAS_SIG, "", "a60806ea", "97e8ce46" },
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

static int decrypt_keysig(const char kas[NWZ_KAS_SIZE], char key[NWZ_KEY_SIZE],
    char sig[NWZ_SIG_SIZE])
{
    uint8_t src[NWZ_KAS_SIZE / 2];
    for(int index = 0; index < NWZ_KAS_SIZE / 2; index++)
    {
        int a = digit_value(kas[index * 2]);
        int b = digit_value(kas[index * 2 + 1]);
        if(a < 0 || b < 0)
        {
            cprintf(GREY, "Invalid KAS !\n");
            return -1;
        }
        src[index] = a << 4 | b;
    }
    fwp_setkey("ed295076");
    fwp_crypt(src, sizeof(src), 1);
    memcpy(key, src, NWZ_KEY_SIZE);
    memcpy(sig, src + NWZ_KEY_SIZE, NWZ_SIG_SIZE);
    return 0;
}

static void encrypt_keysig(char kas[NWZ_KEY_SIZE],
    const char key[NWZ_SIG_SIZE], const char sig[NWZ_KAS_SIZE])
{
    uint8_t src[NWZ_KAS_SIZE / 2];
    fwp_setkey("ed295076");
    memcpy(src, key, NWZ_KEY_SIZE);
    memcpy(src + NWZ_KEY_SIZE, sig, NWZ_SIG_SIZE);
    fwp_crypt(src, sizeof(src), 0);
    for(int i = 0; i < NWZ_KAS_SIZE / 2; i++)
    {
        kas[2 * i] = hex_digit((src[i] >> 4) & 0xf);
        kas[2 * i + 1] = hex_digit(src[i] & 0xf);
    }
}

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

static int get_key_and_sig(bool is_extract, void *encrypted_hdr)
{
    static char keysig[NWZ_KEYSIG_SIZE];
    static char kas[NWZ_KAS_SIZE];
    /* database lookup */
    if(g_model_index != -1)
    {
        if(g_model_list[g_model_index].flags & HAS_KAS)
            g_kas = g_model_list[g_model_index].kas;
        if(g_model_list[g_model_index].flags & HAS_KEY)
            g_key = g_model_list[g_model_index].key;
        if(g_model_list[g_model_index].flags & HAS_SIG)
            g_sig = g_model_list[g_model_index].sig;
    }

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
    /* fall back to key and signature otherwise. The signature is not required
     * when extracting but prevents from checking decryption */
    else if(g_key && (is_extract || g_sig))
    {
        if(strlen(g_key) != 8)
        {
            cprintf(GREY, "The specified key has wrong length (must be 8 hex digits)\n");
            return 4;
        }

        /* if there is a signature, it must have the correct size */
        if(g_sig)
        {
            if(strlen(g_sig) != 8)
            {
                cprintf(GREY, "The specified sig has wrong length (must be 8 hex digits)\n");
                return 5;
            }
        }
        else
        {
            cprintf(GREY, "Warning: you have specified a key but no sig, I won't be able to do any checks\n");
        }
    }
    /* for extraction, we offer a brute force search method from the MD5 */
    else if(is_extract && g_keysig_search != KEYSIG_SEARCH_NONE)
    {
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
        if(!g_sig)
        {
            /* if we extract and don't have a signature, just use a random
             * one, we cannot check it anyway */
            g_sig = keysig;
            memset(g_sig, '?', NWZ_SIG_SIZE);
        }
        g_kas = kas;
        encrypt_keysig(g_kas, g_key, g_sig);
    }

    cprintf(BLUE, "Keys\n");
    cprintf_field("  KAS: ", "%."STR(NWZ_KAS_SIZE)"s\n", g_kas);
    cprintf_field("  Key: ", "%."STR(NWZ_KEY_SIZE)"s\n", g_key);
    if(g_sig)
        cprintf_field("  Sig: ", "%."STR(NWZ_SIG_SIZE)"s\n", g_sig);

    return 0;
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

    int ret = get_key_and_sig(true, md5 + 1);
    if(ret != 0)
        return ret;

    struct upg_header_t *hdr = (void *)(md5 + 1);
    ret = fwp_read(hdr, sizeof(struct upg_header_t), hdr, (void *)g_key);
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

    int ret = get_key_and_sig(false, NULL);
    if(ret != 0)
        return ret;

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

    ret = fwp_write(&hdr, sizeof(hdr), &hdr, (void *)g_key);
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
        for(unsigned i = 0; i < sizeof(g_model_list) / sizeof(g_model_list[0]); i++)
        {
            cprintf(GREEN, "  %s:", g_model_list[i].model);
            if(g_model_list[i].flags & HAS_KAS)
            {
                cprintf(RED, " kas=");
                cprintf(YELLOW, "%."STR(NWZ_KAS_SIZE)"s", g_model_list[i].kas);
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

