#ifndef __DB_H__
#define __DB_H__

#include "config.h"
#include <stdio.h>

#include "array_buffer.h"

struct tag_info {
	char* directory;
	char* filename;		// \0 terminated string's
	char* song;
	char* artist;
	char* album;
	char* genre;
	uint16_t bitrate;
	uint16_t year;
	uint32_t playtime;
	uint16_t track;
	uint16_t samplerate;
};

int db_construct();

int db_destruct();

int db_add(char* file_path, const char* strip_path, const char* add_path);

int db_sort();

int db_write(FILE *fd);

struct tag_info* new_tag_info();

int tag_info_destruct(struct tag_info *t);

#endif
