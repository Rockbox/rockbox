

#ifndef __LOADER_H__
#define __LOADER_H__


typedef struct loader_s
{
	char *rom;
	char *base;
	char *sram;
	char *state;
	int ramloaded;
} loader_t;


extern loader_t loader;


int rom_load(void);
int sram_load(void);
int sram_save(void);
void loader_init(char *s);
void cleanup(void);

#endif


