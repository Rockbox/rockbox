#ifndef _MINIMON_H
#define _MINIMON_H


// Commands
// all multibyte values (address, halfwords) are passed as big endian 
//  (most significant of the bytes first)

// set the address (all read/write commands will auto-increment it)
#define BAUDRATE       0x00 // followed by BRR value; response: command byte
#define ADDRESS        0x01 // followed by 4 bytes address; response: command byte
#define BYTE_READ      0x02 // response: 1 byte content
#define BYTE_WRITE     0x03 // followed by 1 byte content; response: command byte
#define BYTE_READ16    0x04 // response: 16 bytes content
#define BYTE_WRITE16   0x05 // followed by 16 bytes; response: command byte
#define BYTE_FLASH     0x06 // followed by 1 byte content; response: command byte
#define BYTE_FLASH16   0x07 // followed by 16 bytes; response: command byte
#define HALFWORD_READ  0x08 // response: 2 byte content
#define HALFWORD_WRITE 0x09 // followed by 2 byte content; response: command byte
#define EXECUTE        0x0A // response: command byte if call returns


#endif // _MINIMON_H

