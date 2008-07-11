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

#include "pe.h"


shared::pe_file::pe_file(PeLib::PeFile* pef) : _pef(pef)
{
}
shared::pe_file::~pe_file()
{
    if (_pef != NULL)
        delete _pef;
}

bool shared::pe_file::is_valid() const
{
    if (_pef->getBits() == 32)
    {
        PeLib::PeHeader32& pef32 = static_cast<PeLib::PeFile32*>(_pef)->peHeader();
        if (!pef32.isValid())
            return false;
        return true;
    }
    else if (_pef->getBits() == 64)
    {
        PeLib::PeHeader64& pef64 = static_cast<PeLib::PeFile64*>(_pef)->peHeader();
        if (!pef64.isValid())
            return false;
        return true;
    }
    return false;
}

bool shared::pe_file::read(const std::string& filename)
{
    if (_pef != NULL)
    {
        delete _pef;
        _pef = NULL;
    }

    _pef = PeLib::openPeFile(filename);
    if (!_pef)
    {
        return false;
    }
    if (_pef->readMzHeader())
    {
        delete _pef;
        return false;
    }
    if (!_pef->mzHeader().isValid())
    {
        delete _pef;
        return false;
    }
    if (_pef->readPeHeader())
    {
        delete _pef;
        return false;
    }
    if (!is_valid())
    {
        delete _pef;
        return false;
    }
    return true;
}

bool shared::pe_file::find_section(const std::string& name, section_info& info) const
{
    if (_pef->getBits() == 32)
        return find_section(static_cast<PeLib::PeFile32*>(_pef),
                            name, info);
    else if (_pef->getBits() == 64)
        return find_section(static_cast<PeLib::PeFile64*>(_pef),
                            name, info);
    return false;
}

bool shared::pe_file::add_section(const std::string& name,
                                  const bytes& buffer, section_info& info)
{
    if (_pef->getBits() == 32)
    {
         return add_section(static_cast<PeLib::PeFile32*>(_pef),
                            name, buffer, info);
    }
    else if (_pef->getBits() == 64)
    {
         return add_section(static_cast<PeLib::PeFile64*>(_pef),
                            name, buffer, info);
    }
    return false;
}

dword shared::pe_file::get_image_base() const
{
    if (_pef->getBits() == 32)
        return static_cast<PeLib::PeFile32*>(_pef)->peHeader().getImageBase();
    else
        return static_cast<PeLib::PeFile64*>(_pef)->peHeader().getImageBase();
    return 0;
}
dword shared::pe_file::pa_to_va(dword pa) const
{
    if (_pef->getBits() == 32)
        return static_cast<PeLib::PeFile32*>(_pef)->peHeader().offsetToVa(pa);
    else
        return static_cast<PeLib::PeFile64*>(_pef)->peHeader().offsetToVa(pa);
    return 0;
}
