#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <arpa/inet.h>

#include "readbflt.h"

/* Check for the bFLT magic bytes */
int is_bflt_file(unsigned char *data, size_t size)
{
	int retval = 0;

	if(data && size > sizeof(struct bflt_header))
	{
		if(memcmp(data, BFLT_MAGIC, BFLT_MAGIC_SIZE) == 0)
		{
			retval = 1;
		}
	}

	return retval;
}

/* Returns a string representing the bits set in the flags field */
char *get_flags_string(uint32_t flags)
{
	char *fstring = NULL;
	int fstring_len = 256;

	fstring = malloc(fstring_len);
	if(fstring)
	{
		memset(fstring, 0, fstring_len);

		if((flags & RAM) == RAM)
		{
			sprintf(fstring+strlen(fstring), "%s,", RAM_STRING);
		}

		if((flags & GZIP) == GZIP)
		{
			sprintf(fstring+strlen(fstring), "%s,", GZIP_STRING);
		}

		if((flags & GOTPIC) == GOTPIC)
		{
			sprintf(fstring+strlen(fstring), "%s,", GOTPIC_STRING);
		}

		/* Remove trailing comma */
		if(strlen(fstring) > 0)
		{
			memset(fstring+strlen(fstring)-1, 0, 1);
		}
	}

	return fstring;
}

/* Parse the bFLT header and return a bflt_header struct pointer */
struct bflt_header *get_bflt_header(unsigned char *data, size_t size)
{
	struct bflt_header *header = NULL;

	header = malloc(sizeof(struct bflt_header));
	if(header)
	{
		memset(header, 0, sizeof(struct bflt_header));
		memcpy(header, data, sizeof(struct bflt_header));

		/* Swap endianess */
		header->version = ntohl(header->version);
		header->entry = ntohl(header->entry);
		header->icode_start = ntohl(header->icode_start);
		header->data_start = ntohl(header->data_start);
		header->data_end = ntohl(header->data_end);
		header->idata_end = ntohl(header->idata_end);
		header->bss_end = ntohl(header->bss_end);
		header->ibss_end = ntohl(header->ibss_end);
		header->stack_size = ntohl(header->stack_size);
		header->reloc_start = ntohl(header->reloc_start);
		header->reloc_count = ntohl(header->reloc_count);
		header->flags = ntohl(header->flags);
		header->build_date = ntohl(header->build_date);
	}

	return header;
}

/* Free data allocated by get_bflt_gots */
void free_bflt_gots(struct bflt_got **gots, int got_count)
{
	int i = 0;

	if(gots)
	{
		for(i=0; i<got_count; i++)
		{
			if(gots[i])
			{
				free(gots[i]);
			}
		}

		free(gots);
	}
}

/* Returns an array of bflt_got structures, one for each valid GOT entry */
struct bflt_got **get_bflt_gots(unsigned char *data, size_t data_size, int *got_count)
{
	void *got_data = NULL;
	struct bflt_header *header = NULL;
	struct bflt_got **got_entries = NULL;
	uint32_t got_addr = 0, got_data_offset = 0;
	int i = 0, count = 0, malloc_size = 0, possible_got_entries = 0;

	header = get_bflt_header(data, data_size);
	if(header)
	{
		if((header->flags & GOTPIC) == GOTPIC)
		{
			possible_got_entries = (header->data_end - header->data_start) / sizeof(uint32_t);
			malloc_size = sizeof(struct got_entries *) * possible_got_entries;

			got_entries = malloc(malloc_size);
			if(got_entries)
			{
				memset(got_entries, 0, malloc_size);

				for(i=header->data_start; i<header->data_end; i += sizeof(uint32_t))
				{
					memcpy((void *) &got_addr, (data + i), sizeof(uint32_t));
					if(got_addr == -1)
					{
						break;
					}
					else if(got_addr != 0)
					{
						got_data_offset = got_addr + sizeof(struct bflt_header);
						if(got_data_offset < data_size)
						{
							got_data = (void *) (data + got_data_offset);
						}
						else
						{
							got_data = NULL;
						}

						got_entries[count] = malloc(sizeof(struct bflt_got));

						if(got_entries[count])
						{
							got_entries[count]->got_entry = got_addr;
							got_entries[count]->got_data_offset = got_data_offset;
							got_entries[count]->got_data = got_data;
		
							count++;
						}
					}
				}
			}
		}
		
		free(header);
	}

	*got_count = count;
	return got_entries;
}

/* Free data allocated by get_bflt_relocs */
void free_bflt_relocs(struct bflt_reloc **relocs, int reloc_count)
{
        int i = 0;

        if(relocs)
        {
                for(i=0; i<reloc_count; i++)
                {
                        if(relocs[i])
                        {
                                free(relocs[i]);
                        }
                }

                free(relocs);
        }
}

