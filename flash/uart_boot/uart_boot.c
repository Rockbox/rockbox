// uart_boot.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "scalar_types.h" // (U)INT8/16/32
#include "Uart.h" // platform abstraction for UART
#include "client.h" // client functions
#include "flash.h" // flash high level functions

// command line configuration: what shall we do?
struct
{
	char* szPort; // COM port to use
	bool bRecorder; // it's a recorder
	bool bArchos; // use the Archos monitor to load, instead of UART boot
	bool bSpindown; // spindown the harddisk
	bool bReadID; // read manufacturer+device ID
	char* szFlashfile; // file to be programmed
	char* szDumpfile; // file to dump into
	char* szExecfile; // file with the executable
	bool bTest; // debug action
	bool bHold; // hold power (for FMs & V2s)
	bool bBlink; // blink red LED
	bool bNoDownload;
} gCmd;



int ProcessCmdLine(int argc, char* argv[])
{
	argc--; // exclude our name
	argv++;

	memset(&gCmd, 0, sizeof(gCmd));

	if (argc == 0)
	{
		printf("Usage: uart_boot [-option {filename}]\n");
		printf("  uses activated UART boot mod, box has to be fresh started\n");
		printf("The order of the options does not matter, one letter is sufficient.\n");
		printf("Possible options are (in the order of later processing):\n");
		printf("-port <name of COM port to use>\n");
		printf("-recorder  (this is a recorder/FM, default is player if not specified)\n");
		printf("-archos  (use Archos bootloader, this one needs powerup while program waits)\n");
		printf("-nodownload  (no MiniMon download, it's already active)\n");
		printf("-hold  (hold the power, useful for FMs and V2s, so you can release ON)\n");
		printf("-spindown  (spindown the harddisk, else it stays on by default)\n");
		printf("-id  (read manufacturer and device ID of flash, no checks)\n");
		printf("-flash <filename of binary to program into flash>\n");
		printf("-dump <filename to write flash content to>\n");
		printf("-exec <filename of executable for 0x09000000:0x09000200>\n");
		printf("-test  (some test action currently under development, don't use!)\n");
		printf("-blink (blink red LED forever, meant as diagnostics)\n");
		printf("\n");
		printf("Examples:\n");
		printf("uart_boot -r -p COM1 -s -f flashfile.bin -d dumpfile.bin\n");
		printf("  recorder on COM1, spindown HD, program and dump (for e.g. offline verify)\n");
		printf("uart_boot -r -p COM2 -e rockbox.bin\n");
		printf("  recorder on COM2, load Rockbox from file and start it\n");
		exit (0);
	}


	while (argc)
	{
		if (!strncmp("-port", *argv, 2))
		{
			gCmd.szPort = *++argv;
			if (--argc <= 0 || **argv == '-')
			{	
				printf("No argument given for option %s, aborting.\n", argv[-1]);
				exit (-2);
			}
		}
		else if (!strncmp("-recorder", *argv, 2))
		{
			gCmd.bRecorder = true;
		}
		else if (!strncmp("-archos", *argv, 2))
		{
			gCmd.bArchos = true;
		}
		else if (!strncmp("-nodownload", *argv, 2))
		{
			gCmd.bNoDownload = true;
		}
		else if (!strncmp("-spindown", *argv, 2))
		{
			gCmd.bSpindown = true;
		}
		else if (!strncmp("-id", *argv, 2))
		{
			gCmd.bReadID = true;
		}
		else if (!strncmp("-flash", *argv, 2))
		{
			gCmd.szFlashfile = *++argv;
			if (--argc <= 0 || **argv == '-')
			{	
				printf("No argument given for option %s, aborting.\n", argv[-1]);
				exit (-2);
			}
		}
		else if (!strncmp("-dump", *argv, 2))
		{
			gCmd.szDumpfile = *++argv;
			if (--argc <= 0 || **argv == '-')
			{	
				printf("No argument given for option %s, aborting.\n", argv[-1]);
				exit (-3);
			}
		}
		else if (!strncmp("-exec", *argv, 2))
		{
			gCmd.szExecfile = *++argv;
			if (--argc <= 0 || **argv == '-')
			{	
				printf("No argument given for option %s, aborting.\n", argv[-1]);
				exit (-4);
			}
		}
		else if (!strncmp("-test", *argv, 2))
		{
			gCmd.bTest = true;
		}
		else if (!strncmp("-hold", *argv, 2))
		{
			gCmd.bHold = true;
		}
		else if (!strncmp("-blink", *argv, 2))
		{
			gCmd.bBlink = true;
		}
		else
		{
			printf("Unknown option %s, aborting. Use 'uart_boot' without options for help.\n", *argv);
			exit(-1);
		}

		argv++;
		argc--;
	}
	
	return 0;
}


