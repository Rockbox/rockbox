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

#include <iostream>
#include <getpot/getpot.hpp>
#include <cenc.h>
#include <crypt.h>
#include <file.h>
#include <firmware.h>
#include <utils.h>


namespace {
enum command_t
{
    cmd_none = 0,
    cmd_sign,
    cmd_verify,
    cmd_encrypt,
    cmd_decrypt
};

enum mode_t
{
    mode_none = 0,
    mode_cenc,
    mode_fresc,
    mode_tl
};

struct player_info_t
{
    const char* name;
    const char* null_key;  // HMAC-SHA1 key
    const char* fresc_key; // BlowFish key
    const char* tl_key;    // BlowFish key
    bool big_endian;
};
}; //namespace


static const char VERSION[] = "0.1";

static const char null_key_v1[]  = "CTL:N0MAD|PDE0.SIGN.";
static const char null_key_v2[]  = "CTL:N0MAD|PDE0.DPMP.";

static const char fresc_key[]    = "Copyright (C) CTL. -"
                                   " zN0MAD iz v~p0wderful!";

static const char tl_zvm_key[]   = "1sN0TM3D az u~may th1nk*"
                                   "Creative Zen Vision:M";
static const char tl_zvw_key[]   = "1sN0TM3D az u~may th1nk*"
                                   "Creative ZEN Vision W";
static const char tl_zm_key[]    = "1sN0TM3D az u~may th1nk*"
                                   "Creative Zen Micro";
static const char tl_zmp_key[]   = "1sN0TM3D az u~may th1nk*"
                                   "Creative Zen MicroPhoto";
static const char tl_zs_key[]    = "1sN0TM3D az u~may th1nk*"
                                   "Creative Zen Sleek";
static const char tl_zsp_key[]   = "1sN0TM3D az u~may th1nk*"
                                   "Creative Zen Sleek Photo";
static const char tl_zt_key[]    = "1sN0TM3D az u~may th1nk*"
                                   "Creative Zen Touch";
static const char tl_zx_key[]    = "1sN0TM3D az u~may th1nk*"
                                   "NOMAD Jukebox Zen Xtra";

player_info_t players[] = {
    {"Vision:M", null_key_v2, fresc_key, tl_zvm_key, false},
    {"Vision W", null_key_v2, fresc_key, tl_zvw_key, false},
    {"Micro", null_key_v1, fresc_key, tl_zm_key, true},
    {"MicroPhoto", null_key_v1, fresc_key, tl_zmp_key, true},
    {"Sleek", null_key_v1, fresc_key, tl_zs_key, true},
    {"SleekPhoto", null_key_v1, fresc_key, tl_zsp_key, true},
    {"Touch", null_key_v1, fresc_key, tl_zt_key, true},
    {"Xtra", null_key_v1, fresc_key, tl_zx_key, true},
    {NULL, NULL, NULL, NULL, false}
};


player_info_t* find_player_info(std::string player)
{
    for (int i = 0; players[i].name != NULL; i++)
    {
        if (!stricmp(players[i].name, player.c_str()))
        {
            return &players[i];
        }
    }
    return NULL;
}

void print_version()
{
    std::cout
        << "zen_crypt - A utility for encrypting, decrypting or signing"
           " Creative firmwares." << std::endl
        << "Version " << VERSION << std::endl
        << "Copyright (c) 2007 Rasmus Ry" << std::endl;
}

