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

#include "firmware.h"
#include <iostream>
#include <stdexcept>


zen::firmware_entry::firmware_entry(bool big_endian)
    : _big_endian(big_endian)
{
}

zen::firmware_entry::firmware_entry(const firmware_entry& copy)
{
    assign(copy);
}

zen::firmware_entry& zen::firmware_entry::operator=(const firmware_entry& right)
{
    assign(right);
    return *this;
}


bool zen::firmware_entry::read(std::istream& is)
{
    // Read the header.
    is.read((char*)&_header, sizeof(firmware_header_t));
    if (!is.good())
        return false;

    // If the firmware is big-endian, swap the header values to little-endian.
    if (_big_endian)
    {
        _header.tag = shared::swap(_header.tag);
        if (_header.tag != 'NULL')
        {
            _header.size = shared::swap(_header.size);
        }
    }

    // Resize the bytes buffer to the size specified in the header.
    _bytes.resize(_header.size);

    // Read the entry contents.
    is.read(reinterpret_cast<char*>(&_bytes[0]),
            _header.size);

    return is.good();
}

bool zen::firmware_entry::write(std::ostream& os) const
{
    // Form a header using the current size of the bytes buffer.
    firmware_header_t header = {
        _header.tag,
        static_cast<dword>(_bytes.size())
    };

    // If the firmware is big-endian, swap the header values back into big-endian.
    if (_big_endian)
    {
        if (header.tag != 'NULL')
        {
            header.size = shared::swap(header.size);
        }
        header.tag = shared::swap(header.tag);
    }

    // Write the header.
    os.write((const char*)&header, sizeof(firmware_header_t));
    if (!os.good())
        return false;

    // Write the entry contents.
    os.write(reinterpret_cast<const char*>(&_bytes[0]),
             static_cast<std::streamsize>(_bytes.size()));

    return os.good();
}


bool zen::firmware_entry::is_big_endian() const
{
    return _big_endian;
}

const zen::firmware_header_t& zen::firmware_entry::get_header() const
{
    return _header;
}
zen::firmware_header_t& zen::firmware_entry::get_header()
{
    return _header;
}

const shared::bytes& zen::firmware_entry::get_bytes() const
{
    return _bytes;
}
shared::bytes& zen::firmware_entry::get_bytes()
{
    return _bytes;
}


std::string zen::firmware_entry::get_name() const
{
    char name[5];
    *(dword*)name = shared::swap(_header.tag);
    name[4] = '\0';

    // Determine if all characters in the tag are printable.
    bool isprintable = true;
    for (int i = 0; i < 4; i++)
    {
        if (!isprint((byte)name[i]))
        {
            isprintable = false;
            break;
        }
    }

    // If they are, simply return the tag as a string.
    if (isprintable)
    {
        return std::string(name);
    }

    // Otherwise, encode the tag into a hexadecimal string.
    char buffer[11];
    sprintf(buffer, "0x%08x", _header.tag);
    return std::string(buffer);
}

std::string zen::firmware_entry::get_content_name() const
{
    std::string name = get_name();
    if (name == "DATA")
    {
        name = "";
        int nameoff = is_big_endian() ? 1 : 0;
        for (int i = 0; i < 16; i++)
        {
            char c = get_bytes()[i * 2 + nameoff];
            if (!c)
                break;
            name += c;
        }
    }
    else if (name == "EXT0")
    {
        name = "";
        int nameoff = is_big_endian() ? 1 : 0;
        for (int i = 0; i < 12; i++)
        {
            char c = get_bytes()[i * 2 + nameoff];
            if (!c)
                break;
            name += c;
        }
    }
    return name;
}

size_t zen::firmware_entry::get_content_offset() const
{
    std::string name = get_name();
    if (name == "DATA")
    {
        return 32;
    }
    else if (name == "EXT0")
    {
        return 24;
    }
    return 0;
}

size_t zen::firmware_entry::calc_size() const
{
    return _bytes.size() + sizeof(firmware_header_t);
}


void zen::firmware_entry::assign(const firmware_entry& copy)
{
    _big_endian = copy._big_endian;
    _header.tag = copy._header.tag;
    _header.size = copy._header.size;
    _bytes.assign(copy._bytes.begin(), copy._bytes.end());
}



