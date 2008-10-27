/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2008 by Jin Le
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
 * dl_analyser.c ONDA VX767 DL file analyser
 *
 * Copyright (C) 2008 - JinLe
 *
 * This is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, 
 * Boston, MA  02110-1301  USA

    The DL file can not find any entry point,
    so I think it just a dynamic library
    not executable.
    
    IN THE FILE
    +--------------------------
    +      block_header_t
    +--------------------------
    +      block_impt_header_t
    +--------------------------
    +      block_expt_header_t
    +--------------------------
    +      block_raw_header_t
    +--------------------------
    +      import symbol
    +--------------------------
    +      export symbol
    +--------------------------
    +      padding
    +-------------------------- <-----(raw->offset)
    +
    +      raw code seg
    +
    +--------------------------
    +
    +      inited mem seg
    +
    +-------------------------- <-----(raw->offset + raw->size)(bss start)
    
    IN THE MEMORY
    +-------------------------- <-----(raw->mem2)
    +
    +      code seg
    +
    +--------------------------
    +
    +      inited mem seg
    +
    +-------------------------- <-----(raw->mem2 + raw->size)(bss start)
    +
    +      BSS(Not in file)
    +
    +-------------------------- <-----(raw->mem2 + raw->memsize)(bss end)
    
    HOW TO disassemble (Ex: VX767_V1.0.dl)
    
    STEP 1:
    ./dl_analyser VX767_V1.0.dl

    =======================HEADER=====================
    File magic: CCDL
    File Type : 0x00010000
    Offset    : 0x00020001
    Size      : 0x00000004
    BuildDate : 2008/03/26 09:59:19
    PaddindSum: 0x0
    =====================IMPT HEADER==================
    Header magic   : IMPT
    Header Type    : 0x00000008
    Offset         : 0x000000a0
    Size           : 0x0000007c
    PaddindSum     : 0x0
    =====================EXPT HEADER==================
    Header magic   : EXPT
    Header Type    : 0x00000009
    Offset         : 0x00000120
    Size           : 0x00000108
    PaddindSum     : 0x0
    =====================RAWD HEADER==================
    Header magic    : RAWD
    Header Type     : 0x00000001
    Offset          : 0x00000230
    Size            : 0x000058a0
    Paddind1        : 0x0
    BSS Clear Code  : 0x80f82714 start at file 0x2944
    mem_place_start : 0x80f80000 start at file 0x230
    memsize         : 0x5a58
    mem_end(BSS end): 0x80f85a58
    Paddind2Sum     : 0x0
    =====================IMPORT SYMBOL==================
    number symbols  : 0x4
    PaddindSum      : 0x0
    Sym[00] offset 0x0000 padding 0x0 flag 0x20000 address 0x80f82750 name: printf
    Sym[01] offset 0x0008 padding 0x0 flag 0x20000 address 0x80f82758 name: udelay
    Sym[02] offset 0x0010 padding 0x0 flag 0x20000 address 0x80f82760 name: delay_ms
    Sym[03] offset 0x001c padding 0x0 flag 0x20000 address 0x80f82768 name: get_rgb_lcd_buf
    =====================EXPORT SYMBOL==================
    number symbols  : 0x7
    PaddindSum      : 0x0
    Sym[00] offset 0x0000 padding 0x0 flag 0x20000 address 0x80f826dc name: init_lcd_register
    Sym[01] offset 0x0014 padding 0x0 flag 0x20000 address 0x80f80160 name: get_ccpmp_config
    Sym[02] offset 0x0028 padding 0x0 flag 0x20000 address 0x80f82690 name: get_bklight_config
    Sym[03] offset 0x003c padding 0x0 flag 0x20000 address 0x80f81120 name: init_lcd_gpio
    Sym[04] offset 0x004c padding 0x0 flag 0x20000 address 0x80f804d0 name: rgb_user_init
    Sym[05] offset 0x005c padding 0x0 flag 0x20000 address 0x80f806a4 name: get_rgb_frame_buf
    Sym[06] offset 0x0070 padding 0x0 flag 0x20000 address 0x80f8269c name: lcd_set_direction_mode

    STEP 2:
    mips-linux-objdump -bbinary -mmips -D VX767_V1.0.dl > 767.as
    
    STEP 3:
    for function lcd_set_direction_mode(address 0x80f8269c)
    we translate that address into 'file address'
    file address = 0x80f8269c - 0x80f80000 + 0x230 = 0x28CC
    
    STEP 4:
    Find code in 767.as use this 'file address'
    
    2008.10.20 6:23PM
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

/*******************************HEADER*****************************/
typedef struct
{
    char magic[4];
    int type;
    int offset;
    int size;
    unsigned char date[7];
    unsigned char padding[9];
}block_header_t;

typedef struct
{
    char magic[4];
    int type;
    int offset;
    int size;
    int padding[4];
}block_impt_header_t;