void print_help()
{
    print_version();
    std::cout << std::endl
        << "Usage: zen_crypt [command] [options]" << std::endl
        << std::endl
        << " Commands:" << std::endl
        << "  -h,--help" << std::endl
        << "      prints this message." << std::endl
        << "  -s,--sign" << std::endl
        << "      signs a given input file." << std::endl
        << "  -v,--verify" << std::endl
        << "      verifies a signed input file." << std::endl
        << "  -e,--encrypt" << std::endl
        << "      encrypts a given input file." << std::endl
        << "  -d,--decrypt" << std::endl
        << "      decrypts a given input file." << std::endl
        << std::endl
        << " Options:" << std::endl
        << "  -V,--verbose" << std::endl
        << "      prints verbose messages." << std::endl
        << "  -b,--big-endian" << std::endl
        << "      specifies that the input is big-endian, default is"
           " little-endian." << std::endl
        << "  -i,--input [file]" << std::endl
        << "      specifies the input file." << std::endl
        << "  -o,--output [file]" << std::endl
        << "      specifies the output file." << std::endl
        << "  -m,--mode [CENC|FRESC|TL]" << std::endl
        << "      specifies which algorithm to use." << std::endl
        << "  -k,--key [player|key]" << std::endl
        << "      specifies which key to use." << std::endl
        << std::endl
        ;
    std::cout << " Players:" << std::endl;
    for (int i = 0; players[i].name != NULL; i++)
    {
        std::cout << "      " << players[i].name;
        if (!i)
            std::cout << " (default)";
        std::cout << std::endl;
    }
}

size_t find_null_signature(shared::bytes& data)
{
    size_t index = data.size();
    if (index < (20 + 8 + 7))
        return 0;
    index -= 20 + 8;
    for (int i = 0; i < 7; i++)
    {
        if (*(dword*)&data[index-i] == 'NULL' ||
            *(dword*)&data[index-i] == 'LLUN')
        {
            return index-i;
        }
    }
    return 0;
}


bool sign(shared::bytes& data, player_info_t* pi, const std::string& file,
          bool verbose)
{
    if (verbose)
        std::cout << "[*] Checking for the presence of an existing"
                     " NULL signature..." << std::endl;
    size_t index = find_null_signature(data);
    if (index)
    {
        if (verbose)
            std::cout << "[*] Found NULL signature at: "
                      << std::hex << index << std::endl;

        if (verbose)
            std::cout << "[*] Computing digest..." << std::endl;

        shared::bytes digest(20);
        if (!zen::hmac_sha1_calc((const byte*)pi->null_key,
                                 strlen(pi->null_key)+1, &data[0], index,
                                 &digest[0], NULL))
        {
            std::cerr << "Failed to compute digest." << std::endl;
            return false;
        }

        if (verbose)
            std::cout << "[*] Writing file data..." << std::endl;

        if (!shared::write_file(file, data, true))
        {
            std::cerr << "Failed to write file data." << std::endl;
            return false;
        }

        if (verbose)
            std::cout << "[*] Writing digest data..." << std::endl;

        if (!shared::write_file(file, digest, false, index+8))
        {
            std::cerr << "Failed to write digest data." << std::endl;
            return false;
        }
    }
    else
    {
        if (verbose)
            std::cout << "[*] Computing digest..." << std::endl;

        shared::bytes signature(20+8);
        if (!zen::hmac_sha1_calc((const byte*)pi->null_key,
                                 strlen(pi->null_key)+1, &data[0], data.size(),
                                 &signature[8], NULL))
        {
            std::cerr << "Failed to compute digest." << std::endl;
            return false;
        }


        zen::firmware_header_t header = {'NULL', 20};
        if (pi->big_endian)
        {
            header.tag = shared::swap(header.tag);
            header.size = shared::swap(header.size);
        }
        memcpy(&signature[0], &header, sizeof(zen::firmware_header_t));

        if (verbose)
            std::cout << "[*] Writing file data..." << std::endl;

        if (!shared::write_file(file, data, true))
        {
            std::cerr << "Failed to write file data." << std::endl;
            return false;
        }

        if (verbose)
            std::cout << "[*] Writing signature data..." << std::endl;

        if (!shared::write_file(file, signature, false, data.size()))
        {
            std::cerr << "Failed to write signature data." << std::endl;
            return false;
        }

        if (verbose)
            std::cout << "[*] Ensuring that the file length is"
                         " 32-bit aligned..." << std::endl;

        int length = data.size() + signature.size();
        int align = length % 4;
        if (align)
        {
            shared::bytes padding(4 - align, 0);
            if (!shared::write_file(file, padding, false, length))
            {
                std::cerr << "Failed to write padding data." << std::endl;
                return false;
            }
        }
    }

    return true;
}

