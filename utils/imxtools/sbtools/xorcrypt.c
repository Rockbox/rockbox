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
#include "crypto.h"
#include <stdlib.h>
#include "misc.h"

static uint32_t g_ROM_Key_Table[256] =
{
    0x6B8B4567,0x327B23C6,0x643C9869,0x66334873,0x74B0DC51,0x19495CFF,0x2AE8944A,
    0x625558EC,0x238E1F29,0x46E87CCD,0x3D1B58BA,0x507ED7AB,0x2EB141F2,0x41B71EFB,0x79E2A9E3,0x7545E146,
    0x515F007C,0x5BD062C2,0x12200854,0x4DB127F8,0x216231B,0x1F16E9E8,0x1190CDE7,
    0x66EF438D,0x140E0F76,0x3352255A,0x109CF92E,0xDED7263,0x7FDCC233,0x1BEFD79F,0x41A7C4C9,0x6B68079A,
    0x4E6AFB66,0x25E45D32,0x519B500D,0x431BD7B7,0x3F2DBA31,0x7C83E458,0x257130A3,
    0x62BBD95A,0x436C6125,0x628C895D,0x333AB105,0x721DA317,0x2443A858,0x2D1D5AE9,0x6763845E,0x75A2A8D4,
    0x8EDBDAB,0x79838CB2,0x4353D0CD,0xB03E0C6,0x189A769B,0x54E49EB4,0x71F32454,
    0x2CA88611,0x836C40E,0x2901D82,0x3A95F874,0x8138641,0x1E7FF521,0x7C3DBD3D,0x737B8DDC,0x6CEAF087,
    0x22221A70,0x4516DDE9,0x3006C83E,0x614FD4A1,0x419AC241,0x5577F8E1,0x440BADFC,
    0x5072367,0x3804823E,0x77465F01,0x7724C67E,0x5C482A97,0x2463B9EA,0x5E884ADC,0x51EAD36B,0x2D517796,
    0x580BD78F,0x153EA438,0x3855585C,0x70A64E2A,0x6A2342EC,0x2A487CB0,0x1D4ED43B,
    0x725A06FB,0x2CD89A32,0x57E4CCAF,0x7A6D8D3C,0x4B588F54,0x542289EC,0x6DE91B18,0x38437FDB,0x7644A45C,
    0x32FFF902,0x684A481A,0x579478FE,0x749ABB43,0x3DC240FB,0x1BA026FA,0x79A1DEAA,
    0x75C6C33A,0x12E685FB,0x70C6A529,0x520EEDD1,0x374A3FE6,0x4F4EF005,0x23F9C13C,0x649BB77C,0x275AC794,
    0x39386575,0x1CF10FD8,0x180115BE,0x235BA861,0x47398C89,0x354FE9F9,0x15B5AF5C,
    0x741226BB,0xD34B6A8,0x10233C99,0x3F6AB60F,0x61574095,0x7E0C57B1,0x77AE35EB,0x579BE4F1,0x310C50B3,
    0x5FF87E05,0x2F305DEF,0x25A70BF7,0x1DBABF00,0x4AD084E9,0x1F48EAA1,0x1381823A,
    0x5DB70AE5,0x100F8FCA,0x6590700B,0x15014ACB,0x5F5E7FD0,0x98A3148,0x799D0247,0x6B94764,0x42C296BD,
    0x168E121F,0x1EBA5D23,0x661E3F1E,0x5DC79EA8,0x540A471C,0x7BD3EE7B,0x51D9C564,
    0x613EFDC5,0xBF72B14,0x11447B73,0x42963E5A,0xA0382C5,0x8F2B15E,0x1A32234B,0x3B0FD379,0x68EB2F63,
    0x4962813B,0x60B6DF70,0x6A5EE64,0x14330624,0x7FFFCA11,0x1A27709E,0x71EA1109,
    0x100F59DC,0x7FB7E0AA,0x6EB5BD4,0x6F6DD9AC,0x94211F2,0x885E1B,0x76272110,0x4C04A8AF,0x1716703B,
    0x14E17E33,0x3222E7CD,0x74DE0EE3,0x68EBC550,0x2DF6D648,0x46B7D447,0x4A2AC315,
    0x39EE015C,0x57FC4FBB,0xCC1016F,0x43F18422,0x60EF0119,0x26F324BA,0x7F01579B,0x49DA307D,0x7055A5F5,
    0x5FB8370B,0x50801EE1,0x488AC1A,0x5FB8011C,0x6AA78F7F,0x7672BD23,0x6FC75AF8,
    0x6A5F7029,0x7D5E18F8,0x5F3534A4,0x73A1821B,0x7DE67713,0x555C55B5,0x3FA62ACA,0x14FCE74E,0x6A3DD3E8,
    0x71C91298,0x9DAF632,0x53299938,0x1FBFE8E0,0x5092CA79,0x1D545C4D,0x59ADEA3D,
    0x288F1A34,0x2A155DBC,0x1D9F6E5F,0x97E1B4E,0x51088277,0x1CA0C5FA,0x53584BCB,0x415E286C,0x7C58FD05,
    0x23D86AAC,0x45E6D486,0x5C10FE21,0xE7FFA2B,0x3C5991AA,0x4BD8591A,0x78DF6A55,
    0x39B7AAA2,0x2B0D8DBE,0x6C80EC70,0x379E21B5,0x69E373,0x2C27173B,0x4C9B0904,0x6AA7B75C,0x1DF029D3,
    0x5675FF36,0x3DD15094,0x3DB012B3,0x2708C9AF,0x5B25ACE2,0x175DFCF0,0x4F97E3E4,
    0x53B0A9E,0x34FD6B4F,0x5915FF32,0x56438D15,0x519E3149,0x2C6E4AFD,0x17A1B582,0x4DF72E4E,0x5046B5A9
};

