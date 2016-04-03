/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2015 by Amaury Pouly
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

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <stdint.h>
#include <fstream>
#include <sys/stat.h>
#include <getopt.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <vector>
#include <string>
#include <algorithm>
#include <ctime>
#include <sstream>

struct jz4760_fw_hdr_t
{
    uint32_t magic;
    uint32_t fs_size; // unsure, always 0x7f00000
    uint32_t hdr_size; // header size in sectors, always 4
    uint32_t pad;
    char datetime[12]; // format: yyyymmddhhmm
    uint32_t nr_files;
    uint32_t unk; // always 2, seems related to CRC, maybe CRC mode
    uint32_t pad2[3];
    uint8_t machine[8];
    uint8_t pad3[452];
    uint32_t eoh; // end of header
} __attribute__((packed));

const uint32_t JZ4760_FW_MAGIC = 0x49484653; // 'IHFS'
const uint32_t JZ4760_FW_EOH = 0x55aa55aa; // end of header

struct jz4760_fw_file_t
{
    char name[56];
    uint32_t offset; // absolute offset in sectors
    uint32_t size; // in bytes
} __attribute__((packed));

const uint32_t jz4760_crc_key[256] =
{
    0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
    0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
    0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
    0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
    0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
    0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
    0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
    0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
    0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
    0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
    0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
    0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
    0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
    0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
    0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
    0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
    0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
    0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
    0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
    0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
    0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
    0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
    0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
    0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
    0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
    0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
    0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
    0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
    0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
    0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
    0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
    0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0,
};

/* This table is generated programmatically: it contains two copies of a 172-bytes
 * table. This table contains the lower 8-bit of the first 172 prime numbers:
 * 2, 3, 5, 7, ..., 251(=0xfb), 257(=0x101 -> 0x01), ... */