typedef struct
{
    char magic[4];
    int type;
    int offset;
    int size;
    int padding[4];
}block_expt_header_t;

typedef struct
{
    char magic[4];
    int type;
    int offset;
    int size;
    int padding1;
    int mem1;
    int mem2;
    int memsize;
    int padding2[8];
}block_raw_header_t;

/*******************************SYMBOL*****************************/
typedef struct
{
    int offset;
    int padding;
    int flag;
    int address;
    char *name;
}symbol_t;

typedef struct
{
    int numsymbol;
    int padding[3];
    int isimport;
    symbol_t *symbol;
}import_export_symbol_t;

void usage(char *name)
{
    fprintf(stderr, "Usage: %s [dl file]\n", name);
}

void dump_header(block_header_t *header)
{
    int tmp;
    fprintf(stderr, "=======================HEADER=====================\n");
    fprintf(stderr, "File magic: %c%c%c%c\n", header->magic[0], header->magic[1], header->magic[2], header->magic[3]);
    fprintf(stderr, "File Type : 0x%08x\n", header->type);
    fprintf(stderr, "Offset    : 0x%08x\n", header->offset);
    fprintf(stderr, "Size      : 0x%08x\n", header->size);
    fprintf(stderr, "BuildDate : %02x%02x/%02x/%02x %02x:%02x:%02x\n",
            header->date[0], header->date[1],
            header->date[2], header->date[3],
            header->date[4], header->date[5],
            header->date[6]);
    tmp = header->padding[0] + header->padding[1] + header->padding[2] + header->padding[3] + header->padding[4] + 
            header->padding[5] + header->padding[6] + header->padding[7] + header->padding[8];
    fprintf(stderr, "PaddindSum: 0x%x\n", tmp);
}

void dump_import_symbol_header(block_impt_header_t *impt)
{
    int tmp;
    fprintf(stderr, "=====================IMPT HEADER==================\n");
    fprintf(stderr, "Header magic   : %c%c%c%c\n", impt->magic[0], impt->magic[1], impt->magic[2], impt->magic[3]);
    fprintf(stderr, "Header Type    : 0x%08x\n", impt->type);
    fprintf(stderr, "Offset         : 0x%08x\n", impt->offset);
    fprintf(stderr, "Size           : 0x%08x\n", impt->size);
    tmp = impt->padding[0] + impt->padding[1] + impt->padding[2] + impt->padding[3];
    fprintf(stderr, "PaddindSum     : 0x%x\n", tmp);
}

void dump_export_symbol_header(block_expt_header_t *expt)
{
    int tmp;
    fprintf(stderr, "=====================EXPT HEADER==================\n");
    fprintf(stderr, "Header magic   : %c%c%c%c\n", expt->magic[0], expt->magic[1], expt->magic[2], expt->magic[3]);
    fprintf(stderr, "Header Type    : 0x%08x\n", expt->type);
    fprintf(stderr, "Offset         : 0x%08x\n", expt->offset);
    fprintf(stderr, "Size           : 0x%08x\n", expt->size);
    tmp = expt->padding[0] + expt->padding[1] + expt->padding[2] + expt->padding[3];
    fprintf(stderr, "PaddindSum     : 0x%x\n", tmp);
}

void dump_raw_data_header(block_raw_header_t *raw)
{
    int tmp;
    fprintf(stderr, "=====================RAWD HEADER==================\n");
    fprintf(stderr, "Header magic    : %c%c%c%c\n", raw->magic[0], raw->magic[1], raw->magic[2], raw->magic[3]);
    fprintf(stderr, "Header Type     : 0x%08x\n", raw->type);
    fprintf(stderr, "Offset          : 0x%08x\n", raw->offset);
    fprintf(stderr, "Size            : 0x%08x\n", raw->size);
    fprintf(stderr, "Paddind1        : 0x%x\n", raw->padding1);
    fprintf(stderr, "BSS Clear Code  : 0x%x start at file 0x%x\n", raw->mem1, raw->mem1-raw->mem2+raw->offset);
    fprintf(stderr, "mem_start       : 0x%x start at file 0x%x\n", raw->mem2, raw->offset);
    fprintf(stderr, "memsize         : 0x%x\n", raw->memsize);
    fprintf(stderr, "mem_end(BSS end): 0x%x\n", raw->memsize + raw->mem2);
    tmp = raw->padding2[0] + raw->padding2[1] + raw->padding2[2] + raw->padding2[3] +
            raw->padding2[4] + raw->padding2[5] + raw->padding2[6] + raw->padding2[7];
    fprintf(stderr, "Paddind2Sum     : 0x%x\n", tmp);
}

