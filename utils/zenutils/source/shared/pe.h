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

#ifndef SHARED_PE_H_INCLUDED
#define SHARED_PE_H_INCLUDED

#include <string>
#include <pelib/PeLib.h>
#include <utils.h>

namespace shared {
    struct section_info
    {
        word index;
        dword virtual_address;
        dword virtual_size;
        dword raw_address;
        dword raw_size;
        dword characteristics;
    }; //struct section_info

    class pe_file
    {
    public:
        pe_file(PeLib::PeFile* pef = NULL);
        ~pe_file();

        bool is_valid() const;
        bool read(const std::string& filename);
        bool find_section(const std::string& name, section_info& info) const;
        bool add_section(const std::string& name, const bytes& buffer, section_info& info);
        dword get_image_base() const;
        dword pa_to_va(PeLib::dword pa) const;

    protected:
        template <int _Bits>
        static bool find_section(const PeLib::PeFileT<_Bits>* pef,
                          const std::string& name, section_info& info);
        template <int _Bits>
        static bool add_section(PeLib::PeFileT<_Bits>* pef,
                          const std::string& name, const bytes& buffer,
                          section_info& info);

    private:
        PeLib::PeFile* _pef;
    }; //class pe_file


    template <int _Bits>
    bool pe_file::find_section(const PeLib::PeFileT<_Bits>* pef,
                      const std::string& name, section_info& info)
    {
        for (PeLib::word i = 0; i < pef->peHeader().getNumberOfSections(); i++)
        {
            if (pef->peHeader().getSectionName(i) == name)
            {
                info.index = i;
                info.virtual_address = pef->peHeader().getVirtualAddress(i);
                info.virtual_size = pef->peHeader().getVirtualSize(i);
                info.raw_address = pef->peHeader().getPointerToRawData(i);
                info.raw_size = pef->peHeader().getSizeOfRawData(i);
                info.characteristics = pef->peHeader().getCharacteristics(i);
                return true;
            }
        }
        return false;
    }

    template <int _Bits>
    bool pe_file::add_section(PeLib::PeFileT<_Bits>* pef,
                      const std::string& name, const bytes& buffer,
                      section_info& info)
    {
        using namespace PeLib;

        // Check if the last section has the same name as the one being added.
        PeLib::word secnum = pef->peHeader().getNumberOfSections();
        if (pef->peHeader().getSectionName(secnum-1) == name)
        {
            // If it is, we change the attributes of the existing section.
            secnum = secnum - 1;
            pef->peHeader().setSizeOfRawData(secnum,
                alignOffset(buffer.size(),
                            pef->peHeader().getFileAlignment()));
            pef->peHeader().setVirtualSize(secnum,
                alignOffset(buffer.size(),
                            pef->peHeader().getSectionAlignment()));
            PeLib::dword chars = pef->peHeader().getCharacteristics(secnum-1);
            pef->peHeader().setCharacteristics(secnum,
                chars | PELIB_IMAGE_SCN_MEM_WRITE | PELIB_IMAGE_SCN_MEM_READ);
        }
        else
        {
            // Otherwise we add a new section.
            if (pef->peHeader().addSection(name, buffer.size()) != NO_ERROR)
            {
                return false;
            }
            pef->peHeader().makeValid(pef->mzHeader().getAddressOfPeHeader());
            pef->peHeader().write(pef->getFileName(), pef->mzHeader().getAddressOfPeHeader());
        }

        // Save the section headers to the file.
        if (pef->peHeader().writeSections(pef->getFileName()) != NO_ERROR)
        {
            return false;
        }

        // Save the section data to the file.
        if (pef->peHeader().writeSectionData(pef->getFileName(), secnum, buffer) != NO_ERROR)
        {
            return false;
        }

        // Fill out the section information.
        info.index = secnum;
        info.virtual_address = pef->peHeader().getVirtualAddress(secnum);
        info.virtual_size = pef->peHeader().getVirtualSize(secnum);
        info.raw_address = pef->peHeader().getPointerToRawData(secnum);
        info.raw_size = pef->peHeader().getSizeOfRawData(secnum);
        info.characteristics = pef->peHeader().getCharacteristics(secnum);

        return true;
    }
}; //namespace shared

#endif //SHARED_PE_H_INCLUDED
