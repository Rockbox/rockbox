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

#include "utils.h"
#include <fstream>
#include <zlib/zlib.h>


std::string shared::replace_extension(const std::string& filename, const std::string& extension)
{
    std::string newname;
    const char* name = filename.c_str();
    const char* ext = strrchr(name, '.');
    if (ext)
    {
        // If an extension was found, replace it.
        newname.assign(name, ext-name);
        newname += extension;
    }
    else
    {
        // If an extension was not found, append it.
        newname = name;
        newname += extension;
    }
    return newname;
}

std::string shared::remove_extension(const std::string& filename)
{
    std::string newname;
    const char* name = filename.c_str();
    const char* ext = strrchr(name, '.');
    if (ext)
    {
        newname.assign(name, ext-name);
    }
    else
    {
        newname = name;
    }
    return newname;
}

std::string shared::double_quote(const std::string& str)
{
    std::string out;
    for (int i = 0, j = str.length(); i < j; i++)
    {
        if (str[i] == '\\')
            out += "\\\\";
        else
            out += str[i];
    }
    return out;
}

bool shared::inflate_to_file(const bytes& buffer, const char* filename)
{
    // Open output file.
    std::ofstream ofs;
    ofs.open(filename, std::ios::binary);
    if (!ofs)
    {
        return false;
    }

    // Initialize zlib.
    z_stream d_stream; // decompression stream

    d_stream.zalloc = Z_NULL;
    d_stream.zfree = Z_NULL;
    d_stream.opaque = Z_NULL;

    d_stream.next_in  = const_cast<bytes::value_type*>(&buffer[0]);
    d_stream.avail_in = static_cast<uInt>(buffer.size());

    int ret = inflateInit(&d_stream);
    if (ret != Z_OK)
        return false;

    // Allocate buffer to hold the inflated data.
    const size_t BUFSIZE = 1048576;
    Bytef* infbuf = new Bytef[BUFSIZE];
    if (!infbuf)
        return false;

    // Decompress untill the end of the input buffer.
    uLong totalout = 0;
    bool bLoop = true;
    while (bLoop)
    {
        d_stream.next_out = infbuf;
        d_stream.avail_out = BUFSIZE;

        ret = inflate(&d_stream, Z_NO_FLUSH);
        if (ret == Z_STREAM_END)
        {
            bLoop = false;
        }
        else if (ret != Z_OK)
        {
            inflateEnd(&d_stream);
            delete [] infbuf;
            return false;
        }

        // Write the inflated data to the output file.
        if (!ofs.write((const char*)infbuf, d_stream.total_out-totalout))
        {
            inflateEnd(&d_stream);
            delete [] infbuf;
            return false;
        }
        totalout = d_stream.total_out;
    }

    // Cleanup and return.
    inflateEnd(&d_stream);
    delete [] infbuf;

    return true;
}

bool shared::deflate_to_file(const bytes& buffer, const char* filename)
{
    // Open output file.
    std::ofstream ofs;
    ofs.open(filename, std::ios::binary);
    if (!ofs)
    {
        return false;
    }

    // Initialize zlib.
    z_stream c_stream; // compression stream.

    c_stream.zalloc = Z_NULL;
    c_stream.zfree = Z_NULL;
    c_stream.opaque = Z_NULL;

    int ret = deflateInit(&c_stream, Z_BEST_COMPRESSION);
    if (ret != Z_OK)
        return false;

    // Allocate buffer to hold the deflated data.
    const size_t BUFSIZE = 1048576;
    Bytef* defbuf = new Bytef[BUFSIZE];
    if (!defbuf)
        return false;

    c_stream.avail_in = static_cast<uInt>(buffer.size());
    c_stream.next_in = const_cast<bytes::value_type*>(&buffer[0]);

    // Compress until end of the buffer.
    uLong totalout = 0;
    bool bLoop = true;
    while (bLoop)
    {
        c_stream.avail_out = BUFSIZE;
        c_stream.next_out = defbuf;

        ret = deflate(&c_stream, Z_NO_FLUSH);    // no bad return value
        if (ret == Z_STREAM_END)
        {
            bLoop = false;
        }
        else if (ret == Z_BUF_ERROR && !c_stream.avail_in)
        {
            ret = deflate(&c_stream, Z_FINISH);    // no bad return value
            bLoop = false;
        }
        else if (ret != Z_OK)
        {
            deflateEnd(&c_stream);
            delete [] defbuf;
            return false;
        }

        // Write the inflated data to the output file.
        if (!ofs.write((const char*)defbuf, c_stream.total_out-totalout))
        {
            deflateEnd(&c_stream);
            delete [] defbuf;
            return false;
        }

        totalout = c_stream.total_out;
    }

    // Clean up and return.
    deflateEnd(&c_stream);
    delete [] defbuf;

    return true;
}
