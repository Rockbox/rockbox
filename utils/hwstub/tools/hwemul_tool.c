/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2012 by Amaury Pouly
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
#include "hwemul.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <readline/history.h>

bool g_quiet = false;
struct hwemul_device_t hwdev;
struct hwemul_soc_t *cur_soc = NULL;

void print_log(struct hwemul_device_t *hwdev)
{
    do
    {
        char buffer[128];
        int length = hwemul_get_log(hwdev, buffer, sizeof(buffer) - 1);
        if(length <= 0)
            break;
        buffer[length] = 0;
        printf("%s", buffer);
    }while(1);
}

int print_help()
{
    printf("Commands:\n");
    printf("  help\t\tDisplay this help\n");
    printf("  call <addr>\tCall address <addr>\n");
    printf("  quit\t\tQuit this session\n");
    printf("  read32 <addr>\tRead a 32-bit word at <addr>\n");
    printf("  write32 <value> <addr>\tRead the 32-bit word <value> at <addr>\n");
    printf("  read <regname>\tRead a register by name\n");
    printf("  read <regname>.<field>\tRead a register field by name\n");
    printf("  soc <socname>\tSelect the soc description to use\n");
    printf("  write <value> <regname>\tWrite a register by name\n");
    printf("  write <value <regname>.<field>\tWrite a register field by name\n");
    printf("    NOTE: if the register is SCT variant, no read is performed.\n");
    return 1;
}

int syntax_error(char *str)
{
    printf("Syntax error at '%s'. Type 'help' to get some help.\n", str);
    return 1;
}

int parse_uint32(char *str, uint32_t *u)
{
    char *end;
    *u = strtoul(str, &end, 0);
    return *end == 0;
}

int do_call(uint32_t a)
{
    hwemul_call(&hwdev, a);
    return 1;
}

int parse_call()
{
    char *arg = strtok(NULL, " ");
    uint32_t addr;
    if(arg && parse_uint32(arg, &addr))
        return do_call(addr);
    else
        return syntax_error(arg);
}

int do_read32(uint32_t a)
{
    uint32_t val;
    if(hwemul_rw_mem(&hwdev, 1, a, &val, sizeof(val)) == sizeof(val))
        printf("%#x = %#x\n", a, val);
    else
        printf("read error at %#x\n", a);
    return 1;
}

int parse_read32()
{
    char *arg = strtok(NULL, " ");
    uint32_t addr;
    if(arg && parse_uint32(arg, &addr))
        return do_read32(addr);
    else
        return syntax_error(arg);
}

int do_write32(uint32_t val, uint32_t a)
{
    if(hwemul_rw_mem(&hwdev, 0, a, &val, sizeof(val)) == sizeof(val))
        printf("data written\n");
    else
        printf("write error at %#x\n", a);
    return 1;
}

int parse_write32()
{
    char *arg = strtok(NULL, " ");
    uint32_t val;
    if(!arg || !parse_uint32(arg, &val))
        return syntax_error(arg);
    uint32_t addr;
    arg = strtok(NULL, " ");
    if(arg && parse_uint32(arg, &addr))
        return do_write32(val, addr);
    else
        return syntax_error(arg);
}

struct hwemul_soc_t *find_soc_by_name(const char *soc)
{
    struct hwemul_soc_list_t *list = hwemul_get_soc_list();
    for(size_t i = 0; i < list->nr_socs; i++)
        if(strcmp(soc, list->socs[i]->name) == 0)
            return list->socs[i];
    return NULL;
}

struct hwemul_soc_reg_t *find_reg_by_name(struct hwemul_soc_t *soc, const char *reg)
{
    for(size_t i = 0; i < soc->nr_regs; i++)
        if(strcmp(reg, soc->regs_by_name[i]->name) == 0)
            return soc->regs_by_name[i];
    return NULL;
}

struct hwemul_soc_reg_field_t *find_field_by_name(struct hwemul_soc_reg_t *reg, const char *field)
{
    for(size_t i = 0; i < reg->nr_fields; i++)
        if(strcmp(field, reg->fields_by_name[i]->name) == 0)
            return reg->fields_by_name[i];
    return NULL;
}


