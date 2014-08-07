/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 *
 * Copyright (C) 2014 by Marcin Bukat
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
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "rkw.h"

const char *section_name[] = {
   "RKLD",
   "RKRS",
   "RKST"
};

const uint32_t section_magic[] = {
    RKLD_MAGIC,
    RKRS_MAGIC,
    RKST_MAGIC
};

const char *rkrs_action_name[] = {
    [act_null] = "null",
    [act_mkdir] = "mkdir",
    [act_fcopy] = "fcopy",
    [act_fsoper] = "fsoper",
    [act_format] = "format",
    [act_loader] = "loader",
    [act_dispbmp] = "dispbmp",
    [act_dispstr] = "dispstr",
    [act_setfont] = "setfont",
    [act_delay] = "delay",
    [act_system] = "system",
    [act_readme] = "readme",
    [act_copyright] = "copyright",
    [act_select] = "select",
    [act_restart] = "restart",
    [act_regkey] = "regkey",
    [act_version] = "version",
    [act_freplace] = "freplace",
    [act_fpreplace] = "fpreplace",
    [act_fsdel] = "fsdel",
    [act_space] = "space",
    [act_addfile] = "addfile",
    [act_setmem] = "setmem",
    [act_getmem] = "getmem"
};

/* scrambling/descrambling reverse engineered by AleMaxx */
static void encode_page(uint8_t *inpg, uint8_t *outpg, const int size)
{

uint8_t key[] = {
        0x7C, 0x4E, 0x03, 0x04,
        0x55, 0x05, 0x09, 0x07,
        0x2D, 0x2C, 0x7B, 0x38,
        0x17, 0x0D, 0x17, 0x11
};
        int i, i3, x, val, idx;

        uint8_t key1[0x100];
        uint8_t key2[0x100];

        for (i=0; i<0x100; i++) {
                key1[i] = i;
                key2[i] = key[i&0xf];
        }

        i3 = 0;
        for (i=0; i<0x100; i++) {
                x = key1[i];
                i3 = key1[i] + i3;
                i3 += key2[i];
                i3 &= 0xff;
                key1[i] = key1[i3];
                key1[i3] = x;
        }

        idx = 0;
        for (i=0; i<size; i++) {
                x = key1[(i+1) & 0xff];
                val = x;
                idx = (x + idx) & 0xff;
                key1[(i+1) & 0xff] = key1[idx];
                key1[idx] = (x & 0xff);
                val = (key1[(i+1)&0xff] + x) & 0xff;
                val = key1[val];
                outpg[i] = val ^ inpg[i];
        }
}

/* take path as stored in RKST and convert it to unix path
 * with optional prefix
 */
static char *unixpath(char *path, char *prefix)
{
    char *parsed, *ptr;
    size_t size = 0;

    if (NULL == path)
        return NULL;

    size = strlen(path) + 1;

    /* full windows path i.e C:\something */
    if (strlen(path) > 2 && ':' == path[1])
        path += 3;

    if (prefix)
    {
        /* account for '/' after prefix */
        size += strlen(prefix) + 1;
    }

    /* allocate buffer */
    parsed = malloc(size);
    ptr = parsed;

    /* malloc failed */
    if (NULL == ptr)
        return NULL;

    /* copy prefix */
    if (prefix)
    {
        strcpy(ptr, prefix);
        ptr += strlen(prefix);
        *ptr++ = '/';
    }

    do
    {
        if (*path == '\\')
            *ptr = '/';
        else
            *ptr = *path;

        ptr++;
    } while ('\0' != *(path++));

    return parsed;
}

/* returns pointer to the rkrs header in rkw file */
static char *find_section(struct rkw_info_t *rkw_info, enum section_type_t type)
{
    char *ptr;
    struct section_header_t *h;

    switch(type)
    {
        case ST_RKRS:
        case ST_RKST:
            for (ptr=(char *)rkw_info->rkw; ptr<(char *)rkw_info->rkw+rkw_info->size; ptr++)
            {
                h = (struct section_header_t *)ptr;
                if (h->magic == section_magic[type] &&
                    h->size == sizeof(struct section_header_t) &&
                    h->number_of_named_entries != 0)
                {
                    fprintf(stderr, "[info]: %s found at 0x%0x\n",
                            section_name[type], (int)(ptr - rkw_info->rkw));

                    return ptr;
                }
            }
            break;

        default:
            fprintf(stderr, "[error]: Not supported section type %d\n", type);
            return NULL;
    }
    return NULL;
}

/* load rkw file into memory and setup pointers to various sections */
struct rkw_info_t *rkw_slurp(char *filename)
{
    FILE *fp;
    struct rkw_info_t *rkw_info;

    rkw_info = (struct rkw_info_t *)malloc(sizeof(struct rkw_info_t));

    if (NULL == rkw_info)
    {
        fprintf(stderr, "[error]: Can't allocate %ld bytes of memory\n",
                sizeof(struct rkw_info_t));

        return NULL;
    }

    fp = fopen(filename, "rb");

