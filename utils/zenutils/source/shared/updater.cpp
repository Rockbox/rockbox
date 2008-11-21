/* zenutils - Utilities for working with creative firmwares.
 * Copyright 2007 (c) Rasmus Ry <rasmus.ry{at}gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "updater.h"
#include <file.h>
#include <pe.h>
#include <utils.h>


const char* zen::find_firmware_key(const byte* buffer, size_t len)
{
    char szkey1[] = "34d1";
    size_t cchkey1 = strlen(szkey1);
    char szkey2[] = "TbnCboEbn";
    size_t cchkey2 = strlen(szkey2);
    for (int i = 0; i < static_cast<int>(len); i++)
    {
        if (len >= cchkey1)
        {
            if (!strncmp((char*)&buffer[i], szkey1, cchkey1))
            {
                return (const char*)&buffer[i];
            }
        }
        if (len >= cchkey2)
        {
            if (!strncmp((char*)&buffer[i], szkey2, cchkey2))
            {
                return (const char*)&buffer[i];
            }
        }
    }
    return "";
}

dword zen::find_firmware_offset(byte* buffer, size_t len)
{
    for (dword i = 0; i < static_cast<dword>(len); i += 4)
    {
        dword size = *(dword*)&buffer[i];
        if (buffer[i + sizeof(dword)] != 0
            && buffer[i + sizeof(dword) + 1] != 0
            && buffer[i + sizeof(dword) + 2] != 0
            && buffer[i + sizeof(dword) + 3] != 0)
        {
            return i;
        }
        if(i > 0xFF) /* Arbitrary guess */
            return 0;
    }
    return 0;
}

bool zen::find_firmware_archive(const std::string& filename, dword& va, dword& pa)
{
    shared::pe_file pef;
    if (!pef.read(filename))
    {
        return false;
    }
    shared::section_info data_section;
    if (!pef.find_section(".data", data_section))
    {
        return false;
    }
    shared::bytes buffer;
    if (!shared::read_file(filename, buffer, data_section.raw_address,
                           data_section.raw_size))
    {
        return false;
    }
    dword offset = find_firmware_offset(&buffer[0], buffer.size());
    if (!offset)
    {
        return false;
    }
    va = data_section.virtual_address + offset;
    pa = data_section.raw_address + offset;

    return true;
}


bool zen::crypt_firmware(const char* key, byte* buffer, size_t len)
{
#if 1
    char key_cpy[255];
    unsigned int i;
    unsigned int tmp = 0;
    int key_length = strlen(key);
    
    for(i=0; i < strlen(key); i++)
        key_cpy[i] = key[i] - 1;
    
    for(i=0; i < len; i++)
    {
        buffer[i] ^= key_cpy[tmp] | 0x80;
        tmp = (tmp + 1) % key_length;
    }
    
    return true;
#else
    /* Determine if the key length is dword aligned. */
    int keylen = strlen(key);
    int keylen_rem = keylen % sizeof(dword);

    /* Determine how many times the key must be repeated to be dword aligned. */
    int keycycle = keylen_rem ? (sizeof(dword) / keylen_rem) : 1;
    int keyscount = (keylen * keycycle) / sizeof(dword);

    /* Allocate a buffer to hold the key as an array of dwords. */
    dword* keys = new dword[keyscount];

    /* Copy the key into the key array, whilst mutating it. */
    for (int i = 0; i < keyscount; i++)
    {
        dword val;
        int keyoffset = (i * sizeof(dword)) % keylen;
        if ((keyoffset+sizeof(dword)) < keylen)
        {
            val = *(dword*)&key[keyoffset];
        }
        else
        {
            val = key[keyoffset]
                | (key[(keyoffset + 1) % keylen] << 8)
                | (key[(keyoffset + 2) % keylen] << 16)
                | (key[(keyoffset + 3) % keylen] << 24);
        }
        keys[i] = (val - 0x01010101) | 0x80808080;
    }

    /* Determine the number of dwords in the buffer. */
    int len_div = len / sizeof(dword);

    /* Decrypt all dwords of the buffer. */
    for (int i = 0; i < len_div; i++)
    {
        ((dword*)buffer)[i] ^= keys[i % keyscount];
    }

    /* Determine the remaining number of bytes in the buffer. */
    int len_rem = len % sizeof(dword);

    /* Decrypt the remaining number of bytes in the buffer. */
    for (int i = len_div * sizeof(dword); i < len; i++)
    {
        buffer[i] ^= ((key[i % keylen] - 0x01) | 0x80);
    }

    return true;
#endif
}
