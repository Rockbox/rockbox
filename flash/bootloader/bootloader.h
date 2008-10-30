#ifndef NULL
#define NULL ((void*)0)
#endif

#define TRUE 1
#define FALSE 0

// scalar types
typedef unsigned char  UINT8;
typedef unsigned short UINT16;
typedef unsigned long  UINT32;
typedef int BOOL;

typedef void(*tpFunc)(void); // type for execute
typedef int(*tpMain)(void); // type for start vector to main()


// structure of an image in the flash
typedef struct 
{
	UINT32* pDestination; // address to copy it to
	UINT32 size;          // how many bytes of payload (to the next header)
	tpFunc pExecute;      // entry point
	UINT32 flags;         // uncompressed or compressed
	// end of header, now comes the payload
	UINT32 image[];       // the binary image starts here
	// after the payload, the next header may follow, all 0xFF if none
} tImage;

// flags valid for image header
#define IF_NONE   0x00000000
#define IF_UCL_2E 0x00000001 // image is compressed with UCL, algorithm 2e


// resolve platform dependency of F1 button check
#if defined PLATFORM_PLAYER
#define F1_MASK 0x0001 // Player has no F1 button, so we use "-"
#define F2_MASK 0x0008 // Player has no F2 button, so we use "Play"
#define F3_MASK 0x0004 // Player has no F3 button, so we use "+"

#elif defined PLATFORM_RECORDER
#define USE_ADC
#define CHANNEL 4
#define F1_LOWER 250
#define F1_UPPER 499
#define F2_LOWER 500
#define F2_UPPER 699
#define F3_LOWER 900
#define F3_UPPER 1023

#elif defined PLATFORM_FM
#define USE_ADC
#define CHANNEL 4
#define F1_LOWER 150
#define F1_UPPER 384
#define F2_LOWER 385
#define F2_UPPER 544
#define F3_LOWER 700
#define F3_UPPER 1023

#elif defined PLATFORM_ONDIO
#define USE_ADC
#define CHANNEL 4
#define F1_LOWER 0x2EF // Ondio has no F1 button,
#define F1_UPPER 0x3FF //  so we use "Left".
#define F2_LOWER 0x19D // Ondio has no F2 button,
#define F2_UPPER 0x245 //  so we use "Up".
#define F3_LOWER 0x246 // Ondio has no F3 button,
#define F3_UPPER 0x2EE //  so we use "Right".

#else
#error ("No platform given!")
#endif


#define FLASH_BASE 0x02000000 // start of the flash memory
#define FW_VERSION *(unsigned short*)(FLASH_BASE + 0xFE) // firmware version

// prototypes
int ucl_nrv2e_decompress_8(const UINT8 *src, UINT8 *dst, UINT32* dst_len);
void _main(void) __attribute__ ((section (".startup")));
int main(void);

// minimon commands
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
#define VERSION        0x0B // response: version


// linker symbols
extern UINT32 begin_text[];
extern UINT32 end_text[];
extern UINT32 begin_data[];
extern UINT32 end_data[];
extern UINT32 begin_bss[];
extern UINT32 end_bss[];
extern UINT32 begin_stack[];
extern UINT32 end_stack[];
extern UINT32 begin_iramcopy[];
extern UINT32 total_size[];