const unsigned JZ4760_XOR_KEY_SIZE = 344;
uint8_t jz4760_xor_key[JZ4760_XOR_KEY_SIZE] =
{
    0x02, 0x03, 0x05, 0x07, 0x0b, 0x0d, 0x11, 0x13,
    0x17, 0x1d, 0x1f, 0x25, 0x29, 0x2b, 0x2f, 0x35,
    0x3b, 0x3d, 0x43, 0x47, 0x49, 0x4f, 0x53, 0x59,
    0x61, 0x65, 0x67, 0x6b, 0x6d, 0x71, 0x7f, 0x83,
    0x89, 0x8b, 0x95, 0x97, 0x9d, 0xa3, 0xa7, 0xad,
    0xb3, 0xb5, 0xbf, 0xc1, 0xc5, 0xc7, 0xd3, 0xdf,
    0xe3, 0xe5, 0xe9, 0xef, 0xf1, 0xfb, 0x01, 0x07,
    0x0d, 0x0f, 0x15, 0x19, 0x1b, 0x25, 0x33, 0x37,
    0x39, 0x3d, 0x4b, 0x51, 0x5b, 0x5d, 0x61, 0x67,
    0x6f, 0x75, 0x7b, 0x7f, 0x85, 0x8d, 0x91, 0x99,
    0xa3, 0xa5, 0xaf, 0xb1, 0xb7, 0xbb, 0xc1, 0xc9,
    0xcd, 0xcf, 0xd3, 0xdf, 0xe7, 0xeb, 0xf3, 0xf7,
    0xfd, 0x09, 0x0b, 0x1d, 0x23, 0x2d, 0x33, 0x39,
    0x3b, 0x41, 0x4b, 0x51, 0x57, 0x59, 0x5f, 0x65,
    0x69, 0x6b, 0x77, 0x81, 0x83, 0x87, 0x8d, 0x93,
    0x95, 0xa1, 0xa5, 0xab, 0xb3, 0xbd, 0xc5, 0xcf,
    0xd7, 0xdd, 0xe3, 0xe7, 0xef, 0xf5, 0xf9, 0x01,
    0x05, 0x13, 0x1d, 0x29, 0x2b, 0x35, 0x37, 0x3b,
    0x3d, 0x47, 0x55, 0x59, 0x5b, 0x5f, 0x6d, 0x71,
    0x73, 0x77, 0x8b, 0x8f, 0x97, 0xa1, 0xa9, 0xad,
    0xb3, 0xb9, 0xc7, 0xcb, 0xd1, 0xd7, 0xdf, 0xe5,
    0xf1, 0xf5, 0xfb, 0xfd, 0x02, 0x03, 0x05, 0x07,
    0x0b, 0x0d, 0x11, 0x13, 0x17, 0x1d, 0x1f, 0x25,
    0x29, 0x2b, 0x2f, 0x35, 0x3b, 0x3d, 0x43, 0x47,
    0x49, 0x4f, 0x53, 0x59, 0x61, 0x65, 0x67, 0x6b,
    0x6d, 0x71, 0x7f, 0x83, 0x89, 0x8b, 0x95, 0x97,
    0x9d, 0xa3, 0xa7, 0xad, 0xb3, 0xb5, 0xbf, 0xc1,
    0xc5, 0xc7, 0xd3, 0xdf, 0xe3, 0xe5, 0xe9, 0xef,
    0xf1, 0xfb, 0x01, 0x07, 0x0d, 0x0f, 0x15, 0x19,
    0x1b, 0x25, 0x33, 0x37, 0x39, 0x3d, 0x4b, 0x51,
    0x5b, 0x5d, 0x61, 0x67, 0x6f, 0x75, 0x7b, 0x7f,
    0x85, 0x8d, 0x91, 0x99, 0xa3, 0xa5, 0xaf, 0xb1,
    0xb7, 0xbb, 0xc1, 0xc9, 0xcd, 0xcf, 0xd3, 0xdf,
    0xe7, 0xeb, 0xf3, 0xf7, 0xfd, 0x09, 0x0b, 0x1d,
    0x23, 0x2d, 0x33, 0x39, 0x3b, 0x41, 0x4b, 0x51,
    0x57, 0x59, 0x5f, 0x65, 0x69, 0x6b, 0x77, 0x81,
    0x83, 0x87, 0x8d, 0x93, 0x95, 0xa1, 0xa5, 0xab,
    0xb3, 0xbd, 0xc5, 0xcf, 0xd7, 0xdd, 0xe3, 0xe7,
    0xef, 0xf5, 0xf9, 0x01, 0x05, 0x13, 0x1d, 0x29,
    0x2b, 0x35, 0x37, 0x3b, 0x3d, 0x47, 0x55, 0x59,
    0x5b, 0x5f, 0x6d, 0x71, 0x73, 0x77, 0x8b, 0x8f,
    0x97, 0xa1, 0xa9, 0xad, 0xb3, 0xb9, 0xc7, 0xcb,
    0xd1, 0xd7, 0xdf, 0xe5, 0xf1, 0xf5, 0xfb, 0xfd,
};

bool g_verbose = false;

uint8_t *read_file(const std::string& path, size_t& size)
{
    std::ifstream fin(path.c_str(), std::ios::binary);
    if(!fin)
    {
        printf("Error: cannot open '%s'\n", path.c_str());
        return 0;
    }
    fin.seekg(0, std::ios::end);
    size = fin.tellg();
    fin.seekg(0, std::ios::beg);
    uint8_t *buf = new uint8_t[size];
    fin.read((char *)buf, size);
    return buf;
}

bool write_file(const std::string& path, uint8_t *buf, size_t size)
{
    std::ofstream fout(path.c_str(), std::ios::binary);
    if(!fout)
    {
        printf("Error: cannot open '%s'\n", path.c_str());
        return false;
    }
    fout.write((char *)buf, size);
    fout.close();
    return true;
}

bool create_dir(const std::string& dir)
{
    mkdir(dir.c_str(), 0755);
    return true;
}

// the firmware uses '\' as separator
bool create_file(std::string path, uint8_t *buf, size_t size)
{
    size_t pos = -1;
    while((pos = path.find_first_of("/\\", pos + 1)) != std::string::npos)
    {
        path[pos] = '/';
        create_dir(path.substr(0, pos));
    }
    return write_file(path, buf, size);
}

