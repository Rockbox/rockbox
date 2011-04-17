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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "crypto.h"
#include "elf.h"
#include "sb.h"

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

#define bug(...) do { fprintf(stderr,"ERROR: "__VA_ARGS__); exit(1); } while(0)
#define bugp(a) do { perror("ERROR: "a); exit(1); } while(0)

/* all blocks are sized as a multiple of 0x1ff */
#define PAD_TO_BOUNDARY(x) (((x) + 0x1ff) & ~0x1ff)

/* If you find a firmware that breaks the known format ^^ */
#define assert(a) do { if(!(a)) { fprintf(stderr,"Assertion \"%s\" failed in %s() line %d!\n\nPlease send us your firmware!\n",#a,__func__,__LINE__); exit(1); } } while(0)

/* globals */

size_t g_sz;	/* file size */
uint8_t *g_buf; /* file content */
#define PREFIX_SIZE     128
char out_prefix[PREFIX_SIZE];
const char *key_file;

void *xmalloc(size_t s) /* malloc helper, used in elf.c */
{
    void * r = malloc(s);
    if(!r) bugp("malloc");
    return r;
}

static void print_hex(byte *data, int len, bool newline)
{
    for(int i = 0; i < len; i++)
        printf("%02X ", data[i]);
    if(newline)
        printf("\n");
}

static int convxdigit(char digit, byte *val)
{
    if(digit >= '0' && digit <= '9')
    {
        *val = digit - '0';
        return 0;
    }
    else if(digit >= 'A' && digit <= 'F')
    {
        *val = digit - 'A' + 10;
        return 0;
    }
    else if(digit >= 'a' && digit <= 'f')
    {
        *val = digit - 'a' + 10;
        return 0;
    }
    else
        return 1;
}

typedef byte (*key_array_t)[16];

static key_array_t read_keys(int num_keys)
{
    int size;
    struct stat st;
    int fd = open(key_file,O_RDONLY);
    if(fd == -1)
        bugp("opening key file failed");
    if(fstat(fd,&st) == -1)
        bugp("key file stat() failed");
    size = st.st_size;
    char *buf = xmalloc(size);
    if(read(fd, buf, size) != (ssize_t)size)
        bugp("reading key file");
    close(fd);

    key_array_t keys = xmalloc(sizeof(byte[16]) * num_keys);
    int pos = 0;
    for(int i = 0; i < num_keys; i++)
    {
        /* skip ws */
        while(pos < size && isspace(buf[pos]))
            pos++;
        /* enough space ? */
        if((pos + 32) > size)
            bugp("invalid key file (not enough keys)");
        for(int j = 0; j < 16; j++)
        {
            byte a, b;
            if(convxdigit(buf[pos + 2 * j], &a) || convxdigit(buf[pos + 2 * j + 1], &b))
                bugp(" invalid key, it should be a 128-bit key written in hexadecimal\n");
            keys[i][j] = (a << 4) | b;
        }
        pos += 32;
    }
    free(buf);

    return keys;
}

#define ROUND_UP(val, round) ((((val) + (round) - 1) / (round)) * (round))

static uint8_t instruction_checksum(struct sb_instruction_header_t *hdr)
{
    uint8_t sum = 90;
    byte *ptr = (byte *)hdr;
    for(int i = 1; i < 16; i++)
        sum += ptr[i];
    return sum;
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
    elf_write_file(elf, elf_write, fd);
    fclose(fd);
}

static void extract_section(int data_sec, char name[5], byte *buf, int size, const char *indent)
{
    char filename[PREFIX_SIZE + 32];
    snprintf(filename, sizeof filename, "%s%s.bin", out_prefix, name);
    FILE *fd = fopen(filename, "wb");
    if (fd != NULL) {
        fwrite(buf, size, 1, fd);
        fclose(fd);
    }
    if(data_sec)
        return;

    snprintf(filename, sizeof filename, "%s%s", out_prefix, name);

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
            // unsure about rounding
            pos = ROUND_UP(pos, 16);
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
            // fixme: useless as pos is a multiple of 16 and fill struct is 4-bytes wide ?
            pos = ROUND_UP(pos, 16);
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
            // fixme: useless as pos is a multiple of 16 and call struct is 4-bytes wide ?
            pos = ROUND_UP(pos, 16);
        }
        else
        {
            color(RED);
            printf("Unknown instruction %d at address 0x%08lx\n", hdr->opcode, (unsigned long)pos);
            break;
        }
    }

    if(!elf_is_empty(&elf))
        extract_elf_section(&elf, elf_count++, filename, indent);
    elf_release(&elf);
}

void fill_section_name(char name[5], uint32_t identifier)
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