    if (NULL == fp)
    {
        fprintf(stderr, "[error]: Can't open %s\n", filename);
        free(rkw_info);
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    rkw_info->size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    rkw_info->rkw = (char *)malloc(rkw_info->size);

    if (NULL == rkw_info->rkw)
    {
        fprintf(stderr, "[error]: Can't allocate %ld bytes of memory\n",
                rkw_info->size);

        free(rkw_info);
        fclose(fp);
        return NULL;
    }

    if (fread(rkw_info->rkw, rkw_info->size, 1, fp) != 1)
    {
        fprintf(stderr, "[error]: Can't read %s\n", filename);
        free(rkw_info);
        fclose(fp);
        return NULL;
    }

    rkw_info->rkrs_info.header = find_section(rkw_info, ST_RKRS);
    rkw_info->rkrs_info.items = rkw_info->rkrs_info.header +
                                 ((struct section_header_t *)
                                  (rkw_info->rkrs_info.header))->offset_of_named_entries;

    rkw_info->rkst_info.header = find_section(rkw_info, ST_RKST);
    rkw_info->rkst_info.items = rkw_info->rkst_info.header +
                                 ((struct section_header_t *)
                                  (rkw_info->rkst_info.header))->offset_of_named_entries;

    fclose(fp);
    return rkw_info;
}

void rkw_free(struct rkw_info_t *rkw_info)
{
    free(rkw_info->rkw);
    free(rkw_info);
}

static void rkrs_named_item_info(struct rkrs_named_t *item)
{
    fprintf(stderr, "[info]: size=0x%0x (%d)\n", item->size, item->size);
    fprintf(stderr, "[info]: type=0x%0x (%d) %s\n", item->type, item->type, rkrs_action_name[item->type]);
    fprintf(stderr, "[info]: data_offset=0x%0x (%d)\n", item->data_offset, item->data_offset);
    fprintf(stderr, "[info]: data_size=0x%0x (%d)\n", item->data_size, item->data_size);
    fprintf(stderr, "[info]: param[0]=0x%0x (%d)\n", item->param[0], item->param[0]);
    fprintf(stderr, "[info]: param[1]=0x%0x (%d)\n", item->param[1], item->param[1]);
    fprintf(stderr, "[info]: param[2]=0x%0x (%d)\n", item->param[2], item->param[2]);
    fprintf(stderr, "[info]: param[3]=0x%0x (%d)\n", item->param[3], item->param[3]);
}

static void rkst_named_item_info(struct rkst_named_t *item)
{
    fprintf(stderr, "[info]: size=0x%0x (%d)\n", item->size, item->size);
    fprintf(stderr, "[info]: action=0x%0x (%d) %s\n", item->action, item->action, rkrs_action_name[item->action]);
    fprintf(stderr, "[info]: data_offset=0x%0x (%d)\n", item->data_offset, item->data_offset);
    fprintf(stderr, "[info]: data_size=0x%0x (%d)\n", item->data_size, item->data_size);
    fprintf(stderr, "[info]: name=\"%s\"\n", &item->name);
}

static struct rkrs_named_t *find_item(struct rkw_info_t *rkw_info, enum rkst_action_t type, bool search_start)
{
    static struct rkrs_named_t *item;

    if (search_start)
    {
        item  = (struct rkrs_named_t *)rkw_info->rkrs_info.items;
    }
    else
    {
        if (item)
            item++;
        else
            return NULL;
    }

    while (item->size > 0)
    {
        if (item->type == type)
        {
            fprintf(stderr, "[info]: Item type=%d found at 0x%x\n", type,
                    (int)((char *)item - rkw_info->rkw));
            return item;
        }
        item++;
    }