int do_read(char *regname)
{
    char *dot = strchr(regname, '.');
    if(dot != NULL)
        *dot++ = 0;
    if(cur_soc == NULL)
    {
        printf("No soc selected!\n");
        return 1;
    }
    struct hwemul_soc_reg_t *reg = find_reg_by_name(cur_soc, regname);
    if(reg == NULL)
    {
        printf("no reg '%s' found\n", regname);
        return 1;
    }
    uint32_t val;
    if(hwemul_rw_mem(&hwdev, 1, reg->addr, &val, sizeof(val)) != sizeof(val))
    {
        printf("read error at %#x\n", reg->addr);
        return 1;
    }
    if(dot)
    {
        struct hwemul_soc_reg_field_t *field = find_field_by_name(reg, dot);
        if(field == NULL)
        {
            printf("no field '%s' found\n", dot);
            return 1;
        }
        val >>= field->first_bit;
        val &= (1 << (field->last_bit - field->first_bit + 1)) - 1;
        printf("%s.%s = %#x\n", regname, dot, val);
    }
    else
        printf("%s = %#x\n", regname, val);
    return 1;
}

int parse_read()
{
    char *arg = strtok(NULL, " ");
    if(arg)
        return do_read(arg);
    else
        return syntax_error(arg);
}

int do_soc(char *soc)
{
    struct hwemul_soc_t *s = find_soc_by_name(soc);
    if(s == NULL)
        printf("no soc '%s' found\n", soc);
    else
        cur_soc = s;
    return 1;
}

int parse_soc()
{
    char *arg = strtok(NULL, " ");
    if(arg)
        return do_soc(arg);
    else
        return syntax_error(arg);
}

int do_write(uint32_t val, char *regname)
{
    char *dot = strchr(regname, '.');
    if(dot != NULL)
        *dot++ = 0;
    if(cur_soc == NULL)
    {
        printf("No soc selected!\n");
        return 1;
    }
    struct hwemul_soc_reg_t *reg = find_reg_by_name(cur_soc, regname);
    int is_sct = 0;
    uint32_t addr_off = 0;
    if(reg == NULL)
    {
        size_t len = strlen(regname);
        /* try SCT variant */
        if(strcmp(regname + len - 4, "_SET") == 0)
            addr_off = 4;
        else if(strcmp(regname + len - 4, "_CLR") == 0)
            addr_off = 8;
        else if(strcmp(regname + len - 4, "_TOG") == 0)
            addr_off = 12;
        else
        {
            printf("no reg '%s' found\n", regname);
            return 1;
        }
        is_sct = 1;
        regname[len - 4] = 0;
        reg = find_reg_by_name(cur_soc, regname);
        if(reg == NULL)
        {
            printf("no reg '%s' found\n", regname);
            return 1;
        }
    }
    if(dot)
    {
        struct hwemul_soc_reg_field_t *field = find_field_by_name(reg, dot);
        if(field == NULL)
        {
            printf("no field '%s' found\n", dot);
            return 1;
        }
        uint32_t actual_val = 0;
        if(!is_sct)
        {
            if(hwemul_rw_mem(&hwdev, 1, reg->addr, &actual_val, sizeof(actual_val)) != sizeof(actual_val))
            {
                printf("read error at %#x\n", reg->addr);
                return 1;
            }
            printf("read %#x at %#x\n", actual_val, reg->addr);
        }
        uint32_t mask = ((1 << (field->last_bit - field->first_bit + 1)) - 1) << field->first_bit;
        printf("mask=%#x\n", mask);
        val = (actual_val & ~mask) | ((val << field->first_bit) & mask);
    }
    printf("write %#x to %#x\n", val, reg->addr + addr_off);
    if(hwemul_rw_mem(&hwdev, 0, reg->addr + addr_off, &val, sizeof(val)) != sizeof(val))
    {
        printf("write error at %#x\n", reg->addr);
        return 1;
    }
    return 1;
}

