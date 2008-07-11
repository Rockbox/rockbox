/*
* ResourceDirectory.h - Part of the PeLib library.
*
* Copyright (c) 2004 - 2005 Sebastian Porst (webmaster@the-interweb.com)
* All rights reserved.
*
* This software is licensed under the zlib/libpng License.
* For more details see http://www.opensource.org/licenses/zlib-license.php
* or the license information file (license.htm) in the root directory 
* of PeLib.
*/

#include "ResourceDirectory.h"

namespace PeLib
{
// -------------------------------------------------- ResourceChild -------------------------------------------

	ResourceChild::ResourceChild()
	{
	}
	
	ResourceChild::ResourceChild(const ResourceChild& rhs)
	{
		if (dynamic_cast<ResourceNode*>(rhs.child))
		{
			ResourceNode* oldnode = static_cast<ResourceNode*>(rhs.child);
			
			entry = rhs.entry;

			child = new ResourceNode;
			child->uiElementRva = rhs.child->getElementRva();
			static_cast<ResourceNode*>(child)->header = oldnode->header;
			static_cast<ResourceNode*>(child)->children = oldnode->children;
		}
		else
		{
			ResourceLeaf* oldnode = static_cast<ResourceLeaf*>(rhs.child);
			
			child = new ResourceLeaf;
			child->uiElementRva = rhs.child->getElementRva();
			static_cast<ResourceLeaf*>(child)->m_data = oldnode->m_data;
			static_cast<ResourceLeaf*>(child)->entry = oldnode->entry;
		}
	}
	
	ResourceChild& ResourceChild::operator=(const ResourceChild& rhs)
	{
		if (this != &rhs)
		{
			if (dynamic_cast<ResourceNode*>(rhs.child))
			{
				ResourceNode* oldnode = static_cast<ResourceNode*>(rhs.child);
				
				entry = rhs.entry;
	
				child = new ResourceNode;
				child->uiElementRva = rhs.child->getElementRva();
				static_cast<ResourceNode*>(child)->header = oldnode->header;
				static_cast<ResourceNode*>(child)->children = oldnode->children;
			}
			else
			{
				ResourceLeaf* oldnode = static_cast<ResourceLeaf*>(rhs.child);
				
				child = new ResourceLeaf;
				child->uiElementRva = rhs.child->getElementRva();
				static_cast<ResourceLeaf*>(child)->m_data = oldnode->m_data;
				static_cast<ResourceLeaf*>(child)->entry = oldnode->entry;
			}
		}
		
		return *this;
	}
	
	ResourceChild::~ResourceChild()
	{
		delete child;
	}

	/**
	* Compares the resource child's id to the parameter dwId.
	* @param dwId ID of a resource.
	* @return True, if the resource child's id equals the parameter.
	**/
	bool ResourceChild::equalId(dword dwId) const
	{
		return entry.irde.Name == dwId;
	}
	
	/**
	* Compares the resource child's name to the parameter strName.
	* @param strName ID of a resource.
	* @return True, if the resource child's name equals the parameter.
	**/
	bool ResourceChild::equalName(std::string strName) const
	{
		return entry.wstrName == strName;
	}
	
	/**
	* Returns true if the resource was given a name.
	**/
	bool ResourceChild::isNamedResource() const
	{
		return entry.wstrName.size() != 0;
	}
	
	/**
	* The children of a resource must be ordered in a certain way. First come the named resources
	* in sorted order, afterwards followed the unnamed resources in sorted order.
	**/
	bool ResourceChild::operator<(const ResourceChild& rc) const
	{
		if (this->isNamedResource() && !rc.isNamedResource())
		{
			return true;
		}
		else if (!this->isNamedResource() && rc.isNamedResource())
		{
			return false;
		}
		else if (this->isNamedResource() && rc.isNamedResource())
		{
			return this->entry.wstrName < rc.entry.wstrName;
		}
		else
		{
			return this->entry.irde.Name < rc.entry.irde.Name;
		}
	}
	
/*	unsigned int ResourceChild::size() const
	{
		return PELIB_IMAGE_RESOURCE_DIRECTORY_ENTRY::size()
		       + child->size()
		       + (entry.wstrName.size() ? entry.wstrName.size() + 2 : 0);
	}
*/
// -------------------------------------------------- ResourceElement -------------------------------------------

	/**
	* Returns the RVA of a ResourceElement. This is the RVA where the ResourceElement can be
	* found in the file.
	* @return RVA of the ResourceElement.
	**/
	unsigned int ResourceElement::getElementRva() const
	{
		return uiElementRva;
	}

// -------------------------------------------------- ResourceLeaf -------------------------------------------

	/**
	* Checks if a ResourceElement is a leaf or not.
	* @return Always returns true.
	**/
	bool ResourceLeaf::isLeaf() const
	{
		return true;
	}
	
	/**
	* Reads the next resource leaf from the InputBuffer.
	* @param inpBuffer An InputBuffer that holds the complete resource directory.
	* @param uiOffset Offset of the resource leaf that's to be read.
	* @param uiRva RVA of the beginning of the resource directory.
	* @param pad Used for debugging purposes.
	**/
	int ResourceLeaf::read(InputBuffer& inpBuffer, unsigned int uiOffset, unsigned int uiRva/*, const std::string& pad*/)
	{
//		std::cout << pad << "Leaf:" << std::endl;
		
		// Invalid leaf.
		if (uiOffset + PELIB_IMAGE_RESOURCE_DATA_ENTRY::size() > inpBuffer.size())
		{
			return 1;
		}

		uiElementRva = uiOffset + uiRva;

		inpBuffer.set(uiOffset);
		
		inpBuffer >> entry.OffsetToData;
		inpBuffer >> entry.Size;
		inpBuffer >> entry.CodePage;
		inpBuffer >> entry.Reserved;

/*		std::cout << pad << std::hex << "Offset: " << entry.OffsetToData << std::endl;
		std::cout << pad << std::hex << "Size: " << entry.Size << std::endl;
		std::cout << pad << std::hex << "CodePage: " << entry.CodePage << std::endl;
		std::cout << pad << std::hex << "Reserved: " << entry.Reserved << std::endl;
*/		
		// Invalid leaf.
		if (entry.OffsetToData - uiRva + entry.Size > inpBuffer.size())
		{
//			std::cout << entry.OffsetToData << " XXX " << uiRva << " - " << entry.Size << " - " << inpBuffer.size() << std::endl;
			return 1;
		}
		
//		std::cout << entry.OffsetToData << " - " << uiRva << " - " << entry.Size << " - " << inpBuffer.size() << std::endl;
		inpBuffer.set(entry.OffsetToData - uiRva);
		m_data.assign(inpBuffer.data() + inpBuffer.get(), inpBuffer.data() + inpBuffer.get() + entry.Size);
//		std::cout << pad << std::hex << "Vector: " << m_data.size() << std::endl;
//		std::copy(m_data.begin(), m_data.end(), std::ostream_iterator<int>(std::cout << std::hex, " "));
		return 0;
	}

