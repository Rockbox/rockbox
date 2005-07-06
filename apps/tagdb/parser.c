#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "config.h"

int errno;

int read_failure(FILE *fd) {
	fprintf(stderr, "Could not read from file: errno: %u ", errno);
	if( feof(fd) ) fprintf(stderr, "EOF");
	fprintf(stderr, "\n");
	return 1;
}

int mem_failure() {
	fprintf(stderr, "Could not (re)allocate memory\n");
	return 1;
}

int main(int argc, char *argv[]) {
	FILE *fd;
	uint32_t artist_start, album_start, song_start, file_start;
	uint32_t artist_count, album_count, song_count, file_count;
	uint32_t artist_len, album_array_count;
	uint32_t album_len, song_array_count;
	uint32_t song_len, genre_len;
	uint32_t file_len;
#define header_start 0
#define header_len 68

	uint32_t i, j;
	char *ct1 = NULL, *ct2 = NULL;	// char temp 1 and 2
	uint32_t it = 0;		// integer temp

	// input validation
	if( argc != 2 ) {
		fprintf(stderr, "usage: parser dbfile\n");
		return 1;
	}

	// open file
	fd = fopen(argv[1], "r");
	if( fd == NULL ) {
		fprintf(stderr, "Could not open file \"%s\"\n", argv[1]);
		return 1;
	}
	
	// read the header
	ct1 = realloc(ct1, 4); if( ct1 == NULL ) return mem_failure();
	if( fread(ct1, 4, 1, fd) != 1 ) return read_failure(fd);
	if( ct1[0] != 'R' || ct1[1] != 'D' || ct1[2] != 'B' ) {
		printf("No header found\n");
		return 1;
	}
	if( ct1[3] != 0x03 ) {
		printf("Not version 3\n");
		return 1;
	}

	if( fread(&artist_start, 4, 1, fd) != 1 ) return read_failure(fd); artist_start = BE32(artist_start);
	if( fread(&album_start, 4, 1, fd) != 1 ) return read_failure(fd); album_start = BE32(album_start);
	if( fread(&song_start, 4, 1, fd) != 1 ) return read_failure(fd); song_start = BE32(song_start);
	if( fread(&file_start, 4, 1, fd) != 1 ) return read_failure(fd); file_start = BE32(file_start);
	
	if( fread(&artist_count, 4, 1, fd) != 1 ) return read_failure(fd); artist_count = BE32(artist_count);
	if( fread(&album_count, 4, 1, fd) != 1 ) return read_failure(fd); album_count = BE32(album_count);
	if( fread(&song_count, 4, 1, fd) != 1 ) return read_failure(fd); song_count = BE32(song_count);
	if( fread(&file_count, 4, 1, fd) != 1 ) return read_failure(fd); file_count = BE32(file_count);

	if( fread(&artist_len, 4, 1, fd) != 1 ) return read_failure(fd); artist_len = BE32(artist_len);
	if( fread(&album_len, 4, 1, fd) != 1 ) return read_failure(fd); album_len = BE32(album_len);
	if( fread(&song_len, 4, 1, fd) != 1 ) return read_failure(fd); song_len = BE32(song_len);
	if( fread(&genre_len, 4, 1, fd) != 1 ) return read_failure(fd); genre_len = BE32(genre_len);
	if( fread(&file_len, 4, 1, fd) != 1 ) return read_failure(fd); file_len = BE32(file_len);

	if( fread(&song_array_count, 4, 1, fd) != 1 ) return read_failure(fd); song_array_count = BE32(song_array_count);
	if( fread(&album_array_count, 4, 1, fd) != 1 ) return read_failure(fd); album_array_count = BE32(album_array_count);

	if( fread(ct1, 4, 1, fd) != 1 ) return read_failure(fd);

	// print header info
	printf("HEADER");
	printf("\n  Artist start:  0x%08x = %u", artist_start, artist_start);
	if( artist_start != header_start + header_len )
		printf(" should be 0x%08x = %u", header_start + header_len, header_start + header_len);
	printf("\n  Album  start:  0x%08x = %u", album_start, album_start);
	if( album_start != artist_start + artist_count*(artist_len + 4*album_array_count) )
		printf(" should be 0x%08x = %u", artist_start + artist_count*(artist_len + 4*album_array_count),
				                 artist_start + artist_count*(artist_len + 4*album_array_count));
	printf("\n  Song   start:  0x%08x = %u", song_start, song_start);
	if( song_start != album_start + album_count*(album_len + 4 + 4*song_array_count) )
		printf(" should be 0x%08x = %u", album_start + album_count*(album_len + 4 + 4*song_array_count),
				                 album_start + album_count*(album_len + 4 + 4*song_array_count));
	printf("\n  File   start:  0x%08x = %u", file_start, file_start);
	if( file_start != song_start + song_count*(song_len + genre_len + 24) )
		printf(" should be 0x%08x = %u", song_start + song_count*(song_len + genre_len + 24),
				                 song_start + song_count*(song_len + genre_len + 24));
	
	printf("\n  Artist count:  0x%08x = %u\n", artist_count, artist_count);
	printf("  Album  count:  0x%08x = %u\n", album_count, album_count);
	printf("  Song   count:  0x%08x = %u\n", song_count, song_count);
	printf("  File   count:  0x%08x = %u\n", file_count, file_count);
	
	printf("  Artist len:    0x%08x = %u\n", artist_len, artist_len);
	printf("  Album  len:    0x%08x = %u\n", album_len, album_len);
	printf("  Song   len:    0x%08x = %u\n", song_len, song_len);
	printf("  Genre  len:    0x%08x = %u\n", genre_len, genre_len);
	printf("  File   len:    0x%08x = %u\n", file_len, file_len);
	
	printf("  Song[]  count: 0x%08x = %u\n", song_array_count, song_array_count);
	printf("  Album[] count: 0x%08x = %u\n", album_array_count, album_array_count);
	
	printf("  Reserved:      0x%08x\n", ct1[0] & 0xFFFFFFFE);
	printf("  Rundb dirty:   0x%01x\n", ct1[3] & 0x01);

	// iterate over artists:
	ct1 = realloc(ct1, artist_len); if( ct1 == NULL && artist_count!=0 ) return mem_failure();
	for(i=0; i < artist_count; i++) {
		printf("ARTIST %u/%u (offset 0x%08lx)\n", i, artist_count, (unsigned long)ftell(fd));
		
		if( fread(ct1, artist_len, 1, fd) != 1 ) return read_failure(fd);
		printf("  Name: \"%s\"\n", ct1);

		printf("  Albums:\n");
		for(j=0; j < album_array_count; j++) {
			if( fread(&it, 4, 1, fd) != 1 ) return read_failure(fd); it = BE32(it);
			printf("    Offset 0x%08x = ", it);
			if(it != 0) {
				printf("item %u\n", (it - album_start) / (album_len + 4 + 4*song_array_count));
			} else {
				printf("padding\n");
			}
		}
	}
	
	// iterate over albums:
	ct1 = realloc(ct1, album_len); if( ct1 == NULL && album_count!=0) return mem_failure();
	for(i=0; i < album_count; i++) {
		printf("ALBUM %u/%u (offset 0x%08lx)\n", i, album_count, (unsigned long)ftell(fd));
		
		if( fread(ct1, album_len, 1, fd) != 1 ) return read_failure(fd);
		printf("  Name:          \"%s\"\n", ct1);

		if( fread(&it, 4, 1, fd) != 1 ) return read_failure(fd); it = BE32(it);
		printf("  Artist offset: 0x%08x = item %u\n", it, (it - artist_start) / (artist_len + 4*album_array_count));

		printf("  Songs:\n");
		for(j=0; j < song_array_count; j++) {
			if( fread(&it, 4, 1, fd) != 1 ) return read_failure(fd); it = BE32(it);
			printf("    Offset 0x%08x = ", it);
			if(it != 0) {
				printf("item %u\n", (it - song_start) / (song_len + genre_len + 24));
			} else {
				printf("padding\n");
			}
		}
	}

	// iterate over songs:
	ct1 = realloc(ct1, song_len); if( ct1 == NULL && song_count!=0) return mem_failure();
	ct2 = realloc(ct2, genre_len); if( ct2 == NULL && song_count!=0) return mem_failure();
	for(i=0; i < song_count; i++) {
		printf("SONG %u/%u (offset 0x%08lx)\n", i, song_count, (unsigned long)ftell(fd));

		if( fread(ct1, song_len, 1, fd) != 1 ) return read_failure(fd);
		printf("  Name:          \"%s\"\n", ct1);
		
		if( fread(&it, 4, 1, fd) != 1 ) return read_failure(fd); it = BE32(it);
		printf("  Artist offset: 0x%08x = item %u\n", it, (it - artist_start) / (artist_len + 4*album_array_count));
		
		if( fread(&it, 4, 1, fd) != 1 ) return read_failure(fd); it = BE32(it);
		printf("  Album offset:  0x%08x = item %u\n", it, (it - album_start) / (album_len + 4 + 4*song_array_count));
		
		if( fread(&it, 4, 1, fd) != 1 ) return read_failure(fd); it = BE32(it);
		printf("  File offset:   0x%08x = item %u\n", it, (it - file_start) / (file_len + 12));
		
		if( fread(ct2, genre_len, 1, fd) != 1 ) return read_failure(fd);
		printf("  Genre:         \"%s\"\n", ct2);
		
		if( fread(&it, 4, 1, fd) != 1 ) return read_failure(fd); it = BE32(it);
		printf("  Bitrate:       0x%04x = %u\n", (it & 0xFFFF0000) >> 16, (it & 0xFFFF0000) >> 16);
		printf("  Year:          0x%04x = %u\n", it & 0x0000FFFF, it & 0x0000FFFF);
		
		if( fread(&it, 4, 1, fd) != 1 ) return read_failure(fd); it = BE32(it);
		printf("  Playtime:      0x%08x = %u\n", it, it);
		
		if( fread(&it, 4, 1, fd) != 1 ) return read_failure(fd); it = BE32(it);
		printf("  Track:         0x%04x = %u\n", (it & 0xFFFF0000) >> 16, (it & 0xFFFF0000) >> 16);
		printf("  Samplerate:    0x%04x = %u\n", it & 0x0000FFFF, it & 0x0000FFFF);
	}

	// iterate over file:
	ct1 = realloc(ct1, file_len); if( ct1 == NULL && file_count!=0) return mem_failure();
	for(i=0; i < file_count; i++) {
		printf("FILE %u/%u (offset 0x%08lx)\n", i, file_count, (unsigned long)ftell(fd));

		if( fread(ct1, file_len, 1, fd) != 1 ) return read_failure(fd);
		printf("  Name:         \"%s\"\n", ct1);

		if( fread(&it, 4, 1, fd) != 1 ) return read_failure(fd); it = BE32(it);
		printf("  Hash:         0x%08x = %u\n", it, it);
		
		if( fread(&it, 4, 1, fd) != 1 ) return read_failure(fd); it = BE32(it);
		printf("  Song offset:  0x%08x = item %u\n", it, (it - song_start) / (song_len + genre_len + 24));

		if( fread(&it, 4, 1, fd) != 1 ) return read_failure(fd); it = BE32(it);
		printf("  Rundb offset: 0x%08x = %u\n", it, it);
	}
	
	// close the file
	if( fclose(fd) != 0 ) {
		fprintf(stderr, "Could not close file\n");
		return 1;
	}
	
	return 0;
}
