// flash.cpp : higher-level functions for flashing the chip
//

#include "scalar_types.h" // (U)INT8/16/32
#include "Uart.h" // platform abstraction for UART
#include "client.h" // client functions


// read the manufacturer and device ID
int ReadID(tUartHandle serial_handle, UINT32 base, UINT8* pManufacturerID, UINT8* pDeviceID)
{
	base &= 0xFFF80000; // round down to 512k align, to make shure

	WriteByte(serial_handle, base + 0x5555, 0xAA); // enter command mode
	WriteByte(serial_handle, base + 0x2AAA, 0x55);
	WriteByte(serial_handle, base + 0x5555, 0x90); // ID command
	SLEEP(20); // Atmel wants 20ms pause here

	*pManufacturerID = ReadByte(serial_handle, base + 0);
	*pDeviceID		 = ReadByte(serial_handle, base + 1);

	WriteByte(serial_handle, base + 0, 0xF0);	// reset flash (back to normal read mode)
	SLEEP(20); // Atmel wants 20ms pause here

	return 0;
}


// erase the sector which contains the given address
int EraseSector(tUartHandle serial_handle, UINT32 address)
{
	UINT32 base = address & 0xFFF80000; // round down to 512k align
	
	WriteByte(serial_handle, base + 0x5555, 0xAA); // enter command mode
	WriteByte(serial_handle, base + 0x2AAA, 0x55);
	WriteByte(serial_handle, base + 0x5555, 0x80); // eraze command
	WriteByte(serial_handle, base + 0x5555, 0xAA); // enter command mode
	WriteByte(serial_handle, base + 0x2AAA, 0x55);
	WriteByte(serial_handle, address, 0x30); // eraze the sector
	SLEEP(25); // sector eraze time: 25ms

	return 0;
}


// erase the whole flash
int EraseChip(tUartHandle serial_handle, UINT32 base)
{
	base &= 0xFFF80000; // round down to 512k align, to make shure
	
	WriteByte(serial_handle, base + 0x5555, 0xAA); // enter command mode
	WriteByte(serial_handle, base + 0x2AAA, 0x55);
	WriteByte(serial_handle, base + 0x5555, 0x80); // eraze command
	WriteByte(serial_handle, base + 0x5555, 0xAA); // enter command mode
	WriteByte(serial_handle, base + 0x2AAA, 0x55);
	WriteByte(serial_handle, base + 0x5555, 0x10); // chip eraze command
	SLEEP(100); // chip eraze time: 100ms

	return 0;
}


// program a bunch of bytes "by hand"
int ProgramBytes(tUartHandle serial_handle, UINT32 address, UINT8* pData, UINT32 size)
{
	UINT32 base = address & 0xFFF80000; // round down to 512k align

	while (size--)
	{
		WriteByte(serial_handle, base + 0x5555, 0xAA); // enter command mode
		WriteByte(serial_handle, base + 0x2AAA, 0x55);
		WriteByte(serial_handle, base + 0x5555, 0xA0); // byte program command
		WriteByte(serial_handle, address++, *pData++);
		// UART protocol is slow enough such that I don't have to wait 20us here
	}
	return 0;
}