	/**
	* Rebuilds the current resource leaf.
	* @param obBuffer OutputBuffer where the rebuilt resource leaf is stored.
	* @param uiOffset Offset of the resource leaf inside the resource directory.
	* @param uiRva RVA of the resource directory.
	**/
	void ResourceLeaf::rebuild(OutputBuffer& obBuffer, unsigned int& uiOffset, unsigned int uiRva, const std::string&) const
	{
//		std::cout << std::hex << pad << "Leaf: " << uiOffset << std::endl;
		uiOffset += PELIB_IMAGE_RESOURCE_DATA_ENTRY::size();
		
//		obBuffer << entry.OffsetToData;
//		obBuffer << uiOffset;
		obBuffer << uiRva + uiOffset + (uiOffset % 4);
		obBuffer << entry.Size;
		obBuffer << entry.CodePage;
		obBuffer << entry.Reserved;
		
		while (uiOffset % 4)
		{
			uiOffset++;
			obBuffer << (byte)0;
		}
		
		uiOffset += static_cast<unsigned int>(m_data.size());

		for (unsigned int i=0;i<m_data.size();i++)
		{
			obBuffer << m_data[i];
		}
//		std::cout << "LeafChild: " << std::endl;
	}

	void ResourceLeaf::makeValid()
	{
		entry.Size = static_cast<unsigned int>(m_data.size());
	}
	
/*	/// Returns the size of a resource leaf.
	unsigned int ResourceLeaf::size() const
	{
		return PELIB_IMAGE_RESOURCE_DATA_ENTRY::size() + m_data.size();
	}
*/

	/**
	* Returns a vector that contains the raw data of a resource leaf.
	* @return Raw data of the resource.
	**/
	std::vector<byte> ResourceLeaf::getData() const
	{
		return m_data;
	}
	
	/**
	* Overwrites the raw data of a resource.
	* @param vData New data of the resource.
	**/
	void ResourceLeaf::setData(const std::vector<byte>& vData)
	{
		m_data = vData;
	}

	/**
	* Returns the leaf's OffsetToData value. That's the RVA where the raw data of the resource
	* can be found.
	* @return The leaf's OffsetToData value.
	**/
	dword ResourceLeaf::getOffsetToData() const
	{
		return entry.OffsetToData;
	}
	
	/**
	* Returns the leaf's Size value. That's the size of the raw data of the resource.
	* @return The leaf's Size value.
	**/
	dword ResourceLeaf::getSize() const
	{
		return entry.Size;
	}
	
	/**
	* Returns the leaf's CodePage value.
	* @return The leaf's CodePage value.
	**/
	dword ResourceLeaf::getCodePage() const
	{
		return entry.CodePage;
	}
	
	/**
	* Returns the leaf's Reserved value.
	* @return The leaf's Reserved value.
	**/
	dword ResourceLeaf::getReserved() const
	{
		return entry.Reserved;
	}
	
	/**
	* Sets the leaf's OffsetToData value.
	* @param dwValue The leaf's new OffsetToData value.
	**/
	void ResourceLeaf::setOffsetToData(dword dwValue)
	{
		entry.OffsetToData = dwValue;
	}
	
	/**
	* Sets the leaf's Size value.
	* @param dwValue The leaf's new Size value.
	**/
	void ResourceLeaf::setSize(dword dwValue)
	{
		entry.Size = dwValue;
	}
	
	/**
	* Sets the leaf's CodePage value.
	* @param dwValue The leaf's new CodePage value.
	**/
	void ResourceLeaf::setCodePage(dword dwValue)
	{
		entry.CodePage = dwValue;
	}
	
	/**
	* Sets the leaf's Reserved value.
	* @param dwValue The leaf's new Reserved value.
	**/
	void ResourceLeaf::setReserved(dword dwValue)
	{
		entry.Reserved = dwValue;
	}
	

// -------------------------------------------------- ResourceNode -------------------------------------------

	/**
	* Checks if a ResourceElement is a leaf or not.
	* @return Always returns false.
	**/
	bool ResourceNode::isLeaf() const
	{
		return false;
	}
	
	/**
	* Sorts the node's children and corrects the node's header.
	**/
	void ResourceNode::makeValid()
	{
		std::sort(children.begin(), children.end());
		header.NumberOfNamedEntries = static_cast<PeLib::word>(std::count_if(children.begin(), children.end(), std::mem_fun_ref(&ResourceChild::isNamedResource)));
		header.NumberOfIdEntries = static_cast<unsigned int>(children.size()) - header.NumberOfNamedEntries;
	}

	/**
	* Rebuilds the current resource node.
	* @param obBuffer OutputBuffer where the rebuilt resource node is stored.
	* @param uiOffset Offset of the resource node inside the resource directory.
	* @param uiRva RVA of the resource directory.
	* @param pad Used for debugging.
	**/
	void ResourceNode::rebuild(OutputBuffer& obBuffer, unsigned int& uiOffset, unsigned int uiRva, const std::string& pad) const
	{
/*		std::cout << std::hex << pad << uiOffset << std::endl;
		
		std::cout << std::hex << pad << "header.Characteristics: " << header.Characteristics << std::endl;
		std::cout << std::hex << pad << "header.TimeDateStamp: " << header.TimeDateStamp << std::endl;
		std::cout << std::hex << pad << "header.MajorVersion: "  << header.MajorVersion << std::endl;
		std::cout << std::hex << pad << "header.MinorVersion: "  << header.MinorVersion << std::endl;
		std::cout << std::hex << pad << "header.NumberOfNamedEntries: "  << header.NumberOfNamedEntries << std::endl;
		std::cout << std::hex << pad << "header.NumberOfIdEntries: "  << header.NumberOfIdEntries << std::endl;
*/		
		obBuffer << header.Characteristics;
		obBuffer << header.TimeDateStamp;
		obBuffer << header.MajorVersion;
		obBuffer << header.MinorVersion;
		//std::cout << pad << "Children: " << children.size() << std::endl;
		//std::cout << pad << std::count_if(children.begin(), children.end(), std::mem_fun_ref(&ResourceChild::isNamedResource)) << std::endl;
		//std::cout << pad << children.size() - std::count_if(children.begin(), children.end(), std::mem_fun_ref(&ResourceChild::isNamedResource)) << std::endl;
//		obBuffer << std::count_if(children.begin(), children.end(), std::mem_fun_ref(&ResourceChild::isNamedResource));
//		obBuffer << children.size() - std::count_if(children.begin(), children.end(), std::mem_fun_ref(&ResourceChild::isNamedResource));
		obBuffer << header.NumberOfNamedEntries;
		obBuffer << header.NumberOfIdEntries;

		uiOffset += PELIB_IMAGE_RESOURCE_DIRECTORY::size();
		
		for (unsigned int i=0;i<children.size();i++)
		{
			obBuffer << (dword)0;
			obBuffer << (dword)0;
		}

		unsigned int newoffs = (uiOffset + children.size() * 8);
		
		for (unsigned int i=0;i<children.size();i++)
		{
			if (!children[i].entry.wstrName.size())
			{
				obBuffer.update(uiOffset, children[i].entry.irde.Name);
			}
			else
			{
				obBuffer.update(uiOffset, newoffs | PELIB_IMAGE_RESOURCE_NAME_IS_STRING);
				obBuffer << (word)children[i].entry.wstrName.size();
				newoffs += 2;
				for (unsigned int j=0;j<children[i].entry.wstrName.size();j++)
				{
//					std::cout << children[i].entry.wstrName[j];
					obBuffer << (word)children[i].entry.wstrName[j];
					newoffs += 2;
				}
			}
			uiOffset += 4;
//			obBuffer << children[i].entry.OffsetToData;
			obBuffer.update(uiOffset, newoffs | (children[i].entry.irde.OffsetToData & PELIB_IMAGE_RESOURCE_DATA_IS_DIRECTORY));
			uiOffset += 4;
			children[i].child->rebuild(obBuffer, newoffs, uiRva, pad + "  ");
		}
		uiOffset = newoffs;
	}

