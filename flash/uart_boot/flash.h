#ifndef _FLASH_H
#define _FLASH_H

int ReadID(tUartHandle serial_handle, UINT32 base, UINT8* pManufacturerID, UINT8* pDeviceID);
int EraseSector(tUartHandle serial_handle, UINT32 address);
int EraseChip(tUartHandle serial_handle, UINT32 base);
int ProgramBytes(tUartHandle serial_handle, UINT32 address, UINT8* pData, UINT32 size);

#endif