bool verify(shared::bytes& data, player_info_t* pi, bool verbose)
{
    if (verbose)
        std::cout << "[*] Checking for the presence of an existing"
                     " NULL signature..." << std::endl;
    size_t index = find_null_signature(data);
    if (!index)
    {
        std::cerr << "No NULL signature present in the input file."
                  << std::endl;
        return false;
    }
    if (verbose)
        std::cout << "[*] Found NULL signature at: "
                  << std::hex << index << std::endl;

    if (verbose)
        std::cout << "[*] Computing digest..." << std::endl;

    byte digest[20];
    if (!zen::hmac_sha1_calc((const byte*)pi->null_key, strlen(pi->null_key)+1,
                             &data[0], index, digest, NULL))
    {
        std::cerr << "Failed to compute digest." << std::endl;
        return false;
    }

    if (verbose)
        std::cout << "[*] Verifying NULL signature digest..." << std::endl;

    if (memcmp(&digest[0], &data[index+8], 20))
    {
        std::cerr << "The NULL signature contains an incorrect digest."
                  << std::endl;
        return false;
    }

    return true;
}

bool encrypt(shared::bytes& data, int mode, player_info_t* pi,
             const std::string& file, bool verbose)
{
    if (mode == mode_cenc)
    {
        if (verbose)
            std::cout << "[*] Encoding input file..." << std::endl;

        shared::bytes outbuf(data.size() * 2);
        int len = zen::cenc_encode(&data[0], data.size(), &outbuf[0], outbuf.size());
        if (!len)
        {
            std::cerr << "Failed to encode the input file." << std::endl;
            return false;
        }

        if (verbose)
            std::cout << "[*] Writing decoded length to file..." << std::endl;

        shared::bytes length(sizeof(dword));
        *(dword*)&length[0] = pi->big_endian ? shared::swap(data.size()) : data.size();
        if (!shared::write_file(file, length, true))
        {
            std::cerr << "Failed to write the file data." << std::endl;
            return false;
        }

        if (verbose)
            std::cout << "[*] Writing file data..." << std::endl;

        if (!shared::write_file(file, outbuf, sizeof(dword), len))
        {
            std::cerr << "Failed to write the file data." << std::endl;
            return false;
        }
    }
    else if (mode == mode_fresc)
    {
        std::cerr << "FRESC mode is not supported." << std::endl;
        return false;
    }
    else if (mode == mode_tl)
    {
        if (verbose)
            std::cout << "[*] Encoding input file..." << std::endl;

        shared::bytes outbuf(data.size() * 2);
        *(dword*)&outbuf[0] = pi->big_endian ? shared::swap(data.size()) : data.size();
        int len = zen::cenc_encode(&data[0], data.size(),
                                  &outbuf[sizeof(dword)],
                                  outbuf.size()-sizeof(dword));
        if (!len)
        {
            std::cerr << "Failed to encode the input file." << std::endl;
            return false;
        }
        len += sizeof(dword);

        int align = len % 8;
        align = align ? (8 - align) : 0;
        len += align;

        if (verbose)
            std::cout << "[*] Encrypting encoded data..." << std::endl;

        dword iv[2] = {0, shared::swap(len)};
        if (!zen::bf_cbc_encrypt((const byte*)pi->tl_key, strlen(pi->tl_key)+1,
                            &outbuf[0], len, (const byte*)iv))
        {
            std::cerr << "Failed to decrypt the input file." << std::endl;
            return false;
        }

        if (verbose)
            std::cout << "[*] Writing file data..." << std::endl;

        if (!shared::write_file(file, outbuf, true, 0, len))
        {
            std::cerr << "Failed to save the output file." << std::endl;
            return false;
        }
    }
    else
    {
        std::cerr << "Invalid mode specified." << std::endl;
        return false;
    }

    return true;
}

