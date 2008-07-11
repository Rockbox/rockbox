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

#ifndef ZEN_FIRMWARE_H_INCLUDED
#define ZEN_FIRMWARE_H_INCLUDED

#include <list>
#include <utils.h>

namespace zen {
    struct firmware_header_t
    {
        dword tag;
        dword size;
    }; //struct firmware_header_t

    class firmware_entry
    {
    public:
        firmware_entry(bool big_endian);
        firmware_entry(const firmware_entry& copy);
        firmware_entry& operator=(const firmware_entry& right);

        bool read(std::istream& is);
        bool write(std::ostream& os) const;

        bool is_big_endian() const;
        const firmware_header_t& get_header() const;
        firmware_header_t& get_header();
        const shared::bytes& get_bytes() const;
        shared::bytes& get_bytes();

        std::string get_name() const;
        std::string get_content_name() const;
        size_t get_content_offset() const;
        size_t calc_size() const;

    protected:
        void assign(const firmware_entry& copy);

    private:
        bool _big_endian;
        firmware_header_t _header;
        shared::bytes _bytes;
    }; //class firmware_entry

    typedef std::list<firmware_entry> firmware_entries;

    class firmware_archive
    {
    public:
        firmware_archive(bool big_endian);
        firmware_archive(const firmware_archive& copy);
        firmware_archive& operator=(const firmware_archive& right);

        bool read(std::istream& is);
        bool write(std::ostream& os) const;

        bool is_big_endian() const;
        const firmware_entries& get_children() const;
        firmware_entries& get_children();
        const firmware_entries& get_neighbours() const;
        firmware_entries& get_neighbours();
        bool is_signed() const;
        size_t calc_size() const;

    protected:
        void assign(const firmware_archive& copy);

    private:
        firmware_entries _children;
        firmware_entries _neighbours;
        bool _big_endian;
    }; //class firmware_archive
}; //namespace zen

#endif //ZEN_FIRMWARE_ARCHIVE_H_INCLUDED