static void extract(unsigned long filesize)
{
    struct sha_1_params_t sha_1_params;
    /* Basic header info */
    struct sb_header_t *sb_header = (struct sb_header_t *)g_buf;

    if(memcmp(sb_header->signature, "STMP", 4) != 0)
        bugp("Bad signature");
    if(sb_header->image_size * BLOCK_SIZE != filesize)
        bugp("File size mismatch");
    if(sb_header->header_size * BLOCK_SIZE != sizeof(struct sb_header_t))
        bugp("Bad header size");
    if(sb_header->major_ver != IMAGE_MAJOR_VERSION ||
            sb_header->minor_ver != IMAGE_MINOR_VERSION)
        bugp("Bad file format version");
    if(sb_header->sec_hdr_size * BLOCK_SIZE != sizeof(struct sb_section_header_t))
        bugp("Bad section header size");
    
    color(BLUE);
    printf("Basic info:\n");
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

    color(GREEN);
    printf("  Product version   = ");
    color(YELLOW);
    printf("%X.%X.%X\n", sb_header->product_ver.major,
         sb_header->product_ver.minor,  sb_header->product_ver.revision);
    color(GREEN);
    printf("  Component version = ");
    color(YELLOW);
    printf("%X.%X.%X\n", sb_header->component_ver.major,
         sb_header->component_ver.minor,  sb_header->component_ver.revision);
        
    color(GREEN);
    printf("  Drive tag = ");
    color(YELLOW);
    printf("%x\n", sb_header->drive_tag);

    /* encryption cbc-mac */
    key_array_t keys = NULL; /* array of 16-bytes keys */
    byte real_key[16];
    if(sb_header->nr_keys > 0)
    {
        keys = read_keys(sb_header->nr_keys);
        color(BLUE);
        printf("Encryption data\n");
        for(int i = 0; i < sb_header->nr_keys; i++)
        {
            color(RED);
            printf("  Key %d: ", i);
            print_hex(keys[i], 16, true);
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
            cbc_mac(g_buf, NULL, sb_header->header_size + sb_header->nr_sections,
                keys[i], zero, &computed_cbc_mac, 1);
            color(RED);
            if(memcmp(dict_entry->hdr_cbc_mac, computed_cbc_mac, 16) == 0)
                printf(" Ok\n");
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
            cbc_mac(dict_entry->key, decrypted_key, 1, keys[i], iv, NULL, 0);
            printf("    Decrypted key     : ");
            color(YELLOW);
            print_hex(decrypted_key, 16, false);
            /* cross-check or copy */
            if(i == 0)
                memcpy(real_key, decrypted_key, 16);
            else if(memcmp(real_key, decrypted_key, 16) == 0)
            {
                color(RED);
                printf(" Cross-Check Ok");
            }
            else
            {
                color(RED);
                printf(" Cross-Check Failed");
            }
            printf("\n");
        }
    }

    /* sections */
    char *raw_cmd_env = getenv("SB_RAW_CMD");
    if(raw_cmd_env == NULL || strcmp(raw_cmd_env, "YES") != 0)
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
            int encrypted = !(sec_hdr->flags & SECTION_CLEARTEXT);
        
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
        uint32_t offset = sizeof(struct sb_header_t)
            + sizeof(struct sb_section_header_t) * sb_header->nr_sections
            + sizeof(struct sb_key_dictionary_entry_t) * sb_header->nr_keys;
        byte iv[16];
        memcpy(iv, g_buf, 16);
        const char *indent = "    ";
        while(true)
        {
            byte cmd[16];
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
                printf("[Bad checksum]");
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
                printf("flg=0x%08x\n", tag->flags);
                color(OFF);
                offset += sizeof(struct sb_instruction_tag_t);

                char name[5];
                fill_section_name(name, tag->identifier);
                int pos = offset;
                int size = tag->len * BLOCK_SIZE;
                int data_sec = !(tag->flags & SECTION_BOOTABLE);
                int encrypted = !(tag->flags & SECTION_CLEARTEXT);
            
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
                /* restart with IV */
                memcpy(iv, g_buf, 16);
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
    color(GREEN);
    printf("  Encrypted signature:\n");
    color(YELLOW);
    byte *encrypted_block = &g_buf[filesize - 32];
    printf("    ");
    print_hex(encrypted_block, 16, true);
    printf("    ");
    print_hex(encrypted_block + 16, 16, true);
    /* decrypt it */
    byte decrypted_block[32];
    cbc_mac(encrypted_block, decrypted_block, 2, real_key, g_buf, NULL, 0);
    color(GREEN);
    printf("  Decrypted SHA-1:\n    ");
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

int main(int argc, const char **argv)
{
    int fd;
    struct stat st;
    if(argc != 3 && argc != 4)
    {
        printf("Usage: %s <firmware> <key file> [<out prefix>]\n",*argv);
        printf("To use raw command mode, set environment variable SB_RAW_CMD to YES\n");
        return 1;
    }

    if(argc == 4)
        snprintf(out_prefix, PREFIX_SIZE, "%s", argv[3]);
    else
        strcpy(out_prefix, "");

    if( (fd = open(argv[1], O_RDONLY)) == -1 )
        bugp("opening firmware failed");

    key_file = argv[2];

    if(fstat(fd, &st) == -1)
        bugp("firmware stat() failed");
    g_sz = st.st_size;

    g_buf = xmalloc(g_sz);
    if(read(fd, g_buf, g_sz) != (ssize_t)g_sz) /* load the whole file into memory */
        bugp("reading firmware");

    close(fd);

    extract(st.st_size);

    color(OFF);

    free(g_buf);
    return 0;
}
