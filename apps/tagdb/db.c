#include <string.h> // strlen() strcpy() strcat()

#include "malloc.h"
#include "db.h"
#include "header.h"

#include "artist.h"
#include "album.h"
#include "song.h"
#include "file.h"

#include "tag_dummy.h"

#define CEIL32BIT(x) ( ((x) + 3) & 0xfffffffc )
#define CEIL32BIT_LEN(x) CEIL32BIT(strlen(x) + 1) // +1 because we want to store the \0 at least once

#define CATCH_MALLOC(condition) \
	while( condition ) { \
		int rc_catch_malloc = free_ram(); \
		if (rc_catch_malloc != ERR_NONE) { \
			DEBUGF("catch_malloc: " #condition ": could not free memory, failing...\n"); \
			return rc_catch_malloc; \
		} \
	}

#define CATCH_MALLOC_ERR(expr) CATCH_MALLOC( (expr) == ERR_MALLOC )
#define CATCH_MALLOC_NULL(expr) CATCH_MALLOC( (expr) == NULL )
// Loop the expression as long as it returns ERR_MALLOC (for CATCH_MALLOC_ERR)
// or NULL (for CATCH_MALLOC_NULL)
// on each failure, call free_ram() to free some ram. if free_ram() fails, return
// the fail-code
#define CATCH_ERR(expr) \
	CATCH_MALLOC_ERR(rc = expr); \
	if( rc != ERR_NONE ) { \
		DEBUGF("catch_err: " #expr ": failed\n"); \
		return rc; \
	}
// Catches all errors: if it's a MALLOC one, try to free memory,
// if it's another one, return the code

static int fill_artist_offsets(struct artist_entry *e, struct artist_size *max_s);
static int fill_album_offsets(struct album_entry *e, struct album_size *max_s);
static int fill_song_offsets(struct song_entry *e, struct song_size *max_s);
static int fill_file_offsets(struct file_entry *e, struct file_size *max_s);

static int do_add(const struct tag_info *t);

static int tag_empty_get(struct tag_info *t);
/* Adds "<no artist tag>" and "<no album tag>" if they're empty
 */

static int free_ram();
static char in_file = 0;

static int do_write(FILE *fd);

static struct array_buffer *artists;
static struct array_buffer *albums;
static struct array_buffer *songs;
static struct array_buffer *files;
static uint32_t artist_start=0, album_start=0, song_start=0, file_start=0;
static uint32_t artist_entry_len, album_entry_len, song_entry_len, file_entry_len;
static char *artists_file, *albums_file, *songs_file, *files_file;

int db_construct() {
	void *max_size;
	
	// struct array_buffer* new_array_buffer( int      (*cmp)(const void *a, const void *b),
	//                                        int      (*serialize)(FILE *fd, const void *e),
	//                                        int      (*unserialize)(void **e, FILE *fd),
	//                                        uint32_t (*get_length)(const void *size),
	//                                        int      (*write)(FILE *fd, void *e, const void *size),
	//                                        int      (*destruct)(void *e),
	//                                        char*    file_name,
	//                                        void*    max_size,
	//                                        int      (*max_size_update)(void *max_size, const void *e),
	//                                        int      (*max_size_destruct)(void *max_size),
	//                                        int      (*add_item_mem)(void *e, void *s, uint32_t item),
	//                                        int      (*add_item_file)(FILE *fd, void *e, void *s, uint32_t item)
	//                                      );

	if(!( max_size = (void*)new_artist_size() )) {
		DEBUGF("new_db: new_artist_size() failed\n");
		return ERR_MALLOC;
	}
	if(!( artists = new_array_buffer( (int      (*)(const void *a, const void *b)) artist_entry_compare,
	                                  (int      (*)(FILE *fd, const void *e)) artist_entry_serialize,
					  (int      (*)(void **e, FILE *fd)) artist_entry_unserialize,
					  (uint32_t (*)(const void *size)) artist_size_get_length,
					  (int      (*)(FILE *fd, void *e, const void *size)) artist_entry_write,
					  (int      (*)(void *e)) artist_entry_destruct,
					  NULL, // don't allow to switch to file
					  max_size,
					  (int      (*)(void *max_size, const void *e)) artist_size_max,
					  (int      (*)(void *max_size)) artist_size_destruct,
					  (int      (*)(void *e, void *s, uint32_t item)) artist_entry_add_album_mem,
					  (int      (*)(FILE *fd, void *e, void *s, uint32_t item)) artist_entry_add_album_file,
					  (int      (*)(void *e, void *s)) fill_artist_offsets
				        ) )) {
		DEBUGF("new_db: new_array_buffer() failed on artists[]\n");
		return ERR_MALLOC;
	}
	if(!( artists_file = malloc(12) )) { // artists.tmp
		DEBUGF("new_db: could not malloc() for artists[] file_name\n");
		return ERR_MALLOC;
	}
	strcpy(artists_file, "artists.tmp");
	
	if(!( max_size = (void*)new_album_size() )) {
		DEBUGF("new_db: new_album_size() failed\n");
		return ERR_MALLOC;
	}
	if(!( albums = new_array_buffer( (int      (*)(const void *a, const void *b)) album_entry_compare,
	                                 (int      (*)(FILE *fd, const void *e)) album_entry_serialize,
					 (int      (*)(void **e, FILE *fd)) album_entry_unserialize,
					 (uint32_t (*)(const void *size)) album_size_get_length,
					 (int      (*)(FILE *fd, void *e, const void *size)) album_entry_write,
					 (int      (*)(void *e)) album_entry_destruct,
					 NULL, // don't allow to switch to file
					 max_size,
					 (int      (*)(void *max_size, const void *e)) album_size_max,
					 (int      (*)(void *max_size)) album_size_destruct,
					 (int      (*)(void *e, void *s, uint32_t item)) album_entry_add_song_mem,
					 (int      (*)(FILE *fd, void *e, void *s, uint32_t item)) album_entry_add_song_file,
					 (int      (*)(void *e, void *s)) fill_album_offsets
				       ) )) {
		DEBUGF("new_db: new_array_buffer() failed on albums[]\n");
		return ERR_MALLOC;
	}
	if(!( albums_file = malloc(11) )) { // albums.tmp
		DEBUGF("new_db: could not malloc() for albums[] file_name\n");
		return ERR_MALLOC;
	}
	strcpy(albums_file, "albums.tmp");
	
	if(!( max_size = (void*)new_song_size() )) {
		DEBUGF("new_db: new_song_size() failed\n");
		return ERR_MALLOC;
	}
	if(!( songs = new_array_buffer( (int      (*)(const void *a, const void *b)) song_entry_compare,
	                                (int      (*)(FILE *fd, const void *e)) song_entry_serialize,
				        (int      (*)(void **e, FILE *fd)) song_entry_unserialize,
				        (uint32_t (*)(const void *size)) song_size_get_length,
				        (int      (*)(FILE *fd, void *e, const void *size)) song_entry_write,
				        (int      (*)(void *e)) song_entry_destruct,
				        NULL, // may switch to file, but we'd like to know about it
				        max_size,
				        (int      (*)(void *max_size, const void *e)) song_size_max,
				        (int      (*)(void *max_size)) song_size_destruct,
				        NULL,
				        NULL,
				        (int      (*)(void *e, void *s)) fill_song_offsets
				      ) )) {
		DEBUGF("new_db: new_array_buffer() failed on songs[]\n");
		return ERR_MALLOC;
	}
	if(!( songs_file = malloc(10) )) { // songs.tmp
		DEBUGF("new_db: could not malloc() for songs[] file_name\n");
		return ERR_MALLOC;
	}
	strcpy(songs_file, "songs.tmp");
	
	if(!( max_size = (void*)new_file_size() )) {
		DEBUGF("new_db: new_file_size() failed\n");
		return ERR_MALLOC;
	}
	if(!( files = new_array_buffer( (int      (*)(const void *a, const void *b)) file_entry_compare,
	                                (int      (*)(FILE *fd, const void *e)) file_entry_serialize,
				        (int      (*)(void **e, FILE *fd)) file_entry_unserialize,
				        (uint32_t (*)(const void *size)) file_size_get_length,
				        (int      (*)(FILE *fd, void *e, const void *size)) file_entry_write,
				        (int      (*)(void *e)) file_entry_destruct,
				        NULL,
				        max_size,
				        (int      (*)(void *max_size, const void *e)) file_size_max,
				        (int      (*)(void *max_size)) file_size_destruct,
				        NULL,
				        NULL,
				        (int      (*)(void *e, void *s)) fill_file_offsets
				      ) )) {
		DEBUGF("new_db: new_array_buffer() failed on files[]\n");
		return ERR_MALLOC;
	}
	if(!( files_file = malloc(10) )) { // files.tmp
		DEBUGF("new_db: could not malloc() for files[] file_name\n");
		return ERR_MALLOC;
	}
	strcpy(files_file, "files.tmp");
	
	return ERR_NONE;
}

int db_destruct() {
	int rc;

	CATCH_ERR( array_buffer_destruct(artists, 1) );
	artists = NULL;
	free(artists_file);
	artists_file = NULL;

	CATCH_ERR( array_buffer_destruct(albums, 1) );
	albums = NULL;
	free(albums_file);
	albums_file = NULL;

	CATCH_ERR( array_buffer_destruct(songs, 1) );
	songs = NULL;
	free(songs_file);
	songs_file = NULL;

	CATCH_ERR( array_buffer_destruct(files, 1) );
	files = NULL;
	free(files_file);
	files_file = NULL;

	return ERR_NONE;
}

static int do_add(const struct tag_info *t) {
	struct artist_entry *artist; uint32_t artistn;
	struct album_entry *album;  uint32_t albumn;
	struct song_entry *song;  uint32_t songn;
	struct file_entry *file;  uint32_t filen;
	int rc;

	// create file
	CATCH_MALLOC_NULL( file = new_file_entry( CEIL32BIT( strlen(t->directory) + 1 + strlen(t->filename) + 1 ) ) ); // "dir"."/"."file"."\0"
	
	// fill in file
	strcpy(file->name, t->directory);
	strcat(file->name, "/");
	strcat(file->name, t->filename);
	file->hash = 0xffffffff; // TODO
	file->song = songn = array_buffer_get_next_index(songs);
	file->rundb = 0xffffffff; // TODO
	
	// add
	CATCH_ERR( array_buffer_add(files, file, &filen) );
	
	// create artist
	CATCH_MALLOC_NULL( artist = new_artist_entry( CEIL32BIT_LEN(t->artist), 0) );
	// fill in
	strcpy(artist->name, t->artist);
	// see if it is already in
	CATCH_MALLOC_ERR( rc = array_buffer_find_entry(artists, artist, &artistn) );
	if( rc == ERR_NONE ) { // found it
		// remove our self-made one
		artist_entry_destruct(artist);
		artist = NULL;
	} else if( rc == ERR_NOTFOUND ) { // didn't find it
		// fill in the rest and add
		CATCH_ERR( artist_entry_resize(artist, artist->size.name_len, 1) );
		artist->album[0] = albumn = array_buffer_get_next_index(albums); // if artist isn't in, album will not be in either
		CATCH_ERR( array_buffer_add(artists, artist, &artistn) );
		// leave artist != NULL, to indicate that we made a new one
	} else { //error
		DEBUGF("do_add: could not search for artist in artists[]\n");
		return rc;
	}	
	
	
	// create album
	CATCH_MALLOC_NULL( album = new_album_entry(0,0) );
	// malloc for key
	CATCH_MALLOC_NULL( album->key = malloc( strlen(t->album) + 3 + strlen(t->artist) + 3 + strlen(t->directory) + 1 ) );
	// fill in
	strcpy(album->key, t->album);
	strcat(album->key, "___");
	strcat(album->key, t->artist);
	strcat(album->key, "___");
	strcat(album->key, t->directory);
	// see if it is already in
	CATCH_MALLOC_ERR( rc = array_buffer_find_entry(albums, album, &albumn) );
	if( rc == ERR_NONE ) { // found it
		assert(artist == NULL); // make sure artist was found; else we have trouble!
		// Remove our search-album and add the song to the already existing one
		album_entry_destruct(album);
		album = NULL;
		CATCH_ERR( array_buffer_entry_update(albums, albumn, songn) );
	} else if( rc == ERR_NOTFOUND ) { // didn't find it
		// fill in the rest of the info in this album and add it
		CATCH_ERR( album_entry_resize(album, CEIL32BIT_LEN(t->album), 1 ) );
		strcpy(album->name, t->album);
		album->artist = artistn;
		album->song[0] = songn;
		CATCH_ERR( array_buffer_add(albums, album, &albumn) );
	} else { // error
		DEBUGF("do_add: could not search for album in albums[]\n");
		return rc;
	}

	
	if( album != NULL && artist == NULL ) {
		// we have a new album from an already existing artist
		// add it!
		CATCH_ERR( array_buffer_entry_update(artists, artistn, albumn) );
	}


	// song
	CATCH_MALLOC_NULL( song = new_song_entry( CEIL32BIT_LEN(t->song), CEIL32BIT_LEN(t->genre)) );
	// fill in
	strcpy(song->name, t->song);
	song->artist = artistn;
	song->album = albumn;
	song->file = filen;
	strcpy(song->genre, t->genre);
	song->bitrate = t->bitrate;
	song->year = t->year;
	song->playtime = t->playtime;
	song->track = t->track;
	song->samplerate = t->samplerate;
	// add
	CATCH_ERR( array_buffer_add(songs, song, NULL) );

	return ERR_NONE;
}

static int tag_empty_get(struct tag_info *t) {
	assert( t != NULL );

	if( t->song == NULL ) {
		CATCH_MALLOC_NULL( t->song = (char*)malloc(14) );
		strcpy(t->song,   "<no song tag>");
	}
	if( t->genre == NULL ) {
		CATCH_MALLOC_NULL( t->genre = (char*)malloc(15) );
		strcpy(t->genre,  "<no genre tag>");
	}
	if( t->artist == NULL ) {
		CATCH_MALLOC_NULL( t->artist = (char*)malloc(16) );
		strcpy(t->artist, "<no artist tag>");
	}
	if( t->album == NULL ) {
		CATCH_MALLOC_NULL( t->album = (char*)malloc(15) );
		strcpy(t->album,  "<no album tag>");
	}

	return ERR_NONE;
}

int db_add(char* file_path, const char* strip_path, const char* add_path) {
	char *basename, *dir;
	struct tag_info *t;
	int rc;
	
	assert(file_path != NULL);

	// Create a new tag_info structure
	CATCH_MALLOC_NULL( t = new_tag_info() );

	// fill in the file_name
	basename = strrchr(file_path, '/');	// TODO: add \ for windows
	if( basename == NULL ) {
		basename = file_path; // no / in the path, so it's only a filename
		dir = NULL;
	} else {
		dir = file_path;
		basename[0] = '\0'; // set the / to \0 to split the string
		basename++; // skip past the /
	}
	CATCH_MALLOC_NULL( t->filename = malloc(strlen(basename)+1) ); // +1 for the '\0' termination
	strcpy(t->filename, basename);

	// convert the path
	if( strip_path != NULL && strlen(strip_path) > 0) {
		if( dir == NULL || strncmp(file_path, strip_path, strlen(strip_path)) ) {
			printf("db_add: could not strip path from \"%s\"\n", file_path);
		} else {
			dir += strlen(strip_path); // skip the path to strip 
		}
	}
	if( add_path != NULL ) {
		CATCH_MALLOC_NULL( t->directory = malloc( strlen(add_path) + strlen(dir) + 1 ) ); // +1 for '\0' termination
		strcpy(t->directory, add_path);
		strcat(t->directory, dir);
	} else {
		CATCH_MALLOC_NULL( t->directory = malloc( strlen(dir) + 1 ) );
		strcpy(t->directory, dir);
	}

	// restore the file_path to it's original state
	if( dir != NULL) *(basename-1) = '/';

	// So far we have:
	//  filename
	//  directory
	// try to get the rest from tag-information:
	//tag_id3v2_get(file_path, t);
	//tag_id3v1_get(file_path, t);
	tag_dummy(file_path, t);
	
	// If it is still empty here, skip this file
	if( t->artist==NULL && t->song==NULL && t->album==NULL && t->genre==NULL) {
		tag_info_destruct(t); // we won't need it anymore
		return ERR_NONE;
	}

	// fill in empty tags with "<no ... tag>"
	CATCH_ERR( tag_empty_get(t) );
	
	// all filled in, now add it
	CATCH_ERR( do_add(t) );

	tag_info_destruct(t); // we won't need it anymore

	return ERR_NONE;
}

static int free_ram() {
	// put things in file that we won't need to search a lot:
	//  files[] and songs[] are write only
	//  artists[] and albums[] should stay in memory as long as possible
	// albums[] is updated for every song;
	// artists[] for every album: artists[] will be the first to loose ram...
	if(!( in_file & 0x01 )) { // files[] is still in ram
		in_file |= 0x01;
		// switch files[] to file-mode
		files->file_name = files_file;
		files_file = NULL; // since array_buffer will clean this up
		return array_buffer_switch_to_file(files);
	} else if(!( in_file & 0x02 )) { // song[] is still in ram
		in_file |= 0x02;
		// switch songs[] to file-mode
		songs->file_name = songs_file;
		songs_file = NULL; // since array_buffer will clean this up
		return array_buffer_switch_to_file(songs);
	} else if(!( in_file & 0x04 )) { // artists[] is still in ram
		in_file |= 0x04;
		// switch artists[] to file-mode
		artists->file_name = artists_file;
		artists_file = NULL; // since array_buffer will clean this up
		return array_buffer_switch_to_file(artists);
	} else if(!( in_file & 0x08 )) { // albums[] is still in ram
		in_file |= 0x08;
		// switch albums[] to file-mode
		albums->file_name = albums_file;
		albums_file = NULL; // since array_buffer will clean this up
		return array_buffer_switch_to_file(albums);
	} else {
		// all is already in file mode, sorry...
		DEBUGF("free_ram: everything is already in file-mode, cannot free more ram, sorry...\n");
		return ERR_MALLOC;
	}
}

static int fill_artist_offsets(struct artist_entry *e, struct artist_size *max_s) {
	uint32_t i;
	
	assert(e != NULL);
	assert(album_start != 0);
	
	for(i=0; i<e->size.album_count; i++) {
		e->album[i] = album_start + e->album[i] * album_entry_len;
	}
	return ERR_NONE;
}

static int fill_album_offsets(struct album_entry *e, struct album_size *max_s) {
	uint32_t i;
	
	assert(e != NULL);
	assert(song_start != 0);
	
	e->artist = artist_start + e->artist * artist_entry_len;
	for(i=0; i<e->size.song_count; i++) {
		e->song[i] = song_start + e->song[i] * song_entry_len;
	}
	return ERR_NONE;
}

static int fill_song_offsets(struct song_entry *e, struct song_size *max_s) {
	
	assert(e != NULL);
	assert(artist_start != 0);
	assert(album_start != 0);
	assert(file_start != 0);
	
	e->artist = artist_start + e->artist * artist_entry_len;
	e->album  = album_start  + e->album  * album_entry_len;
	e->file   = file_start   + e->file   * file_entry_len;
	return ERR_NONE;
}

static int fill_file_offsets(struct file_entry *e, struct file_size *max_s) {
	
	assert(e != NULL);
	assert(song_start != 0);

	e->song = song_start + e->song * song_entry_len;
	return ERR_NONE;
}

static int do_write(FILE *fd) {
	int rc;
	struct header h;
	
	assert(fd != NULL);

	// make a header
	h.magic[0] = 'R'; h.magic[1] = 'D'; h.magic[2] = 'B';
	h.version = 0x03;
	
	h.artist_start = artist_start = HEADER_SIZE;
	h.album_start  = album_start  = h.artist_start + array_buffer_get_length(artists); // TODO error check
	h.song_start   = song_start   = h.album_start + array_buffer_get_length(albums);
	h.file_start   = file_start   = h.song_start + array_buffer_get_length(songs);

	h.artist_count = artists->count;
	h.album_count = albums->count;
	h.song_count = songs->count;
	h.file_count = files->count;

	h.artist_len = ((struct artist_size*)artists->max_size)->name_len;
	h.album_len  = ((struct album_size*)albums->max_size)->name_len;
	h.song_len   = ((struct song_size*)songs->max_size)->name_len;
	h.genre_len  = ((struct song_size*)songs->max_size)->genre_len;
	h.file_len   = ((struct file_size*)files->max_size)->name_len;

	artist_entry_len = artist_size_get_length(artists->max_size); // TODO error check
	album_entry_len = album_size_get_length(albums->max_size);
	song_entry_len = song_size_get_length(songs->max_size);
	file_entry_len = file_size_get_length(files->max_size);

	h.song_array_count = ((struct album_size*)albums->max_size)->song_count;
	h.album_array_count = ((struct artist_size*)artists->max_size)->album_count;

	h.flags.reserved = 0;
	h.flags.rundb_dirty = 1;

	// write the header
	CATCH_ERR( header_write(fd, &h) );

	// write the arrays
	CATCH_ERR( array_buffer_write(fd, artists) );
	CATCH_ERR( array_buffer_write(fd, albums) );
	CATCH_ERR( array_buffer_write(fd, songs) );
	CATCH_ERR( array_buffer_write(fd, files) );

	return ERR_NONE;
}

int db_write(FILE *fd) {
	int rc;
	// sort everything
	CATCH_ERR( array_buffer_sort(artists) );
	CATCH_ERR( array_buffer_sort(albums) );
	CATCH_ERR( array_buffer_sort(songs) );
	CATCH_ERR( array_buffer_sort(files) );

	CATCH_ERR( do_write(fd) );

	return ERR_NONE;
}

struct tag_info* new_tag_info() {
	struct tag_info *t;
	t = malloc(sizeof(struct tag_info));
	if( t == NULL ) {
		DEBUGF("new_tag_info: could not malloc() for tag_info\n");
		return NULL;
	}

	t->directory = NULL;
	t->filename = NULL;
	t->song = NULL;
	t->artist = NULL;
	t->album = NULL;
	t->genre = NULL;
	t->bitrate = 0;
	t->year = 0;
	t->playtime = 0;
	t->track = 0;
	t->samplerate = 0;

	return t;
}

int tag_info_destruct(struct tag_info *t) {
	assert(t != NULL);

	free(t->directory);
	t->directory = NULL;
	free(t->filename);
	t->filename = NULL;
	free(t->song);
	t->song = NULL;
	free(t->artist);
	t->artist = NULL;
	free(t->album);
	t->album = NULL;
	free(t->genre);
	t->genre = NULL;
	t->bitrate = 0;
	t->year = 0;
	t->playtime = 0;
	t->track = 0;
	t->samplerate = 0;

	free(t);

	return ERR_NONE;
}
