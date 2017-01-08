/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2016 Amaury Pouly
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
#ifndef __UPG_H__
#define __UPG_H__

#include "misc.h"
#include "fwp.h"
#include "mg.h"

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

/* API */

struct nwz_model_t
{
    const char *model; /* rockbox model codename */
    bool confirmed;
    /* If the KAS is confirmed, it is the one extracted from the device. Otherwise,
     * it is a KAS built from a key and sig brute-forced from an upgrade. In this
     * case, the KAS might be different from the 'official' one although for all
     * intent and purposes it should not make any difference. */
    char *kas;
};

/* list of models with keys and status. Sentinel NULL entry at the end */
extern struct nwz_model_t g_model_list[];

/* An entry in the UPG file */
struct upg_file_entry_t
{
    void *data;
    size_t size;
};

struct upg_file_t
{
    int nr_files;
    struct upg_file_entry_t *files;
};

/* decrypt a KAS into a key and signature, return <0 if the KAS contains a non-hex
 * character */
int decrypt_keysig(const char kas[NWZ_KAS_SIZE], char key[NWZ_KEY_SIZE],
    char sig[NWZ_SIG_SIZE]);
/* encrypt a key and signature into a KAS */
void encrypt_keysig(char kas[NWZ_KEY_SIZE],
    const char key[NWZ_SIG_SIZE], const char sig[NWZ_KAS_SIZE]);

/* Read a UPG file: return a structure on a success or NULL on error.
 * Note that the memory buffer is modified to perform in-place decryption. */
struct upg_file_t *upg_read_memory(void *file, size_t size, char key[NWZ_KEY_SIZE],
    char sig[NWZ_SIG_SIZE], void *u, generic_printf_t printf);
/* Write a UPG file: return a buffer containing the whole image, or NULL on error. */
void *upg_write_memory(struct upg_file_t *file, char key[NWZ_KEY_SIZE],
    char sig[NWZ_SIG_SIZE], size_t *out_size, void *u, generic_printf_t printf);
/* create empty upg file */
struct upg_file_t *upg_new(void);
/* append a file to a upg, data is NOT copied */
void upg_append(struct upg_file_t *file, void *data, size_t size);
/* release upg file, will free file data pointers */
void upg_free(struct upg_file_t *file);

#endif /* __UPG_H__ */
