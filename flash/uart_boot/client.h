#ifndef _CLIENT_H
#define _CLIENT_H


// setup function for monitor download
int DownloadMonitor(tUartHandle serial_handle, bool bRecorder, char* szFilename);
int DownloadArchosMonitor(tUartHandle serial_handle, char* szFilename);

// target functions using the Monitor Protocol
UINT8 ReadByte(tUartHandle serial_handle, UINT32 addr);
int WriteByte(tUartHandle serial_handle, UINT32 addr, UINT8 byte);
int ReadByteMultiple(tUartHandle serial_handle, UINT32 addr, UINT32 size, UINT8* pBuffer);
int WriteByteMultiple(tUartHandle serial_handle, UINT32 addr, UINT32 size, UINT8* pBuffer);
int FlashByteMultiple(tUartHandle serial_handle, UINT32 addr, UINT32 size, UINT8* pBuffer);
UINT16 ReadHalfword(tUartHandle serial_handle, UINT32 addr);
int WriteHalfword(tUartHandle serial_handle, UINT32 addr, UINT16 halfword);
int SetTargetBaudrate(tUartHandle serial_handle, long lClock, long lBaudrate);
int Execute(tUartHandle serial_handle, UINT32 addr, bool bReturns);


#endif

