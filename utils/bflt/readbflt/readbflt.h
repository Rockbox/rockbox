#ifndef __BFLT_H__
#define __BFLT_H__

#include <stdint.h>

#define BFLT_MAGIC 		"bFLT"
#define BFLT_MAGIC_SIZE 	4
#define BFLT_FILLER_SIZE 	2

#define TEXT 			".text"
#define ICODE			".icode"
#define DATA 			".data"
#define IDATA			".idata"
#define BSS  			".bss"
#define IBSS			".ibss"

#define RAM			0x01
#define GOTPIC			0x02
#define GZIP			0x04

#define RAM_STRING 		"RAM"
#define GOTPIC_STRING 		"GOTPIC"
#define GZIP_STRING 		"GZIP"

#pragma pack(1)
struct bflt_header
{
	uint8_t magic[BFLT_MAGIC_SIZE];
	uint32_t version;
	uint32_t entry;
	uint32_t icode_start;
	uint32_t data_start;
	uint32_t data_end;
	uint32_t idata_end;
	uint32_t bss_end;
	uint32_t ibss_end;
	uint32_t stack_size;
	uint32_t reloc_start;
	uint32_t reloc_count;
	uint32_t flags;
	uint32_t build_date;
	uint32_t filler[BFLT_FILLER_SIZE];
};

struct bflt_got
{
	uint32_t got_entry;		/* GOT address, as stored in the GOT entry */
	uint32_t got_data_offset;	/* Actual offset of data (got_entry + sizeof(bflt_header)) */
	void *got_data;			/* Pointer to GOT data */
};

struct bflt_reloc
{
	uint32_t reloc_entry;		/* Relocation address, as stored in the relocation entry */
	uint32_t reloc_patch_address;	/* Address of the actual bytes to patch */
	uint32_t reloc_data_offset;	/* Absolute file offset of relocated data (reloc_pointer + section file offset) */
	char *reloc_section;		/* Pointer to TEXT, DATA or BSS, as appropriate */
	void *reloc_data;		/* Pointer to relocated data */
};
#pragma pack(0)

char *get_flags_string(uint32_t flags);
void free_bflt(const void *data, size_t size);
unsigned char *read_bflt(char *file, size_t *fsize);
int is_bflt_file(unsigned char *data, size_t size);
void free_bflt_gots(struct bflt_got **gots, int got_count);
void free_bflt_relocs(struct bflt_reloc **relocs, int reloc_count);
struct bflt_header *get_bflt_header(unsigned char *data, size_t size);
struct bflt_got **get_bflt_gots(unsigned char *data, size_t data_size, int *got_count);
struct bflt_reloc **get_bflt_relocs(unsigned char *data, size_t data_size, int *reloc_count);

#endif
