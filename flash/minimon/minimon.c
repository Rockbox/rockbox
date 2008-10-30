// minimalistic monitor
// to be loaded with the UART boot feature
// capable of reading and writing bytes, commanded by UART

#include "sh7034.h"
#include "minimon.h"

// scalar types
typedef unsigned char  UINT8;
typedef unsigned short UINT16;
typedef unsigned long  UINT32;

typedef void(*tpFunc)(void); // type for exec
typedef int(*tpMain)(void); // type for start vector to main()


// prototypes
int main(void);

// our binary has to start with a vector to the entry point
tpMain start_vector[] __attribute__ ((section (".startvector"))) = {main};


UINT8 uart_read(void)
{
	UINT8 byte;
	while (!(SSR1 & SCI_RDRF)); // wait for char to be available
	byte = RDR1;
	SSR1 &= ~SCI_RDRF;
	return byte;
}


void uart_write(UINT8 byte)
{
	while (!(SSR1 & SCI_TDRE)); // wait for transmit buffer empty
	TDR1 = byte;
	SSR1 &= ~SCI_TDRE;
}


int main(void)
{
	UINT8 cmd;
	UINT32 addr;
	UINT32 size;
	UINT32 content;
	volatile UINT8* paddr = 0;
	volatile UINT8* pflash = 0; // flash base address

	while (1)
	{
		cmd = uart_read();
		switch (cmd)
		{
		case BAUDRATE:
			content = uart_read();
			uart_write(cmd); // acknowledge by returning the command value
			while (!(SSR1 & SCI_TEND)); // wait for empty shift register, before changing baudrate
			BRR1 = content;
			break;

		case ADDRESS:
			addr = (uart_read() << 24) | (uart_read() << 16) | (uart_read() << 8) | uart_read();
			paddr = (UINT8*)addr;
			pflash = (UINT8*)(addr & 0xFFF80000); // round down to 512k align
			uart_write(cmd); // acknowledge by returning the command value
			break;

		case BYTE_READ:
			content = *paddr++;
			uart_write(content); // the content is the ack	
			break;

		case BYTE_WRITE:
			content = uart_read();
			*paddr++ = content;						
			uart_write(cmd); // acknowledge by returning the command value
			break;

		case BYTE_READ16:
			size = 16;
			while (size--)
			{
				content = *paddr++;
				uart_write(content); // the content is the ack		
			}
			break;

		case BYTE_WRITE16:
			size = 16;
			while (size--)
			{
				content = uart_read();
				*paddr++ = content;						
			}
			uart_write(cmd); // acknowledge by returning the command value
			break;

		case BYTE_FLASH:
			content = uart_read();
			pflash[0x5555] = 0xAA; // set flash to command mode
			pflash[0x2AAA] = 0x55;
			pflash[0x5555] = 0xA0; // byte program command
			*paddr++ = content;						
			uart_write(cmd); // acknowledge by returning the command value
			break;

		case BYTE_FLASH16:
			size = 16;
			while (size--)
			{
				content = uart_read();
				pflash[0x5555] = 0xAA; // set flash to command mode
				pflash[0x2AAA] = 0x55;
				pflash[0x5555] = 0xA0; // byte program command
				*paddr++ = content;						
			}
			uart_write(cmd); // acknowledge by returning the command value
			break;

		case HALFWORD_READ:
			content = *(UINT16*)paddr;
			paddr += 2;
			uart_write(content >> 8); // highbyte
			uart_write(content & 0xFF); // lowbyte
			break;
		
		case HALFWORD_WRITE:
			content = uart_read() << 8 | uart_read();
			*(UINT16*)paddr = content;
			paddr += 2;
			uart_write(cmd); // acknowledge by returning the command value
			break;
		
		case EXECUTE:
			{
				tpFunc pFunc = (tpFunc)paddr;
				pFunc();
				uart_write(cmd); // acknowledge by returning the command value
			}
			break;


		default:
			{
				volatile UINT16* pPortB = (UINT16*)0x05FFFFC2;
				*pPortB |= 1 << 6; // bit 6 is red LED on
				uart_write(~cmd); // error acknowledge
			}

		} // case
	}

	return 0;
}