	/**
	* Reads the next resource node from the InputBuffer.
	* @param inpBuffer An InputBuffer that holds the complete resource directory.
	* @param uiOffset Offset of the resource node that's to be read.
	* @param uiRva RVA of the beginning of the resource directory.
	* @param pad Something I need for debugging. Will be removed soon.
	**/
	int ResourceNode::read(InputBuffer& inpBuffer, unsigned int uiOffset, unsigned int uiRva/*, const std::string& pad*/)
	{
		// Not enough space to be a valid node.
		if (uiOffset + PELIB_IMAGE_RESOURCE_DIRECTORY::size() > inpBuffer.size())
		{
			return 1;
		}
		
		uiElementRva = uiOffset + uiRva;
		
		inpBuffer.set(uiOffset);
		
		inpBuffer >> header.Characteristics;
		inpBuffer >> header.TimeDateStamp;
		inpBuffer >> header.MajorVersion;
		inpBuffer >> header.MinorVersion;
		inpBuffer >> header.NumberOfNamedEntries;
		inpBuffer >> header.NumberOfIdEntries;
			
		// Not enough space to be a valid node.
		if (uiOffset + PELIB_IMAGE_RESOURCE_DIRECTORY::size()
		   + (header.NumberOfNamedEntries + header.NumberOfIdEntries) * PELIB_IMAGE_RESOURCE_DIRECTORY_ENTRY::size()
		   > inpBuffer.size())
		{
			return 1;
		}
		
//		std::cout << std::hex << pad << "Characteristics: " << header.Characteristics << std::endl;
//		std::cout << std::hex << pad << "TimeDateStamp: " << header.TimeDateStamp << std::endl;
//		std::cout << std::hex << pad << "MajorVersion: " << header.MajorVersion << std::endl;
//		std::cout << std::hex << pad << "MinorVersion: " << header.MinorVersion << std::endl;
//		std::cout << std::hex << pad << "NumberOfNamedEntries: " << header.NumberOfNamedEntries << std::endl;
//		std::cout << std::hex << pad << "NumberOfIdEntries: " << header.NumberOfIdEntries << std::endl;
			
		for (int i=0;i<header.NumberOfNamedEntries + header.NumberOfIdEntries;i++)
		{
			ResourceChild rc;
			inpBuffer >> rc.entry.irde.Name;
			inpBuffer >> rc.entry.irde.OffsetToData;
			
			unsigned int lastPos = inpBuffer.get();

			if (rc.entry.irde.Name & PELIB_IMAGE_RESOURCE_NAME_IS_STRING)
			{
				// Enough space to read string length?
				if ((rc.entry.irde.Name & ~PELIB_IMAGE_RESOURCE_NAME_IS_STRING) + 2 < inpBuffer.size())
				{
					inpBuffer.set(rc.entry.irde.Name & ~PELIB_IMAGE_RESOURCE_NAME_IS_STRING);
					word strlen;
					inpBuffer >> strlen;
					
					// Enough space to read string?
					if ((rc.entry.irde.Name & ~PELIB_IMAGE_RESOURCE_NAME_IS_STRING) + 2 * strlen < inpBuffer.size())
					{
						wchar_t c;
						for (word i=0;i<strlen;i++)
						{
							inpBuffer >> c;
							rc.entry.wstrName += c;
						}
					}
				}
				
//				std::wcout << rc.entry.wstrName << std::endl;
				
//				std::cout << strlen << std::endl;
				inpBuffer.set(lastPos);
			}
			
			if (rc.entry.irde.OffsetToData & PELIB_IMAGE_RESOURCE_DATA_IS_DIRECTORY)
			{
				rc.child = new ResourceNode;
				rc.child->read(inpBuffer, rc.entry.irde.OffsetToData & ~PELIB_IMAGE_RESOURCE_DATA_IS_DIRECTORY, uiRva/*, pad + "  "*/);
			}
			else
			{
				rc.child = new ResourceLeaf;
				rc.child->read(inpBuffer, rc.entry.irde.OffsetToData, uiRva/*, pad + "  "*/);
			}
//			std::cout << std::hex << pad << "Entry " << i << "(Name): " << rc.entry.irde.Name << std::endl;
//			std::cout << std::hex << pad << "Entry " << i << "(Offset): " << rc.entry.irde.OffsetToData << std::endl;

			children.push_back(rc);
			inpBuffer.set(lastPos);
		}
		
		return 0;
	}
	
	/**
	* Returns the number of children of the current node. Note that this number is the number
	* of defined children, not the value from the header.
	* @return Number of node's children.
	**/
	unsigned int ResourceNode::getNumberOfChildren() const
	{
		return static_cast<unsigned int>(children.size());
	}
	
	/**
	* Adds another child to the current node.
	**/
	void ResourceNode::addChild()
	{
		ResourceChild c;
		c.child = 0;
		children.push_back(c);
	}
		  
	/**
	* Returns a node's child.
	* @param uiIndex Index of the child.
	* @return The child identified by uiIndex. This child can be either a ResourceNode or a ResourceLeaf.
	**/
	ResourceElement* ResourceNode::getChild(unsigned int uiIndex)
	{
		return children[uiIndex].child;
	}
	
	/**
	* Removes a child from the current node.
	* @param uiIndex Index of the child.
	**/
	void ResourceNode::removeChild(unsigned int uiIndex)
	{
		children.erase(children.begin() + uiIndex);
	}
		  
