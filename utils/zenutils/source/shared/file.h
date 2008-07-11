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

#ifndef SHARED_FILE_H_INCLUDED
#define SHARED_FILE_H_INCLUDED

#include <string>
#include <iostream>
#include "utils.h"

namespace shared {
    bool read_file(const std::string& filename, bytes& buffer,
                   std::streampos offset = 0, std::streamsize count = -1);
    bool write_file(const std::string& filename, bytes& buffer, bool truncate,
                    std::streampos offset = 0, std::streamsize count = -1);
    bool file_exists(const std::string& filename);
    bool copy_file(const std::string& srcname, const std::string& dstname);
    bool backup_file(const std::string& filename, bool force = false);
}; //namespace shared

#endif //SHARED_FILE_H_INCLUDED
