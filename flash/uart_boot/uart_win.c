// UART wrapper implementation for the Win32 platform
// make a new version of this file for different systems, e.g. Linux

#include <windows.h>
#include "scalar_types.h" // (U)INT8/16/32
#include "Uart.h"

// COMx for windows, returns NULL on error
tUartHandle UartOpen(char* szPortName)
{
	HANDLE serial_handle; 
	DCB  dcb;
	COMMTIMEOUTS cto = { 0, 0, 0, 0, 0 };

	memset(&dcb,0,sizeof(dcb));

	/* -------------------------------------------------------------------- */
	// set DCB to configure the serial port
	dcb.DCBlength       = sizeof(dcb);

	dcb.fOutxCtsFlow    = 0;
	dcb.fOutxDsrFlow    = 0;
	dcb.fDtrControl     = DTR_CONTROL_ENABLE; // enable for power
	dcb.fDsrSensitivity = 0;
	dcb.fRtsControl     = RTS_CONTROL_ENABLE; // enable for power
	dcb.fOutX           = 0;
	dcb.fInX            = 0;

	/* ----------------- misc parameters ----- */
	dcb.fErrorChar      = 0;
	dcb.fBinary         = 1;
	dcb.fNull           = 0;
	dcb.fAbortOnError   = 0;
	dcb.wReserved       = 0;
	dcb.XonLim          = 2;
	dcb.XoffLim         = 4;
	dcb.XonChar         = 0x13;
	dcb.XoffChar        = 0x19;
	dcb.EvtChar         = 0;

	/* ----------------- defaults ----- */
	dcb.BaudRate = 4800;
	dcb.Parity   = NOPARITY;
	dcb.fParity  = 0;
	dcb.StopBits = ONESTOPBIT;
	dcb.ByteSize = 8;


	/* -------------------------------------------------------------------- */
	// opening serial port
	serial_handle = CreateFile(szPortName, GENERIC_READ | GENERIC_WRITE,
		0, NULL, OPEN_EXISTING, FILE_FLAG_WRITE_THROUGH, NULL);

	if (serial_handle == INVALID_HANDLE_VALUE)
	{
		//printf("Cannot open port \n");
		return NULL;
	}

	SetCommMask(serial_handle, 0);
	SetCommTimeouts(serial_handle, &cto);

	if(!SetCommState(serial_handle, &dcb))
	{
		//printf("Error setting up COM params\n");
		CloseHandle(serial_handle);
		return NULL;
	}

	return serial_handle;
}

// returns true on success, false on error
bool UartConfig(tUartHandle handle, long lBaudRate, tParity nParity, tStopBits nStopBits, int nByteSize)
{
	DCB  dcb;
	
	if (!GetCommState (handle, &dcb))
	{
		return false;
	}

	dcb.BaudRate = lBaudRate;
	dcb.Parity	 = nParity;
	dcb.StopBits = nStopBits;
	dcb.ByteSize = nByteSize;

	if(!SetCommState(handle, &dcb))
	{
		//DWORD dwErr = GetLastError();
		//printf("Error %d setting up COM params for baudrate byte\n", dwErr);
		return false;
	}

	return true;
}

// returns how much data was actually transmitted
long UartWrite(tUartHandle handle, unsigned char* pData, long lSize)
{
	BOOL success;
	DWORD result_nbr;

	success = WriteFile(handle, pData, lSize, &result_nbr, NULL);

	if(!success)
	{
		return 0;
	}

	return result_nbr;
}

// returns how much data was actually received
long UartRead(tUartHandle handle, unsigned char* pBuffer, long lSize)
{
	BOOL success;
	DWORD read_nbr;

	success = ReadFile(handle, pBuffer, lSize, &read_nbr, NULL);
	if(!success)
	{
		return 0;
	}

	return read_nbr;
}


void UartClose(tUartHandle handle)
{
	if (handle != NULL)
	{
		CloseHandle(handle);
	}

	return;
}