	/**
	* Returns the name of a child.
	* @param uiIndex Index of the child.
	* @return Either the name of the specified child or an empty string.
	**/
	std::string ResourceNode::getChildName(unsigned int uiIndex) const
	{
		return children[uiIndex].entry.wstrName;
	}
	
	/**
	* Returns the Name value of a child.
	* @param uiIndex Index of the child.
	* @return Name value of a child.
	**/
	dword ResourceNode::getOffsetToChildName(unsigned int uiIndex) const
	{
		return children[uiIndex].entry.irde.Name;
	}
	
	/**
	* Returns the OffsetToData value of a child.
	* @param uiIndex Index of the child.
	* @return OffsetToData value of a child.
	**/
	dword ResourceNode::getOffsetToChildData(unsigned int uiIndex) const
	{
		return children[uiIndex].entry.irde.OffsetToData;
	}
	
		  
	/**
	* Sets the name of a child.
	* @param uiIndex Index of the child.
	* @param strNewName New name of the resource.
	**/
	void ResourceNode::setChildName(unsigned int uiIndex, const std::string& strNewName)
	{
		children[uiIndex].entry.wstrName = strNewName;
	}
	
	/**
	* Sets the Name value of a child.
	* @param uiIndex Index of the child.
	* @param dwNewOffset New Name value of the resource.
	**/
	void ResourceNode::setOffsetToChildName(unsigned int uiIndex, dword dwNewOffset)
	{
		children[uiIndex].entry.irde.Name = dwNewOffset;
	}
	
	/**
	* Sets the OffsetToData value of a child.
	* @param uiIndex Index of the child.
	* @param dwNewOffset New OffsetToData value of the resource.
	**/
	void ResourceNode::setOffsetToChildData(unsigned int uiIndex, dword dwNewOffset)
	{
		children[uiIndex].entry.irde.OffsetToData = dwNewOffset;
	}
		  
	/**
	* Returns the Characteristics value of the node.
	* @return Characteristics value of the node.
	**/
	dword ResourceNode::getCharacteristics() const
	{
		return header.Characteristics;
	}
	
	/**
	* Returns the TimeDateStamp value of the node.
	* @return TimeDateStamp value of the node.
	**/
	dword ResourceNode::getTimeDateStamp() const
	{
		return header.TimeDateStamp;
	}
	
	/**
	* Returns the MajorVersion value of the node.
	* @return MajorVersion value of the node.
	**/
	word ResourceNode::getMajorVersion() const
	{
		return header.MajorVersion;
	}
	
	/**
	* Returns the MinorVersion value of the node.
	* @return MinorVersion value of the node.
	**/
	word ResourceNode::getMinorVersion() const
	{
		return header.MinorVersion;
	}
	
	/**
	* Returns the NumberOfNamedEntries value of the node.
	* @return NumberOfNamedEntries value of the node.
	**/
	word ResourceNode::getNumberOfNamedEntries() const
	{
		return header.NumberOfNamedEntries;
	}
	
	/**
	* Returns the NumberOfIdEntries value of the node.
	* @return NumberOfIdEntries value of the node.
	**/
	word ResourceNode::getNumberOfIdEntries() const
	{
		return header.NumberOfIdEntries;
	}
		  
	/**
	* Sets the Characteristics value of the node.
	* @param value New Characteristics value of the node.
	**/
	void ResourceNode::setCharacteristics(dword value)
	{
		header.Characteristics = value;
	}
	
	/**
	* Sets the TimeDateStamp value of the node.
	* @param value New TimeDateStamp value of the node.
	**/
	void ResourceNode::setTimeDateStamp(dword value)
	{
		header.TimeDateStamp = value;
	}
	
	/**
	* Sets the MajorVersion value of the node.
	* @param value New MajorVersion value of the node.
	**/
	void ResourceNode::setMajorVersion(word value)
	{
		header.MajorVersion = value;
	}
	
	/**
	* Sets the MinorVersion value of the node.
	* @param value New MinorVersion value of the node.
	**/
	void ResourceNode::setMinorVersion(word value)
	{
		header.MinorVersion = value;
	}
	
	/**
	* Sets the NumberOfNamedEntries value of the node.
	* @param value New NumberOfNamedEntries value of the node.
	**/
	void ResourceNode::setNumberOfNamedEntries(word value)
	{
		header.NumberOfNamedEntries = value;
	}
	
	/**
	* Sets the NumberOfIdEntries value of the node.
	* @param value New NumberOfIdEntries value of the node.
	**/
	void ResourceNode::setNumberOfIdEntries(word value)
	{
		header.NumberOfIdEntries = value;
	}
	
		  
/*	/// Returns the size of a resource node.
	unsigned int ResourceNode::size() const
	{
		if (children.size())
		{
			std::cout << std::accumulate(children.begin(), children.end(), 0, accumulate<ResourceChild>) << std::endl;
			return PELIB_IMAGE_RESOURCE_DIRECTORY::size()
			         + std::accumulate(children.begin(), children.end(), 0, accumulate<ResourceChild>);
		}
		else
		{
			return 0;
		}
	}
*/	
// -------------------------------------------------- ResourceDirectory -------------------------------------------
	
	/**
	* Returns the root node of the resource directory.
	* @return Root node of the resource directory.
	**/
	ResourceNode* ResourceDirectory::getRoot()
	{
		return &m_rnRoot;
	}

	/**
	* Correctly sorts the resource nodes of the resource tree. This function should be called
	* before calling rebuild.
	**/
	void ResourceDirectory::makeValid()
	{
		m_rnRoot.makeValid();
	}

	/**
	* Reads the resource directory from a file.
	* @param strFilename Name of the file.
	* @param uiOffset File offset of the resource directory.
	* @param uiSize Raw size of the resource directory.
	* @param uiResDirRva RVA of the beginning of the resource directory.
	**/
	int ResourceDirectory::read(const std::string& strFilename, unsigned int uiOffset, unsigned int uiSize, unsigned int uiResDirRva)
	{
		if (!uiSize || !uiOffset)
		{
			return 1;
		}
		
		std::ifstream ifFile(strFilename.c_str(), std::ios::binary);
		
		if (!ifFile)
		{
//			throw Exceptions::CannotOpenFile(ResourceDirectoryId, __LINE__);
			return 1;
		}
		
		if (fileSize(ifFile) < uiOffset + uiSize)
		{
//			throw Exceptions::InvalidFormat(ResourceDirectoryId, __LINE__);
			return 1;
		}

		ifFile.seekg(uiOffset, std::ios::beg);
		
		PELIB_IMAGE_RESOURCE_DIRECTORY irdCurrRoot;
		
		std::vector<byte> vResourceDirectory(uiSize);
		ifFile.read(reinterpret_cast<char*>(&vResourceDirectory[0]), uiSize);

		InputBuffer inpBuffer(vResourceDirectory);
		
//		ResourceNode currNode;
		return m_rnRoot.read(inpBuffer, 0, uiResDirRva/*, ""*/);
//		std::swap(currNode, m_rnRoot);
	}

