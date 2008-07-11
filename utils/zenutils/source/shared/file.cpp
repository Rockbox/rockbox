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

#include "file.h"
#include <fstream>


bool shared::read_file(const std::string& filename, bytes& buffer,
                       std::streampos offset, std::streamsize count)
{
    std::ifstream ifs;
    ifs.open(filename.c_str(), std::ios::binary);
    if (!ifs)
    {
        return false;
    }

    std::ifstream::pos_type startpos = offset;
    ifs.seekg(offset, std::ios::beg);
    if (count == -1)
        ifs.seekg(0, std::ios::end);
    else
        ifs.seekg(count, std::ios::cur);
    std::ifstream::pos_type endpos = ifs.tellg();

    buffer.resize(endpos-startpos);
    ifs.seekg(offset, std::ios::beg);

    ifs.read((char*)&buffer[0], endpos-startpos);

    ifs.close();
    return ifs.good();
}


bool shared::write_file(const std::string& filename, bytes& buffer,
                        bool truncate, std::streampos offset,
                        std::streamsize count)
{
    std::ios::openmode mode = std::ios::in|std::ios::out|std::ios::binary;
    if (truncate)
        mode |= std::ios::trunc;

    std::fstream ofs;
    ofs.open(filename.c_str(), mode);
    if (!ofs)
    {
        return false;
    }

    if (count == -1)
        count = buffer.size();
    else if (count > buffer.size())
        return false;

    ofs.seekg(offset, std::ios::beg);

    ofs.write((char*)&buffer[0], count);

    ofs.close();
    return ofs.good();
}

bool shared::file_exists(const std::string& filename)
{
    std::ifstream ifs;
    ifs.open(filename.c_str(), std::ios::in);
    if (ifs.is_open())
    {
        ifs.close();
        return true;
    }
    return false;
}

bool shared::copy_file(const std::string& srcname, const std::string& dstname)
{
    bytes buffer;
    if (!read_file(srcname, buffer))
        return false;
    return write_file(dstname, buffer, true);
}

bool shared::backup_file(const std::string& filename, bool force)
{
    std::string backupname = filename + ".bak";
    if (!force)
        if (file_exists(backupname))
            return true;
    return copy_file(filename, backupname);
}
