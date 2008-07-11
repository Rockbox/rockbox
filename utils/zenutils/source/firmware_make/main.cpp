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
#include <sstream>
#include <getpot/getpot.hpp>
#include <file.h>
#include <firmware.h>
#include <utils.h>


static const char VERSION[] = "0.1";

void print_version()
{
    std::cout
        << "firmware_make - Creates a Creative firmware archive." << std::endl
        << "Version " << VERSION << std::endl
        << "Copyright (c) 2007 Rasmus Ry" << std::endl;
}

void print_help()
{
    print_version();
    std::cout << std::endl
        << "Usage: firmware_make [command] [options]" << std::endl
        << std::endl
        << " Commands:" << std::endl
        << "  -h,--help" << std::endl
        << "      prints this message." << std::endl
        << "  -m,--makefile [file]" << std::endl
        << "      specifies the .mk file to build the firmware archive from."
        << std::endl << std::endl
        << " Options:" << std::endl
        << "  -V,--verbose" << std::endl
        << "      prints verbose messages." << std::endl
        << "  -f,--firmware [file]" << std::endl
        << "      specifies the output firmware file name" << std::endl
        << std::endl
        ;
}

dword get_tag_value(std::string tag)
{
    if (tag[0] == '0' && tag[1] == 'x')
    {
        dword val = 0;
        if (sscanf(tag.c_str(), "0x%08X", &val) == 1)
            return val;
        if (sscanf(tag.c_str(), "0x%08x", &val) == 1)
            return val;
    }
    else
    {
        return shared::swap(*(dword*)&tag[0]);
    }
    return 0;
}

bool process_child(const GetPot& mkfile, const std::string& root, int index,
                   zen::firmware_entry& entry)
{
    std::stringstream sstm;
    sstm << root << "/" << index;
    std::string var = sstm.str() + "/tag";
    std::string tag = mkfile(var.c_str(), "");
    var = sstm.str() + "/name";
    std::string name = mkfile(var.c_str(), "");
    var = sstm.str() + "/file";
    std::string file = mkfile(var.c_str(), "");

    if (file.empty() || tag.empty())
    {
        std::cerr << "Invalid file or tag for var: " << sstm.str()
                  << std::endl;
        return false;
    }

    shared::bytes buffer;
    if (!shared::read_file(file, buffer))
    {
        std::cerr << "Failed to read the file: " << file << std::endl;
        return false;
    }

    entry.get_bytes().clear();
    entry.get_header().tag = get_tag_value(tag);
    size_t contoff = entry.get_content_offset();
    if (contoff)
    {
        entry.get_bytes().resize(contoff, 0);
        if (!name.empty())
        {
            size_t endoff = entry.is_big_endian() ? 1 : 0;
            for (int i = 0; i < name.size(); ++i)
                entry.get_bytes()[i * 2 + endoff] = name[i];
        }
    }
    entry.get_bytes().insert(entry.get_bytes().end(), buffer.begin(),
                             buffer.end());

    entry.get_header().size = entry.get_bytes().size();

    return true;
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

    std::string makefile;
    if (cl.search("-m") || cl.search("--makefile"))
        makefile = cl.next("");
    if (makefile.empty())
    {
        std::cerr << "Makefile must be specified." << std::endl;
        return 2;
    }

    std::string firmware;
    if (cl.search("-f") || cl.search("--firmware"))
        firmware = cl.next("");
    if (firmware.empty())
    {
        std::cerr << "Firmware must be specified." << std::endl;
        return 3;
    }

    bool verbose = false;
    if (cl.search("-V") || cl.search("--verbose"))
        verbose = true;

    GetPot mkfile(makefile.c_str());
    if (verbose)
        mkfile.print();

    bool big_endian;
    std::string endian = mkfile("endian", "little");
    if (endian == "little")
    {
        big_endian = false;
    }
    else if (endian == "big")
    {
        big_endian = true;
    }
    else
    {
        std::cerr << "Invalid value of 'endian'" << std::endl;
        return 4;
    }

    zen::firmware_archive archive(big_endian);
    int childcount = mkfile("children/count", 0);
    if (!childcount)
    {
        std::cerr << "A firmware archive must have at least one child entry."
                  << std::endl;
        return 5;
    }

    for (int i = 0; i < childcount; i++)
    {
        zen::firmware_entry entry(big_endian);
        if (!process_child(mkfile, "children", i, entry))
        {
            return 6;
        }
        archive.get_children().push_back(entry);
    }

    int neighbourcount = mkfile("neighbours/count", 0);
    for (int i = 0; i < neighbourcount; i++)
    {
        zen::firmware_entry entry(big_endian);
        if (!process_child(mkfile, "neighbours", i, entry))
        {
            return 7;
        }
        archive.get_neighbours().push_back(entry);
    }

    std::ofstream ofs;
    ofs.open(firmware.c_str(), std::ios::out|std::ios::binary|std::ios::trunc);
    if (!ofs)
    {
        std::cerr << "Failed to create the firmware file." << std::endl;
        return 8;
    }

    if (!archive.write(ofs))
    {
        std::cerr << "Failed to save the firmware archive." << std::endl;
        return 9;
    }
    ofs.close();

    size_t length = archive.calc_size();
    if (!length)
    {
        std::cerr << "Failed to determine the size of the firmware archive."
                  << std::endl;
        return 10;
    }

    int align = length % 4;
    if (align)
    {
        shared::bytes padding(4 - align, 0);
        if (!shared::write_file(firmware, padding, false, length))
        {
            std::cerr << "Failed to write padding data." << std::endl;
            return 11;
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
