/*
* Relocations.cpp - Part of the PeLib library.
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
#include "RelocationsDirectory.h"

namespace PeLib
{
	void RelocationsDirectory::setRelocationData(unsigned int ulRelocation, unsigned int ulDataNumber, word wData)
	{
		m_vRelocations[ulRelocation].vRelocData[ulDataNumber] = wData;
	}
	
	void RelocationsDirectory::read(InputBuffer& inputbuffer, unsigned int uiSize)
	{
		IMG_BASE_RELOC ibrCurr;

		std::vector<IMG_BASE_RELOC> vCurrReloc;

		do
		{
			inputbuffer >> ibrCurr.ibrRelocation.VirtualAddress;
			inputbuffer >> ibrCurr.ibrRelocation.SizeOfBlock;

			ibrCurr.vRelocData.clear();

			// That's not how to check if there are relocations, some DLLs start at VA 0.
			// if (!ibrCurr.ibrRelocation.VirtualAddress) break;

			for (unsigned int i=0;i<(ibrCurr.ibrRelocation.SizeOfBlock - PELIB_IMAGE_SIZEOF_BASE_RELOCATION) / sizeof(word);i++)
			{
				word wData;
				inputbuffer >> wData;
				ibrCurr.vRelocData.push_back(wData);
			}

			vCurrReloc.push_back(ibrCurr);
		} while (ibrCurr.ibrRelocation.VirtualAddress && inputbuffer.get() < uiSize);

		std::swap(vCurrReloc, m_vRelocations);
	}
	
	// TODO: Return value is wrong if buffer was too small.
	int RelocationsDirectory::read(const unsigned char* buffer, unsigned int buffersize)
	{
		std::vector<unsigned char> vRelocDirectory(buffer, buffer + buffersize);

		InputBuffer ibBuffer(vRelocDirectory);
		read(ibBuffer, buffersize);
	
		return NO_ERROR;
	}

	int RelocationsDirectory::read(const std::string& strFilename, unsigned int uiOffset, unsigned int uiSize)
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

		std::vector<unsigned char> vRelocDirectory(uiSize);
		ifFile.read(reinterpret_cast<char*>(&vRelocDirectory[0]), uiSize);

		InputBuffer ibBuffer(vRelocDirectory);
		read(ibBuffer, uiSize);
		
		return NO_ERROR;
	}
	
	unsigned int RelocationsDirectory::size() const
	{
		unsigned int size = static_cast<unsigned int>(m_vRelocations.size()) * PELIB_IMAGE_BASE_RELOCATION::size();
		
		for (unsigned int i=0;i<m_vRelocations.size();i++)
		{
			size += static_cast<unsigned int>(m_vRelocations[i].vRelocData.size()) * sizeof(word);
		}
		
		return size;
	}
	
	void RelocationsDirectory::rebuild(std::vector<byte>& vBuffer) const
	{
		OutputBuffer obBuffer(vBuffer);
		
		for (unsigned int i=0;i<m_vRelocations.size();i++)
		{
			obBuffer << m_vRelocations[i].ibrRelocation.VirtualAddress;
			obBuffer << m_vRelocations[i].ibrRelocation.SizeOfBlock;

			for (unsigned int j=0;j<m_vRelocations[i].vRelocData.size();j++)
			{
				obBuffer << m_vRelocations[i].vRelocData[j];
			}
		}
	}

	unsigned int RelocationsDirectory::calcNumberOfRelocations() const
	{
		return static_cast<unsigned int>(m_vRelocations.size());
	}

	dword RelocationsDirectory::getVirtualAddress(unsigned int ulRelocation) const
	{
		return m_vRelocations[ulRelocation].ibrRelocation.VirtualAddress;
	}

	dword RelocationsDirectory::getSizeOfBlock(unsigned int ulRelocation) const
	{
		return m_vRelocations[ulRelocation].ibrRelocation.SizeOfBlock;
	}

	unsigned int RelocationsDirectory::calcNumberOfRelocationData(unsigned int ulRelocation) const
	{
		return static_cast<unsigned int>(m_vRelocations[ulRelocation].vRelocData.size());
	}

	word RelocationsDirectory::getRelocationData(unsigned int ulRelocation, unsigned int ulDataNumber) const
	{
		return m_vRelocations[ulRelocation].vRelocData[ulDataNumber];
	}

	void RelocationsDirectory::setVirtualAddress(unsigned int ulRelocation, dword dwValue)
	{
		m_vRelocations[ulRelocation].ibrRelocation.VirtualAddress = dwValue;
	}

	void RelocationsDirectory::setSizeOfBlock(unsigned int ulRelocation, dword dwValue)
	{
		m_vRelocations[ulRelocation].ibrRelocation.SizeOfBlock = dwValue;
	}
	
	void RelocationsDirectory::addRelocation()
	{
		IMG_BASE_RELOC newrelocation;
		m_vRelocations.push_back(newrelocation);
	}

	void RelocationsDirectory::addRelocationData(unsigned int ulRelocation, word wValue)
	{
		m_vRelocations[ulRelocation].vRelocData.push_back(wValue);
	}

/*	void RelocationsDirectory::removeRelocationData(unsigned int ulRelocation, word wValue)
	{
		// If you get an error with Borland C++ here you have two options: Upgrade your compiler
		// or use the commented line instead of the line below.
		m_vRelocations[ulRelocation].vRelocData.erase(std::remove(m_vRelocations[ulRelocation].vRelocData.begin(), m_vRelocations[ulRelocation].vRelocData.end(), wValue), m_vRelocations[ulRelocation].vRelocData.end());
	}
*/
	void RelocationsDirectory::removeRelocation(unsigned int index)
	{
		m_vRelocations.erase(m_vRelocations.begin() + index);
	}
	
	void RelocationsDirectory::removeRelocationData(unsigned int relocindex, unsigned int dataindex)
	{
		m_vRelocations[relocindex].vRelocData.erase(m_vRelocations[relocindex].vRelocData.begin() + dataindex);
	}
		  
	int RelocationsDirectory::write(const std::string& strFilename, unsigned int uiOffset) const
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

		ofFile.write(reinterpret_cast<const char*>(&vBuffer[0]), static_cast<std::streamsize>(vBuffer.size()));

		ofFile.close();
		
		return NO_ERROR;
	}
}