bool decrypt(shared::bytes& data, int mode, player_info_t* pi,
             const std::string& file, bool verbose)
{
    if (mode == mode_cenc)
    {
        dword length = *(dword*)&data[0];
        length = pi->big_endian ? shared::swap(length) : length;

        if (verbose)
            std::cout << "[*] Decoding input file..." << std::endl;

        shared::bytes outbuf(length);
        if (!zen::cenc_decode(&data[sizeof(dword)], data.size()-sizeof(dword),
                              &outbuf[0], length))
        {
            std::cerr << "Failed to decode the input file." << std::endl;
            return false;
        }

        if (verbose)
            std::cout << "[*] Writing file data..." << std::endl;

        if (!shared::write_file(file, outbuf, true))
        {
            std::cerr << "Failed to write the file data." << std::endl;
            return false;
        }
    }
    else if (mode == mode_fresc)
    {
        if (verbose)
            std::cout << "[*] Decrypting input file..." << std::endl;

        dword iv[2] = {shared::swap(data.size()), 0};
        if (!zen::bf_cbc_decrypt((const byte*)pi->fresc_key,
                                 strlen(pi->fresc_key)+1, &data[0],
                                 data.size(), (const byte*)iv))
        {
            std::cerr << "Failed to decrypt the input file." << std::endl;
            return false;
        }

        if (verbose)
            std::cout << "[*] Writing file data..." << std::endl;

        if (!shared::write_file(file, data, true))
        {
            std::cerr << "Failed to save the output file." << std::endl;
            return false;
        }
    }
    else if (mode == mode_tl)
    {
        if (verbose)
            std::cout << "[*] Decrypting input file..." << std::endl;

        dword iv[2] = {0, shared::swap(data.size())};
        if (!zen::bf_cbc_decrypt((const byte*)pi->tl_key, strlen(pi->tl_key)+1,
                            &data[0], data.size(), (const byte*)iv))
        {
            std::cerr << "Failed to decrypt the input file." << std::endl;
            return false;
        }

        dword length = *(dword*)&data[0];
        length = pi->big_endian ? shared::swap(length) : length;
        if (length > (data.size() * 3))
        {
            std::cerr << "Decrypted length is unexpectedly large: "
                      << std::hex << length
                      << " Check the endian and key settings." << std::endl;
            return false;
        }

        if (verbose)
            std::cout << "[*] Decoding decrypted data..." << std::endl;

        shared::bytes outbuf(length);
        if (!zen::cenc_decode(&data[sizeof(dword)], data.size()-sizeof(dword),
                              &outbuf[0], length))
        {
            std::cerr << "Failed to decode the input file." << std::endl;
            return false;
        }

        if (verbose)
            std::cout << "[*] Writing file data..." << std::endl;

        if (!shared::write_file(file, outbuf, true))
        {
            std::cerr << "Failed to save the output file." << std::endl;
            return false;
        }
    }
    else
    {
        std::cerr << "Invalid mode specified." << std::endl;
        return false;
    }

    return true;
}