int parse_write()
{
    char *arg = strtok(NULL, " ");
    uint32_t val;
    if(!arg || !parse_uint32(arg, &val))
        return syntax_error(arg);
    arg = strtok(NULL, " ");
    if(arg)
        return do_write(val, arg);
    else
        return syntax_error(arg);
}

int parse_command(char *cmd)
{
    if(strcmp(cmd, "help") == 0)
        return print_help();
    if(strcmp(cmd, "quit") == 0)
        return 0;
    if(strcmp(cmd, "call") == 0)
        return parse_call();
    if(strcmp(cmd, "read32") == 0)
        return parse_read32();
    if(strcmp(cmd, "write32") == 0)
        return parse_write32();
    if(strcmp(cmd, "read") == 0)
        return parse_read();
    if(strcmp(cmd, "soc") == 0)
        return parse_soc();
    if(strcmp(cmd, "write") == 0)
        return parse_write();
    return syntax_error(cmd);
}

void interactive_mode(void)
{
    rl_bind_key('\t', rl_complete);
    while(1)
    {
        char *input = readline("> ");
        if(!input)
            break;
        add_history(input);
        int ret = parse_command(input);
        free(input);
        if(ret == 0)
            break;
    }
}

void usage(void)
{
    printf("hwemul_tool, compiled with hwemul %d.%d.%d\n",
        HWEMUL_VERSION_MAJOR, HWEMUL_VERSION_MINOR, HWEMUL_VERSION_REV);
    printf("available soc descriptions:");
    for(unsigned i = 0; i < hwemul_get_soc_list()->nr_socs; i++)
        printf(" %s", hwemul_get_soc_list()->socs[i]->name);
    printf("\n");
    printf("usage: hwemul_tool [options]\n");
    printf("options:\n");
    printf("  --help/-?\tDisplay this help\n");
    printf("  --quiet/-q\tQuiet non-command messages\n");
    exit(1);
}

