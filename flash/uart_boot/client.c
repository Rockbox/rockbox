// client.cpp : functions for monitor download and communication.
//

#include <stdio.h>
#include <stdlib.h>
#include "scalar_types.h" // (U)INT8/16/32
#include "Uart.h" // platform abstraction for UART
#include "minimon.h" // protocol of my little monitor

// do the baudrate configuration for the Player
int ConfigFirstlevelPlayer (tUartHandle serial_handle)
{
	UINT32 result_nbr;

	if(!UartConfig(serial_handle, 4800, eMARKPARITY, eTWOSTOPBITS, 8))
	{
		UINT32 dwErr = GET_LAST_ERR();
		printf("Error %lu setting up COM params for baudrate byte\n", dwErr);
		exit(1);
	}

	// this will read as 0x19 when viewed with 2300 baud like the player does
	result_nbr = UartWrite(serial_handle, (UINT8*)"\x86\xC0", 2);
	if (result_nbr != 2)
	{
		UINT32 dwErr = GET_LAST_ERR();
		printf("Error %lu setting up COM params for baudrate byte\n", dwErr);
	}

	SLEEP(100); // wait for the chars to be sent, is there a better way?

	// the read 0x19 means 14423 baud with 12 MHz
	if(!UartConfig(serial_handle, 14400, eNOPARITY, eONESTOPBIT, 8))
	{
		printf("Error setting up COM params for 1st level loader\n");
		exit(1);
	}

	return 0;
}


// do the baudrate configuration for the Recoder/FM
int ConfigFirstlevelRecorder (tUartHandle serial_handle)
{
	UINT32 result_nbr;

	if(!UartConfig(serial_handle, 4800, eNOPARITY, eTWOSTOPBITS, 8))
	{
		UINT32 dwErr = GET_LAST_ERR();
		printf("Error %lu setting up COM params for baudrate byte\n", dwErr);
		exit(1);
	}

	// this will read as 0x08 when viewed with 2120 baud like the recorder does
	result_nbr = UartWrite(serial_handle, (UINT8*)"\x00\x00", 2);
	if(result_nbr != 2)
	{
		printf("Error transmitting baudrate byte\n");
		exit(1);
	}

	SLEEP(100); // wait for the chars to be sent, is there a better way?

	// the read 0x08 means 38400 baud with 11.0592 MHz
	if(!UartConfig(serial_handle, 38400, eNOPARITY, eONESTOPBIT, 8))
	{
		UINT32 dwErr = GET_LAST_ERR();
		printf("Error %lu setting up COM params for 1st level loader\n", dwErr);
		exit(1);
	}

	return 0;
}


// transfer a byte for the monitor download, with or without acknowledge
int DownloadByte(tUartHandle serial_handle, unsigned char byte, bool bAck)
{
	unsigned char received;

	while (1)
	{
		UartWrite(serial_handle, &byte, 1);
		if (bAck)
		{
			UartRead(serial_handle, &received, 1);
			if (received == byte)
			{
				UartWrite(serial_handle, (UINT8*)"\x01", 1); // ack success
				break; // exit the loop
			}
			else
			{
				printf("Error transmitting monitor byte 0x%02X, got 0x%0X\n", byte, received);
				UartWrite(serial_handle, (UINT8*)"\x00", 1); // ack fail, try again
			}
		}
		else
			break; // no loop
	}
	return 1;
}


