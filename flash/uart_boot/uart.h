// A general definition for the required UART functionality.
// This will be used to gain platform abstraction.

#ifndef _UART_H
#define _UART_H

// data types

typedef void* tUartHandle;
#define INVALID_UART_HANDLE (tUartHandle)-1

typedef enum
{
	eNOPARITY,
	eODDPARITY, 
	eEVENPARITY,
	eMARKPARITY,
	eSPACEPARITY,
} tParity;

typedef enum
{
	eONESTOPBIT,
	eONE5STOPBITS,
	eTWOSTOPBITS,
} tStopBits;


// prototypes

tUartHandle UartOpen(   // returns NULL on error
	char* szPortName);  // COMx for windows

bool UartConfig(         // returns true on success, false on error
	tUartHandle handle,  // the handle returned from UartOpen()
	long lBaudRate,      // must be one of the "standard" baudrates
	tParity nParity,     // what kind of parity
	tStopBits nStopBits, // how many stop bits
	int nByteSize);      // size of the "payload", can be 5 to 8

long UartWrite(           // returns how much data was actually transmitted
	tUartHandle handle,   // the handle returned from UartOpen()
	unsigned char* pData, // pointer to the data to be transmitted
	long lSize);          // how many bytes

long UartRead(              // returns how much data was actually received
	tUartHandle handle,     // the handle returned from UartOpen()
	unsigned char* pBuffer, // pointer to the destination
	long lSize);            // how many bytes to read (pBuffer must have enough room)


void UartClose(tUartHandle handle);



#endif // _UART_H

