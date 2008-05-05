/***************************************************************************
 *             __________               __   ___.
 *   Open      \______   \ ____   ____ |  | _\_ |__   _______  ___
 *   Source     |       _//  _ \_/ ___\|  |/ /| __ \ /  _ \  \/  /
 *   Jukebox    |    |   (  <_> )  \___|    < | \_\ (  <_> > <  <
 *   Firmware   |____|_  /\____/ \___  >__|_ \|___  /\____/__/\_ \
 *                     \/            \/     \/    \/            \/
 * $Id$
 *
 * Copyright (C) 2003 by Jörg Hohensohn 
 * 
 * Tool to extract the scrambled image out of an Archos flash ROM dump
 *
 * All files in this archive are subject to the GNU General Public License.
 * See the file COPYING in the source tree root for full license agreement.
 *
 * This software is distributed on an "AS IS" basis, WITHOUT WARRANTY OF ANY
 * KIND, either express or implied.
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#define UINT8 unsigned char
#define UINT16 unsigned short
#define UINT32 unsigned long

#define IMAGE_HEADER 0x6000 // a 32 byte header in front of the software image
#define IMAGE_START  0x6020	// software image position in Flash


// place a 32 bit value into memory, big endian
void Write32(UINT8* pByte, UINT32 value)
{
	pByte[0] = (UINT8)(value >> 24);	
	pByte[1] = (UINT8)(value >> 16);	
	pByte[2] = (UINT8)(value >> 8);	
	pByte[3] = (UINT8)(value);	
}


// read a 32 bit value from memory, big endian
UINT32 Read32(UINT8* pByte)
{
	UINT32 value = 0;

	value |= (UINT32)pByte[0] << 24;
	value |= (UINT32)pByte[1] << 16;
	value |= (UINT32)pByte[2] << 8;
	value |= (UINT32)pByte[3];

	return value;
}


// entry point
int main(int argc, char* argv[])
{
	FILE* pInFile;
	FILE* pOutFile;
	UINT8 aHeader[6];
	UINT8 aImage[256*1024];
	UINT32 i;
	UINT32 uiSize, uiStart;
	UINT16 usChecksum = 0;
	
	if (argc < 2)
	{
		printf("Extract the software image out of an original Archos Flash ROM dump.\n");
		printf("Result is a scrambled file, use the descramble tool to get the binary,\n");
		printf(" always without the -fm option, even if processing an FM software.\n\n");
		printf("Usage: extract <flash dump file> <output file>\n");
		printf("Example: extract internal_rom_2000000-203FFFF.bin archos.ajz\n");
		exit(0);
	}
	
	pInFile = fopen(argv[1], "rb");
	if (pInFile == NULL)
	{
		printf("Error opening input file %s\n", argv[1]);
		exit(1);
	}

	if (fread(aImage, 1, sizeof(aImage), pInFile) != sizeof(aImage))
	{
		printf("Error reading input file %s, must be 256kB in size.\n", argv[1]);
		fclose(pInFile);
		exit(2);
	}
	fclose(pInFile);

	// find out about the type
	uiStart = Read32(aImage + 8);
	uiSize = Read32(aImage + 12); // booted ROM image
	if (uiStart == 0x02000100 && uiSize > 20000)
	{	// Player has no loader, starts directly with the image
		uiStart = 0x0100;
	}
	else
	{	// Recorder / FM / V2 Recorder
		uiStart = IMAGE_START;
		uiSize = Read32(aImage + IMAGE_HEADER + 4); // size record of header
	}

	// sanity check
	if (uiSize > sizeof(aImage) - uiStart || uiSize < 40000)
	{	
		printf("Error: Impossible image size &d bytes.\n", uiSize);
		exit(3);
	}

	// generate checksum
	for (i=0; i<uiSize; i++)
	{
		UINT8 byte;
		byte = aImage[uiStart + i];
		byte = ~((byte >> 1) | ((byte << 7) & 0x80)); /* poor man's ROR */
		usChecksum += byte;
	}

	// make header
	Write32(aHeader + 2, usChecksum); // checksum in 5th and 6th byte
	Write32(aHeader, uiSize); // size in first 4 bytes

	pOutFile = fopen(argv[2], "wb");
	if (pOutFile == NULL)
	{
		printf("Error opening output file %s\n", argv[2]);
		exit(4);
	}

	if (fwrite(aHeader, 1, sizeof(aHeader), pOutFile) != sizeof(aHeader)
	 || fwrite(aImage + uiStart, 1, uiSize, pOutFile) != uiSize)
	{
		printf("Write error\n");
		fclose(pOutFile);
		exit(5);
	}

	fclose(pOutFile);

	return 0;
}