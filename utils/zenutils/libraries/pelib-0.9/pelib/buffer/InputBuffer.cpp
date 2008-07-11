/*
* InputBuffer.cpp - Part of the PeLib library.
*
* Copyright (c) 2004 - 2005 Sebastian Porst (webmaster@the-interweb.com)
* All rights reserved.
*
* This software is licensed under the zlib/libpng License.
* For more details see http://www.opensource.org/licenses/zlib-license.php
* or the license information file (license.htm) in the root directory 
* of PeLib.
*/

#include "InputBuffer.h"

namespace PeLib
{
	unsigned long InputBuffer::get()
	{
		return ulIndex;
	}

	InputBuffer::InputBuffer(std::vector<unsigned char>& vBuffer) : m_vBuffer(vBuffer), ulIndex(0)
	{
	}

	const unsigned char* InputBuffer::data() const
	{
		return &m_vBuffer[0];
	}

	unsigned long InputBuffer::size()
	{
		return static_cast<unsigned long>(m_vBuffer.size());
	}

	void InputBuffer::read(char* lpBuffer, unsigned long ulSize)
	{
		std::copy(&m_vBuffer[ulIndex], &m_vBuffer[ulIndex + ulSize], lpBuffer);
		ulIndex += ulSize;
	}

	void InputBuffer::reset()
	{
		m_vBuffer.clear();
	}

	void InputBuffer::set(unsigned long ulIndex)
	{
		this->ulIndex = ulIndex;
	}

	void InputBuffer::setBuffer(std::vector<unsigned char>& vBuffer)
	{
		m_vBuffer = vBuffer;
		ulIndex = 0;
	}
}

