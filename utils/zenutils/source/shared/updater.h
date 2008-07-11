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

#ifndef ZEN_UPDATER_H_INCLUDED
#define ZEN_UPDATER_H_INCLUDED

#include <pelib/PeLib.h>
#include <utils.h>

namespace zen {
    const char* find_firmware_key(const byte* buffer, size_t len);
    dword find_firmware_offset(byte* buffer, size_t len);
    bool find_firmware_archive(const std::string& filename, dword& va, dword& pa);
    bool crypt_firmware(const char* key, byte* buffer, size_t len);
}; //namespace zen

#endif //ZEN_UPDATER_H_INCLUDED