static uint32_t do_round(union xorcrypt_key_t *key)
{
    uint32_t k7 = key->k[7];
    uint32_t k2 = key->k[2];
    uint32_t k5_3_1 = key->k[5] ^ key->k[3] ^ key->k[1];
    key->k[1] = k2;
    uint32_t k_11_7_5_3_1 = key->k[11] ^ k7 ^ k5_3_1;
    key->k[2] = key->k[3] ^ k2;
    key->k[3] = key->k[4];
    key->k[4] = key->k[5];
    key->k[5] = key->k[6];
    uint32_t k0 = key->k[0];
    key->k[7] = k7 ^ key->k[8];
    uint32_t k13 = key->k[13];
    key->k[8] = key->k[9];
    uint32_t k10 = key->k[10];
    key->k[6] = k7;
    key->k[9] = k10;
    uint32_t k11 = key->k[11];
    key->k[0] = k0 ^ k13 ^ k_11_7_5_3_1;
    key->k[10] = (k11 >> 1) | (k11 << 31);
    uint32_t k11_12 = k11 ^ key->k[12];
    uint32_t k14 = key->k[14];
    key->k[12] = k13;
    key->k[13] = k14;
    uint32_t k15 = key->k[15];
    key->k[15] = k0;
    key->k[11] = k11_12;
    key->k[14] = k15;
    return key->k[0];
}

static uint32_t do_unround(union xorcrypt_key_t *key)
{
    uint32_t k7 = key->k[6];
    uint32_t k2 = key->k[1];
    uint32_t k11 = key->k[10] >> 31 | key->k[10] << 1;
    uint32_t k0 = key->k[0];
    key->k[0] = key->k[15];
    key->k[15] = key->k[14];
    key->k[14] = key->k[13];
    key->k[13] = key->k[12];
    key->k[12] = key->k[11] ^ k11;
    key->k[11] = k11;
    key->k[10] = key->k[9];
    key->k[9] = key->k[8];
    key->k[8] = key->k[7] ^ k7;
    key->k[7] = key->k[6];
    key->k[6] = key->k[5];
    key->k[5] = key->k[4];
    key->k[4] = key->k[3];
    key->k[3] = key->k[2] ^ k2;
    key->k[2] = key->k[1];
    key->k[1] = k0 ^ key->k[0] ^ key->k[5] ^ key->k[3] ^ key->k[7] ^ key->k[11] ^ key->k[13];
    return 0;
}

static void __attribute__((unused)) test_round(union xorcrypt_key_t keys[2])
{
    union xorcrypt_key_t save[2];
    memcpy(save, keys, sizeof(save));
    do_round(keys);
    do_unround(keys);
    if(memcmp(save, keys, sizeof(save)))
    {
        printf("Mismatch\n");
        for(int i = 0; i < 16; i++)
            printf(" %s%08x", save[0].k[i] == keys[0].k[i] ? YELLOW : RED, save[0].k[i]);
        printf("\n");
        for(int i = 0; i < 16; i++)
            printf(" %s%08x", save[0].k[i] == keys[0].k[i] ? YELLOW : RED, keys[0].k[i]);
        printf("\n");
    }
}

void xor_generate_key(uint32_t laserfuse[3], union xorcrypt_key_t key[2])
{
    uint16_t bitmask = 0x36c9;
    uint8_t magic = (laserfuse[2] >> 24 | laserfuse[2] >> 16) & 0xff;
    uint32_t magic2 = laserfuse[0] | laserfuse[1];
    uint8_t magic3 = (laserfuse[2] >> 8 | laserfuse[2] >> 0) & 0xff;

    for(int i = 0; i < 16; i++, bitmask >>= 1, magic++)
        key[0].k[i] = (bitmask & 1) ? 0 : g_ROM_Key_Table[magic];
    bitmask = 0x36c9;
    for(int i = 0; i < 16; i++, bitmask >>= 1, magic++)
        key[1].k[i] = (bitmask & 1) ? 0 : g_ROM_Key_Table[magic];
    key[0].k[0] ^= magic2;
    key[1].k[0] ^= magic2;

#if 1
    for(int i = 0; i < magic3 + 57; i++)
        do_round(&key[0]);
    for(int i = 0; i < magic3 + 57; i++)
        do_round(&key[1]);
#endif
}

uint32_t xor_encrypt(union xorcrypt_key_t keys[2], void *_data, int size)
{
    if(size % 4)
        bugp("xor_encrypt: size is not a multiple of 4 !\n");
    size /= 4;
    uint32_t *data = _data;
    uint32_t final_xor = 0;
    for(int i = 0; i < size; i += 4)
    {
        uint32_t x = do_round(&keys[1]);
        /* xor first key's first word with data (at most 4 words of data) */
        for(int j = i; j < i + 4 && j < size; j++)
        {
            keys[0].k[0] ^= data[j];
            data[j] ^= x;
        }
        final_xor = do_round(&keys[0]);
    }
    return final_xor ^ do_round(&keys[1]);
}

uint32_t xor_decrypt(union xorcrypt_key_t keys[2], void *_data, int size)
{
    if(size % 4)
        bugp("xor_decrypt: size is not a multiple of 4 !\n");
    size /= 4;
    uint32_t *data = _data;
    uint32_t final_xor = 0;
    for(int i = 0; i < size; i += 4)
    {
        uint32_t x = do_round(&keys[1]);
        /* xor first key's first word with data (at most 4 words of data) */
        for(int j = i; j < i + 4 && j < size; j++)
        {
            data[j] ^= x;
            keys[0].k[0] ^= data[j];
        }
        final_xor = do_round(&keys[0]);
    }
    return final_xor ^ do_round(&keys[1]);
}