void dump_symbol_table(import_export_symbol_t *sym, char *prefix)
{
    int tmp;
    int i;
    
    fprintf(stderr, "=====================%s==================\n", prefix);
    fprintf(stderr, "number symbols  : 0x%x\n", sym->numsymbol);
    tmp = sym->padding[0] + sym->padding[1] + sym->padding[2];
    fprintf(stderr, "PaddindSum      : 0x%x\n", tmp);
    for(i=0; i<sym->numsymbol; i++)
    {
        fprintf(stderr, "Sym[%02d] offset 0x%04x padding 0x%x flag 0x%x address 0x%x name: %s\n", 
                        i, sym->symbol[i].offset, sym->symbol[i].padding,
                        sym->symbol[i].flag, sym->symbol[i].address, sym->symbol[i].name);
    }
}

static int read_symbols(int fd, import_export_symbol_t *sym)
{
    int numbers = sym->numsymbol;
    int i, ret;
    int len = 0, flag = 0;
    char buffer;
    int nametab_offset;
    
    if(numbers == 0 || fd < 0)
        return 0;
    /*Read table*/
    sym->symbol = (symbol_t *)malloc(sizeof(symbol_t) * numbers);
    for(i=0; i<numbers; i++)
    {
        /*Offset*/
        if((ret = read(fd, &sym->symbol[i].offset, sizeof(int))) < 0)
            return -1;
        /*Padding*/
        if((ret = read(fd, &sym->symbol[i].padding, sizeof(int))) < 0)
            return -1;
        /*Flag*/
        if((ret = read(fd, &sym->symbol[i].flag, sizeof(int))) < 0)
            return -1;
        /*Address*/
        if((ret = read(fd, &sym->symbol[i].address, sizeof(int))) < 0)
            return -1;
    }
    /*Read name*/
    nametab_offset = lseek(fd, 0, SEEK_CUR);
    for(i=0; i<numbers; i++)
    {
        /*Set seek start*/
        lseek(fd, nametab_offset + sym->symbol[i].offset, SEEK_SET);
        /*get length of name*/
        while(flag != 2)
        {
            if((ret = read(fd, &buffer, sizeof(char))) < 0)
                return -1;
            if(buffer != 0)
                len++;
            else
                flag++;
        }
        if(len == 0)
            break;
        /*Reset seek start*/
        lseek(fd, nametab_offset + sym->symbol[i].offset, SEEK_SET);
        /*Read name*/
        sym->symbol[i].name = (char *)malloc(sizeof(char) * (len+1));
        memset(sym->symbol[i].name, 0, len+1);
        if((ret = read(fd, sym->symbol[i].name, sizeof(char)*len)) < 0)
            return -1;
        flag = len = 0;
    }
    return i;
}

int analyze_dl(int fd)
{
    int ret = -1;
    block_header_t header;
    block_impt_header_t impt;
    block_expt_header_t expt;
    block_raw_header_t raw;
    import_export_symbol_t isym;
    import_export_symbol_t esym;
    
    /*Read Header*/
    if((ret = read(fd, &header, sizeof(block_header_t))) < 0)
        return -1;
    dump_header(&header);
    /*Read Import header*/
    if((ret = read(fd, &impt, sizeof(block_impt_header_t))) < 0)
        return -1;
    dump_import_symbol_header(&impt);
    /*Read Export header*/
    if((ret = read(fd, &expt, sizeof(block_expt_header_t))) < 0)
        return -1;
    dump_export_symbol_header(&expt);
    /*Read Raw data header*/
    if((ret = read(fd, &raw, sizeof(block_raw_header_t))) < 0)
        return -1;
    dump_raw_data_header(&raw);
    
    /*read import symbol*/
    lseek(fd, impt.offset, SEEK_SET);
    /*number*/
    if((ret = read(fd, &isym.numsymbol, sizeof(int))) < 0)
        return -1;
    /*padding*/
    if((ret = read(fd, &isym.padding, sizeof(int)*3)) < 0)
        return -1;
    if((ret = read_symbols(fd, &isym)) < 0)
    {
        return -1;
    }
    dump_symbol_table(&isym, "IMPORT SYMBOL");
    
    /*read export symbol*/
    lseek(fd, expt.offset, SEEK_SET);
    /*number*/
    if((ret = read(fd, &esym.numsymbol, sizeof(int))) < 0)
        return -1;
    /*padding*/
    if((ret = read(fd, &esym.padding, sizeof(int)*3)) < 0)
        return -1;
    if((ret = read_symbols(fd, &esym)) < 0)
    {
        return -1;
    }
    dump_symbol_table(&esym, "EXPORT SYMBOL");
    return 0;
}

int main(int argc, char *argv[])
{
    int fd = -1;
    int ret = -1;

    if(argc != 2)
    {
        usage(argv[0]);
        return -1;
    }
    fd = open(argv[1], O_RDONLY);
    if(fd < 0)
    {
        perror("Open");
        return -1;
    }
    ret = analyze_dl(fd);
    return ret;
}