bool list_files(const std::string& prefix, const std::string& path,
    std::vector< std::string >& list)
{
    DIR *dir = opendir((prefix + "/" + path).c_str());
    if(!dir)
    {
        printf("Error: cannot open directory '%s'\n", path.c_str());
        goto Lerr;
    }
    struct dirent *entry;
    while((entry = readdir(dir)))
    {
        std::string p = entry->d_name;
        if(p == "." || p == "..")
            continue;
        std::string subname = prefix + "/" + path + "/" + entry->d_name;
        struct stat st;
        if(lstat(subname.c_str(), &st) != 0)
        {
            printf("Error: cannot stat '%s'\n", subname.c_str());
            goto Lerr;
        }
        if(!path.empty())
            p = path + "/" + p;
        if(st.st_mode & S_IFREG)
            list.push_back(p);
        if(st.st_mode & S_IFDIR)
        {
            if(!list_files(prefix, p, list))
                goto Lerr;
        }
    }
    closedir(dir);
    return true;
Lerr:
    closedir(dir);
    return false;
}

std::vector< std::string > list_files(const std::string& path, bool& ok)
{
    std::vector< std::string > list;
    ok = list_files(path, "", list);
    return list;
}

uint32_t jz4760_crc(uint32_t crc, uint8_t *buf, size_t size)
{
    while(size--)
        crc = jz4760_crc_key[*buf++ ^ ((crc & 0xff00) >> 8)] ^ (crc << 8);
    return crc;
}

int unpack(int argc, char **argv)
{
    std::string input_file;
    std::string output_dir;
    while(1)
    {
        int c = getopt(argc, argv, "i:o:v");
        if(c == -1)
            break;
        switch(c)
        {
            case 'v':
                g_verbose = true;
                break;
            case 'i':
                input_file = optarg;
                break;
            case 'o':
                output_dir = optarg;
                break;
            default:
                abort();
        }
    }
    if(optind != argc)
        return printf("Error: extra arguments on command line\n");
    if(input_file.empty())
        return printf("Error: you need to specify the input file\n");
    if(output_dir.empty())
        printf("Warning: you did not specify an output directory, this will be a dry run\n");
    size_t fw_size;
    uint8_t *fw_buf = read_file(input_file, fw_size);
    if(!fw_buf)
        return 1;
    /* firmware has N blocks of 512 plus a 4 byte checksum at the end
     * each file must have at least 4 sectors for the header and 256 for the entries
     * for a total of 260 */
    if(fw_size < 512 * 260)
        return printf("Error: firmware is too small\n");
    if((fw_size - 4) % 512)
        return printf("Error: firmware has bad size\n");
    struct jz4760_fw_hdr_t *fw_hdr = (struct jz4760_fw_hdr_t *)fw_buf;
    if(fw_hdr->magic != JZ4760_FW_MAGIC)
        return printf("Error: firmware has wrong magic value\n");
    if(fw_hdr->eoh != JZ4760_FW_EOH)
        return printf("Error: firmware has wrong end of header value\n");
    if(g_verbose)
    {
        printf("Header:\n");
        printf("  Magic: %#x\n", fw_hdr->magic);
        printf("  FS Size: %#x\n", fw_hdr->fs_size);
        printf("  Header Size: %d\n", fw_hdr->hdr_size);
        printf("  Date: %c%c/%c%c/%c%c%c%c\n", fw_hdr->datetime[6],
            fw_hdr->datetime[7], fw_hdr->datetime[4], fw_hdr->datetime[5],
            fw_hdr->datetime[0], fw_hdr->datetime[1], fw_hdr->datetime[2],
            fw_hdr->datetime[3]);
        printf("  Time: %c%c:%c%c\n", fw_hdr->datetime[8],
            fw_hdr->datetime[9], fw_hdr->datetime[10], fw_hdr->datetime[11]);
        printf("  Files: %u\n", fw_hdr->nr_files);
        printf("  Unknown: %x\n", fw_hdr->unk);
        printf("  Machine: %.8s\n", fw_hdr->machine);
    }
    uint32_t crc = *reinterpret_cast< uint32_t* >(fw_buf + fw_size - 4);
    if(crc != jz4760_crc(0, fw_buf, fw_size - 4))
        return printf("Error: CRC does not match\n");
    // check number of files
    if(fw_hdr->nr_files > 2048)
        return printf("Error: too many files\n");
    struct jz4760_fw_file_t *fw_file = (struct jz4760_fw_file_t *)(fw_buf + 4 * 512);
    for(unsigned i = 0; i < fw_hdr->nr_files; i++)
    {
        if(g_verbose)
        {
            printf("File #%d:\n", i);
            printf("  Name: %s\n", fw_file[i].name);
            printf("  Offset: %#x\n", fw_file[i].offset);
            printf("  Size: %u\n", fw_file[i].size);
        }
        if(output_dir.empty())
            continue;
        create_file(output_dir + "/" + std::string(fw_file[i].name, sizeof(fw_file[i].name)),
            fw_buf + 512 * fw_file[i].offset, fw_file[i].size);
    }
    return 0;
}

