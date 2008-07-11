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

#ifndef SHARED_UTILS_H_INCLUDED
#define SHARED_UTILS_H_INCLUDED

#include <vector>
#include <pelib/PeLib.h>

#ifndef byte
typedef PeLib::byte byte;
#endif
#ifndef word
typedef PeLib::word word;
#endif
#ifndef dword
typedef PeLib::dword dword;
#endif

namespace shared {
    typedef std::vector<byte> bytes;

    inline dword swap(dword val)
    {
        return ((val & 0xFF) << 24)
               | ((val & 0xFF00) << 8)
               | ((val & 0xFF0000) >> 8)
               | ((val & 0xFF000000) >> 24);
    }

    template <typename _Type>
    inline void reverse(_Type* start, _Type* end)
    {
        while (start < end)
        {
            *start ^= *end;
            *end ^= *start;
            *start ^= *end;
            start++;
            end--;
        }
    }

    std::string replace_extension(const std::string& filename, const std::string& extension);
    std::string remove_extension(const std::string& filename);
    std::string get_path(const std::string& filename);
    std::string double_quote(const std::string& str);

    bool inflate_to_file(const bytes& buffer, const char* filename);
    bool deflate_to_file(const bytes& buffer, const char* filename);
}; //namespace shared

#endif //SHARED_UTILS_H_INCLUDED