int process_arguments(int argc, char*argv[])
{
    //--------------------------------------------------------------------
    // Parse input variables.
    //--------------------------------------------------------------------

    GetPot cl(argc, argv);
    if (cl.size() == 1 || cl.search(2, "-h", "--help"))
    {
        print_help();
        return 1;
    }

    int command = cmd_none;
    if (cl.search(2, "-s", "--sign"))
        command = cmd_sign;
    else if (cl.search(2, "-v", "--verify"))
        command = cmd_verify;
    else if (cl.search(2, "-e", "--encrypt"))
        command = cmd_encrypt;
    else if (cl.search(2, "-d", "--decrypt"))
        command = cmd_decrypt;

    if (command == cmd_none)
    {
        std::cerr << "No command specified." << std::endl;
        return 2;
    }

    int mode = mode_none;
    if (command == cmd_encrypt || command == cmd_decrypt)
    {
        if (!cl.search(2, "-m", "--mode"))
        {
            std::cerr << "The specified command requires that"
                         " a mode is specified."
                      << std::endl;
            return 3;
        }
        std::string name = cl.next("");
        if (!name.empty())
        {
            if (!stricmp(name.c_str(), "CENC"))
                mode = mode_cenc;
            else if (!stricmp(name.c_str(), "FRESC"))
                mode = mode_fresc;
            else if (!stricmp(name.c_str(), "TL"))
                mode = mode_tl;
        }
        if (mode == mode_none)
        {
            std::cerr << "Invalid mode specified." << std::endl;
            return 4;
        }
    }

    bool verbose = false;
    if (cl.search(2, "-V", "--verbose"))
        verbose = true;

    bool big_endian = false;
    if (cl.search(2, "-b", "--big-endian"))
        big_endian = true;

    std::string infile;
    if (cl.search(2, "-i", "--input"))
        infile = cl.next("");
    if (infile.empty())
    {
        std::cerr << "An input file must be specified." << std::endl;
        return 5;
    }

    std::string outfile = infile;
    if (cl.search(2, "-o", "--output"))
        outfile = cl.next(outfile.c_str());

    player_info_t* pi = &players[0];
    std::string key;
    if (cl.search(2, "-k", "--key"))
        key = cl.next("");
    if (!key.empty())
    {
        player_info_t* pitmp = find_player_info(key);
        if (pitmp != NULL)
            pi = pitmp;
        else
        {
            static player_info_t player = {
                NULL, key.c_str(), key.c_str(), key.c_str(), false
            };
            pi = &player;
        }
    }
    if (big_endian)
        pi->big_endian = big_endian;


    //--------------------------------------------------------------------
    // Read the input file.
    //--------------------------------------------------------------------

    if (verbose)
        std::cout << "[*] Reading input file..." << std::endl;

    shared::bytes buffer;
    if (!shared::read_file(infile, buffer))
    {
        std::cerr << "Failed to read the input file." << std::endl;
        return 6;
    }


    //--------------------------------------------------------------------
    // Process the input file.
    //--------------------------------------------------------------------

    switch (command)
    {
    case cmd_sign:
        if (verbose)
            std::cout << "[*] Signing input file..." << std::endl;
        if (!sign(buffer, pi, outfile, verbose))
            return 7;
        std::cout << "Successfully signed the input file." << std::endl;
        break;
    case cmd_verify:
        if (verbose)
            std::cout << "[*] Verifying signature on input file..."
                      << std::endl;
        if (!verify(buffer, pi, verbose))
            return 8;
        std::cout << "Successfully verified the input file signature."
                  << std::endl;
        break;
    case cmd_encrypt:
        if (verbose)
            std::cout << "[*] Encrypting input file..." << std::endl;
        if (!encrypt(buffer, mode, pi, outfile, verbose))
            return 9;
        std::cout << "Successfully encrypted the input file." << std::endl;
        break;
    case cmd_decrypt:
        if (verbose)
            std::cout << "[*] Decrypting input file..." << std::endl;
        if (!decrypt(buffer, mode, pi, outfile, verbose))
            return 10;
        std::cout << "Successfully decrypted the input file." << std::endl;
        break;
    };

    return 0;
}

int main(int argc, char* argv[])
{
    try
    {
        return process_arguments(argc, argv);
    }
    catch (const std::exception& xcpt)
    {
        std::cerr << "Exception caught: " << xcpt.what() << std::endl;
        return -1;
    }
    catch (...)
    {
        std::cerr << "Unknown exception caught." << std::endl;
        return -2;
    }
    return -3;
}
