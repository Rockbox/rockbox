#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "readbflt.h"

#define MAX_STR_LEN 50
#define STRING_CONTINUED "..."

int is_printable(char *data);
char *truncate_text(char *string);
void usage(char *appname);

int main(int argc, char *argv[])
{
        size_t data_size = 0;
	uint32_t print_uint32 = 0;
	unsigned char *data = NULL;
	struct bflt_got **gots = NULL;
	struct bflt_header *header = NULL;
	struct bflt_reloc **relocations = NULL;
	char *filename = NULL, *print_string = NULL, *flags_string = NULL;
	int i = 0, c = 0, retval = EXIT_FAILURE, show_gots = 0, show_relocs = 0, got_count = 0, reloc_count = 0;

	fprintf(stderr, "\nReadbflt v0.1\n");
    	fprintf(stderr, "Copyright (c) 2012, Tactical Network Solutions, Craig Heffner <cheffner@tacnetsol.com>\n\n");

	if(argc == 1)
	{
		usage(argv[0]);
		goto end;
	}

	while((c = getopt(argc, argv, "grh")) != -1)
	{
		switch(c)
		{
			case 'g':
				show_gots = 1;
				break;
			case 'r':
				show_relocs = 1;
				break;
			default:
				usage(argv[0]);
				goto end;
		}
	}

	filename = argv[argc-1];

        data = read_bflt(filename, &data_size);

	if(data)
	{
		if(!is_bflt_file(data, data_size))
		{
			fprintf(stderr, "ERROR: Invalid %s file\n", BFLT_MAGIC);
		}
		else
		{
			header = get_bflt_header(data, data_size);
			if(header)
			{
				flags_string = get_flags_string(header->flags);

				printf("\n");

				printf("Rev:            %d\n", header->version);
                	        printf("Flags:          %s [0x%.2X]\n", flags_string, header->flags);
                	        printf("File size:      0x%.8X\n", (unsigned int)data_size);
                	        printf("Stack Size:     0x%.8X\n", header->stack_size);
                	        printf("Reloc Count:    %d\n", header->reloc_count);
                	        printf("Reloc Start:    0x%.8X\n", header->reloc_start);
				printf("Build date:     0x%.8X\n", header->build_date);
				printf("\n");

				if(flags_string) free(flags_string);

                	        printf("bFLT Sections:\n");
				printf("                %s     0x%.8X - 0x%.8X\n", TEXT, header->entry, header->icode_start);
				printf("                %s    0x%.8X - 0x%.8X\n", ICODE, header->icode_start, header->data_start);
                	        printf("                %s     0x%.8X - 0x%.8X\n", DATA, header->data_start, header->data_end);
				printf("                %s    0x%.8X - 0x%.8X\n", IDATA, header->data_end, header->idata_end);
                	        printf("                %s      0x%.8X - 0x%.8X\n", BSS, header->data_end, header->bss_end);
				printf("                %s     0x%.8X - 0x%.8X\n", IBSS, header->bss_end, header->ibss_end);
				printf("\n");

				if(show_gots)
				{
					gots = get_bflt_gots(data, data_size, &got_count);
				
					if(gots && got_count)
					{
						printf("bFLT GOT Entries:\n");

						for(i=0; i<got_count; i++)
						{
							printf("                0x%.8X => 0x%.8X ", gots[i]->got_entry, gots[i]->got_data_offset);

							if(gots[i]->got_data == NULL)
							{
								printf("[]\n");
							}
							else if(is_printable(gots[i]->got_data))
							{
								print_string = truncate_text(gots[i]->got_data);
								printf("[%s]\n", print_string);
								free(print_string);
							}
							else
							{
								memcpy((void *) &print_uint32, gots[i]->got_data, sizeof(uint32_t));
								printf("[0x%.8X]\n", print_uint32);
							}
						}
					
						printf("\n");
					}

					free_bflt_gots(gots, got_count);
				}

				if(show_relocs)
				{
					relocations = get_bflt_relocs(data, data_size, &reloc_count);
				
					if(relocations && reloc_count)
					{
						printf("bFLT Relocations:\n");

						for(i=0; i<reloc_count; i++)
						{
							printf("                0x%.8X => %s:0x%.8X -> 0x%.8X ", relocations[i]->reloc_entry, relocations[i]->reloc_section, relocations[i]->reloc_patch_address, relocations[i]->reloc_data_offset);

							if(relocations[i]->reloc_data == NULL)
							{
								printf("[]\n");
							}
							else if(is_printable(relocations[i]->reloc_data))
							{
								print_string = truncate_text(relocations[i]->reloc_data);
								printf("[%s]\n", print_string);
								if(print_string) free(print_string);
							}
							else
							{
									memcpy((void *) &print_uint32, relocations[i]->reloc_data, sizeof(uint32_t));
									printf("[0x%.8X]\n", print_uint32);
							}
						}

						printf("\n");
					}
					
					free_bflt_relocs(relocations, reloc_count);
				}

				free(header);
				retval = EXIT_SUCCESS;
			}
		}

		free_bflt(data, data_size);
	}

end:
	return retval;
}

/* Check for un-printable characters in the string. Returns 1 if all characters are printable, 0 if not. */
int is_printable(char *data)
{
	int i = 0, printable = 1;

	if(data)
	{
		for(i=0; data[i] != '\0' && i < strlen(data); i++)
		{
			if(data[i] < ' ' || data[i] > '~')
			{
				if(data[i] != '\r' && data[i] != '\n')
				{
					printable = 0;
				}

				break;
			}
		}
	}
	else
	{
		printable = 0;
	}

	return printable;
}

/* Truncates string to MAX_STR_LEN or at '\r' / '\n', whichever comes first.  */
char *truncate_text(char *string)
{
	char *nstring = NULL;
	int i = 0, size = 0;

	size = strlen(string) + strlen(STRING_CONTINUED) + 1;

	nstring = malloc(size);
	if(nstring)
	{
		memset(nstring, 0, size);
		
		for(i=0; i<strlen(string); i++)
		{
			if(i > MAX_STR_LEN || string[i] == '\n' || string[i] == '\r')
			{
				strcat(nstring, STRING_CONTINUED);
				break;
			}
			else
			{
				strncat(nstring, string+i, 1);
			}
		}
		
	}

	return nstring;
}

void usage(char *appname)
{
	fprintf(stderr, "Usage: %s [OPTIONS] <file>\n", appname);
	fprintf(stderr, "\n");
	
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "\t-r      Show relocation entries\n");
	fprintf(stderr, "\t-g      Show GOT entries\n");
	fprintf(stderr, "\t-h      Show program help\n");
	fprintf(stderr, "\n");

	return;
}
