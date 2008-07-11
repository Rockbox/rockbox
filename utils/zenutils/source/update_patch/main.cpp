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
#include <cstdio>
#include <getpot/getpot.hpp>
#include <file.h>
#include <updater.h>
#include <utils.h>
#include <pe.h>


static const char VERSION[] = "0.1";

void print_version()
{
    std::cout
        << "update_patch - Patches a Creative firmware into an updater"
           " executable." << std::endl
        << "Version " << VERSION << std::endl
        << "Copyright (c) 2007 Rasmus Ry" << std::endl;
}

void print_help()
{
    print_version();
    std::cout << std::endl
        << "Usage: update_patch [command] [options]" << std::endl
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

    bool verbose = false;
    if (cl.search("-V") || cl.search("--verbose"))
        verbose = true;

    if (verbose)
        std::cout << "[*] Parsing options file..." << std::endl;

    GetPot optfile(options_name(updatername.c_str()).c_str());
    if (verbose)
        optfile.print();

    std::string firmwarename = optfile("firmware",
        default_firmware_name(updatername).c_str());
    dword offset_pa = optfile("offset", 0);
    dword size = optfile("size", 0);
    std::string key = optfile("key", "");

    if (cl.search("-f") || cl.search("--firmware"))
        firmwarename = cl.next(firmwarename.c_str());

    std::string offset;
    if (cl.search("-o") || cl.search("--offset"))
        offset = cl.next("");

    if (offset.empty() && !offset_pa)
    {
        if (verbose)
            std::cout << "[*] Looking for firmware archive offset..."
                      << std::endl;

        dword offset_va = 0;
        if (!zen::find_firmware_archive(updatername, offset_va, offset_pa))
        {
            std::cerr << "Failed to find the firmware archive offset."
                      << std::endl;
            return 3;
        }
    }
    else if (!offset_pa)
    {
        int offset_val;
        if (!sscanf(offset.c_str(), "0x%x", &offset_val))
        {
            if (!sscanf(offset.c_str(), "0x%X", &offset_val))
            {
                std::cerr << "\'" << offset
                    << "\' is not a valid c-style hexadecimal value."
                    << std::endl;
                return 4;
            }
        }
        offset_pa = static_cast<dword>(offset_val);
    }

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
            return 5;
        }
        key = zen::find_firmware_key(&buffer[0], buffer.size());
        if (key.empty())
        {
            std::cerr << "Failed to find the firmware archive key."
                      << std::endl;
            return 6;
        }
    }

    if (verbose)
    {
        std::cout << "[*] Printing input variables..." << std::endl;
        std::cout << "    Updater executable:  " << updatername << std::endl;
        std::cout << "    Firmware archive:    " << firmwarename << std::endl;
        std::cout << "      Key:               " << key << std::endl;
        std::cout << "      Offset:            "
                  << std::hex << std::showbase << std::setw(10)
                  << std::setfill('0') << std::internal
                  << offset_pa << std::endl;
        std::cout << "      Size:              "
                  << std::hex << std::showbase << std::setw(10)
                  << std::setfill('0') << std::internal
                  << size << std::endl;
    }


    //--------------------------------------------------------------------
    // Prepare the firmware archive for being patched into the updater.
    //--------------------------------------------------------------------

    if (verbose)
        std::cout << "[*] Reading firmware archive..." << std::endl;

    shared::bytes buffer;
    if (!shared::read_file(firmwarename, buffer))
    {
        std::cerr << "Failed to read the firmware archive." << std::endl;
        return 7;
    }

    if (verbose)
        std::cout << "    Bytes read:          "
                  << std::hex << std::showbase << std::setw(10)
                  << std::setfill('0') << std::internal
                  << buffer.size() << std::endl;

    if (verbose)
        std::cout << "[*] Compressing firmware archive..." << std::endl;

    std::string compfirmware = shared::replace_extension(firmwarename, ".def");
    if (!shared::deflate_to_file(buffer, compfirmware.c_str()))
    {
        std::cerr << "Failed to compress the firmware archive." << std::endl;
        return 8;
    }

    if (verbose)
        std::cout << "[*] Reading compressed firmware archive..." << std::endl;

    if (!shared::read_file(compfirmware, buffer))
    {
        std::cerr << "Failed to read the compressed firmware archive."
                  << std::endl;
        return 9;
    }

    if (verbose)
        std::cout << "    Bytes read:          "
                  << std::hex << std::showbase << std::setw(10)
                  << std::setfill('0') << std::internal
                  << buffer.size() << std::endl;

    // Delete the temporary firmware file.
    std::remove(compfirmware.c_str());

    if (verbose)
        std::cout << "[*] Encrypting compressed firmware archive..."
                  << std::endl;

    if (!zen::crypt_firmware(key.c_str(), &buffer[0], buffer.size()))
    {
        std::cerr << "Failed to encrypt the compressed firmware archive."
                  << std::endl;
        return 10;
    }


    //--------------------------------------------------------------------
    // Backup the updater and patch the firmware archive into it.
    //--------------------------------------------------------------------

    if (verbose)
        std::cout << "[*] Backing up the updater executable..." << std::endl;

    if (!shared::backup_file(updatername))
    {
        std::cerr << "Failed to backup the updater executable." << std::endl;
        return 11;
    }

    // Is there enough space within the existing firmware archive
    //  to hold the new one?
    if (size < buffer.size())
    {
        // No, we need to add a new section to hold the new firmware archive.
        if (verbose)
            std::cout << "[*] Adding new section to the updater executable..."
                      << std::endl;

        // Construct a new buffer with the archive size prepended.
        shared::bytes newbuffer(buffer.size() + sizeof(dword));
        *(dword*)&newbuffer[0] = static_cast<dword>(buffer.size());
        std::copy(buffer.begin(), buffer.end(), &newbuffer[4]);

        // Read the updater portable executable.
        shared::pe_file pef;
        if (!pef.read(updatername))
        {
            std::cerr << "Failed to read the updater portable executable"
                         " structure." << std::endl;
            return 12;
        }

        // Add a new section to the updater, containing the encrypted
        //  firmware archive.
        shared::section_info newsection;
        if (!pef.add_section(".firm", newbuffer, newsection))
        {
            std::cerr << "Failed to add an extra section to the updater"
                         " executable." << std::endl;
            return 13;
        }

        if (verbose)
            std::cout << "[*] Relocating code references to the firmware"
                         " archive..." << std::endl;

        // Locate the code section.
        shared::section_info textsection;
        if (!pef.find_section(".text", textsection))
        {
            std::cerr << "Failed to find the code section in the updater"
                         " executable." << std::endl;
            return 14;
        }

        // Read the code section data.
        if (!shared::read_file(updatername, buffer, textsection.raw_address,
                               textsection.raw_size))
        {
            std::cerr << "Failed to read the code section from the updater"
                         " executable." << std::endl;
            return 15;
        }

        // Determine the addresses of the new and old firmware archives.
        dword oldva = pef.pa_to_va(offset_pa);
        dword newva = pef.pa_to_va(newsection.raw_address);
        if (!oldva || !newva)
        {
            std::cerr << "Failed to compute address of the new or old"
                         " archive." << std::endl;
            return 16;
        }

        // Relocate references to the old firmware archive.
        dword imgbase = pef.get_image_base();
        for (int i = 0, j = buffer.size() - sizeof(dword) + 1; i < j; i++)
        {
            dword val = *(dword*)&buffer[i];
            if (val >= oldva && val <= (oldva + 3))
            {
                *(dword*)&buffer[i] = newva + (val - oldva);
                if (verbose)
                    std::cout << "    "
                              << std::hex << std::showbase << std::setw(10)
                              << std::setfill('0') << std::internal
                              << (imgbase + textsection.virtual_address + i)
                              << ": "
                              << std::hex << std::showbase << std::setw(10)
                              << std::setfill('0') << std::internal
                              << val
                              << " -> "
                              << std::hex << std::showbase << std::setw(10)
                              << std::setfill('0') << std::internal
                              << (newva + (val - oldva)) << std::endl;
            }
        }

        // Write the relocated code section data.
        if (!shared::write_file(updatername, buffer, false, textsection.raw_address,
                                buffer.size()))
        {
            std::cerr << "Failed to write the relocated code section to the"
                         " updater executable." << std::endl;
            return 17;
        }
    } //if (size < buffer.size())
    else
    {
        // Yes, overwrite the existing firmware archive.
        if (verbose)
            std::cout << "[*] Overwriting existing firmware archive..."
                      << std::endl;

        shared::bytes archive_size(sizeof(dword));
        *(dword*)&archive_size[0] = buffer.size();

        if (!shared::write_file(updatername, archive_size, false, offset_pa,
                                archive_size.size()))
        {
            std::cerr << "Failed to write archive size to the updater"
                         " executable." << std::endl;
            return 18;
        }

        if (!shared::write_file(updatername, buffer, false,
                                offset_pa+archive_size.size(), buffer.size()))
        {
            std::cerr << "Failed to write the new archive to the updater"
                         " exectuable." << std::endl;
            return 19;
        }
    }

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