int main(int argc, char **argv)
{
    while(1)
    {
        static struct option long_options[] =
        {
            {"help", no_argument, 0, '?'},
            {"quiet", no_argument, 0, 'q'},
            {0, 0, 0, 0}
        };

        int c = getopt_long(argc, argv, "?q", long_options, NULL);
        if(c == -1)
            break;
        switch(c)
        {
            case -1:
                break;
            case 'q':
                g_quiet = true;
                break;
            case '?':
                usage();
                break;
            default:
                abort();
        }
    }

    if(argc - optind != 0)
    {
        usage();
        return 1;
    }
    
    libusb_context *ctx;
    libusb_init(&ctx);
    libusb_set_debug(ctx, 3);

    if(!g_quiet)
        printf("Looking for device %#04x:%#04x...\n", HWEMUL_USB_VID, HWEMUL_USB_PID);

    libusb_device_handle *handle = libusb_open_device_with_vid_pid(ctx,
        HWEMUL_USB_VID, HWEMUL_USB_PID);
    if(handle == NULL)
    {
        printf("No device found\n");
        return 1;
    }

    libusb_device *mydev = libusb_get_device(handle);
    if(!g_quiet)
    {
        printf("device found at %d:%d\n",
            libusb_get_bus_number(mydev),
            libusb_get_device_address(mydev));
    }
    hwdev.handle = handle;
    if(hwemul_probe(&hwdev))
    {
        printf("Cannot probe device!\n");
        return 1;
    }

    struct usb_resp_info_version_t ver;
    int ret = hwemul_get_info(&hwdev, HWEMUL_INFO_VERSION, &ver, sizeof(ver));
    if(ret != sizeof(ver))
    {
        printf("Cannot get version!\n");
        goto Lerr;
    }
    if(!g_quiet)
        printf("Device version: %d.%d.%d\n", ver.major, ver.minor, ver.revision);

    struct usb_resp_info_layout_t layout;
    ret = hwemul_get_info(&hwdev, HWEMUL_INFO_LAYOUT, &layout, sizeof(layout));
    if(ret != sizeof(layout))
    {
        printf("Cannot get layout: %d\n", ret);
        goto Lerr;
    }
    if(!g_quiet)
    {
        printf("Device layout:\n");
        printf("  Code: 0x%x (0x%x)\n", layout.oc_code_start, layout.oc_code_size);
        printf("  Stack: 0x%x (0x%x)\n", layout.oc_stack_start, layout.oc_stack_size);
        printf("  Buffer: 0x%x (0x%x)\n", layout.oc_buffer_start, layout.oc_buffer_size);
    }

    struct usb_resp_info_features_t features;
    ret = hwemul_get_info(&hwdev, HWEMUL_INFO_FEATURES, &features, sizeof(features));
    if(ret != sizeof(features))
    {
        printf("Cannot get features: %d\n", ret);
        goto Lerr;
    }
    if(!g_quiet)
    {
        printf("Device features:");
        if(features.feature_mask & HWEMUL_FEATURE_LOG)
            printf(" log");
        if(features.feature_mask & HWEMUL_FEATURE_MEM)
            printf(" mem");
        if(features.feature_mask & HWEMUL_FEATURE_CALL)
            printf(" call");
        if(features.feature_mask & HWEMUL_FEATURE_JUMP)
            printf(" jump");
        if(features.feature_mask & HWEMUL_FEATURE_AES_OTP)
            printf(" aes_otp");
        printf("\n");
    }

    struct usb_resp_info_stmp_t stmp;
    ret = hwemul_get_info(&hwdev, HWEMUL_INFO_STMP, &stmp, sizeof(stmp));
    if(ret != sizeof(stmp))
    {
        printf("Cannot get stmp: %d\n", ret);
        goto Lerr;
    }
    if(!g_quiet)
    {
        printf("Device stmp:\n");
        printf("  chip ID: %x (%s)\n", stmp.chipid,hwemul_get_product_string(&stmp));
        printf("  revision: %d (%s)\n", stmp.rev, hwemul_get_rev_string(&stmp));
        printf("  supported: %d\n", stmp.is_supported);
    }

    if(!g_quiet)
    {
        void *rom = malloc(64 * 1024);
        ret = hwemul_rw_mem(&hwdev, 1, 0xc0000000, rom, 64 * 1024);
        if(ret != 64 * 1024)
        {
            printf("Cannot read ROM: %d\n", ret);
            goto Lerr;
        }
    
        printf("ROM successfully read!\n");
        FILE *f = fopen("rom.bin", "wb");
        fwrite(rom, 64 * 1024, 1, f);
        fclose(f);
    }

    if(!g_quiet)
    {
        struct
        {
            uint8_t iv[16];
            uint8_t data[16];
        } __attribute__((packed)) dcp_test;

        for(int i = 0; i < 16; i++)
            dcp_test.iv[i] = rand();
        for(int i = 0; i < 16; i++)
            dcp_test.data[i] = rand();
        printf("DCP\n");
        printf("  IN\n");
        printf("    IV:");
        for(int i = 0; i < 16; i++)
            printf(" %02x", dcp_test.iv[i]);
        printf("\n");
        printf("    IV:");
        for(int i = 0; i < 16; i++)
            printf(" %02x", dcp_test.data[i]);
        printf("\n");

        if(!hwemul_aes_otp(&hwdev, &dcp_test, sizeof(dcp_test), HWEMUL_AES_OTP_ENCRYPT))
        {
            printf("  OUT\n");
            printf("    IV:");
            for(int i = 0; i < 16; i++)
                printf(" %02x", dcp_test.iv[i]);
            printf("\n");
            printf("    IV:");
            for(int i = 0; i < 16; i++)
                printf(" %02x", dcp_test.data[i]);
            printf("\n");
        }
        else
            printf("DCP error!\n");
    }

    if(!g_quiet)
        printf("Starting interactive session. Type 'help' to get help.\n");

    interactive_mode();

    Lerr:
    if(features.feature_mask & HWEMUL_FEATURE_LOG)
    {
        if(!g_quiet)
            printf("Device log:\n");
        print_log(&hwdev);
    }
    hwemul_release(&hwdev);
    return 1;
}
