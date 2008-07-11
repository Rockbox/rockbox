/*
* PeLibAux.cpp - Part of the PeLib library.
*
* Copyright (c) 2004 - 2005 Sebastian Porst (webmaster@the-interweb.com)
* All rights reserved.
*
* This software is licensed under the zlib/libpng License.
* For more details see http://www.opensource.org/licenses/zlib-license.php
* or the license information file (license.htm) in the root directory 
* of PeLib.
*/

#include "PeLibInc.h"
#include "PeLibAux.h"
#include "PeFile.h"

#ifdef _MSC_VER
	#include <ctype.h>
#endif

namespace PeLib
{
    const qword PELIB_IMAGE_ORDINAL_FLAGS<64>::IMAGE_ORDINAL_FLAG = 0x8000000000000000ULL;
	
	bool PELIB_IMAGE_SECTION_HEADER::biggerFileOffset(const PELIB_IMAGE_SECTION_HEADER& ish) const
	{
		return PointerToRawData < ish.PointerToRawData;
	}

	bool PELIB_IMAGE_SECTION_HEADER::biggerVirtualAddress(const PELIB_IMAGE_SECTION_HEADER& ish) const
	{
		return VirtualAddress < ish.VirtualAddress;
	}

	unsigned int alignOffset(unsigned int uiOffset, unsigned int uiAlignment)
	{
		if (!uiAlignment) return uiAlignment;
		return (uiOffset % uiAlignment) ? uiOffset + (uiAlignment - uiOffset % uiAlignment) : uiOffset;
	}

	unsigned int fileSize(const std::string& filename)
	{
		std::fstream file(filename.c_str());
		file.seekg(0, std::ios::end);
		return file.tellg();
	}

	unsigned int fileSize(std::ifstream& file)
	{
		unsigned int oldpos = file.tellg();
		file.seekg(0, std::ios::end);
		unsigned int filesize = file.tellg();
		file.seekg(oldpos, std::ios::beg);
		return filesize;
	}
	
	unsigned int fileSize(std::fstream& file)
	{
		unsigned int oldpos = file.tellg();
		file.seekg(0, std::ios::end);
		unsigned int filesize = file.tellg();
		file.seekg(oldpos, std::ios::beg);
		return filesize;
	}
	
	unsigned int fileSize(std::ofstream& file)
	{
		unsigned int oldpos = file.tellp();
		file.seekp(0, std::ios::end);
		unsigned int filesize = file.tellp();
		file.seekp(oldpos, std::ios::beg);
		return filesize;
	}
	
	bool isEqualNc(const std::string& s1, const std::string& s2)
	{
		std::string t1 = s1;
		std::string t2 = s2;
	
		// No std:: to make VC++ happy
		#ifdef _MSC_VER
			std::transform(t1.begin(), t1.end(), t1.begin(), toupper);
			std::transform(t2.begin(), t2.end(), t2.begin(), toupper);
		#else
			// Weird syntax to make Borland C++ happy
			std::transform(t1.begin(), t1.end(), t1.begin(), (int(*)(int))std::toupper);
			std::transform(t2.begin(), t2.end(), t2.begin(), (int(*)(int))std::toupper);
		#endif
		return t1 == t2;
	}

	PELIB_IMAGE_DOS_HEADER::PELIB_IMAGE_DOS_HEADER()
	{
		e_magic = 0;
		e_cblp = 0;
		e_cp = 0;
		e_crlc = 0;
		e_cparhdr = 0;
		e_minalloc = 0;
		e_maxalloc = 0;
		e_ss = 0;
		e_sp = 0;
		e_csum = 0;
		e_ip = 0;
		e_cs = 0;
		e_lfarlc = 0;
		e_ovno = 0;

		for (unsigned int i=0;i<sizeof(e_res)/sizeof(e_res[0]);i++)
		{
			e_res[i] = 0;
		}

		e_oemid = 0;
		e_oeminfo = 0;

		for (unsigned int i=0;i<sizeof(e_res2)/sizeof(e_res2[0]);i++)
		{
			e_res2[i] = 0;
		}

		e_lfanew = 0;
	}

	PELIB_EXP_FUNC_INFORMATION::PELIB_EXP_FUNC_INFORMATION()
	{
		addroffunc = 0;
		addrofname = 0;
		ordinal = 0;
	}

	PELIB_IMAGE_RESOURCE_DIRECTORY::PELIB_IMAGE_RESOURCE_DIRECTORY()
	{
		Characteristics = 0;
		TimeDateStamp = 0;
		MajorVersion = 0;
		MinorVersion = 0;
		NumberOfNamedEntries = 0;
		NumberOfIdEntries = 0;
	}