    return NULL;
}

void rkrs_list_named_items(struct rkw_info_t *rkw_info)
{
    struct rkrs_named_t *item = (struct rkrs_named_t *)rkw_info->rkrs_info.items;
    struct section_header_t *rkrs_header = (struct section_header_t *)(rkw_info->rkrs_info.header);
    int i;

    for (i=0; i<rkrs_header->number_of_named_entries; i++)
    {
        fprintf(stderr, "[info]: rkrs named entry %d\n", i);
        rkrs_named_item_info(item++);
        fprintf(stderr, "\n");
    }
}

void rkst_list_named_items(struct rkw_info_t *rkw_info)
{
    struct rkst_named_t *item = (struct rkst_named_t *)rkw_info->rkst_info.items;
    struct section_header_t *rkst_header = (struct section_header_t *)(rkw_info->rkst_info.header);
    int i;

    for (i=0; i<rkst_header->number_of_named_entries; i++)
    {
        fprintf(stderr, "[info]: rkst named entry %d\n", i);
        rkst_named_item_info(item);
        item = (struct rkst_named_t *)((char *)item + item->size);
        fprintf(stderr, "\n");
    }
}

void unpack_bootloader(struct rkw_info_t *rkw_info, char *prefix)
{
    FILE *fp;
    char *ptr;
    size_t size;
    int len;
    char *buf;
    struct rkrs_named_t *item = find_item(rkw_info, act_loader, true);

    if (NULL == item)
    {
        fprintf(stderr, "[error]: Can't find nand bootloader\n");
        return;
    }

    ptr = (char *)(rkw_info->rkrs_info.header) + item->data_offset;
    size = item->param[0];
    buf = malloc(size);

    if (NULL == buf)
    {
        fprintf(stderr, "[error]: Can't allocate %ld bytes of memory\n",
                size);
        return;
    }

    /* make a copy for decryption */
    memcpy(buf, ptr, size);
    encode_page((uint8_t *)buf, (uint8_t *)buf, size);
    fp = fopen(unixpath("s1.bin", prefix), "w");

    if (NULL == fp)
    {
        fprintf(stderr, "[error]: Can't open s1.bin for writing\n");
        free(buf);
        return;
    }

    if (fwrite(buf, size, 1, fp) != 1)
    {
        fprintf(stderr, "[error]: Can't write s1.bin file\n");
        free(buf);
        fclose(fp);
        return;
    }

    fclose(fp);

    ptr = (char *)(rkw_info->rkrs_info.header) + item->param[1];
    size = item->param[2];
    len = size;
    buf = realloc(buf, size);

    if (NULL == buf)
    {
        fprintf(stderr, "[error]: Can't allocate %ld bytes of memory\n",
                size);

        free(buf);
        return;
    }

    memcpy(buf, ptr, size);
    ptr = buf;

    while (len >= 0x200)
    {
        encode_page((uint8_t *)ptr, (uint8_t *)ptr, 0x200);
        ptr += 0x200;
        len -= 0x200;
    }
    encode_page((uint8_t *)ptr, (uint8_t *)ptr, len);

    fp = fopen(unixpath("s2.bin", prefix), "w");

    if (NULL == fp)
    {
        fprintf(stderr, "[error]: Can't open s2.bin for writing\n");
        free(buf);
        return;
    }

    if (fwrite(buf, size, 1, fp) != 1)
    {
        fprintf(stderr, "[error]: Can't write s2.bin file\n");
        free(buf);
        fclose(fp);
        return;
    }

    fclose(fp);
    free(buf);  
    fprintf(stderr, "[info]: Extracted bootloader version: %x.%x\n",
            (item->param[3] >> 8) & 0xff, item->param[3] & 0xff);
}

void unpack_addfile(struct rkw_info_t *rkw_info, char *prefix)
{
    FILE *fp;
    char *name;
    int name_len;

    struct rkrs_named_t *item = find_item(rkw_info, act_addfile, true);

    do
    {
        name = unixpath(rkw_info->rkrs_info.header + item->data_offset, prefix);
        name_len = item->param[0];

        fprintf(stderr, "[info]: unpacking addfile %s\n", name);

        fp = fopen(name, "w");

        if (NULL == fp)
        {
            fprintf(stderr, "[error]: Can't open %s for writing\n", name);
            return;
        }

        if (fwrite(rkw_info->rkrs_info.header + item->data_offset + name_len,
               item->data_size - name_len, 1, fp) != 1)
        {
            fprintf(stderr, "[error]: Can't write %s file\n", name);
            fclose(fp);
            return;
        }

        fclose(fp);
    } while (NULL != (item = find_item(rkw_info, act_addfile, false)));
}

/* unpack content of RKST section
 * this mimics what is done when processing 'fsoper' field of RKRS
 */
void unpack_rkst(struct rkw_info_t *rkw_info, char *prefix)
{
    FILE *fp;
    struct rkst_named_t *item = (struct rkst_named_t *)rkw_info->rkst_info.items;
    struct section_header_t *rkst_header = (struct section_header_t *)(rkw_info->rkst_info.header);
    char *name;
    int i;

    if (prefix)
    {
        if (0 != mkdir(prefix, 0755))
        {
            fprintf(stderr, "[error]: Can't create %s directory (%s)\n",
                    prefix, strerror(errno));
            return;
        }
    }

    fprintf(stderr, "[info]: Unpacking content of RKST section\n");

    for (i=0; i<rkst_header->number_of_named_entries; i++)
    {
        name = unixpath((char *)&(item->name), prefix);

        switch (item->action)
        {
            case act_mkdir:
                if (0 != mkdir(name, 0755))
                {
                    fprintf(stderr, "[error]: Can't create %s directory (%s)\n",
                            name, strerror(errno));
                    return;
                }
                fprintf(stderr, "[info]: mkdir %s\n", name);
                break;

            case act_fcopy:
                fp = fopen(name, "w");
                if (NULL == fp)
                {
                    fprintf(stderr, "[error]: Can't open %s for writing (%s)\n",
                            name, strerror(errno));
                    return;
                }

                fwrite((char *)rkst_header + item->data_offset, item->data_size, 1, fp);
                fprintf(stderr, "[info]: unpack %s\n", name);
                fclose(fp);
                break;

            default:
                break;
        }

        if (name) free(name);
        item = (struct rkst_named_t *)((char *)item + item->size);
    }
}
