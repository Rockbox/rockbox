// Nintendo NES/Famicom NSFE file info parser

// Game_Music_Emu 0.5.5
#ifndef NSFE_INFO_H
#define NSFE_INFO_H

#include "blargg_common.h"

struct Nsf_Emu;

// Allows reading info from NSFE file without creating emulator
struct Nsfe_Info {
	int playlist_size;
	int track_times_size;
	int track_count;
	int actual_track_count_;
	bool playlist_disabled;
	
	unsigned char playlist [256];
	int32_t track_times [256];
};

void Info_init( struct Nsfe_Info* this );
blargg_err_t Info_load( struct Nsfe_Info* this, void *data, long size, struct Nsf_Emu* );
void Info_disable_playlist( struct Nsfe_Info* this, bool b );
int Info_remap_track( struct Nsfe_Info* this, int i );
void Info_unload( struct Nsfe_Info* this );


#endif
