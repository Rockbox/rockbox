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
 * Autoring tool for the firmware image to be programmed into Flash ROM
 * It composes the flash content with header, bootloader and image(s)
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
#include <string.h>

#define UINT8 unsigned char
#define UINT16 unsigned short
#define UINT32 unsigned long
#define BOOL int
#define TRUE 1
#define FALSE 0

// size of one flash sector, the granularity with which it can be erased
#define SECTORSIZE 4096 

#define BOOTLOAD_DEST 0x0FFFF500 // for the "normal" one
#define FLASH_START  0x02000000
#define BOOTLOAD_SCR 0x02000100
#define ROCKBOX_DEST 0x09000000
#define ROCKBOX_EXEC 0x09000200


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


UINT32 CalcCRC32 (const UINT8* buf, UINT32 len) 
{ 
	static const UINT32 crc_table[256] = 
	{	// CRC32 lookup table for polynomial 0x04C11DB7
		0x00000000, 0x04C11DB7, 0x09823B6E, 0x0D4326D9, 0x130476DC, 0x17C56B6B, 
		0x1A864DB2, 0x1E475005, 0x2608EDB8, 0x22C9F00F, 0x2F8AD6D6, 0x2B4BCB61, 
		0x350C9B64, 0x31CD86D3, 0x3C8EA00A, 0x384FBDBD, 0x4C11DB70, 0x48D0C6C7, 
		0x4593E01E, 0x4152FDA9, 0x5F15ADAC, 0x5BD4B01B, 0x569796C2, 0x52568B75, 
		0x6A1936C8, 0x6ED82B7F, 0x639B0DA6, 0x675A1011, 0x791D4014, 0x7DDC5DA3, 
		0x709F7B7A, 0x745E66CD, 0x9823B6E0, 0x9CE2AB57, 0x91A18D8E, 0x95609039, 
		0x8B27C03C, 0x8FE6DD8B, 0x82A5FB52, 0x8664E6E5, 0xBE2B5B58, 0xBAEA46EF, 
		0xB7A96036, 0xB3687D81, 0xAD2F2D84, 0xA9EE3033, 0xA4AD16EA, 0xA06C0B5D, 
		0xD4326D90, 0xD0F37027, 0xDDB056FE, 0xD9714B49, 0xC7361B4C, 0xC3F706FB, 
		0xCEB42022, 0xCA753D95, 0xF23A8028, 0xF6FB9D9F, 0xFBB8BB46, 0xFF79A6F1, 
		0xE13EF6F4, 0xE5FFEB43, 0xE8BCCD9A, 0xEC7DD02D, 0x34867077, 0x30476DC0, 
		0x3D044B19, 0x39C556AE, 0x278206AB, 0x23431B1C, 0x2E003DC5, 0x2AC12072, 
		0x128E9DCF, 0x164F8078, 0x1B0CA6A1, 0x1FCDBB16, 0x018AEB13, 0x054BF6A4, 
		0x0808D07D, 0x0CC9CDCA, 0x7897AB07, 0x7C56B6B0, 0x71159069, 0x75D48DDE, 
		0x6B93DDDB, 0x6F52C06C, 0x6211E6B5, 0x66D0FB02, 0x5E9F46BF, 0x5A5E5B08, 
		0x571D7DD1, 0x53DC6066, 0x4D9B3063, 0x495A2DD4, 0x44190B0D, 0x40D816BA, 
		0xACA5C697, 0xA864DB20, 0xA527FDF9, 0xA1E6E04E, 0xBFA1B04B, 0xBB60ADFC, 
		0xB6238B25, 0xB2E29692, 0x8AAD2B2F, 0x8E6C3698, 0x832F1041, 0x87EE0DF6, 
		0x99A95DF3, 0x9D684044, 0x902B669D, 0x94EA7B2A, 0xE0B41DE7, 0xE4750050, 
		0xE9362689, 0xEDF73B3E, 0xF3B06B3B, 0xF771768C, 0xFA325055, 0xFEF34DE2, 
		0xC6BCF05F, 0xC27DEDE8, 0xCF3ECB31, 0xCBFFD686, 0xD5B88683, 0xD1799B34, 
		0xDC3ABDED, 0xD8FBA05A, 0x690CE0EE, 0x6DCDFD59, 0x608EDB80, 0x644FC637, 
		0x7A089632, 0x7EC98B85, 0x738AAD5C, 0x774BB0EB, 0x4F040D56, 0x4BC510E1, 
		0x46863638, 0x42472B8F, 0x5C007B8A, 0x58C1663D, 0x558240E4, 0x51435D53, 
		0x251D3B9E, 0x21DC2629, 0x2C9F00F0, 0x285E1D47, 0x36194D42, 0x32D850F5, 
		0x3F9B762C, 0x3B5A6B9B, 0x0315D626, 0x07D4CB91, 0x0A97ED48, 0x0E56F0FF, 
		0x1011A0FA, 0x14D0BD4D, 0x19939B94, 0x1D528623, 0xF12F560E, 0xF5EE4BB9, 
		0xF8AD6D60, 0xFC6C70D7, 0xE22B20D2, 0xE6EA3D65, 0xEBA91BBC, 0xEF68060B, 
		0xD727BBB6, 0xD3E6A601, 0xDEA580D8, 0xDA649D6F, 0xC423CD6A, 0xC0E2D0DD, 
		0xCDA1F604, 0xC960EBB3, 0xBD3E8D7E, 0xB9FF90C9, 0xB4BCB610, 0xB07DABA7, 
		0xAE3AFBA2, 0xAAFBE615, 0xA7B8C0CC, 0xA379DD7B, 0x9B3660C6, 0x9FF77D71, 
		0x92B45BA8, 0x9675461F, 0x8832161A, 0x8CF30BAD, 0x81B02D74, 0x857130C3, 
		0x5D8A9099, 0x594B8D2E, 0x5408ABF7, 0x50C9B640, 0x4E8EE645, 0x4A4FFBF2, 
		0x470CDD2B, 0x43CDC09C, 0x7B827D21, 0x7F436096, 0x7200464F, 0x76C15BF8, 
		0x68860BFD, 0x6C47164A, 0x61043093, 0x65C52D24, 0x119B4BE9, 0x155A565E, 
		0x18197087, 0x1CD86D30, 0x029F3D35, 0x065E2082, 0x0B1D065B, 0x0FDC1BEC, 
		0x3793A651, 0x3352BBE6, 0x3E119D3F, 0x3AD08088, 0x2497D08D, 0x2056CD3A, 
		0x2D15EBE3, 0x29D4F654, 0xC5A92679, 0xC1683BCE, 0xCC2B1D17, 0xC8EA00A0, 
		0xD6AD50A5, 0xD26C4D12, 0xDF2F6BCB, 0xDBEE767C, 0xE3A1CBC1, 0xE760D676, 
		0xEA23F0AF, 0xEEE2ED18, 0xF0A5BD1D, 0xF464A0AA, 0xF9278673, 0xFDE69BC4, 
		0x89B8FD09, 0x8D79E0BE, 0x803AC667, 0x84FBDBD0, 0x9ABC8BD5, 0x9E7D9662, 
		0x933EB0BB, 0x97FFAD0C, 0xAFB010B1, 0xAB710D06, 0xA6322BDF, 0xA2F33668, 
		0xBCB4666D, 0xB8757BDA, 0xB5365D03, 0xB1F740B4 
	};
	UINT32 i;
	UINT32 crc = 0xffffffff;

	for (i = 0; i < len; i++)
		crc = (crc << 8) ^ crc_table[((crc >> 24) ^ *buf++) & 0xFF];

	return crc; 
} 


