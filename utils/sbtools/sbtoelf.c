/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2010 Bertrik Sikken
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

/*
 * .sb file parser and chunk extractor
 *
 * Based on amsinfo, which is
 * Copyright © 2008 Rafaël Carré <rafael.carre@gmail.com>
 */

#define _ISOC99_SOURCE /* snprintf() */
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <stdarg.h>
#include <strings.h>
#include <getopt.h>

#include "crypto.h"
#include "elf.h"
#include "sb.h"
#include "misc.h"

#if 1 /* ANSI colors */

#	define color(a) printf("%s",a)
char OFF[] 		= { 0x1b, 0x5b, 0x31, 0x3b, '0', '0', 0x6d, '\0' };

char GREY[] 	= { 0x1b, 0x5b, 0x31, 0x3b, '3', '0', 0x6d, '\0' };
char RED[] 		= { 0x1b, 0x5b, 0x31, 0x3b, '3', '1', 0x6d, '\0' };
char GREEN[] 	= { 0x1b, 0x5b, 0x31, 0x3b, '3', '2', 0x6d, '\0' };
char YELLOW[] 	= { 0x1b, 0x5b, 0x31, 0x3b, '3', '3', 0x6d, '\0' };
char BLUE[] 	= { 0x1b, 0x5b, 0x31, 0x3b, '3', '4', 0x6d, '\0' };

#else
	/* disable colors */
#	define color(a)
#endif

/* all blocks are sized as a multiple of 0x1ff */
#define PAD_TO_BOUNDARY(x) (((x) + 0x1ff) & ~0x1ff)

/* If you find a firmware that breaks the known format ^^ */
#define assert(a) do { if(!(a)) { fprintf(stderr,"Assertion \"%s\" failed in %s() line %d!\n\nPlease send us your firmware!\n",#a,__func__,__LINE__); exit(1); } } while(0)

#define crypto_cbc(...) \
    do { int ret = crypto_cbc(__VA_ARGS__); \
        if(ret != CRYPTO_ERROR_SUCCESS) \
            bug("crypto_cbc error: %d\n", ret); \
    }while(0)

/* globals */

uint8_t *g_buf; /* file content */
char *g_out_prefix;
bool g_debug;
bool g_raw_mode;

static uint8_t instruction_checksum(struct sb_instruction_header_t *hdr)
{
    uint8_t sum = 90;
    byte *ptr = (byte *)hdr;
    for(int i = 1; i < 16; i++)
        sum += ptr[i];
    return sum;
}