/* Parses the relocations in the bFLT data and returns an array of bflt_reloc structures */
struct bflt_reloc **get_bflt_relocs(unsigned char *data, size_t data_size, int *reloc_count)
{
	int i = 0, count = 0;
	void *reloc_data = NULL;
	char *reloc_section = NULL;
	struct bflt_header *header = NULL;
	struct bflt_reloc **relocations = NULL;
	uint32_t reloc_entry_offset = 0, base_offset = 0, reloc_entry = 0, reloc_pointer = 0, reloc_data_offset = 0;

	/* Addresses need to be adjusted based on the size of the bflt header */
	base_offset = sizeof(struct bflt_header);
	
	header = get_bflt_header(data, data_size);
	if(header)
	{
		relocations = malloc(sizeof(struct bflt_reloc *) * header->reloc_count);
		if(relocations)
		{
			memset(relocations, 0, (sizeof(struct bflt_reloc *) * header->reloc_count));

			for(i=0; i<header->reloc_count; i++)
			{
				reloc_entry = 0;
				reloc_pointer = 0;
				reloc_data_offset = 0;
				reloc_data = NULL;
				reloc_section = NULL;

				reloc_entry_offset = header->reloc_start + (i * sizeof(uint32_t));

				/* Sanity check; make sure we aren't trying to read past the end of data */
				if(reloc_entry_offset < data_size)
				{
					/* Get the offset of the relocation pointer for this relocation entry */
					memcpy((void *) &reloc_entry, data+reloc_entry_offset, sizeof(uint32_t));
					reloc_entry = ntohl(reloc_entry);

					/* Identify which section this relocation is in */
					if(reloc_entry < header->icode_start)
					{
						reloc_section = TEXT;
					}
					else if(reloc_entry < header->data_start)
					{
						reloc_section = ICODE;
					}
					else if(reloc_entry < header->data_end)
					{
						reloc_section = DATA;
					}
					else if(reloc_entry < header->idata_end)
					{
						reloc_section = IDATA;
					}
					else if(reloc_entry < header->bss_end)
					{
						reloc_section = BSS;
					}
					else
					{
						reloc_section = IBSS;
					}

					/* Calculate the address of the relocation pointer */
					reloc_pointer = reloc_entry + base_offset;

					/* Sanity check; make sure relocation pointer is not pointing past the end of data */
					if(reloc_pointer < data_size)
					{
						/* The relocation pointer address contains the offset to the actual data, minus the base_offset */
						memcpy((void *) &reloc_data_offset, (data+reloc_pointer), sizeof(uint32_t));
						reloc_data_offset = ntohl(reloc_data_offset) + base_offset;
	
						/* Sanity check; make sure the data offset isn't pointing past the end of data */
						if(reloc_data_offset < data_size)
						{
							reloc_data = data+reloc_data_offset;
						}
						else
						{
							reloc_data = NULL;
						}
					}	
					
					/* Create and populate a new bflt_reloc structure */
					relocations[count] = malloc(sizeof(struct bflt_reloc));
					if(relocations[count])
					{
						memset(relocations[count], 0, sizeof(struct bflt_reloc));
	
						relocations[count]->reloc_entry = reloc_entry;
						relocations[count]->reloc_patch_address = reloc_pointer;
						relocations[count]->reloc_data_offset = reloc_data_offset;
						relocations[count]->reloc_section = reloc_section;
						relocations[count]->reloc_data = reloc_data;
						count++;
					}
				}
			}
		}

		free(header);
	}

	*reloc_count = count;
	return relocations;
}

/* Unmap file contents */
void free_bflt(const void *data, size_t size)
{
        munmap((void *) data, size);
}

/* Reads in and returns the contents and size of a given file */
unsigned char *read_bflt(char *file, size_t *fsize)
{
        int fd = 0;
        size_t file_size = 0;
        struct stat _fstat = { 0 };
        unsigned char *buffer = NULL;

        fd = open(file, O_RDONLY);
        if(!fd)
        {
                perror(file);
                goto end;
        }

        if(stat(file, &_fstat) == -1)
        {
                perror(file);
                goto end;
        }

        if(_fstat.st_size > 0)
        {
                file_size = _fstat.st_size;
        }

        if(file_size > 0)
        {
                buffer = mmap(NULL, file_size, PROT_READ, (MAP_SHARED | MAP_NORESERVE), fd, 0);
                if(buffer == MAP_FAILED)
                {
                        perror("mmap");
                        buffer = NULL;
                }
                else
                {
                        *fsize = file_size;
                }
        }

end:
        if(fd) close(fd);
        return buffer;
}