int main(int argc, char* argv[])
{
	tUartHandle serial_handle;
	UINT16 reg;
	FILE* pFile;
	size_t size;
	static UINT8 abFirmware[256*1024]; // blocksize
	memset(abFirmware, 0xFF, sizeof(abFirmware));

	ProcessCmdLine(argc, argv); // what to do

	if (!gCmd.szPort)
	{
		printf("No serial port given, use 'uart_boot' without parameters for options.\n");
		exit(-1);
	}

	serial_handle = UartOpen(gCmd.szPort); // opening serial port
	if (serial_handle == NULL)
	{
		printf("Cannot open port %s\n", gCmd.szPort);
		return -1;
	}
		
	if (gCmd.bNoDownload)
	{	// just set our speed
        int baudrate = gCmd.bRecorder ? 115200 : 14400;
        if (!gCmd.bRecorder && gCmd.bTest)
        {   // experimental Player speedup to 38400 baud
            baudrate = 38400;
        }

		if (!UartConfig(serial_handle, baudrate, eNOPARITY, eONESTOPBIT, 8))
		{
			printf("Error setting up COM params\n");
			exit(1);
		}
	}
	else
	{	// download the monitor program
		if (gCmd.bArchos)
		{
			printf("Waiting for box startup to download monitor...");
			DownloadArchosMonitor(serial_handle, "minimon_archos.bin"); // load the monitor image
			printf("\b\b\b done.\n");
		}
		else
		{
			printf("Downloading monitor...");
			DownloadMonitor(serial_handle, gCmd.bRecorder, "minimon.bin"); // load the monitor image
			// From now on, we can talk to the box.
			printf("\b\b\b done.\n");

			if (gCmd.bRecorder)
			{	// we can be faster
				SetTargetBaudrate(serial_handle, 11059200, 115200); // set to 115200
			}
            else if (gCmd.bTest) // experimental Player speedup to 38400 baud
	        {
    			SetTargetBaudrate(serial_handle, 12000000, 38400); // set to 38400
            }
		}
	}


	// do the action
	if (gCmd.bHold)
	{
		// hold power for FM
		reg = ReadHalfword(serial_handle, 0x05FFFFC2); // PBDR
		reg |= 0x0020; // set PB5 to keep power
		WriteHalfword(serial_handle, 0x05FFFFC2, reg);

		reg = ReadHalfword(serial_handle, 0x05FFFFC6); // PBIOR
		reg |= 0x0020; // make PB5 an output
		WriteHalfword(serial_handle, 0x05FFFFC6, reg);
		printf("Power hold, you can release ON button now.\n");
	}


	if (gCmd.bSpindown)
	{
		// power down the disk
        if (gCmd.bRecorder)
        {   // Recorder (V1+V2) and FM have disk power control on PA5
		    reg = ReadHalfword(serial_handle, 0x05FFFFCA); // PACR2
		    reg &= ~0x0400; // clear bit 10: GPIO
		    WriteHalfword(serial_handle, 0x05FFFFCA, reg);

		    reg = ReadHalfword(serial_handle, 0x05FFFFC4); // PAIOR
		    reg |= 0x0020; // set bit 5: output
		    WriteHalfword(serial_handle, 0x05FFFFC4, reg);

		    reg = ReadHalfword(serial_handle, 0x05FFFFC0); // PADR
		    reg &= ~0x0020; // clear PA5 to power down
		    WriteHalfword(serial_handle, 0x05FFFFC0, reg);
        }
        else
        {   // new Players have disk power control on PB4
		    reg = ReadHalfword(serial_handle, 0x05FFFFC4); // PBIOR
		    reg |= 0x0010; // set bit 4: output
		    WriteHalfword(serial_handle, 0x05FFFFC6, reg);

		    reg = ReadHalfword(serial_handle, 0x05FFFFC0); // PBDR
		    reg &= ~0x0010; // clear PB4 to power down
		    WriteHalfword(serial_handle, 0x05FFFFC2, reg);
        }
		printf("Harddisk powered down.\n");
	}


	if (gCmd.bReadID)
	{
		UINT8 bMan, bID;
		ReadID(serial_handle, 0x02000000, &bMan, &bID);
		printf("Manufacturer ID = 0x%02X, Device ID = 0x%02X\n", bMan, bID);
	}


	if (gCmd.szFlashfile)
	{
		// flash a firmware file
		printf("Flashing file %s...", gCmd.szFlashfile);
		pFile = fopen(gCmd.szFlashfile, "rb");
		if (pFile == NULL)
		{
			printf("\nFlash file %s not found, exiting.\n", gCmd.szFlashfile);
			return -2;
		}
		size = fread(abFirmware, 1, sizeof(abFirmware), pFile);
		fclose (pFile);
		
		EraseChip(serial_handle, 0x02000000);
		FlashByteMultiple(serial_handle, 0x02000000, size, abFirmware);
		printf("\b\b\b done.\n");
	}


	if (gCmd.szDumpfile)
	{
		// dump the flash content
		printf("Writing flash dump into file %s...", gCmd.szDumpfile);
		ReadByteMultiple(serial_handle, 0x02000000, sizeof(abFirmware), abFirmware);
		pFile = fopen(gCmd.szDumpfile, "wb");
		if (pFile == NULL)
		{
			printf("\nDump file %s cannot be opened, exiting.\n", gCmd.szDumpfile);
			return -3;
		}
		fwrite(abFirmware, 1, sizeof(abFirmware), pFile);
		fclose (pFile);
		printf("\b\b\b done.\n");
	}


	if (gCmd.szExecfile)
	{
		UINT32 size;
		
		printf("Downloading program...");

		// init the DRAM controller like the flash boot does
		reg = ReadHalfword(serial_handle, 0x05FFFFCA); // PACR2
		reg &= 0xFFFB; // PA1 config: /RAS
		reg |= 0x0008;
		WriteHalfword(serial_handle, 0x05FFFFCA, reg); // PACR2
		reg = 0xAFFF; // CS1, CS3 config: /CASH. /CASL
		WriteHalfword(serial_handle, 0x05FFFFEE, reg); // CASCR
		reg = ReadHalfword(serial_handle, 0x05FFFFA0); // BCR  
		reg |= 0x8000; // DRAM enable, default bus 
		WriteHalfword(serial_handle, 0x05FFFFA0, reg); // BCR 
		reg = ReadHalfword(serial_handle, 0x05FFFFA2); // WCR1	 
		reg &= 0xFDFD; // 1-cycle CAS 
		WriteHalfword(serial_handle, 0x05FFFFA2, reg); // WCR1	
		reg = 0x0E00; // CAS 35%, multiplexed, 10 bit row addr.
		WriteHalfword(serial_handle, 0x05FFFFA8, reg); // DCR
		reg = 0x5AB0; // refresh, 4 cycle waitstate
		WriteHalfword(serial_handle, 0x05FFFFAC, reg); // RCR
		reg = 0x9605; // refresh constant
		WriteHalfword(serial_handle, 0x05FFFFB2, reg); // RTCOR
		reg = 0xA518; // phi/32
		WriteHalfword(serial_handle, 0x05FFFFAE, reg); // RTCSR


		// download Rockbox/gdb
		pFile = fopen(gCmd.szExecfile, "rb");
		if (pFile == NULL)
		{
			printf("\nExecutable file %s cannot be opened, exiting.\n", gCmd.szExecfile);
			return -3;
		}

		size = fread(abFirmware, 1, sizeof(abFirmware), pFile);
		WriteByteMultiple(serial_handle, 0x09000000, size, abFirmware);
		fclose (pFile);
		printf("\b\b\b done.\n");

		// start rockbox/gdb
		printf("Starting program...");
		Execute(serial_handle, 0x09000200, false);
		printf("\b\b\b done.\n");
	}


	if (gCmd.bBlink)
	{
		// blinking LED
		UINT8 byte;
		printf("Flashing red LED forever... (stop with Ctrl-C)\n");
		byte = ReadByte(serial_handle, 0x05FFFFC3);
		while (1)
		{
			byte ^= 0x40;
			WriteByte(serial_handle, 0x05FFFFC3, byte);
			Sleep(200);
		}
	}

	return 0;
}