	/**
	* Rebuilds the resource directory.
	* @param vBuffer Buffer the source directory will be written to.
	* @param uiRva RVA of the resource directory.
	**/
	void ResourceDirectory::rebuild(std::vector<byte>& vBuffer, unsigned int uiRva) const
	{
		OutputBuffer obBuffer(vBuffer);
		unsigned int offs = 0;
//		std::cout << "Root: " << m_rnRoot.children.size() << std::endl;
		m_rnRoot.rebuild(obBuffer, offs, uiRva, "");
	}
	
	/**
	* Returns the size of the entire rebuilt resource directory. That's the size of the entire
	* structure as it's written back to a file.
	**/
/*	unsigned int ResourceDirectory::size() const
	{
		return m_rnRoot.size();
	}
*/	
	/**
	* Writes the current resource directory back into a file.
	* @param strFilename Name of the output file.
	* @param uiOffset File offset where the resource directory will be written to.
	* @param uiRva RVA of the file offset.
	**/
	int ResourceDirectory::write(const std::string& strFilename, unsigned int uiOffset, unsigned int uiRva) const
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
		rebuild(vBuffer, uiRva);

		ofFile.write(reinterpret_cast<const char*>(&vBuffer[0]), static_cast<unsigned int>(vBuffer.size()));

		ofFile.close();
		