bool parse_time(const std::string& time, std::tm& tm)
{
    std::istringstream iss(time);
    iss >> tm.tm_mday;
    if(iss.get() != '/')
        goto Lwarn;
    iss >> tm.tm_mon;
    if(iss.get() != '/')
        goto Lwarn;
    iss >> tm.tm_year;
    iss >> tm.tm_hour;
    if(iss.get() != ':')
        goto Lwarn;
    iss >> tm.tm_min;
    return true;
Lwarn:
    printf("Error: wrong time format\n");
    return false;
}

bool istrcmp(const std::string& a, const std::string& b)
{
    return strcasecmp(a.c_str(), b.c_str()) < 0;
}

int pack(int argc, char **argv)
{
    std::string input_dir;
    std::string output_file;
    std::string machine;
    std::string datetime;
    while(1)
    {
        int c = getopt(argc, argv, "i:o:m:vt:");
        if(c == -1)
            break;
        switch(c)
        {
            case 'v':
                g_verbose = true;
                break;
            case 'i':
                input_dir = optarg;
                break;
            case 'o':
                output_file = optarg;
                break;
            case 'm':
                machine = optarg;
                break;
            case 't':
                datetime = optarg;
                break;
            default:
                abort();
        }
    }
    if(optind != argc)
        return printf("Error: extra arguments on command line\n");
    if(input_dir.empty())
        return printf("Error: you need to specify the input file\n");
    if(machine.empty())
        printf("Warning: you did not specify the machine flags, the produce file may be invalid\n");
    if(output_file.empty())
        return printf("Error: you must specify an output file\n");
    bool ok = true;
    std::vector< std::string > list = list_files(input_dir, ok);
    if(!ok)
        return printf("Error: there were some errors while listing files\n");
    /* NOTE: keep compatibility with original packtools which sorts files,
     * I doubt the sorted order is really necessary but not costly anyway */
    std::sort(list.begin(), list.end(), istrcmp);
    if(g_verbose)
    {
        for(size_t i = 0; i < list.size(); i++)
            printf("%s\n", list[i].c_str());
    }
    if(list.size() > 2048)
        return printf("Error: cannot make an archive with more than 2048 files\n");
    struct jz4760_fw_hdr_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic = JZ4760_FW_MAGIC;
    hdr.fs_size = 0x7f00000;
    hdr.hdr_size = 4;
    std::time_t time = std::time(0);
    std::tm *tm = localtime(&time);
    if(!datetime.empty())
        if(!parse_time(datetime, *tm))
            return 1;
    char str[13];
    snprintf(str, sizeof(str), "%04d%02d%02d%02d%02d", tm->tm_year, tm->tm_mon,
        tm->tm_mday, tm->tm_hour, tm->tm_min);
    memcpy(hdr.datetime, str, sizeof(hdr.datetime));
    hdr.unk = 2;
    hdr.nr_files = list.size();
    memcpy(hdr.machine, machine.c_str(), sizeof(hdr.machine));
    hdr.eoh = JZ4760_FW_EOH;
    uint32_t crc = jz4760_crc(0, (uint8_t *)&hdr, sizeof(hdr));

    std::ofstream fout(output_file.c_str(), std::ios::binary);
    fout.write((char *)&hdr, sizeof(hdr));
    // three empty sectors
    uint8_t zero_sector[512];
    memset(zero_sector, 0, sizeof(zero_sector));
    crc = jz4760_crc(crc, zero_sector, sizeof(zero_sector));
    fout.write((char *)zero_sector, sizeof(zero_sector));
    crc = jz4760_crc(crc, zero_sector, sizeof(zero_sector));
    fout.write((char *)zero_sector, sizeof(zero_sector));
    crc = jz4760_crc(crc, zero_sector, sizeof(zero_sector));
    fout.write((char *)zero_sector, sizeof(zero_sector));
    // write file headers
    size_t tot_size = 260; // 4 for headers, 256 for file headers
    for(size_t i = 0; i < list.size(); i++)
    {
        jz4760_fw_file_t file;
        memset(&file, 0, sizeof(file));
        std::string name = list[i];
        for(size_t j = 0; j < name.size(); j++)
            if(name[j] == '/')
                name[j] = '\\';
        if(name.size() > sizeof(file.name))
            return printf("Error: filename '%s' is too long\n", name.c_str());
        memcpy(file.name, name.c_str(), name.size());
        struct stat st;
        if(lstat((input_dir + "/" + list[i]).c_str(), &st))
            return printf("Error: cannot stat file '%s'\n", list[i].c_str());
        file.size = st.st_size;
        file.offset = tot_size;
        tot_size += (file.size + 511) / 512;
        crc = jz4760_crc(crc, (uint8_t *)&file, sizeof(file));
        fout.write((char *)&file, sizeof(file));
    }
    // finish writing the 256 sectors
    for(size_t i = list.size(); i < 2048; i++)
    {
        crc = jz4760_crc(crc, zero_sector, sizeof(jz4760_fw_file_t));
        fout.write((char *)zero_sector, sizeof(jz4760_fw_file_t));
    }
    // write the files
    for(size_t i = 0; i < list.size(); i++)
    {
        size_t sz;
        uint8_t *data = read_file(input_dir + "/" + list[i], sz);
        if(!data)
            return printf("Error: cannot read file '%s'\n", list[i].c_str());
        crc = jz4760_crc(crc, data, sz);
        fout.write((char *)data, sz);
        delete data;
        if(sz % 512)
        {
            crc = jz4760_crc(crc, zero_sector, 512 - (sz % 512));
            fout.write((char *)zero_sector, 512 - (sz % 512));
        }
    }
    // write the crc
    fout.write((char *)&crc, sizeof(crc));
    fout.close();
    return 0;
}

