#include "config.h"

#include <stdio.h>
#include <string.h>	// strcmp()
#include <dirent.h>	// opendir() readdir() closedir()
#include <sys/stat.h>	// IS_DIR

#include "malloc.h"
#include "db.h"

extern int out_of_memory;

// dir-is-album: all files in the dir ARE the same album, use the first name found.
// dir-is-album-name: if no tag found, use the dir's instead of "<no album tag>"
//
// files in different dirs are ALWAYS different albums

static char* strip_path = NULL;
static char* add_path = NULL;

static int iterate_dir(char* dir);
/* Iterates over each item in the given directory
 * calls add_file() on each file
 * calls iterate_directory() on each directory (recursively)
 */

static int iterate_dir(char* dir) {
	DIR *d;
	struct dirent *e;
	struct stat s;
	int rc;

	assert(dir != NULL);

	if(!( d = opendir(dir) )) {
		DEBUGF("iterate_dir: could not open directory \"%s\"\n", dir);
		return ERR_FILE;
	}

	while(( e = readdir(d) )) {
		char *path;

		if( strcmp(e->d_name, ".") == 0 || strcmp(e->d_name, "..") == 0 )
			continue; // we don't want to descend or loop around...

		path = malloc(strlen(dir) + 1 + strlen(e->d_name) + 1); // "dir/d_name\0"
		if( path == NULL ) {
			DEBUGF("iterate_dir: could not malloc() directory-entry-name\n");
			return ERR_MALLOC;
		}
		strcpy(path, dir);
		strcat(path, "/");
		strcat(path, e->d_name);
#if defined OS_LINUX
		if( stat(path, &s) ) {
			DEBUGF("iterate_dir: could not stat(\"%s\")\n", path);
			return ERR_FILE;
		}
		
		if( S_ISDIR(s.st_mode) ) {
#elif defined OS_ROCKBOX
#error "Rockbox: not yet implemented: don't know how to list directory"
		if( false ) {
#elif defined OS_WINDOWS
		if( false ) {
#error "Windows: not yet implemented: don't know how to list directory"
#else
		if( false ) {
#error "No OS specified: don't know how to list directory"
#endif
			if(( rc = iterate_dir(path) )) {
				closedir(d);
				return rc;
			}
		} else {
			if(( rc = db_add(path, strip_path, add_path) )) {
				closedir(d);
				return rc;
			}
		}
		free(path);
	}

	if( closedir(d) ) {
		DEBUGF("iterate_dir: could not close directory \"%s\", ignoring...\n", dir);
	}

	return ERR_NONE;
}

int main(int argc, char* argv[]) {
	FILE *fd;
	
	if( argc != 2 ) {
		printf("usage: ./songdb dir\n");
		return 1;
	}

	strip_path = "/home/niels/";
	add_path   = "TEST/";

	db_construct();

	iterate_dir(argv[1]);
	
	fd = fopen("xxx.db", "w");
	db_write(fd);
	fclose(fd);

	db_destruct();

	malloc_stats();
	
	return 0;
}