		return 0;
	}
	
	/**
	* Adds another resource type. The new resource type is identified by the ID dwResTypeId.
	* @param dwResTypeId ID which identifies the resource type.
	**/
	int ResourceDirectory::addResourceType(dword dwResTypeId)
	{
		std::vector<ResourceChild>::iterator Iter = std::find_if(m_rnRoot.children.begin(), m_rnRoot.children.end(), std::bind2nd(std::mem_fun_ref(&ResourceChild::equalId), dwResTypeId));
		if (Iter != m_rnRoot.children.end())
		{
			return 1;
			// throw Exceptions::EntryAlreadyExists(ResourceDirectoryId, __LINE__);
		}
		
		ResourceChild rcCurr;
		rcCurr.child = new ResourceNode;
		rcCurr.entry.irde.Name = dwResTypeId;
		m_rnRoot.children.push_back(rcCurr);
		
		return 0;
	}
	
	/**
	* Adds another resource type. The new resource type is identified by the name strResTypeName.
	* @param strResTypeName Name which identifies the resource type.
	**/
	int ResourceDirectory::addResourceType(const std::string& strResTypeName)
	{
		std::vector<ResourceChild>::iterator Iter = std::find_if(m_rnRoot.children.begin(), m_rnRoot.children.end(), std::bind2nd(std::mem_fun_ref(&ResourceChild::equalName), strResTypeName));
		if (Iter != m_rnRoot.children.end())
		{
			return 1;
//			throw Exceptions::EntryAlreadyExists(ResourceDirectoryId, __LINE__);
		}
		
		ResourceChild rcCurr;
		rcCurr.entry.wstrName = strResTypeName;
		rcCurr.child = new ResourceNode;
		m_rnRoot.children.push_back(rcCurr);
		
		return 0;
	}
	
	/**
	* Removes the resource type identified by the ID dwResTypeId.
	* @param dwResTypeId ID which identifies the resource type.
	**/
	int ResourceDirectory::removeResourceType(dword dwResTypeId)
	{
		std::vector<ResourceChild>::iterator Iter = std::find_if(m_rnRoot.children.begin(), m_rnRoot.children.end(), std::bind2nd(std::mem_fun_ref(&ResourceChild::equalId), dwResTypeId));
		if (Iter == m_rnRoot.children.end())
		{
			return 1;
//			throw Exceptions::ResourceTypeDoesNotExist(ResourceDirectoryId, __LINE__);
		}
		
		bool isNamed = false;
		if (Iter->isNamedResource()) isNamed = true;

		m_rnRoot.children.erase(Iter);
		
		if (isNamed) m_rnRoot.header.NumberOfNamedEntries = static_cast<PeLib::word>(m_rnRoot.children.size());
		else m_rnRoot.header.NumberOfIdEntries = static_cast<PeLib::word>(m_rnRoot.children.size());
		
		return 0;
	}
	
	/**
	* Removes the resource type identified by the name strResTypeName.
	* @param strResTypeName Name which identifies the resource type.
	**/
	int ResourceDirectory::removeResourceType(const std::string& strResTypeName)
	{
		std::vector<ResourceChild>::iterator Iter = std::find_if(m_rnRoot.children.begin(), m_rnRoot.children.end(), std::bind2nd(std::mem_fun_ref(&ResourceChild::equalName), strResTypeName));
		if (Iter == m_rnRoot.children.end())
		{
			return 1;
		//	throw Exceptions::ResourceTypeDoesNotExist(ResourceDirectoryId, __LINE__);
		}
		
		bool isNamed = false;
		if (Iter->isNamedResource()) isNamed = true;
		
		m_rnRoot.children.erase(Iter);
		
		if (isNamed) m_rnRoot.header.NumberOfNamedEntries = static_cast<PeLib::word>(m_rnRoot.children.size());
		else m_rnRoot.header.NumberOfIdEntries = static_cast<PeLib::word>(m_rnRoot.children.size());
		
		return 0;
	}

	/**
	* Removes the resource type identified by the index uiIndex.
	* @param uiIndex Index which identifies the resource type.
	**/
	int ResourceDirectory::removeResourceTypeByIndex(unsigned int uiIndex)
	{
		bool isNamed = false;
		if (m_rnRoot.children[uiIndex].isNamedResource()) isNamed = true;
		
		m_rnRoot.children.erase(m_rnRoot.children.begin() + uiIndex);
		
		if (isNamed) m_rnRoot.header.NumberOfNamedEntries = static_cast<PeLib::word>(m_rnRoot.children.size());
		else m_rnRoot.header.NumberOfIdEntries = static_cast<PeLib::word>(m_rnRoot.children.size());
		
		return 0;
	}
		  
	/**
	* Adds another resource to the resource tree. The first parameter identifies the resource type
	* of the new resource, the second parameter identifies the resource itself.
	* @param dwResTypeId ID of the resource type.
	* @param dwResId ID of the resource.
	**/
	int ResourceDirectory::addResource(dword dwResTypeId, dword dwResId)
	{
		ResourceChild rcCurr;
		rcCurr.entry.irde.Name = dwResId;
		return addResourceT(dwResTypeId, dwResId, rcCurr);
	}
	
	/**
	* Adds another resource to the resource tree. The first parameter identifies the resource type
	* of the new resource, the second parameter identifies the resource itself.
	* @param dwResTypeId ID of the resource type.
	* @param strResName Name of the resource.
	**/
	int ResourceDirectory::addResource(dword dwResTypeId, const std::string& strResName)
	{
		ResourceChild rcCurr;
		rcCurr.entry.wstrName = strResName;
		return addResourceT(dwResTypeId, strResName, rcCurr);
	}
	
	/**
	* Adds another resource to the resource tree. The first parameter identifies the resource type
	* of the new resource, the second parameter identifies the resource itself.
	* @param strResTypeName Name of the resource type.
	* @param dwResId ID of the resource.
	**/
	int ResourceDirectory::addResource(const std::string& strResTypeName, dword dwResId)
	{
		ResourceChild rcCurr;
		rcCurr.entry.irde.Name = dwResId;
		return addResourceT(strResTypeName, dwResId, rcCurr);
	}

	/**
	* Adds another resource to the resource tree. The first parameter identifies the resource type
	* of the new resource, the second parameter identifies the resource itself.
	* @param strResTypeName Name of the resource type.
	* @param strResName Name of the resource.
	**/
	int ResourceDirectory::addResource(const std::string& strResTypeName, const std::string& strResName)
	{
		ResourceChild rcCurr;
		rcCurr.entry.wstrName = strResName;
		return addResourceT(strResTypeName, strResName, rcCurr);
	}

	/**
	* Removes a resource from the resource tree. The first parameter identifies the resource type
	* of the new resource, the second parameter identifies the resource itself.
	* @param dwResTypeIndex ID of the resource type.
	* @param dwResId ID of the resource.
	**/
	int ResourceDirectory::removeResource(dword dwResTypeIndex, dword dwResId)
	{
		return removeResourceT(dwResTypeIndex, dwResId);
	}
	
	/**
	* Removes a resource from the resource tree. The first parameter identifies the resource type
	* of the new resource, the second parameter identifies the resource itself.
	* @param dwResTypeIndex ID of the resource type.
	* @param strResName Name of the resource.
	**/
	int ResourceDirectory::removeResource(dword dwResTypeIndex, const std::string& strResName)
	{
		return removeResourceT(dwResTypeIndex, strResName);
	}
	
	/**
	* Removes a resource from the resource tree. The first parameter identifies the resource type
	* of the new resource, the second parameter identifies the resource itself.
	* @param strResTypeName Name of the resource type.
	* @param dwResId ID of the resource.
	**/
	int ResourceDirectory::removeResource(const std::string& strResTypeName, dword dwResId)
	{
		return removeResourceT(strResTypeName, dwResId);
	}
	
	/**
	* Removes a resource from the resource tree. The first parameter identifies the resource type
	* of the new resource, the second parameter identifies the resource itself.
	* @param strResTypeName Name of the resource type.
	* @param strResName Name of the resource.
	**/
	int ResourceDirectory::removeResource(const std::string& strResTypeName, const std::string& strResName)
	{
		return removeResourceT(strResTypeName, strResName);
	}
		  
	/**
	* Returns the number of resource types.
	**/
	unsigned int ResourceDirectory::getNumberOfResourceTypes() const
	{
		return static_cast<unsigned int>(m_rnRoot.children.size());
	}

	/**
	* Returns the ID of a resource type which was specified through an index.
	* The valid range of the parameter uiIndex is 0...getNumberOfResourceTypes() - 1.
	* Leaving the invalid range leads to undefined behaviour.
	* @param uiIndex Index which identifies a resource type.
	* @return The ID of the specified resource type.
	**/
	dword ResourceDirectory::getResourceTypeIdByIndex(unsigned int uiIndex) const
	{
		return m_rnRoot.children[uiIndex].entry.irde.Name;
	}
	
	/**
	* Returns the name of a resource type which was specified through an index.
	* The valid range of the parameter uiIndex is 0...getNumberOfResourceTypes() - 1.
	* Leaving the invalid range leads to undefined behaviour.
	* @param uiIndex Index which identifies a resource type.
	* @return The name of the specified resource type.
	**/
	std::string ResourceDirectory::getResourceTypeNameByIndex(unsigned int uiIndex) const
	{
		return m_rnRoot.children[uiIndex].entry.wstrName;
	}

	/**
	* Converts the ID of a resource type to an index.
	* @param dwResTypeId ID of the resource type.
	* @return Index of that resource type.
	**/
	int ResourceDirectory::resourceTypeIdToIndex(dword dwResTypeId) const
	{
		std::vector<ResourceChild>::const_iterator Iter = std::find_if(m_rnRoot.children.begin(), m_rnRoot.children.end(), std::bind2nd(std::mem_fun_ref(&ResourceChild::equalId), dwResTypeId));
		if (Iter == m_rnRoot.children.end()) return -1;
		return static_cast<unsigned int>(std::distance(m_rnRoot.children.begin(), Iter));
	}
	
	/**
	* Converts the name of a resource type to an index.
	* @param strResTypeName ID of the resource type.
	* @return Index of that resource type.
	**/
	int ResourceDirectory::resourceTypeNameToIndex(const std::string& strResTypeName) const
	{
		std::vector<ResourceChild>::const_iterator Iter = std::find_if(m_rnRoot.children.begin(), m_rnRoot.children.end(), std::bind2nd(std::mem_fun_ref(&ResourceChild::equalName), strResTypeName));
		if (Iter == m_rnRoot.children.end()) return -1;
		return static_cast<unsigned int>(std::distance(m_rnRoot.children.begin(), Iter));
	}
	
	/**
	* Returns the number of resources of a specific resource type.
	* @param dwId ID of the resource type.
	* @return Number of resources of resource type dwId.
	**/
	unsigned int ResourceDirectory::getNumberOfResources(dword dwId) const
	{
//		std::vector<ResourceChild>::const_iterator IterD = m_rnRoot.children.begin();
//		std::cout << dwId << std::endl;
//		while (IterD != m_rnRoot.children.end())
//		{
//			std::cout << IterD->entry.irde.Name << std::endl;
//			++IterD;
//		}
		
		std::vector<ResourceChild>::const_iterator Iter = std::find_if(m_rnRoot.children.begin(), m_rnRoot.children.end(), std::bind2nd(std::mem_fun_ref(&ResourceChild::equalId), dwId));
		if (Iter == m_rnRoot.children.end())
		{
			return 0xFFFFFFFF;
		}
		else
		{
			ResourceNode* currNode = static_cast<ResourceNode*>(Iter->child);
			return static_cast<unsigned int>(currNode->children.size());
		}
	}

	/**
	* Returns the number of resources of a specific resource type.
	* @param strResTypeName Name of the resource type.
	* @return Number of resources of resource type strResTypeName.
	**/
	unsigned int ResourceDirectory::getNumberOfResources(const std::string& strResTypeName) const
	{
		std::vector<ResourceChild>::const_iterator Iter = std::find_if(m_rnRoot.children.begin(), m_rnRoot.children.end(), std::bind2nd(std::mem_fun_ref(&ResourceChild::equalName), strResTypeName));
		if (Iter == m_rnRoot.children.end())
		{
			return 0xFFFFFFFF;
		}
		else
		{
			ResourceNode* currNode = static_cast<ResourceNode*>(Iter->child);
			return static_cast<unsigned int>(currNode->children.size());
		}
	}

	/**
	* Returns the number of resources of a resource type which was specified through an index.
	* The valid range of the parameter uiIndex is 0...getNumberOfResourceTypes() - 1.
	* Leaving the invalid range leads to undefined behaviour.
	* @param uiIndex Index which identifies a resource type.
	* @return The number of resources of the specified resource type.
	**/
	unsigned int ResourceDirectory::getNumberOfResourcesByIndex(unsigned int uiIndex) const
	{
		ResourceNode* currNode = static_cast<ResourceNode*>(m_rnRoot.children[uiIndex].child);
		return static_cast<unsigned int>(currNode->children.size());
	}
	
	/**
	* Gets the resource data of a specific resource.
	* @param dwResTypeId Identifies the resource type of the resource.
	* @param dwResId Identifies the resource.
	* @param data Vector where the data is stored.
	**/
	void ResourceDirectory::getResourceData(dword dwResTypeId, dword dwResId, std::vector<byte>& data) const
	{
		getResourceDataT(dwResTypeId, dwResId, data);
	}
	
	/**
	* Gets the resource data of a specific resource.
	* @param dwResTypeId Identifies the resource type of the resource.
	* @param strResName Identifies the resource.
	* @param data Vector where the data is stored.
	**/
	void ResourceDirectory::getResourceData(dword dwResTypeId, const std::string& strResName, std::vector<byte>& data) const
	{
		getResourceDataT(dwResTypeId, strResName, data);
	}
	
	/**
	* Gets the resource data of a specific resource.
	* @param strResTypeName Identifies the resource type of the resource.
	* @param dwResId Identifies the resource.
	* @param data Vector where the data is stored.
	**/
	void ResourceDirectory::getResourceData(const std::string& strResTypeName, dword dwResId, std::vector<byte>& data) const
	{
		getResourceDataT(strResTypeName, dwResId, data);
	}
	
	/**
	* Gets the resource data of a specific resource.
	* @param strResTypeName Identifies the resource type of the resource.
	* @param strResName Identifies the resource.
	* @param data Vector where the data is stored.
	**/
	void ResourceDirectory::getResourceData(const std::string& strResTypeName, const std::string& strResName, std::vector<byte>& data) const
	{
		getResourceDataT(strResTypeName, strResName, data);
	}
	
	/**
	* Gets the resource data of a specific resource by index.
	* The valid range of the parameter uiResTypeIndex is 0...getNumberOfResourceTypes() - 1.
	* The valid range of the parameter uiResIndex is 0...getNumberOfResources() - 1.
	* Leaving the invalid range leads to undefined behaviour.
	* @param uiResTypeIndex Identifies the resource type of the resource.
	* @param uiResIndex Identifies the resource.
	* @param data Vector where the data is stored.
	**/
	void ResourceDirectory::getResourceDataByIndex(unsigned int uiResTypeIndex, unsigned int uiResIndex, std::vector<byte>& data) const
	{
		ResourceNode* currNode = static_cast<ResourceNode*>(m_rnRoot.children[uiResTypeIndex].child);
		currNode = static_cast<ResourceNode*>(currNode->children[uiResIndex].child);
		ResourceLeaf* currLeaf = static_cast<ResourceLeaf*>(currNode->children[0].child);
		
		data.assign(currLeaf->m_data.begin(), currLeaf->m_data.end());
	}
		  
	/**
	* Sets the resource data of a specific resource.
	* @param dwResTypeId Identifies the resource type of the resource.
	* @param dwResId Identifies the resource.
	* @param data The new resource data.
	**/
	void ResourceDirectory::setResourceData(dword dwResTypeId, dword dwResId, std::vector<byte>& data)
	{
		setResourceDataT(dwResTypeId, dwResId, data);
	}
	
	/**
	* Sets the resource data of a specific resource.
	* @param dwResTypeId Identifies the resource type of the resource.
	* @param strResName Identifies the resource.
	* @param data The new resource data.
	**/
	void ResourceDirectory::setResourceData(dword dwResTypeId, const std::string& strResName, std::vector<byte>& data)
	{
		setResourceDataT(dwResTypeId, strResName, data);
	}
	
	/**
	* Sets the resource data of a specific resource.
	* @param strResTypeName Identifies the resource type of the resource.
	* @param dwResId Identifies the resource.
	* @param data The new resource data.
	**/
	void ResourceDirectory::setResourceData(const std::string& strResTypeName, dword dwResId, std::vector<byte>& data)
	{
		setResourceDataT(strResTypeName, dwResId, data);
	}
	
	/**
	* Sets the resource data of a specific resource.
	* @param strResTypeName Identifies the resource type of the resource.
	* @param strResName Identifies the resource.
	* @param data The new resource data.
	**/
	void ResourceDirectory::setResourceData(const std::string& strResTypeName, const std::string& strResName, std::vector<byte>& data)
	{
		setResourceDataT(strResTypeName, strResName, data);
	}
	
	/**
	* Sets the resource data of a specific resource by index.
	* The valid range of the parameter uiResTypeIndex is 0...getNumberOfResourceTypes() - 1.
	* The valid range of the parameter uiResIndex is 0...getNumberOfResources() - 1.
	* Leaving the invalid range leads to undefined behaviour.
	* @param uiResTypeIndex Identifies the resource type of the resource.
	* @param uiResIndex Identifies the resource.
	* @param data The new resource data.
	**/
	void ResourceDirectory::setResourceDataByIndex(unsigned int uiResTypeIndex, unsigned int uiResIndex, std::vector<byte>& data)
	{
		ResourceNode* currNode = static_cast<ResourceNode*>(m_rnRoot.children[uiResTypeIndex].child);
		currNode = static_cast<ResourceNode*>(currNode->children[uiResIndex].child);
		ResourceLeaf* currLeaf = static_cast<ResourceLeaf*>(currNode->children[0].child);
		currLeaf->m_data.assign(data.begin(), data.end());
	}
		  
	/**
	* Gets the ID of a specific resource.
	* @param dwResTypeId Identifies the resource type of the resource.
	* @param strResName Identifies the resource.
	* @return ID of the specified resource.
	**/
	dword ResourceDirectory::getResourceId(dword dwResTypeId, const std::string& strResName) const
	{
		return getResourceIdT(dwResTypeId, strResName);
	}
	
	/**
	* Gets the ID of a specific resource.
	* @param strResTypeName Identifies the resource type of the resource.
	* @param strResName Identifies the resource.
	* @return ID of the specified resource.
	**/
	dword ResourceDirectory::getResourceId(const std::string& strResTypeName, const std::string& strResName) const
	{
		return getResourceIdT(strResTypeName, strResName);
	}
	
	/**
	* Gets the ID of a specific resource by index.
	* @param uiResTypeIndex Identifies the resource type of the resource.
	* @param uiResIndex Identifies the resource.
	* @return ID of the specified resource.
	**/
	dword ResourceDirectory::getResourceIdByIndex(unsigned int uiResTypeIndex, unsigned int uiResIndex) const
	{
		ResourceNode* currNode = static_cast<ResourceNode*>(m_rnRoot.children[uiResTypeIndex].child);
		return currNode->children[uiResIndex].entry.irde.Name;
	}

	/**
	* Sets the ID of a specific resource.
	* @param dwResTypeId Identifies the resource type of the resource.
	* @param dwResId Identifies the resource.
	* @param dwNewResId New ID of the resource.
	**/
	void ResourceDirectory::setResourceId(dword dwResTypeId, dword dwResId, dword dwNewResId)
	{
		setResourceIdT(dwResTypeId, dwResId, dwNewResId);
	}
	
	/**
	* Sets the ID of a specific resource.
	* @param dwResTypeId Identifies the resource type of the resource.
	* @param strResName Identifies the resource.
	* @param dwNewResId New ID of the resource.
	**/
	void ResourceDirectory::setResourceId(dword dwResTypeId, const std::string& strResName, dword dwNewResId)
	{
		setResourceIdT(dwResTypeId, strResName, dwNewResId);
	}
	
	/**
	* Sets the ID of a specific resource.
	* @param strResTypeName Identifies the resource type of the resource.
	* @param dwResId Identifies the resource.
	* @param dwNewResId New ID of the resource.
	**/
	void ResourceDirectory::setResourceId(const std::string& strResTypeName, dword dwResId, dword dwNewResId)
	{
		setResourceIdT(strResTypeName, dwResId, dwNewResId);
	}
	
	/**
	* Sets the ID of a specific resource.
	* @param strResTypeName Identifies the resource type of the resource.
	* @param strResName Identifies the resource.
	* @param dwNewResId New ID of the resource.
	**/
	void ResourceDirectory::setResourceId(const std::string& strResTypeName, const std::string& strResName, dword dwNewResId)
	{
		setResourceIdT(strResTypeName, strResName, dwNewResId);
	}
	
	/**
	* Sets the ID of a specific resource by index.
	* @param uiResTypeIndex Identifies the resource type of the resource.
	* @param uiResIndex Identifies the resource.
	* @param dwNewResId New ID of the specified resource.
	**/
	void ResourceDirectory::setResourceIdByIndex(unsigned int uiResTypeIndex, unsigned int uiResIndex, dword dwNewResId)
	{
		ResourceNode* currNode = static_cast<ResourceNode*>(m_rnRoot.children[uiResTypeIndex].child);
		currNode->children[uiResIndex].entry.irde.Name = dwNewResId;
	}

	/**
	* Gets the Name of a specific resource.
	* @param dwResTypeId Identifies the resource type of the resource.
	* @param dwResId Identifies the resource.
	* @return Name of the specified resource.
	**/
	std::string ResourceDirectory::getResourceName(dword dwResTypeId, dword dwResId) const
	{
		return getResourceNameT(dwResTypeId, dwResId);
	}
	
	/**
	* Gets the Name of a specific resource.
	* @param strResTypeName Identifies the resource type of the resource.
	* @param dwResId Identifies the resource.
	* @return Name of the specified resource.
	**/
	std::string ResourceDirectory::getResourceName(const std::string& strResTypeName, dword dwResId) const
	{
		return getResourceNameT(strResTypeName, dwResId);
	}
	
	/**
	* Gets the name of a specific resource by index.
	* @param uiResTypeIndex Identifies the resource type of the resource.
	* @param uiResIndex Identifies the resource.
	* @return Name of the specified resource.
	**/
	std::string ResourceDirectory::getResourceNameByIndex(unsigned int uiResTypeIndex, unsigned int uiResIndex) const
	{
		ResourceNode* currNode = static_cast<ResourceNode*>(m_rnRoot.children[uiResTypeIndex].child);
		return currNode->children[uiResIndex].entry.wstrName;
	}

	/**
	* Sets the name of a specific resource.
	* @param dwResTypeId Identifies the resource type of the resource.
	* @param dwResId Identifies the resource.
	* @param strNewResName New name of the specified resource.
	**/
	void ResourceDirectory::setResourceName(dword dwResTypeId, dword dwResId, const std::string& strNewResName)
	{
		setResourceNameT(dwResTypeId, dwResId, strNewResName);
	}
	
	/**
	* Sets the name of a specific resource.
	* @param dwResTypeId Identifies the resource type of the resource.
	* @param strResName Identifies the resource.
	* @param strNewResName New name of the specified resource.
	**/
	void ResourceDirectory::setResourceName(dword dwResTypeId, const std::string& strResName, const std::string& strNewResName)
	{
		setResourceNameT(dwResTypeId, strResName, strNewResName);
	}
	
	/**
	* Sets the name of a specific resource.
	* @param strResTypeName Identifies the resource type of the resource.
	* @param dwResId Identifies the resource.
	* @param strNewResName New name of the specified resource.
	**/
	void ResourceDirectory::setResourceName(const std::string& strResTypeName, dword dwResId, const std::string& strNewResName)
	{
		setResourceNameT(strResTypeName, dwResId, strNewResName);
	}
	
	/**
	* Sets the name of a specific resource.
	* @param strResTypeName Identifies the resource type of the resource.
	* @param strResName Identifies the resource.
	* @param strNewResName New name of the specified resource.
	**/
	void ResourceDirectory::setResourceName(const std::string& strResTypeName, const std::string& strResName, const std::string& strNewResName)
	{
		setResourceNameT(strResTypeName, strResName, strNewResName);
	}
		  
	/**
	* Sets the name of a specific resource by index.
	* @param uiResTypeIndex Identifies the resource type of the resource.
	* @param uiResIndex Identifies the resource.
	* @param strNewResName New name of the specified resource.
	**/
	void ResourceDirectory::setResourceNameByIndex(unsigned int uiResTypeIndex, unsigned int uiResIndex, const std::string& strNewResName)
	{
		ResourceNode* currNode = static_cast<ResourceNode*>(m_rnRoot.children[uiResTypeIndex].child);
		currNode->children[uiResIndex].entry.wstrName = strNewResName;
	}
	
}