int descramble(int argc, char **argv)
{
    std::string input_file;
    std::string output_file;
    while(1)
    {
        int c = getopt(argc, argv, "i:o:");
        if(c == -1)
            break;
        switch(c)
        {
            case 'i':
                input_file = optarg;
                break;
            case 'o':
                output_file = optarg;
                break;
            default:
                abort();
        }
    }
    if(optind != argc)
        return printf("Error: extra arguments on command line\n");
    if(input_file.empty() || output_file.empty())
        return printf("Error: you need to specify the input and output files\n");
    size_t fw_size;
    uint8_t *fw_buf = read_file(input_file, fw_size);
    if(!fw_buf)
        return 1;
    for(size_t i = 0; i < fw_size; i++)
        fw_buf[i] ^= jz4760_xor_key[i % JZ4760_XOR_KEY_SIZE];
    if(!write_file(output_file, fw_buf, fw_size))
        return 1;
    return 0;
}

void usage()
{
    printf("usage: [--pack|--unpack|--descramble|--scramble] <options>\n");
    printf("  unpack options:\n");
    printf("    -i <file>  Input file\n");
    printf("    -o <dir>   Output directory\n");
    printf("    -v         Verbose output\n");
    printf("  pack options:\n");
    printf("    -i <dir>   Input directory\n");
    printf("    -o <file>  Output file\n");
    printf("    -m <mach>  Machine flags\n");
    printf("    -v         Verbose output\n");
    printf("    -t <time>  Override date/time (dd/mm/yy hh:mm)\n");
    printf("  (de)scramble options:\n");
    printf("    -i <file>  Input file\n");
    printf("    -o <file>  Output file\n");
    exit(1);
}

int main(int argc, char **argv)
{
    if(argc <= 1)
        usage();
    if(strcmp(argv[1], "--unpack") == 0)
        return unpack(argc - 1, argv + 1);
    if(strcmp(argv[1], "--pack") == 0)
        return pack(argc - 1, argv + 1);
    if(strcmp(argv[1], "--descramble") == 0 || strcmp(argv[1], "--scramble") == 0)
        return descramble(argc - 1, argv + 1);
    usage();
    return 1;
}

