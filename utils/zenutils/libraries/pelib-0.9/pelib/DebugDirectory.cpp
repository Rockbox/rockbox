/*
* DebugDirectory.cpp - Part of the PeLib library.
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
#include "DebugDirectory.h"

namespace PeLib
{
	void DebugDirectory::clear()
	{
		m_vDebugInfo.clear();
	}
	
	std::vector<PELIB_IMG_DEBUG_DIRECTORY> DebugDirectory::read(InputBuffer& ibBuffer, unsigned int uiSize)
	{
		std::vector<PELIB_IMG_DEBUG_DIRECTORY> currDebugInfo;
		
		PELIB_IMG_DEBUG_DIRECTORY iddCurr;
		
		for (unsigned int i=0;i<uiSize/PELIB_IMAGE_DEBUG_DIRECTORY::size();i++)
		{
			
			ibBuffer >> iddCurr.idd.Characteristics;
			ibBuffer >> iddCurr.idd.TimeDateStamp;
			ibBuffer >> iddCurr.idd.MajorVersion;
			ibBuffer >> iddCurr.idd.MinorVersion;
			ibBuffer >> iddCurr.idd.Type;
			ibBuffer >> iddCurr.idd.SizeOfData;
			ibBuffer >> iddCurr.idd.AddressOfRawData;
			ibBuffer >> iddCurr.idd.PointerToRawData;
		
			currDebugInfo.push_back(iddCurr);
		}
		
		return currDebugInfo;
	}
	
	int DebugDirectory::read(unsigned char* buffer, unsigned int buffersize)
	{
		// XXX: Note, debug data is not read at all. This might or might not change
		//      in the future.
		
		std::vector<byte> vDebugDirectory(buffer, buffer + buffersize);
		
		InputBuffer ibBuffer(vDebugDirectory);
		
		std::vector<PELIB_IMG_DEBUG_DIRECTORY> currDebugInfo = read(ibBuffer, buffersize);
		
		std::swap(currDebugInfo, m_vDebugInfo);
		
		return NO_ERROR;
	}
	
	/**
	* @param strFilename Name of the file which will be read.
	* @param uiOffset File offset of the Debug directory.
	* @param uiSize Size of the Debug directory.
	**/
	int DebugDirectory::read(const std::string& strFilename, unsigned int uiOffset, unsigned int uiSize)
	{
		std::ifstream ifFile(strFilename.c_str(), std::ios::binary);
		unsigned int ulFileSize = fileSize(ifFile);

		if (!ifFile)
		{
			return ERROR_OPENING_FILE;
		}
		
		if (ulFileSize < uiOffset + uiSize)
		{
			return ERROR_INVALID_FILE;
		}

		ifFile.seekg(uiOffset, std::ios::beg);

		std::vector<byte> vDebugDirectory(uiSize);
		ifFile.read(reinterpret_cast<char*>(&vDebugDirectory[0]), uiSize);
		
		InputBuffer ibBuffer(vDebugDirectory);
		
		std::vector<PELIB_IMG_DEBUG_DIRECTORY> currDebugInfo = read(ibBuffer, uiSize);
		
		for (unsigned int i=0;i<currDebugInfo.size();i++)
		{
			ifFile.seekg(currDebugInfo[i].idd.PointerToRawData, std::ios::beg);
			currDebugInfo[i].data.resize(currDebugInfo[i].idd.SizeOfData);
			ifFile.read(reinterpret_cast<char*>(&currDebugInfo[i].data[0]), currDebugInfo[i].idd.SizeOfData);
			if (!ifFile) return ERROR_INVALID_FILE;
		}
		
		std::swap(currDebugInfo, m_vDebugInfo);
		
		return NO_ERROR;
	}
	
	/**
	* Rebuilds the current debug directory.
	* @param vBuffer Buffer where the rebuilt directory is stored.
	**/
	void DebugDirectory::rebuild(std::vector<byte>& vBuffer) const
	{
		OutputBuffer obBuffer(vBuffer);
		
		for (unsigned int i=0;i<m_vDebugInfo.size();i++)
		{
			obBuffer << m_vDebugInfo[i].idd.Characteristics;
			obBuffer << m_vDebugInfo[i].idd.TimeDateStamp;
			obBuffer << m_vDebugInfo[i].idd.MajorVersion;
			obBuffer << m_vDebugInfo[i].idd.MinorVersion;
			obBuffer << m_vDebugInfo[i].idd.Type;
			obBuffer << m_vDebugInfo[i].idd.SizeOfData;
			obBuffer << m_vDebugInfo[i].idd.AddressOfRawData;
			obBuffer << m_vDebugInfo[i].idd.PointerToRawData;
		}
	}

	/**
	* @return Size of the debug directory.
	**/
	unsigned int DebugDirectory::size() const
	{
		return static_cast<unsigned int>(m_vDebugInfo.size()) * PELIB_IMAGE_DEBUG_DIRECTORY::size();
	}

	/**
	* @param strFilename Name of the file which will be written.
	* @param uiOffset File offset where the debug directory will be stored.
	**/
	int DebugDirectory::write(const std::string& strFilename, unsigned int uiOffset) const
	{
		std::fstream ofFile(strFilename.c_str(), std::ios_base::in);
		
		if (!ofFile)
		{
			ofFile.clear();
			ofFile.open(strFilename.c_str(), std::ios_base::out | std::ios_base::binary);
		}
		else
		{
			ofFile.close();
			ofFile.open(strFilename.c_str(), std::ios_base::in | std::ios_base::out | std::ios_base::binary);
		}

		if (!ofFile)
		{
			return ERROR_OPENING_FILE;
		}

		ofFile.seekp(uiOffset, std::ios::beg);

		std::vector<unsigned char> vBuffer;
		rebuild(vBuffer);

		ofFile.write(reinterpret_cast<const char*>(&vBuffer[0]), static_cast<unsigned int>(vBuffer.size()));

		ofFile.close();
		
		return NO_ERROR;
	}

	/**
	* @return Number of debug structures in the current Debug directory.
	**/
	unsigned int DebugDirectory::calcNumberOfEntries() const
	{
		return static_cast<unsigned int>(m_vDebugInfo.size());
	}

	/**
	* Adds a new debug structure to the debug directory. The initial values of all members of the structure
	* are undefined.
	**/
	void DebugDirectory::addEntry()
	{
		PELIB_IMG_DEBUG_DIRECTORY p;
		m_vDebugInfo.push_back(p);
	}

	/**
	* Removes a debug structure from the current debug directory. If an invalid structure is specified
	* by the parameter uiIndex the result will be undefined behaviour.
	* @param uiIndex Identifies the debug structure.
	**/
	void DebugDirectory::removeEntry(unsigned int uiIndex)
	{
		m_vDebugInfo.erase(m_vDebugInfo.begin() + uiIndex);
	}
		  
	/**
	* Returns the Characteristics value of a debug structure. If an invalid structure is specified
	* by the parameter uiIndex the result will be undefined behaviour.
	* @param uiIndex Identifies the debug structure.
	* @return Characteristics value of the debug structure.
	**/
	dword DebugDirectory::getCharacteristics(unsigned int uiIndex) const
	{
		return m_vDebugInfo[uiIndex].idd.Characteristics;
	}
	
	/**
	* Returns the TimeDateStamp value of a debug structure. If an invalid structure is specified
	* by the parameter uiIndex the result will be undefined behaviour.
	* @param uiIndex Identifies the debug structure.
	* @return TimeDateStamp value of the debug structure.
	**/
	dword DebugDirectory::getTimeDateStamp(unsigned int uiIndex) const
	{
		return m_vDebugInfo[uiIndex].idd.TimeDateStamp;
	}
	
	/**
	* Returns the MajorVersion value of a debug structure. If an invalid structure is specified
	* by the parameter uiIndex the result will be undefined behaviour.
	* @param uiIndex Identifies the debug structure.
	* @return MajorVersion value of the debug structure.
	**/
	word DebugDirectory::getMajorVersion(unsigned int uiIndex) const
	{
		return m_vDebugInfo[uiIndex].idd.MajorVersion;
	}
	
	/**
	* Returns the MinorVersion value of a debug structure. If an invalid structure is specified
	* by the parameter uiIndex the result will be undefined behaviour.
	* @param uiIndex Identifies the debug structure.
	* @return MinorVersion value of the debug structure.
	**/
	word DebugDirectory::getMinorVersion(unsigned int uiIndex) const
	{
		return m_vDebugInfo[uiIndex].idd.MinorVersion;
	}
	
	/**
	* Returns the Type value of a debug structure. If an invalid structure is specified
	* by the parameter uiIndex the result will be undefined behaviour.
	* @param uiIndex Identifies the debug structure.
	* @return Type value of the debug structure.
	**/
	dword DebugDirectory::getType(unsigned int uiIndex) const
	{
		return m_vDebugInfo[uiIndex].idd.Type;
	}
	
	/**
	* Returns the SizeOfData value of a debug structure. If an invalid structure is specified
	* by the parameter uiIndex the result will be undefined behaviour.
	* @param uiIndex Identifies the debug structure.
	* @return SizeOfData value of the debug structure.
	**/
	dword DebugDirectory::getSizeOfData(unsigned int uiIndex) const
	{
		return m_vDebugInfo[uiIndex].idd.SizeOfData;
	}
	
	/**
	* Returns the AddressOfRawData value of a debug structure. If an invalid structure is specified
	* by the parameter uiIndex the result will be undefined behaviour.
	* @param uiIndex Identifies the debug structure.
	* @return AddressOfRawData value of the debug structure.
	**/
	dword DebugDirectory::getAddressOfRawData(unsigned int uiIndex) const
	{
		return m_vDebugInfo[uiIndex].idd.AddressOfRawData;
	}
	
	/**
	* Returns the PointerToRawData value of a debug structure. If an invalid structure is specified
	* by the parameter uiIndex the result will be undefined behaviour.
	* @param uiIndex Identifies the debug structure.
	* @return PointerToRawData value of the debug structure.
	**/
	dword DebugDirectory::getPointerToRawData(unsigned int uiIndex) const
	{
		return m_vDebugInfo[uiIndex].idd.PointerToRawData;
	}
	
	std::vector<byte> DebugDirectory::getData(unsigned int index) const
	{
		return m_vDebugInfo[index].data;
	}

	/**
	* Changes the Characteristics value of a debug structure. If an invalid structure is specified
	* by the parameter uiIndex the result will be undefined behaviour.
	* @param uiIndex Identifies the debug structure.
	* @param dwValue New value of the Characteristics value of the debug structure.
	**/
	void DebugDirectory::setCharacteristics(unsigned int uiIndex, dword dwValue)
	{
		m_vDebugInfo[uiIndex].idd.Characteristics = dwValue;
	}
	
	/**
	* Changes the TimeDateStamp value of a debug structure. If an invalid structure is specified
	* by the parameter uiIndex the result will be undefined behaviour.
	* @param uiIndex Identifies the debug structure.
	* @param dwValue New value of the TimeDateStamp value of the debug structure.
	**/
	void DebugDirectory::setTimeDateStamp(unsigned int uiIndex, dword dwValue)
	{
		m_vDebugInfo[uiIndex].idd.TimeDateStamp = dwValue;
	}
	
	/**
	* Changes the MajorVersion value of a debug structure. If an invalid structure is specified
	* by the parameter uiIndex the result will be undefined behaviour.
	* @param uiIndex Identifies the debug structure.
	* @param wValue New value of the MajorVersion value of the debug structure.
	**/
	void DebugDirectory::setMajorVersion(unsigned int uiIndex, word wValue)
	{
		m_vDebugInfo[uiIndex].idd.MajorVersion = wValue;
	}
	
	/**
	* Changes the MinorVersion value of a debug structure. If an invalid structure is specified
	* by the parameter uiIndex the result will be undefined behaviour.
	* @param uiIndex Identifies the debug structure.
	* @param wValue New value of the MinorVersion value of the debug structure.
	**/
	void DebugDirectory::setMinorVersion(unsigned int uiIndex, word wValue)
	{
		m_vDebugInfo[uiIndex].idd.MinorVersion = wValue;
	}
	
	/**
	* Changes the Type value of a debug structure. If an invalid structure is specified
	* by the parameter uiIndex the result will be undefined behaviour.
	* @param uiIndex Identifies the debug structure.
	* @param dwValue New value of the Type value of the debug structure.
	**/
	void DebugDirectory::setType(unsigned int uiIndex, dword dwValue)
	{
		m_vDebugInfo[uiIndex].idd.Type = dwValue;
	}
	
	/**
	* Changes the SizeOfData value of a debug structure. If an invalid structure is specified
	* by the parameter uiIndex the result will be undefined behaviour.
	* @param uiIndex Identifies the debug structure.
	* @param dwValue New value of the SizeOfData value of the debug structure.
	**/
	void DebugDirectory::setSizeOfData(unsigned int uiIndex, dword dwValue)
	{
		m_vDebugInfo[uiIndex].idd.SizeOfData = dwValue;
	}
	
	/**
	* Changes the AddressOfRawData value of a debug structure. If an invalid structure is specified
	* by the parameter uiIndex the result will be undefined behaviour.
	* @param uiIndex Identifies the debug structure.
	* @param dwValue New value of the AddressOfRawData value of the debug structure.
	**/
	void DebugDirectory::setAddressOfRawData(unsigned int uiIndex, dword dwValue)
	{
		m_vDebugInfo[uiIndex].idd.AddressOfRawData = dwValue;
	}
	
	/**
	* Changes the PointerToRawData value of a debug structure. If an invalid structure is specified
	* by the parameter uiIndex the result will be undefined behaviour.
	* @param uiIndex Identifies the debug structure.
	* @param dwValue New value of the PointerToRawData value of the debug structure.
	**/
	void DebugDirectory::setPointerToRawData(unsigned int uiIndex, dword dwValue)
	{
		m_vDebugInfo[uiIndex].idd.PointerToRawData = dwValue;
	}
	
	void DebugDirectory::setData(unsigned int index, const std::vector<byte>& data)
	{
		m_vDebugInfo[index].data = data;
	}
}
