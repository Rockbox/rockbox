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
#include <ctime>
#include <getpot/getpot.hpp>
#include <utils.h>
#include <firmware.h>


static const char VERSION[] = "0.1";


void print_version()
{
    std::cout
        << "firmware_extract - Extracts files from a Creative firmware."
        << std::endl
        << "Version " << VERSION << std::endl
        << "Copyright (c) 2007 Rasmus Ry" << std::endl;
}

void print_help()
{
    print_version();
    std::cout
        << "Usage: firmware_extract [command] [options]" << std::endl
        << std::endl
        << " Commands:" << std::endl
        << "  -h,--help" << std::endl
        << "      prints this message." << std::endl
        << "  -f,--firmware [file]" << std::endl
        << "      specifies the firmware arhive file name." << std::endl
        << std::endl
        << " Options:" << std::endl
        << "  -V,--verbose" << std::endl
        << "      prints verbose messages." << std::endl
        << "  -p,--prefix [prefix]" << std::endl
        << "      specifies a file name prefix for the extracted files." << std::endl
        << std::endl
        ;
}


struct save_entry_functor
{
    save_entry_functor(const std::string& fileprefix)
        : _fileprefix(fileprefix) {}

    bool operator()(const zen::firmware_entry& entry)
    {
        std::string filename = _fileprefix + entry.get_content_name();
        std::ofstream ofs;
        ofs.open(filename.c_str(), std::ios::binary);
        if (!ofs)
            false;

        size_t off = entry.get_content_offset();
        std::streamsize size = entry.get_bytes().size() - off;
        ofs.write((const char*)&entry.get_bytes()[off], size);

        return ofs.good();
    }

    const std::string& _fileprefix;
}; //struct save_entry_functor

struct print_entry_functor
{
    print_entry_functor(std::ostream& os, const std::string& fileprefix)
        : _os(os), _fileprefix(fileprefix), num(0) {}

    bool operator()(const zen::firmware_entry& entry)
    {
        std::string filename = _fileprefix + entry.get_content_name();
        if (!num)
            _os << "[./" << num++ << "]" << std::endl;
        else
            _os << "[../" << num++ << "]" << std::endl;
        _os << "tag = " << entry.get_name() << std::endl;

        if (entry.get_content_offset())
            _os << "name = " << entry.get_content_name() << std::endl;

        _os << "file = \'" << shared::double_quote(filename) << "\'"
            << std::endl;

        return _os.good();
    }

    std::ostream& _os;
    const std::string& _fileprefix;
    int num;
}; //struct print_entry_functor


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

    std::string firmwarename;
    if (cl.search("-f") || cl.search("--firmware"))
        firmwarename = cl.next("");
    if (firmwarename.empty())
    {
        std::cerr << "Firmware archive must be specified." << std::endl;
        return 2;
    }

    bool verbose = false;
    if (cl.search("-V") || cl.search("--verbose"))
        verbose = true;

    std::string prefixname = shared::remove_extension(firmwarename) + "_";
    if (cl.search("-p") || cl.search("--prefix"))
        prefixname = cl.next(prefixname.c_str());


    //--------------------------------------------------------------------
    // Read the firmware archive.
    //--------------------------------------------------------------------

    if (verbose)
        std::cout << "[*] Reading firmware archive..." << std::endl;

    zen::firmware_archive archive(false);
    std::ifstream ifs;
    ifs.open(firmwarename.c_str(), std::ios::binary);
    if (!ifs)
    {
        std::cerr << "Failed to open the firmware archive." << std::endl;
        return 3;
    }

    if (!archive.read(ifs))
    {
        std::cerr << "Failed to read the firmware archive." << std::endl;
        return 4;
    }


    //--------------------------------------------------------------------
    // Generate a make file for the extracted firmware archive.
    //--------------------------------------------------------------------

    // Get make filename for the given input file.
    std::string makefile = shared::replace_extension(firmwarename, ".mk");

    if (verbose)
        std::cout << "[*] Producing make file..." << std::endl;


    // Produce make file for the given input file.
    std::ofstream ofs;
    ofs.open(makefile.c_str(), std::ios::binary);
    if (!ofs)
    {
        std::cerr << "Failed to create firmware archive make file."
                  << std::endl;
        return 5;
    }

    time_t timeval = time(NULL);
    ofs << "# Make file generated at: " << ctime(&timeval);
    ofs << "endian = " << (archive.is_big_endian() ? "big" : "little")
        << std::endl;
    ofs << "signed = " << (archive.is_signed() ? "true" : "false")
        << std::endl;

    ofs << "[children]" << std::endl;
    ofs << "count = " << archive.get_children().size() << std::endl;

    std::for_each(archive.get_children().begin(),
                  archive.get_children().end(),
                  print_entry_functor(ofs, prefixname));

    ofs << "[neighbours]" << std::endl;
    ofs << "count = " << archive.get_neighbours().size() << std::endl;
    std::for_each(archive.get_neighbours().begin(),
                  archive.get_neighbours().end(),
                  print_entry_functor(ofs, prefixname));


    //--------------------------------------------------------------------
    // Save firmware entries.
    //--------------------------------------------------------------------

    if (verbose)
        std::cout << "[*] Saving firmware entries..." << std::endl;

    std::for_each(archive.get_children().begin(),
                  archive.get_children().end(),
                  save_entry_functor(prefixname));

    std::for_each(archive.get_neighbours().begin(),
                  archive.get_neighbours().end(),
                  save_entry_functor(prefixname));

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