	PELIB_IMAGE_RESOURCE_DIRECTORY_ENTRY::PELIB_IMAGE_RESOURCE_DIRECTORY_ENTRY()
	{
		Name = 0;
		OffsetToData = 0;
	}

	bool PELIB_IMG_RES_DIR_ENTRY::operator<(const PELIB_IMG_RES_DIR_ENTRY& first) const
	{
		if (irde.Name & PELIB_IMAGE_RESOURCE_NAME_IS_STRING && first.irde.Name & PELIB_IMAGE_RESOURCE_NAME_IS_STRING)
		{
			return wstrName < first.wstrName;
		}
		else if (irde.Name & PELIB_IMAGE_RESOURCE_NAME_IS_STRING)
		{
			return true;
		}
		else if (first.irde.Name & PELIB_IMAGE_RESOURCE_NAME_IS_STRING)
		{
			return false;
		}
		else
		{
			return irde.Name < first.irde.Name;
		}
	}

	PELIB_IMAGE_BASE_RELOCATION::PELIB_IMAGE_BASE_RELOCATION()
	{
		VirtualAddress = 0;
		SizeOfBlock = 0;
	}

	PELIB_IMAGE_COR20_HEADER::PELIB_IMAGE_COR20_HEADER()
	{
		cb = 0;
		MajorRuntimeVersion = 0;
		MinorRuntimeVersion = 0;
		MetaData.VirtualAddress = 0;
		MetaData.Size = 0;
		Flags = 0;
		EntryPointToken = 0;
		Resources.VirtualAddress = 0;
		Resources.Size = 0;
		StrongNameSignature.VirtualAddress = 0;
		StrongNameSignature.Size = 0;
		CodeManagerTable.VirtualAddress = 0;
		CodeManagerTable.Size = 0;
		VTableFixups.VirtualAddress = 0;
		VTableFixups.Size = 0;
		ExportAddressTableJumps.VirtualAddress = 0;
		ExportAddressTableJumps.Size = 0;
		ManagedNativeHeader.VirtualAddress = 0;
		ManagedNativeHeader.Size = 0;
	}

	/** Compares the passed filename to the struct's filename.
	* @param strModuleName A filename.
	* @return True, if the passed filename equals the struct's filename. The comparison is case-sensitive.
	**/
	bool PELIB_IMAGE_BOUND_DIRECTORY::equal(const std::string strModuleName) const
	{
		return this->strModuleName == strModuleName;
	}

	bool PELIB_EXP_FUNC_INFORMATION::equal(const std::string strFunctionName) const
	{
		return isEqualNc(this->funcname, strFunctionName);
	}
	
	/**
	* @param strFilename Name of a file.
	* @return Either PEFILE32, PEFILE64 or PEFILE_UNKNOWN
	**/
	unsigned int getFileType(const std::string strFilename)
	{
		word machine, magic;
		
		PeFile32 pef(strFilename);
		if (pef.readMzHeader() != NO_ERROR) return PEFILE_UNKNOWN;
		if (pef.readPeHeader() != NO_ERROR) return PEFILE_UNKNOWN;
			
		machine = pef.peHeader().getMachine();
		magic = pef.peHeader().getMagic();
	
		if (machine == PELIB_IMAGE_FILE_MACHINE_I386 && magic == PELIB_IMAGE_NT_OPTIONAL_HDR32_MAGIC)
		{
			return PEFILE32;
		}
		// 0x8664 == AMD64; no named constant yet
		else if ((machine == 0x8664 || machine == PELIB_IMAGE_FILE_MACHINE_IA64) && magic == PELIB_IMAGE_NT_OPTIONAL_HDR64_MAGIC)
		{
			return PEFILE64;
		}
		else
		{
			return PEFILE_UNKNOWN;
		}
	}
	
	/**
	* Opens a PE file. The return type is either PeFile32 or PeFile64 object. If an error occurs the return
	* value is 0.
	* @param strFilename Name of a file.
	* @return Either a PeFile32 object, a PeFil64 object or 0.
	**/
	PeFile* openPeFile(const std::string& strFilename)
	{
		unsigned int type = getFileType(strFilename);
		
		if (type == PEFILE32)
		{
			return new PeFile32(strFilename);
		}
		else if (type == PEFILE64)
		{
			return new PeFile64(strFilename);
		}
		else
		{
			return 0;
		}
	}
	
	unsigned int PELIB_IMAGE_BOUND_DIRECTORY::size() const
	{
		unsigned int size = 0;
		for (unsigned int i=0;i<moduleForwarders.size();i++)
		{
			size += moduleForwarders[i].size();
		}

		return size + PELIB_IMAGE_BOUND_IMPORT_DESCRIPTOR::size() + strModuleName.size() + 1;
	}
}
