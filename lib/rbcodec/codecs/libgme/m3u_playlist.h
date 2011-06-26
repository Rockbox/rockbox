// M3U entries parser, with support for subtrack information

// Game_Music_Emu 0.5.2
#ifndef M3U_PLAYLIST_H
#define M3U_PLAYILST_H

#include "blargg_common.h"

struct entry_t
{
	unsigned char track;  // 1-based
	int length; // milliseconds
};

/* Short version of the m3u playlist */
struct M3u_Playlist
{
	unsigned char size;
	struct entry_t *entries;
};

static inline void M3u_load_data(struct M3u_Playlist* this, void *addr)
{
	if( addr == NULL ) return;
	/* m3u entries data must be at offset 100,
		the first 99 bytes are used by metadata info */
	this->size = *(unsigned char *)(addr + 99);
	this->entries = (struct entry_t *)(addr+100);
}

#endif