UINT32 PlaceImage(char* filename, UINT32 pos, UINT8* pFirmware, UINT32 limit)
{
	UINT32 size, read;
	FILE* pFile;
	UINT32 align;
	UINT32 flags;
	UINT32 load_addr = ROCKBOX_DEST, exec_addr = ROCKBOX_EXEC; // defaults

	// magic file header for compressed files
	static const UINT8 magic[8] = { 0x00,0xe9,0x55,0x43,0x4c,0xff,0x01,0x1a };
	UINT8 ucl_header[26];

	pFile = fopen(filename, "rb"); // open the current image
	if (pFile == NULL)
	{
		printf("Image file %s not found!\n", filename);
		exit(5);
	}

	fseek(pFile, 0, SEEK_END);
	size = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);

	// determine if compressed
	flags = 0x00000000; // default: flags for uncompressed
	fread(ucl_header, 1, sizeof(ucl_header), pFile);
	if (memcmp(magic, ucl_header, sizeof(magic)) == 0)
	{
		if (ucl_header[12] != 0x2E // check algorithm
			&& ucl_header[12] != 0x2B) // or uncompressed
		{
			printf("UCL compressed files must use algorithm 2e, not %d\n", ucl_header[12]);
			printf("Generate with: uclpack --best --2e rockbox.bin %s\n", filename);
			exit(6);
		}
		
		size = Read32(ucl_header + 22); // compressed size
		if (Read32(ucl_header + 18) > size) // compare with uncompressed size
		{	// normal case
			flags = 0x00000001; // flags for UCL compressed
		}

		if (ucl_header[12] == 0x2B) // uncompressed means "ROMbox", for direct flash execution
		{
			UINT8 reset_vec[4];
			fread(reset_vec, 1, sizeof(reset_vec), pFile); // read the reset vector from image
			fseek(pFile, 0-sizeof(reset_vec), SEEK_CUR); // wind back
			load_addr = FLASH_START + pos + 16; // behind 16 byte header
			exec_addr = Read32(reset_vec);
		}
	}
	else
	{
		fseek(pFile, 0, SEEK_SET); // go back
	}

	if (pos + 16 + size > limit) // enough space for all that?
	{
		printf("Exceeding maximum image size %d\n", limit);
		exit(7);
	}
	
	// write header
	align = (pos + 16 + size + SECTORSIZE-1) & ~(SECTORSIZE-1); // round up to next flash sector
	Write32(pFirmware + pos, load_addr); // load address
	Write32(pFirmware + pos + 4, align - (pos + 16)); // image size
	Write32(pFirmware + pos + 8, exec_addr); // execution address
	Write32(pFirmware + pos + 12, flags); // compressed or not
	pos += 16;

	// load image
	read = fread(pFirmware + pos, 1, size, pFile);
	if (read != size)
	{
		printf("Read error, expecting %d bytes, got only %d\n", size, read);
		exit(8);
	}
	fclose (pFile);

	pos += size;

	return pos;
}