zen::firmware_archive::firmware_archive(bool big_endian)
    : _big_endian(big_endian)
{
}

zen::firmware_archive::firmware_archive(const firmware_archive& copy)
{
    assign(copy);
}

zen::firmware_archive& zen::firmware_archive::operator=(const firmware_archive& right)
{
    assign(right);
    return *this;
}


bool zen::firmware_archive::read(std::istream& is)
{
    // Read the root entry's header.
    firmware_header_t root;
    is.read((char*)&root, sizeof(firmware_header_t));
    if (!is.good())
        return false;

    if ((root.tag != 'CIFF') && (root.tag != 'FFIC'))
    {
        throw std::runtime_error("Invalid firmware archive format!");
    }

    _big_endian = root.tag == 'FFIC' ? true : false;
    if (_big_endian)
    {
        root.tag  = shared::swap(root.tag);
        root.size = shared::swap(root.size);
    }

    // Save the current stream position.
    std::istream::pos_type endpos = is.tellg();
    std::istream::pos_type curpos = endpos;
    endpos += std::istream::pos_type(root.size);

    // Read untill the end of the root entry contents.
    while (curpos < endpos)
    {
        firmware_entry entry(_big_endian);
        if (!entry.read(is))
            return false;

        _children.push_back(entry);
        curpos = is.tellg();
    }

    curpos = is.tellg();
    is.seekg(0, std::ios::end);
    endpos = is.tellg();
    is.seekg(curpos);

    // Read untill the end of the file.
    while (((size_t)curpos + sizeof(firmware_header_t)) < endpos)
    {
        firmware_entry entry(_big_endian);
        if (!entry.read(is))
            return false;

        _neighbours.push_back(entry);
        curpos = is.tellg();
    }

    return true;
}

bool zen::firmware_archive::write(std::ostream& os) const
{
    // Read the root entry's header.
    firmware_header_t root = {'CIFF', 0};

    // Calculate the total size of all the children entries.
    for (firmware_entries::const_iterator i = _children.begin();
         i != _children.end(); ++i)
    {
        root.size += i->calc_size();
    }

    // If the firmware is big-endian, swap the header values back into big-endian.
    if (_big_endian)
    {
        root.tag  = shared::swap(root.tag);
        root.size = shared::swap(root.size);
    }

    // Write the header.
    os.write((const char*)&root, sizeof(firmware_header_t));
    if (!os.good())
        return false;

    // Write all the child entries.
    for (firmware_entries::const_iterator i = _children.begin();
         i != _children.end(); ++i)
    {
        if (!i->write(os))
            return false;
    }

    // Write all the neighbour entries.
    for (firmware_entries::const_iterator i = _neighbours.begin();
         i != _neighbours.end(); ++i)
    {
        if (!i->write(os))
            return false;
    }

    return true;
}


bool zen::firmware_archive::is_big_endian() const
{
    return _big_endian;
}

const zen::firmware_entries& zen::firmware_archive::get_children() const
{
    return _children;
}
zen::firmware_entries& zen::firmware_archive::get_children()
{
    return _children;
}

const zen::firmware_entries& zen::firmware_archive::get_neighbours() const
{
    return _neighbours;
}
zen::firmware_entries& zen::firmware_archive::get_neighbours()
{
    return _neighbours;
}

bool zen::firmware_archive::is_signed() const
{
    for (firmware_entries::const_iterator i = _neighbours.begin();
         i != _neighbours.end(); i++)
    {
        if (i->get_name() == "NULL")
            return true;
    }
    return false;
}

size_t zen::firmware_archive::calc_size() const
{
    size_t size = sizeof(firmware_header_t);

    for (firmware_entries::const_iterator i = _children.begin();
         i != _children.end(); i++)
    {
        size += i->calc_size();
    }

    for (firmware_entries::const_iterator i = _neighbours.begin();
         i != _neighbours.end(); i++)
    {
        size += i->calc_size();
    }

    return size;
}


void zen::firmware_archive::assign(const firmware_archive& copy)
{
    _big_endian = copy._big_endian;
    _children.assign(copy._children.begin(), copy._children.end());
    _neighbours.assign(copy._neighbours.begin(), copy._neighbours.end());
}