static void elf_printf(void *user, bool error, const char *fmt, ...)
{
    if(!g_debug && !error)
        return;
    (void) user;
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

static void elf_write(void *user, uint32_t addr, const void *buf, size_t count)
{
    FILE *f = user;
    fseek(f, addr, SEEK_SET);
    fwrite(buf, count, 1, f);
}

static void extract_elf_section(struct elf_params_t *elf, int count, const char *prefix,
    const char *indent)
{
    char *filename = xmalloc(strlen(prefix) + 32);
    sprintf(filename, "%s.%d.elf", prefix, count);
    printf("%swrite %s\n", indent, filename);
    
    FILE *fd = fopen(filename, "wb");
    free(filename);
    
    if(fd == NULL)
        return ;
    elf_write_file(elf, elf_write, elf_printf, fd);
    fclose(fd);
}

static void extract_section(int data_sec, char name[5], byte *buf, int size, const char *indent)
{
    char *filename = xmalloc(strlen(g_out_prefix) + strlen(name) + 5);
    if(g_out_prefix)
    {
        sprintf(filename, "%s%s.bin", g_out_prefix, name);
        FILE *fd = fopen(filename, "wb");
        if (fd != NULL)
        {
            fwrite(buf, size, 1, fd);
            fclose(fd);
        }
    }
    if(data_sec)
        return;

    sprintf(filename, "%s%s", g_out_prefix, name);

    /* elf construction */
    struct elf_params_t elf;
    elf_init(&elf);
    int elf_count = 0;
    /* Pretty print the content */
    int pos = 0;
    while(pos < size)
    {
        struct sb_instruction_header_t *hdr = (struct sb_instruction_header_t *)&buf[pos];
        printf("%s", indent);
        uint8_t checksum = instruction_checksum(hdr);
        if(checksum != hdr->checksum)
        {
            color(GREY);
            printf("[Bad checksum]");
        }
        if(hdr->flags != 0)
        {
            color(GREY);
            printf("[");
            color(BLUE);
            printf("f=%x", hdr->flags);
            color(GREY);
            printf("] ");
        }
        if(hdr->opcode == SB_INST_LOAD)
        {
            struct sb_instruction_load_t *load = (struct sb_instruction_load_t *)&buf[pos];
            color(RED);
            printf("LOAD");
            color(OFF);printf(" | ");
            color(BLUE);
            printf("addr=0x%08x", load->addr);
            color(OFF);printf(" | ");
            color(GREEN);
            printf("len=0x%08x", load->len);
            color(OFF);printf(" | ");
            color(YELLOW);
            printf("crc=0x%08x", load->crc);
            /* data is padded to 16-byte boundary with random data and crc'ed with it */
            uint32_t computed_crc = crc(&buf[pos + sizeof(struct sb_instruction_load_t)],
                ROUND_UP(load->len, 16));
            color(RED);
            if(load->crc == computed_crc)
                printf("  Ok\n");
            else
                printf("  Failed (crc=0x%08x)\n", computed_crc);

            /* elf construction */
            elf_add_load_section(&elf, load->addr, load->len,
                &buf[pos + sizeof(struct sb_instruction_load_t)]);

            pos += load->len + sizeof(struct sb_instruction_load_t);
        }
        else if(hdr->opcode == SB_INST_FILL)
        {
            struct sb_instruction_fill_t *fill = (struct sb_instruction_fill_t *)&buf[pos];
            color(RED);
            printf("FILL");
            color(OFF);printf(" | ");
            color(BLUE);
            printf("addr=0x%08x", fill->addr);
            color(OFF);printf(" | ");
            color(GREEN);
            printf("len=0x%08x", fill->len);
            color(OFF);printf(" | ");
            color(YELLOW);
            printf("pattern=0x%08x\n", fill->pattern);
            color(OFF);

            /* elf construction */
            elf_add_fill_section(&elf, fill->addr, fill->len, fill->pattern);

            pos += sizeof(struct sb_instruction_fill_t);
        }
        else if(hdr->opcode == SB_INST_CALL ||
                hdr->opcode == SB_INST_JUMP)
        {
            int is_call = (hdr->opcode == SB_INST_CALL);
            struct sb_instruction_call_t *call = (struct sb_instruction_call_t *)&buf[pos];
            color(RED);
            if(is_call)
                printf("CALL");
            else
                printf("JUMP");
            color(OFF);printf(" | ");
            color(BLUE);
            printf("addr=0x%08x", call->addr);
            color(OFF);printf(" | ");
            color(GREEN);
            printf("arg=0x%08x\n", call->arg);
            color(OFF);

            /* elf construction */
            elf_set_start_addr(&elf, call->addr);
            extract_elf_section(&elf, elf_count++, filename, indent);
            elf_release(&elf);
            elf_init(&elf);

            pos += sizeof(struct sb_instruction_call_t);
        }
        else if(hdr->opcode == SB_INST_MODE)
        {
            struct sb_instruction_mode_t *mode = (struct sb_instruction_mode_t *)hdr;
            color(RED);
            printf("MODE");
            color(OFF);printf(" | ");
            color(BLUE);
            printf("mod=0x%08x\n", mode->mode);
            color(OFF);
            pos += sizeof(struct sb_instruction_mode_t);
        }
        else if(hdr->opcode == SB_INST_NOP)
        {
            color(RED);
            printf("NOOP\n");
            pos += sizeof(struct sb_instruction_mode_t);
        }
        else
        {
            color(RED);
            printf("Unknown instruction %d at address 0x%08lx\n", hdr->opcode, (unsigned long)pos);
            break;
        }

        pos = ROUND_UP(pos, BLOCK_SIZE);
    }

    if(!elf_is_empty(&elf))
        extract_elf_section(&elf, elf_count++, filename, indent);
    elf_release(&elf);
}

static void fill_section_name(char name[5], uint32_t identifier)
{
    name[0] = (identifier >> 24) & 0xff;
    name[1] = (identifier >> 16) & 0xff;
    name[2] = (identifier >> 8) & 0xff;
    name[3] = identifier & 0xff;
    for(int i = 0; i < 4; i++)
        if(!isprint(name[i]))
            name[i] = '_';
    name[4] = 0;
}

static uint16_t swap16(uint16_t t)
{
    return (t << 8) | (t >> 8);
}

static void fix_version(struct sb_version_t *ver)
{
    ver->major = swap16(ver->major);
    ver->minor = swap16(ver->minor);
    ver->revision = swap16(ver->revision);
}

static void extract(unsigned long filesize)
{
    struct sha_1_params_t sha_1_params;
    /* Basic header info */
    struct sb_header_t *sb_header = (struct sb_header_t *)g_buf;

    if(memcmp(sb_header->signature, "STMP", 4) != 0)
        bugp("Bad signature");
    /*
    if(sb_header->image_size * BLOCK_SIZE > filesize)
        bugp("File size mismatch");
    */
    if(sb_header->header_size * BLOCK_SIZE != sizeof(struct sb_header_t))
        bugp("Bad header size");
    if(sb_header->sec_hdr_size * BLOCK_SIZE != sizeof(struct sb_section_header_t))
        bugp("Bad section header size");

    if(filesize > sb_header->image_size * BLOCK_SIZE)
    {
        color(GREY);
        printf("[Restrict file size from %lu to %d bytes]\n", filesize,
            sb_header->image_size * BLOCK_SIZE);
        filesize = sb_header->image_size * BLOCK_SIZE;
    }

    color(BLUE);
    printf("Basic info:\n");
    color(GREEN);
    printf("  SB version: ");
    color(YELLOW);
    printf("%d.%d\n", sb_header->major_ver, sb_header->minor_ver);
    color(GREEN);
    printf("  Header SHA-1: ");
    byte *hdr_sha1 = sb_header->sha1_header;
    color(YELLOW);
    print_hex(hdr_sha1, 20, false);
    /* Check SHA1 sum */
    byte computed_sha1[20];
    sha_1_init(&sha_1_params);
    sha_1_update(&sha_1_params, &sb_header->signature[0],
        sizeof(struct sb_header_t) - sizeof(sb_header->sha1_header));
    sha_1_finish(&sha_1_params);
    sha_1_output(&sha_1_params, computed_sha1);
    color(RED);
    if(memcmp(hdr_sha1, computed_sha1, 20) == 0)
        printf(" Ok\n");
    else
        printf(" Failed\n");
    color(GREEN);
    printf("  Flags: ");
    color(YELLOW);
    printf("%x\n", sb_header->flags);
    color(GREEN);
    printf("  Total file size : ");
    color(YELLOW);
    printf("%ld\n", filesize);
    
    /* Sizes and offsets */
    color(BLUE);
    printf("Sizes and offsets:\n");
    color(GREEN);
    printf("  # of encryption keys = ");
    color(YELLOW);
    printf("%d\n", sb_header->nr_keys);
    color(GREEN);
    printf("  # of sections = ");
     color(YELLOW);
    printf("%d\n", sb_header->nr_sections);

    /* Versions */
    color(BLUE);
    printf("Versions\n");
    color(GREEN);

    printf("  Random 1: ");
    color(YELLOW);
    print_hex(sb_header->rand_pad0, sizeof(sb_header->rand_pad0), true);
    color(GREEN);
    printf("  Random 2: ");
    color(YELLOW);
    print_hex(sb_header->rand_pad1, sizeof(sb_header->rand_pad1), true);
    
    uint64_t micros = sb_header->timestamp;
    time_t seconds = (micros / (uint64_t)1000000L);
    struct tm tm_base = {0, 0, 0, 1, 0, 100, 0, 0, 1, 0, NULL}; /* 2000/1/1 0:00:00 */
    seconds += mktime(&tm_base);
    struct tm *time = gmtime(&seconds);
    color(GREEN);
    printf("  Creation date/time = ");
    color(YELLOW);
    printf("%s", asctime(time));

    struct sb_version_t product_ver = sb_header->product_ver;
    fix_version(&product_ver);
    struct sb_version_t component_ver = sb_header->component_ver;
    fix_version(&component_ver);

    color(GREEN);
    printf("  Product version   = ");
    color(YELLOW);
    printf("%X.%X.%X\n", product_ver.major, product_ver.minor, product_ver.revision);
    color(GREEN);
    printf("  Component version = ");
    color(YELLOW);
    printf("%X.%X.%X\n", component_ver.major, component_ver.minor, component_ver.revision);
        
    color(GREEN);
    printf("  Drive tag = ");
    color(YELLOW);
    printf("%x\n", sb_header->drive_tag);
    color(GREEN);
    printf("  First boot tag offset = ");
    color(YELLOW);
    printf("%x\n", sb_header->first_boot_tag_off);
    color(GREEN);
    printf("  First boot section ID = ");
    color(YELLOW);
    printf("0x%08x\n", sb_header->first_boot_sec_id);

    /* encryption cbc-mac */
    byte real_key[16];
    bool valid_key = false; /* false until a matching key was found */
    if(sb_header->nr_keys > 0)
    {
        if(sb_header->nr_keys > g_nr_keys)
        {
            color(GREY);
            bug("SB file has %d keys but only %d were specified on command line\n",
                sb_header->nr_keys, g_nr_keys);
        }
        color(BLUE);
        printf("Encryption data\n");
        for(int i = 0; i < sb_header->nr_keys; i++)
        {
            color(RED);
            printf("  Key %d: ", i);
            print_key(&g_key_array[i], true);
            color(GREEN);
            printf("    CBC-MAC of headers: ");

            uint32_t ofs = sizeof(struct sb_header_t)
                + sizeof(struct sb_section_header_t) * sb_header->nr_sections
                + sizeof(struct sb_key_dictionary_entry_t) * i;
            struct sb_key_dictionary_entry_t *dict_entry =
                (struct sb_key_dictionary_entry_t *)&g_buf[ofs];
            /* cbc mac */
            color(YELLOW);
            print_hex(dict_entry->hdr_cbc_mac, 16, false);
            /* check it */
            byte computed_cbc_mac[16];
            byte zero[16];
            memset(zero, 0, 16);
            crypto_cbc(g_buf, NULL, sb_header->header_size + sb_header->nr_sections,
                &g_key_array[i], zero, &computed_cbc_mac, 1);
            color(RED);
            bool ok = memcmp(dict_entry->hdr_cbc_mac, computed_cbc_mac, 16) == 0;
            if(ok)
            {
                valid_key = true;
                printf(" Ok\n");
            }
            else
                printf(" Failed\n");
            color(GREEN);
            
            printf("    Encrypted key     : ");
            color(YELLOW);
            print_hex(dict_entry->key, 16, true);
            color(GREEN);
            /* decrypt */
            byte decrypted_key[16];
            byte iv[16];
            memcpy(iv, g_buf, 16); /* uses the first 16-bytes of SHA-1 sig as IV */
            crypto_cbc(dict_entry->key, decrypted_key, 1, &g_key_array[i], iv, NULL, 0);
            printf("    Decrypted key     : ");
            color(YELLOW);
            print_hex(decrypted_key, 16, false);
            /* cross-check or copy */
            if(valid_key && ok)
                memcpy(real_key, decrypted_key, 16);
            else if(valid_key)
            {
                if(memcmp(real_key, decrypted_key, 16) == 0)
                {
                    color(RED);
                    printf(" Cross-Check Ok");
                }
                else
                {
                    color(RED);
                    printf(" Cross-Check Failed");
                }
            }
            printf("\n");
        }
    }

    if(getenv("SB_REAL_KEY") != 0)
    {
        struct crypto_key_t k;
        char *env = getenv("SB_REAL_KEY");
        if(!parse_key(&env, &k) || *env)
            bug("Invalid SB_REAL_KEY");
        memcpy(real_key, k.u.key, 16);
    }

    color(RED);
    printf("  Summary:\n");
    color(GREEN);
    printf("    Real key: ");
    color(YELLOW);
    print_hex(real_key, 16, true);
    color(GREEN);
    printf("    IV      : ");
    color(YELLOW);
    print_hex(g_buf, 16, true);

    /* sections */
    if(!g_raw_mode)
    {
        color(BLUE);
        printf("Sections\n");
        for(int i = 0; i < sb_header->nr_sections; i++)
        {
            uint32_t ofs = sb_header->header_size * BLOCK_SIZE + i * sizeof(struct sb_section_header_t);
            struct sb_section_header_t *sec_hdr = (struct sb_section_header_t *)&g_buf[ofs];
        
            char name[5];
            fill_section_name(name, sec_hdr->identifier);
            int pos = sec_hdr->offset * BLOCK_SIZE;
            int size = sec_hdr->size * BLOCK_SIZE;
            int data_sec = !(sec_hdr->flags & SECTION_BOOTABLE);
            int encrypted = !(sec_hdr->flags & SECTION_CLEARTEXT) && sb_header->nr_keys > 0;
        
            color(GREEN);
            printf("  Section ");
            color(YELLOW);
            printf("'%s'\n", name);
            color(GREEN);
            printf("    pos   = ");
            color(YELLOW);
            printf("%8x - %8x\n", pos, pos+size);
            color(GREEN);
            printf("    len   = ");
            color(YELLOW);
            printf("%8x\n", size);
            color(GREEN);
            printf("    flags = ");
            color(YELLOW);
            printf("%8x", sec_hdr->flags);
            color(RED);
            if(data_sec)
                printf("  Data Section");
            else
                printf("  Boot Section");
            if(encrypted)
                printf(" (Encrypted)");
            printf("\n");
            
            /* save it */
            byte *sec = xmalloc(size);
            if(encrypted)
                cbc_mac(g_buf + pos, sec, size / BLOCK_SIZE, real_key, g_buf, NULL, 0);
            else
                memcpy(sec, g_buf + pos, size);
            
            extract_section(data_sec, name, sec, size, "      ");
            free(sec);
        }
    }
    else
    {
        /* advanced raw mode */
        color(BLUE);
        printf("Commands\n");
        uint32_t offset = sb_header->first_boot_tag_off * BLOCK_SIZE;
        byte iv[16];
        const char *indent = "    ";
        while(true)
        {
            /* restart with IV */
            memcpy(iv, g_buf, 16);
            byte cmd[BLOCK_SIZE];
            if(sb_header->nr_keys > 0)
                cbc_mac(g_buf + offset, cmd, 1, real_key, iv, &iv, 0);
            else
                memcpy(cmd, g_buf + offset, BLOCK_SIZE);
            struct sb_instruction_header_t *hdr = (struct sb_instruction_header_t *)cmd;
            printf("%s", indent);
            uint8_t checksum = instruction_checksum(hdr);
            if(checksum != hdr->checksum)
            {
                color(GREY);
                printf("[Bad checksum']");
            }
            
            if(hdr->opcode == SB_INST_NOP)
            {
                color(RED);
                printf("NOOP\n");
                offset += BLOCK_SIZE;
            }
            else if(hdr->opcode == SB_INST_TAG)
            {
                struct sb_instruction_tag_t *tag = (struct sb_instruction_tag_t *)hdr;
                color(RED);
                printf("BTAG");
                color(OFF);printf(" | ");
                color(BLUE);
                printf("sec=0x%08x", tag->identifier);
                color(OFF);printf(" | ");
                color(GREEN);
                printf("cnt=0x%08x", tag->len);
                color(OFF);printf(" | ");
                color(YELLOW);
                printf("flg=0x%08x", tag->flags);
                color(OFF);
                if(tag->hdr.flags & SB_INST_LAST_TAG)
                {
                    printf(" | ");
                    color(RED);
                    printf(" Last section");
                    color(OFF);
                }
                printf("\n");
                offset += sizeof(struct sb_instruction_tag_t);

                char name[5];
                fill_section_name(name, tag->identifier);
                int pos = offset;
                int size = tag->len * BLOCK_SIZE;
                int data_sec = !(tag->flags & SECTION_BOOTABLE);
                int encrypted = !(tag->flags & SECTION_CLEARTEXT) && sb_header->nr_keys > 0;
            
                color(GREEN);
                printf("%sSection ", indent);
                color(YELLOW);
                printf("'%s'\n", name);
                color(GREEN);
                printf("%s  pos   = ", indent);
                color(YELLOW);
                printf("%8x - %8x\n", pos, pos+size);
                color(GREEN);
                printf("%s  len   = ", indent);
                color(YELLOW);
                printf("%8x\n", size);
                color(GREEN);
                printf("%s  flags = ", indent);
                color(YELLOW);
                printf("%8x", tag->flags);
                color(RED);
                if(data_sec)
                    printf("  Data Section");
                else
                    printf("  Boot Section");
                if(encrypted)
                    printf(" (Encrypted)");
                printf("\n");

                /* save it */
                byte *sec = xmalloc(size);
                if(encrypted)
                    cbc_mac(g_buf + pos, sec, size / BLOCK_SIZE, real_key, g_buf, NULL, 0);
                else
                    memcpy(sec, g_buf + pos, size);
                
                extract_section(data_sec, name, sec, size, "      ");
                free(sec);

                /* last one ? */
                if(tag->hdr.flags & SB_INST_LAST_TAG)
                    break;
                offset += size;
            }
            else
            {
                color(RED);
                printf("Unknown instruction %d at address 0x%08lx\n", hdr->opcode, (long)offset);
                break;
            }
        }
    }
    
    /* final signature */
    color(BLUE);
    printf("Final signature:\n");
    byte decrypted_block[32];
    if(sb_header->nr_keys > 0)
    {
        color(GREEN);
        printf("  Encrypted SHA-1:\n");
        color(YELLOW);
        byte *encrypted_block = &g_buf[filesize - 32];
        printf("    ");
        print_hex(encrypted_block, 16, true);
        printf("    ");
        print_hex(encrypted_block + 16, 16, true);
        /* decrypt it */
        cbc_mac(encrypted_block, decrypted_block, 2, real_key, g_buf, NULL, 0);
    }
    else
        memcpy(decrypted_block, &g_buf[filesize - 32], 32);
    color(GREEN);
    printf("  File SHA-1:\n    ");
    color(YELLOW);
    print_hex(decrypted_block, 20, false);
    /* check it */
    sha_1_init(&sha_1_params);
    sha_1_update(&sha_1_params, g_buf, filesize - 32);
    sha_1_finish(&sha_1_params);
    sha_1_output(&sha_1_params, computed_sha1);
    color(RED);
    if(memcmp(decrypted_block, computed_sha1, 20) == 0)
        printf(" Ok\n");
    else
        printf(" Failed\n");
}

void usage(void)
{
    printf("Usage: sbtoelf [options] sb-file\n");
    printf("Options:\n");
    printf("  -?/--help\tDisplay this message\n");
    printf("  -o <file>\tSet output prefix\n");
    printf("  -d/--debug\tEnable debug output\n");
    printf("  -k <file>\tAdd key file\n");
    printf("  -z\t\tAdd zero key\n");
    printf("  -r\t\tUse raw command mode\n");
    printf("  --add-key <key>\tAdd single key (hex or usbotp)\n");
    exit(1);
}

static struct crypto_key_t g_zero_key =
{
    .method = CRYPTO_KEY,
    .u.key = {0}
};

int main(int argc, char **argv)
{
    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, '?'},
            {"debug", no_argument, 0, 'd'},
            {"add-key", required_argument, 0, 'a'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "?do:k:zra:", long_options, NULL);
        if(c == -1)
            break;
        switch(c)
        {
            case -1:
                break;
            case 'd':
                g_debug = true;
                break;
            case '?':
                usage();
                break;
            case 'o':
                g_out_prefix = optarg;
                break;
            case 'k':
            {
                add_keys_from_file(optarg);
                break;
            }
            case 'z':
            {
                add_keys(&g_zero_key, 1);
                break;
            }
            case 'r':
                g_raw_mode = true;
                break;
            case 'a':
            {
                struct crypto_key_t key;
                char *s = optarg;
                if(!parse_key(&s, &key))
                    bug("Invalid key specified as argument");
                if(*s != 0)
                    bug("Trailing characters after key specified as argument");
                add_keys(&key, 1);
                break;
            }
            default:
                abort();
        }
    }

    if(g_out_prefix == NULL)
        g_out_prefix = "";

    if(argc - optind != 1)
    {
        usage();
        return 1;
    }

    const char *sb_file = argv[optind];
    FILE *fd = fopen(sb_file, "rb");
    if(fd == NULL)
        bug("Cannot open input file\n");
    fseek(fd, 0, SEEK_END);
    size_t size = ftell(fd);
    fseek(fd, 0, SEEK_SET);

    g_buf = xmalloc(size);
    if(fread(g_buf, 1, size, fd) != size) /* load the whole file into memory */
        bugp("reading firmware");

    fclose(fd);

    extract(size);

    color(OFF);

    free(g_buf);
    return 0;
}