int main(int argc, char* argv[])
{
	static UINT8 aFirmware[512*1024]; // maximum with exchanged chip
	FILE* pFile;
	UINT32 size; // size of loaded item
	UINT32 pos; // current position in firmware
	UINT32 crc32; // checksum of "payload"
	BOOL hasBootRom; // flag if regular boot ROM or directly starts from flash
	UINT32 template_F8, template_FC; // my platform ID, mask and version

	int i;
	
	if (argc <= 4)
	{
		printf("Usage:\n");
		printf("make_firmware <output> <template.bin> <bootloader.ajz> <image1.ucl> {image2.ucl}\n");
		printf("<template.bin> is the original firmware from your box\n");
		printf("<bootloader.ajz> is the scrambled bootloader\n");
		printf("<image1.ucl> is the first image, compressed (recommended) or uncompressed\n");
		printf("<image1.ucl> is the second image, compressed (recommended) or uncompressed\n");
		printf("More images may follow, but keep the flash size in mind!\n");
		printf("Compression must be UCL, algorithm 2e.\n");
		printf("Generated with: uclpack --best --2e rockbox.bin imageN.ucl\n");
		exit(0);
	}
	
	memset(aFirmware, 0xFF, sizeof(aFirmware));
	
	/******* process template *******/
	
	pFile = fopen(argv[2], "rb"); // open the template
	if (pFile == NULL)
	{
		printf("Template file %s not found!\n", argv[2]);
		exit(1);
	}
	size = fread(aFirmware, 1, 256, pFile); // need only the header
	fclose(pFile);
	if (size < 256) // need at least the firmware header
	{
		printf("Template file %s too small, need at least the header!\n", argv[2]);
		exit(2);
	}

	if (strncmp(aFirmware, "ARCH", 4) == 0)
	{
		hasBootRom = TRUE;
		pos = 256; // place bootloader after this "boot block"
	}
	else if (Read32(aFirmware) == 0x0200)
	{
		hasBootRom = FALSE;
		pos = 0; // directly start with the bootloader
		template_F8 = Read32(aFirmware + 0xF8); // my platform ID and future info
		template_FC = Read32(aFirmware + 0xFC); // use mask+version from template
	}
	else
	{
		printf("Template file %s invalid!\n", argv[2]);
		exit(3);
	}
	
	/******* process bootloader *******/

	pFile = fopen(argv[3], "rb"); // open the bootloader
	if (pFile == NULL)
	{
		printf("Bootloader file %s not found!\n", argv[3]);
		exit(4);
	}
	if (hasBootRom && fseek(pFile, 6, SEEK_SET)) // skip the ajz header
	{
		printf("Bootloader file %s too short!\n", argv[3]);
		exit(5);
	}

	// place bootloader after header
	size = fread(aFirmware + pos, 1, sizeof(aFirmware) - pos, pFile);
	fclose(pFile);

	if (hasBootRom)
	{
		Write32(aFirmware + 4, BOOTLOAD_DEST); // boot code destination address
		
		for (i=0x08; i<=0x28; i+=8)
		{
			Write32(aFirmware + i, BOOTLOAD_SCR); // boot code source address
			Write32(aFirmware + i + 4, size); // boot code size
		}
	}
	else
	{
		Write32(aFirmware + 0xF8, template_F8); // values from template
		Write32(aFirmware + 0xFC, template_FC); // mask and version
	}

	size = (size + 3) & ~3; // make shure it's 32 bit aligned
	pos += size; // prepare position for first image

	/******* process images *******/
	for (i = 4; i < argc; i++)
	{
		pos = PlaceImage(argv[i], pos, aFirmware, sizeof(aFirmware));

		if (i < argc-1)
		{	// not the last: round up to next flash sector
			pos = (pos + SECTORSIZE-1) & ~(SECTORSIZE-1);
		}
	}
	

	/******* append CRC32 checksum *******/
	crc32 = CalcCRC32(aFirmware, pos);
	Write32(aFirmware + pos, crc32);
	pos += sizeof(crc32); // 4 bytes
	
	
	/******* save result to output file *******/

	pFile = fopen(argv[1], "wb"); // open the output file
	if (pFile == NULL)
	{
		printf("Output file %s cannot be created!\n", argv[1]);
		exit(9);
	}
	size = fwrite(aFirmware, 1, pos, pFile);
	fclose(pFile);
	
	if (size != pos)
	{
		printf("Error writing %d bytes to output file %s!\n", pos, argv[1]);
		exit(10);
	}

	printf("Firmware file generated with %d bytes.\n", pos);

	return 0;
}