// download our little monitor, the box must have been just freshly switched on for this to work
int DownloadMonitor(tUartHandle serial_handle, bool bRecorder, char* szFilename)
{
	FILE* pFile;
	size_t filesize;
	UINT8 byte;
	unsigned i;

	// hard-coded parameters
	bool bAck = true; // configure if acknowledged download (without useful for remote pin boot)
	UINT32 TargetLoad = 0x0FFFF000; // target load address

	pFile = fopen(szFilename, "rb");
	if (pFile == NULL)
	{
		printf("\nMonitor file %s not found, exiting\n", szFilename);
		exit(1);
	}

	// determine file size
	fseek(pFile, 0, SEEK_END);
	filesize = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);

	// This is _really_ tricky! The box expects a BRR value in a nonstandard baudrate,
	//	which a PC can't generate. I'm using a higher one with some wild settings
	//	to generate a pulse series that:
	//	1) looks like a stable byte when sampled with the nonstandard baudrate
	//	2) gives a BRR value to the box which results in a baudrate the PC can also use
	if (bRecorder)
	{
		ConfigFirstlevelRecorder(serial_handle);
	}
	else
	{
		ConfigFirstlevelPlayer(serial_handle);
	}

	UartWrite(serial_handle, bAck ? (UINT8*)"\x01" : (UINT8*)"\x00", 1); // ACK mode

	// transmit the size, little endian
	DownloadByte(serial_handle, (UINT8)( filesize	   & 0xFF), bAck);
	DownloadByte(serial_handle, (UINT8)((filesize>>8)  & 0xFF), bAck);
	DownloadByte(serial_handle, (UINT8)((filesize>>16) & 0xFF), bAck);
	DownloadByte(serial_handle, (UINT8)((filesize>>24) & 0xFF), bAck);

	// transmit the load address, little endian
	DownloadByte(serial_handle, (UINT8)( TargetLoad 	 & 0xFF), bAck);
	DownloadByte(serial_handle, (UINT8)((TargetLoad>>8)  & 0xFF), bAck);
	DownloadByte(serial_handle, (UINT8)((TargetLoad>>16) & 0xFF), bAck);
	DownloadByte(serial_handle, (UINT8)((TargetLoad>>24) & 0xFF), bAck);

	// transmit the command byte
	DownloadByte(serial_handle, 0xFF, bAck); // 0xFF means execute the transferred image

	// transmit the image
	for (i=0; i<filesize; i++)
	{
		fread(&byte, 1, 1, pFile);
		DownloadByte(serial_handle, byte, bAck);
	}

	fclose (pFile);

	// now the image should have been started, red LED off

	return 0;
}


// wait for a fixed string to be received (no foolproof algorithm,
//	may overlook if the searched string contains repeatitions)
int WaitForString(tUartHandle serial_handle, char* pszWait)
{
	int i = 0;
	unsigned char received;
	
	while(pszWait[i] != '\0')
	{
		UartRead(serial_handle, &received, 1);

		printf("%c", received); // debug

		if (received == pszWait[i])
			i++; // continue
		else 
			i=0; // mismatch, start over
	}
	return 0;
}


// send a sting and check the echo
int SendWithEcho(tUartHandle serial_handle, char* pszSend)
{
	int i = 0;
	unsigned char received;
	
	while(pszSend[i] != '\0')
	{
		UartWrite(serial_handle, (unsigned char*)(pszSend + i), 1); // send char
		do
		{
			UartRead(serial_handle, &received, 1); // receive echo
			printf("%c", received); // debug
		}
		while (received != pszSend[i]); // should normally be equal
		i++; // next char
	}
	return 0;
}


// rarely used variant: download our monitor using the built-in Archos monitor
int DownloadArchosMonitor(tUartHandle serial_handle, char* szFilename)
{
	FILE* pFile;
	size_t filesize;
	UINT8 byte;
	UINT16 checksum = 0;
	unsigned i;

	// the onboard monitor uses 115200 baud
	if(!UartConfig(serial_handle, 115200, eNOPARITY, eONESTOPBIT, 8))
	{
		UINT32 dwErr = GET_LAST_ERR();
		printf("Error %lu setting up COM params for baudrate %d\n", dwErr, 115200);
		exit(1);
	}

	// wait for receiving "#SERIAL#"
	WaitForString(serial_handle, "#SERIAL#");
	
	// send magic "SRL" command to get interactive mode
	SendWithEcho(serial_handle, "SRL\r");
	
	// wait for menu completion: "ROOT>" at the end
	WaitForString(serial_handle, "ROOT>");
	
	// send upload command "UP"
	SendWithEcho(serial_handle, "UP\r");
	
	pFile = fopen(szFilename, "rb");
	if (pFile == NULL)
	{
		printf("\nMonitor file %s not found, exiting\n", szFilename);
		exit(1);
	}

	// determine file size
	fseek(pFile, 0, SEEK_END);
	filesize = ftell(pFile);
	fseek(pFile, 0, SEEK_SET);
	
	// calculate checksum
	for (i=0; i<filesize; i++)
	{
		fread(&byte, 1, 1, pFile);
		checksum += byte;
	}
	fseek(pFile, 0, SEEK_SET);
	
	// send header

	// size as 32 bit little endian
	byte = (UINT8)( filesize	  & 0xFF);
	UartWrite(serial_handle, &byte, 1);
	byte = (UINT8)((filesize>>8)  & 0xFF);
	UartWrite(serial_handle, &byte, 1);
	byte = (UINT8)((filesize>>16) & 0xFF);
	UartWrite(serial_handle, &byte, 1);
	byte = (UINT8)((filesize>>24) & 0xFF);
	UartWrite(serial_handle, &byte, 1);

	// checksum as 16 bit little endian
	byte = (UINT8)( checksum	  & 0xFF);
	UartWrite(serial_handle, &byte, 1);
	byte = (UINT8)((checksum>>8)  & 0xFF);
	UartWrite(serial_handle, &byte, 1);
	
	UartWrite(serial_handle, (unsigned char*)"\x00", 1); // kind (3 means flash)
	UartWrite(serial_handle, (unsigned char*)"\x00", 1); // ignored byte

	// wait for monitor to accept data
	WaitForString(serial_handle, "#OKCTRL#");
	
	// transmit the image
	for (i=0; i<filesize; i++)
	{
		fread(&byte, 1, 1, pFile);
		UartWrite(serial_handle, &byte, 1); // payload
	}
	fclose (pFile);
	
	UartWrite(serial_handle, (unsigned char*)"\x00", 1); // ignored byte

	// wait for menu completion: "ROOT>" at the end
	WaitForString(serial_handle, "ROOT>");

	// send start program command "SPRO"
	SendWithEcho(serial_handle, "SPRO\r");

	SLEEP(100); // wait a little while for startup

	return 0;
}


