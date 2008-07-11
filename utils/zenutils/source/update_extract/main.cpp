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
#include <iomanip>
#include <ctime>
#include <getpot/getpot.hpp>
#include <file.h>
#include <updater.h>
#include <utils.h>


static const char VERSION[] = "0.1";

void print_version()
{
    std::cout
        << "update_extract - Extracts a Creative firmware from an updater"
           " executable." << std::endl
        << "Version " << VERSION << std::endl
        << "Copyright (c) 2007 Rasmus Ry" << std::endl;
}

void print_help()
{
    print_version();
    std::cout << std::endl
        << "Usage: update_extract [command] [options]" << std::endl
        << std::endl
        << " Commands:" << std::endl
        << "  -h,--help" << std::endl
        << "      prints this message." << std::endl
        << "  -u,--updater [file]" << std::endl
        << "      specifies the updater executable." << std::endl
        << std::endl
        << " Options:" << std::endl
        << "  -V,--verbose" << std::endl
        << "      prints verbose messages." << std::endl
        << "  -f,--firmware [file]" << std::endl
        << "      specifies the firmware arhive file name." << std::endl
        << "  -k,--key [key]" << std::endl
        << "      specifies the firmware archive key." << std::endl
        << "  -o,--offset [offset]" << std::endl
        << "      specifies the firmware archive offset in c-style"
           " hexadecimal." << std::endl
        << std::endl
        ;
}

std::string options_name(const std::string& name)
{
    return shared::replace_extension(name, ".opt");
}

std::string default_firmware_name(const std::string& name)
{
    return shared::replace_extension(name, "_rk.bin");
}

int process_arguments(int argc, char* argv[])
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

    std::string updatername;
    if (cl.search("-u") || cl.search("--updater"))
        updatername = cl.next("");
    if (updatername.empty())
    {
        std::cerr << "Updater executable must be specified." << std::endl;
        return 2;
    }

    std::string firmarename = default_firmware_name(updatername);
    if (cl.search("-f") || cl.search("--firmware"))
        firmarename = cl.next(firmarename.c_str());

    bool verbose = false;
    if (cl.search("-V") || cl.search("--verbose"))
        verbose = true;

    // Get or find the firmware archive key.
    std::string key;
    if (cl.search("-k") || cl.search("--key"))
        key = cl.next("");

    if (key.empty())
    {
        if (verbose)
            std::cout << "[*] Looking for firmware archive key..."
                      << std::endl;
        shared::bytes buffer;
        if (!shared::read_file(updatername, buffer))
        {
            std::cerr << "Failed to read the firmware updater executable."
                      << std::endl;
            return 3;
        }
        key = zen::find_firmware_key(&buffer[0], buffer.size());
        if (key.empty())
        {
            std::cerr << "Failed to find the firmware archive key."
                      << std::endl;
            return 4;
        }
    }

    // Get or find the firmware archive offset.
    std::string offset;
    dword offset_pa = 0;
    if (cl.search("-o") || cl.search("--ofset"))
        offset = cl.next("");

    if (offset.empty())
    {
        if (verbose)
            std::cout << "[*] Looking for firmware archive offset..."
                      << std::endl;

        dword offset_va = 0;
        if (!zen::find_firmware_archive(updatername, offset_va, offset_pa))
        {
            std::cerr << "Failed to find the firmware archive offset."
                      << std::endl;
            return 5;
        }
    }
    else
    {
        int offset_val;
        if (!sscanf(offset.c_str(), "0x%x", &offset_val))
        {
            if (!sscanf(offset.c_str(), "0x%X", &offset_val))
            {
                std::cerr << "\'" << offset
                    << "\' is not a valid c-style hexadecimal value."
                    << std::endl;
                return 6;
            }
        }
        offset_pa = static_cast<dword>(offset_val);
    }

    // Read firmware archive size.
    shared::bytes buffer;
    if (!shared::read_file(updatername, buffer, offset_pa, sizeof(dword)))
    {
        std::cerr << "Failed to read the firmware archive size." << std::endl;
        return 7;
    }
    dword archive_size = *(dword*)&buffer[0];

    if (verbose)
    {
        std::cout << "[*] Printing input variables..." << std::endl;
        std::cout << "    Updater executable:  " << updatername << std::endl;
        std::cout << "    Firmware archive:    " << firmarename << std::endl;
        std::cout << "      Key:               " << key << std::endl;
        std::cout << "      Offset:            "
                  << std::hex << std::showbase << std::setw(10)
                  << std::setfill('0') << std::internal
                  << offset_pa << std::endl;
        std::cout << "      Size:              "
                  << std::hex << std::showbase << std::setw(10)
                  << std::setfill('0') << std::internal
                  << archive_size << std::endl;
    }


    //--------------------------------------------------------------------
    // Extract the firmware archive from the updater.
    //--------------------------------------------------------------------

    if (verbose)
        std::cout << "[*] Reading firmware archive..." << std::endl;

    // Read the firmware archive.
    offset_pa += sizeof(dword);
    if (!shared::read_file(updatername, buffer, offset_pa, archive_size))
    {
        std::cerr << "Failed to read the firmware archive." << std::endl;
        return 8;
    }

    if (verbose)
        std::cout << "[*] Decrypting firmware archive..." << std::endl;

    // Decrypt the firmware archive.
    if (!zen::crypt_firmware(key.c_str(), &buffer[0], buffer.size()))
    {
        std::cerr << "Failed to decrypt the firmware archive." << std::endl;
        return 9;
    }

    if (verbose)
        std::cout << "[*] Decompressing firmware archive..." << std::endl;

    // Inflate the firmware archive to the output file.
    if (!shared::inflate_to_file(buffer, firmarename.c_str()))
    {
        std::cerr << "Failed to decompress the firmware archive." << std::endl;
        return 10;
    }


    //--------------------------------------------------------------------
    // Generate an options file for the extracted firmware archive.
    //--------------------------------------------------------------------

    // Get options filename for the given input file.
    std::string optionsname = options_name(updatername);

    if (verbose)
        std::cout << "[*] Producing options file..." << std::endl;

    // Produce options file for the given input file.
    std::ofstream ofs;
    ofs.open(optionsname.c_str(), std::ios::binary);
    if (!ofs)
    {
        std::cerr << "Failed to create firmware archive options file."
                  << std::endl;
        return 11;
    }

    time_t timeval = time(NULL);
    ofs << "# Options file generated at: " << ctime(&timeval)
        << "updater  = \'" << shared::double_quote(updatername) << "\'"
        << std::endl
        << "firmware = \'" << shared::double_quote(firmarename) << "\'"
        << std::endl
        << "offset   = " << (offset_pa - sizeof(dword)) << std::endl
        << "size     = " << archive_size << std::endl
        << "key      = \'" << key << "\'" << std::endl;

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
