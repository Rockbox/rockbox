/*
* PeLib.cpp - Part of the PeLib library.
*
* Copyright (c) 2004 - 2005 Sebastian Porst (webmaster@the-interweb.com)
* All rights reserved.
*
* This software is licensed under the zlib/libpng License.
* For more details see http://www.opensource.org/licenses/zlib-license.php
* or the license information file (license.htm) in the root directory 
* of PeLib.
*/

#include "PeFile.h"

namespace PeLib
{
	PeFile::~PeFile()
	{
	}
	
	PeFile32::PeFile32() : PeFileT<32>()
	{
	}
	
	PeFile32::PeFile32(const std::string& strFlename) : PeFileT<32>(strFlename)
	{
	}
	
	PeFile64::PeFile64() : PeFileT<64>()
	{
	}
	
	PeFile64::PeFile64(const std::string& strFlename) : PeFileT<64>(strFlename)
	{
	}
	
	/**
	* @return A reference to the file's MZ header.
	**/
	
	const MzHeader& PeFile::mzHeader() const
	{
		return m_mzh;
	}

	/**
	* @return A reference to the file's MZ header.
	**/
	
	MzHeader& PeFile::mzHeader()
	{
		return m_mzh;
	}

	/**
	* @return A reference to the file's export directory.
	**/
	
	const ExportDirectory& PeFile::expDir() const
	{
		return m_expdir;
	}

	/**
	* @return A reference to the file's export directory.
	**/
	
	ExportDirectory& PeFile::expDir()
	{
		return m_expdir;
	}
	
	/**
	* @return A reference to the file's bound import directory.
	**/
	
	const BoundImportDirectory& PeFile::boundImpDir() const
	{
		return m_boundimpdir;
	}
	
	/**
	* @return A reference to the file's bound import directory.
	**/
	
	BoundImportDirectory& PeFile::boundImpDir()
	{
		return m_boundimpdir;
	}
	
	/**
	* @return A reference to the file's resource directory.
	**/
	
	const ResourceDirectory& PeFile::resDir() const
	{
		return m_resdir;
	}
	
	/**
	* @return A reference to the file's resource directory.
	**/
	
	ResourceDirectory& PeFile::resDir()
	{
		return m_resdir;
	}
	
	/**
	* @return A reference to the file's relocations directory.
	**/
	
	const RelocationsDirectory& PeFile::relocDir() const
	{
		return m_relocs;
	}

	/**
	* @return A reference to the file's relocations directory.
	**/
	
	RelocationsDirectory& PeFile::relocDir()
	{
		return m_relocs;
	}

	/**
	* @return A reference to the file's COM+ descriptor directory.
	**/
	
	const ComHeaderDirectory& PeFile::comDir() const
	{
		return m_comdesc;
	}

	/**
	* @return A reference to the file's COM+ descriptor directory.
	**/
	
	ComHeaderDirectory & PeFile::comDir()
	{
		return m_comdesc;
	}

	
	const IatDirectory& PeFile::iatDir() const
	{
		return m_iat;
	}

	
	IatDirectory& PeFile::iatDir()
	{
		return m_iat;
	}

	
	const DebugDirectory& PeFile::debugDir() const
	{
		return m_debugdir;
	}

	
	DebugDirectory& PeFile::debugDir()
	{
		return m_debugdir;
	}

}
