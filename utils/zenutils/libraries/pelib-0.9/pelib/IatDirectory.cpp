/*
* IatDirectory.h - Part of the PeLib library.
*
* Copyright (c) 2004 - 2005 Sebastian Porst (webmaster@the-interweb.com)
* All rights reserved.
*
* This software is licensed under the zlib/libpng License.
* For more details see http://www.opensource.org/licenses/zlib-license.php
* or the license information file (license.htm) in the root directory 
* of PeLib.
*/

#include "IatDirectory.h"

namespace PeLib
{
	int IatDirectory::read(InputBuffer& inputBuffer, unsigned int size)
	{
		dword dwAddr;

		std::vector<dword> vIat;

		for (unsigned int i=0;i<size/sizeof(dword);i++)
		{
			inputBuffer >> dwAddr;
			vIat.push_back(dwAddr);
		}
		
		std::swap(vIat, m_vIat);
		
		return NO_ERROR;
	}
	
	/**
	* Reads the Import Address table from a file.
	* @param strFilename Name of the file.
	* @param dwOffset File offset of the IAT (see #PeFile::PeHeader::getIDIatRVA).
	* @param dwSize Size of the IAT (see #PeFile::PeHeader::getIDIatSize).
	**/
	int IatDirectory::read(const std::string& strFilename, unsigned int dwOffset, unsigned int dwSize)
	{
		std::ifstream ifFile(strFilename.c_str(), std::ios::binary);
		
		if (!ifFile)
		{
			return ERROR_OPENING_FILE;
		}
		
		if (fileSize(ifFile) < dwOffset + dwSize)
		{
			return ERROR_INVALID_FILE;
		}
		
		ifFile.seekg(dwOffset, std::ios::beg);
		
		std::vector<byte> vBuffer(dwSize);
		ifFile.read(reinterpret_cast<char*>(&vBuffer[0]), dwSize);

		InputBuffer inpBuffer(vBuffer);
		return read(inpBuffer, dwSize);
	}
	
	int IatDirectory::read(unsigned char* buffer, unsigned int buffersize)
	{
		std::vector<byte> vBuffer(buffer, buffer + buffersize);
		InputBuffer inpBuffer(vBuffer);
		return read(inpBuffer, buffersize);
	}
	
	/**
	* Returns the number of fields in the IAT. This is equivalent to the number of
	* imported functions.
	* @return Number of fields in the IAT.
	**/
	unsigned int IatDirectory::calcNumberOfAddresses() const
	{
		return static_cast<unsigned int>(m_vIat.size());
	}

	/**
	* Returns the dwValue of a field in the IAT.
	* @param dwAddrnr Number identifying the field.
	* @return dwValue of the field.
	**/
	dword IatDirectory::getAddress(unsigned int index) const
	{
		return m_vIat[index];
	}
	
	/**
	* Updates the dwValue of a field in the IAT.
	* @param dwAddrnr Number identifying the field.
	* @param dwValue New dwValue of the field.
	**/
	void IatDirectory::setAddress(dword dwAddrnr, dword dwValue)
	{
		m_vIat[dwAddrnr] = dwValue;
	}

	/**
	* Adds another field to the IAT.
	* @param dwValue dwValue of the new field.
	**/
	void IatDirectory::addAddress(dword dwValue)
	{
		m_vIat.push_back(dwValue);
	}
	
	/**
	* Removes an address from the IAT.
	* @param dwAddrnr Number identifying the field.
	**/
	void IatDirectory::removeAddress(unsigned int index)
	{
		std::vector<dword>::iterator pos = m_vIat.begin() + index;
		m_vIat.erase(pos);
	}
	
	/**
	* Delete all entries from the IAT.
	**/
	void IatDirectory::clear()
	{
		m_vIat.clear();
	}
	
	/**
	* Rebuilds the complete Import Address Table.
	* @param vBuffer Buffer where the rebuilt IAT will be stored.
	**/
	void IatDirectory::rebuild(std::vector<byte>& vBuffer) const
	{
		vBuffer.reserve(size());
		OutputBuffer obBuffer(vBuffer);
	
		for (unsigned int i=0;i<m_vIat.size();i++)
		{
			obBuffer << m_vIat[i];
		}
	}

	unsigned int IatDirectory::size() const
	{
		return static_cast<unsigned int>(m_vIat.size())* sizeof(dword);
	}

	/// Writes the current IAT to a file.
	int IatDirectory::write(const std::string& strFilename, unsigned int uiOffset) const
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
}