/********** Target functions using the Monitor Protocol **********/

// read a byte using the target monitor
UINT8 ReadByte(tUartHandle serial_handle, UINT32 addr)
{
	UINT8 send;
	UINT8 received;

	// send the address command
	send = ADDRESS;
	UartWrite(serial_handle, &send, 1);

	// transmit the address, big endian
	send = (UINT8)((addr>>24) & 0xFF);
	UartWrite(serial_handle, &send, 1);
	send = (UINT8)((addr>>16) & 0xFF);
	UartWrite(serial_handle, &send, 1);
	send = (UINT8)((addr>>8) & 0xFF);
	UartWrite(serial_handle, &send, 1);
	send = (UINT8)(addr & 0xFF);
	UartWrite(serial_handle, &send, 1);

	UartRead(serial_handle, &received, 1); // response
	if (received != ADDRESS)
	{
		printf("Protocol error!\n");
		return 1;
	}

	// send the read command
	send = BYTE_READ;
	UartWrite(serial_handle, &send, 1);

	UartRead(serial_handle, &received, 1); // response

	return received;
}


// write a byte using the target monitor
int WriteByte(tUartHandle serial_handle, UINT32 addr, UINT8 byte)
{
	UINT8 send;
	UINT8 received;

	// send the address command
	send = ADDRESS;
	UartWrite(serial_handle, &send, 1);

	// transmit the address, big endian
	send = (UINT8)((addr>>24) & 0xFF);
	UartWrite(serial_handle, &send, 1);
	send = (UINT8)((addr>>16) & 0xFF);
	UartWrite(serial_handle, &send, 1);
	send = (UINT8)((addr>>8) & 0xFF);
	UartWrite(serial_handle, &send, 1);
	send = (UINT8)(addr & 0xFF);
	UartWrite(serial_handle, &send, 1);

	UartRead(serial_handle, &received, 1); // response
	if (received != ADDRESS)
	{
		printf("Protocol error, receiced 0x%02X!\n", received);
		return 1;
	}

	// send the write command
	send = BYTE_WRITE;
	UartWrite(serial_handle, &send, 1);

	// transmit the data
	UartWrite(serial_handle, &byte, 1);
	
	UartRead(serial_handle, &received, 1); // response

	if (received != BYTE_WRITE)
	{
		printf("Protocol error!\n");
		return 1;
	}

	return 0;
}


// read many bytes using the target monitor
int ReadByteMultiple(tUartHandle serial_handle, UINT32 addr, UINT32 size, UINT8* pBuffer)
{
	UINT8 send, received;

	// send the address command
	send = ADDRESS;
	UartWrite(serial_handle, &send, 1);

	// transmit the address, big endian
	send = (UINT8)((addr>>24) & 0xFF);
	UartWrite(serial_handle, &send, 1);
	send = (UINT8)((addr>>16) & 0xFF);
	UartWrite(serial_handle, &send, 1);
	send = (UINT8)((addr>>8) & 0xFF);
	UartWrite(serial_handle, &send, 1);
	send = (UINT8)(addr & 0xFF);
	UartWrite(serial_handle, &send, 1);

	UartRead(serial_handle, &received, 1); // response
	if (received != ADDRESS)
	{
		printf("Protocol error!\n");
		return 1;
	}

	while (size)
	{
		if (size >= 16)
		{	// we can use a "burst" command
			send = BYTE_READ16;
			UartWrite(serial_handle, &send, 1); // send the read command
			UartRead(serial_handle, pBuffer, 16); // data response
			pBuffer += 16;
			size -= 16;
		}
		else
		{	// use single byte command
			send = BYTE_READ;
			UartWrite(serial_handle, &send, 1); // send the read command
			UartRead(serial_handle, pBuffer++, 1); // data response
			size--;
		}
	}

	return 0;
}


// write many bytes using the target monitor
int WriteByteMultiple(tUartHandle serial_handle, UINT32 addr, UINT32 size, UINT8* pBuffer)
{
	UINT8 send, received;

	// send the address command
	send = ADDRESS;
	UartWrite(serial_handle, &send, 1);

	// transmit the address, big endian
	send = (UINT8)((addr>>24) & 0xFF);
	UartWrite(serial_handle, &send, 1);
	send = (UINT8)((addr>>16) & 0xFF);
	UartWrite(serial_handle, &send, 1);
	send = (UINT8)((addr>>8) & 0xFF);
	UartWrite(serial_handle, &send, 1);
	send = (UINT8)(addr & 0xFF);
	UartWrite(serial_handle, &send, 1);

	UartRead(serial_handle, &received, 1); // response
	if (received != ADDRESS)
	{
		printf("Protocol error!\n");
		return 1;
	}

	while (size)
	{
		if (size >= 16)
		{	// we can use a "burst" command
			send = BYTE_WRITE16;
			UartWrite(serial_handle, &send, 1); // send the write command
			UartWrite(serial_handle, pBuffer, 16); // transmit the data
			UartRead(serial_handle, &received, 1); // response
			if (received != BYTE_WRITE16)
			{
				printf("Protocol error!\n");
				return 1;
			}
			pBuffer += 16;
			size -= 16;
		}
		else
		{	// use single byte command
			send = BYTE_WRITE;
			UartWrite(serial_handle, &send, 1); // send the write command
			UartWrite(serial_handle, pBuffer++, 1); // transmit the data
			UartRead(serial_handle, &received, 1); // response
			if (received != BYTE_WRITE)
			{
				printf("Protocol error!\n");
				return 1;
			}
			size--;
		}
	}

	return 0;
}


// write many bytes using the target monitor
int FlashByteMultiple(tUartHandle serial_handle, UINT32 addr, UINT32 size, UINT8* pBuffer)
{
	UINT8 send, received;

	// send the address command
	send = ADDRESS;
	UartWrite(serial_handle, &send, 1);

	// transmit the address, big endian
	send = (UINT8)((addr>>24) & 0xFF);
	UartWrite(serial_handle, &send, 1);
	send = (UINT8)((addr>>16) & 0xFF);
	UartWrite(serial_handle, &send, 1);
	send = (UINT8)((addr>>8) & 0xFF);
	UartWrite(serial_handle, &send, 1);
	send = (UINT8)(addr & 0xFF);
	UartWrite(serial_handle, &send, 1);

	UartRead(serial_handle, &received, 1); // response
	if (received != ADDRESS)
	{
		printf("Protocol error!\n");
		return 1;
	}

	while (size)
	{
		if (size >= 16)
		{	// we can use a "burst" command
			send = BYTE_FLASH16;
			UartWrite(serial_handle, &send, 1); // send the write command
			UartWrite(serial_handle, pBuffer, 16); // transmit the data
			UartRead(serial_handle, &received, 1); // response
			if (received != BYTE_FLASH16)
			{
				printf("Protocol error!\n");
				return 1;
			}
			pBuffer += 16;
			size -= 16;
		}
		else
		{	// use single byte command
			send = BYTE_FLASH;
			UartWrite(serial_handle, &send, 1); // send the write command
			UartWrite(serial_handle, pBuffer++, 1); // transmit the data
			UartRead(serial_handle, &received, 1); // response
			if (received != BYTE_FLASH)
			{
				printf("Protocol error!\n");
				return 1;
			}
			size--;
		}
	}

	return 0;
}


// read a 16bit halfword using the target monitor
UINT16 ReadHalfword(tUartHandle serial_handle, UINT32 addr)
{
	UINT8 send;
	UINT8 received;
	UINT16 halfword;

	// send the address command
	send = ADDRESS;
	UartWrite(serial_handle, &send, 1);

	// transmit the address, big endian
	send = (UINT8)((addr>>24) & 0xFF);
	UartWrite(serial_handle, &send, 1);
	send = (UINT8)((addr>>16) & 0xFF);
	UartWrite(serial_handle, &send, 1);
	send = (UINT8)((addr>>8) & 0xFF);
	UartWrite(serial_handle, &send, 1);
	send = (UINT8)(addr & 0xFF);
	UartWrite(serial_handle, &send, 1);

	UartRead(serial_handle, &received, 1); // response
	if (received != ADDRESS)
	{
		printf("Protocol error!\n");
		return 1;
	}

	// send the read command
	send = HALFWORD_READ;
	UartWrite(serial_handle, &send, 1);

	UartRead(serial_handle, &received, 1); // response
	halfword = received << 8; // highbyte
	UartRead(serial_handle, &received, 1);
	halfword |= received; // lowbyte

	return halfword;
}


// write a 16bit halfword using the target monitor
int WriteHalfword(tUartHandle serial_handle, UINT32 addr, UINT16 halfword)
{
	UINT8 send;
	UINT8 received;

	// send the address command
	send = ADDRESS;
	UartWrite(serial_handle, &send, 1);

	// transmit the address, big endian
	send = (UINT8)((addr>>24) & 0xFF);
	UartWrite(serial_handle, &send, 1);
	send = (UINT8)((addr>>16) & 0xFF);
	UartWrite(serial_handle, &send, 1);
	send = (UINT8)((addr>>8) & 0xFF);
	UartWrite(serial_handle, &send, 1);
	send = (UINT8)(addr & 0xFF);
	UartWrite(serial_handle, &send, 1);

	UartRead(serial_handle, &received, 1); // response
	if (received != ADDRESS)
	{
		printf("Protocol error!\n");
		return 1;
	}

	// send the write command
	send = HALFWORD_WRITE;
	UartWrite(serial_handle, &send, 1);

	// transmit the data
	send = halfword >> 8; // highbyte
	UartWrite(serial_handle, &send, 1);
	send = halfword & 0xFF; // lowbyte
	UartWrite(serial_handle, &send, 1);
	
	UartRead(serial_handle, &received, 1); // response

	if (received != HALFWORD_WRITE)
	{
		printf("Protocol error!\n");
		return 1;
	}

	return 0;
}


// change baudrate using target monitor
int SetTargetBaudrate(tUartHandle serial_handle, long lClock, long lBaudrate)
{
	UINT8 send;
	UINT8 received;
	UINT8 brr;
	long lBRR;

	lBRR = lClock / lBaudrate;
	lBRR = ((lBRR + 16) / 32) - 1; // with rounding
	brr = (UINT8)lBRR;

	// send the command
	send = BAUDRATE;
	UartWrite(serial_handle, &send, 1);
	UartWrite(serial_handle, &brr, 1); // send the BRR value
	UartRead(serial_handle, &received, 1); // response ack

	if (received != BAUDRATE)
	{	// bad situation, now we're unclear about the baudrate of the target
		printf("Protocol error!\n");
		return 1;
	}

	SLEEP(100); // give it some time to settle

	// change our baudrate, too
	UartConfig(serial_handle, lBaudrate, eNOPARITY, eONESTOPBIT, 8); 

	return 0;
}


// call a subroutine using the target monitor
int Execute(tUartHandle serial_handle, UINT32 addr, bool bReturns)
{
	UINT8 send;
	UINT8 received;

	// send the address command
	send = ADDRESS;
	UartWrite(serial_handle, &send, 1);

	// transmit the address, big endian
	send = (UINT8)((addr>>24) & 0xFF);
	UartWrite(serial_handle, &send, 1);
	send = (UINT8)((addr>>16) & 0xFF);
	UartWrite(serial_handle, &send, 1);
	send = (UINT8)((addr>>8) & 0xFF);
	UartWrite(serial_handle, &send, 1);
	send = (UINT8)(addr & 0xFF);
	UartWrite(serial_handle, &send, 1);

	UartRead(serial_handle, &received, 1); // response
	if (received != ADDRESS)
	{
		printf("Protocol error!\n");
		return 1;
	}

	// send the execute command
	send = EXECUTE;
	UartWrite(serial_handle, &send, 1);
	if (bReturns)
	{	// we expect the call to return control to minimon
		UartRead(serial_handle, &received, 1); // response

		if (received != EXECUTE)
		{
			printf("Protocol error!\n");
			return 1;
		}
	}

	return 0;
}


