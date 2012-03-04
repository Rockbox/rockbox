/*	MikMod sound library
	(c) 2003-2004 Raphael Assenat and others - see file
	AUTHORS for complete list.

	This library is free software; you can redistribute it and/or modify
	it under the terms of the GNU Library General Public License as
	published by the Free Software Foundation; either version 2 of
	the License, or (at your option) any later version.
 
	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU Library General Public License for more details.
 
	You should have received a copy of the GNU Library General Public
	License along with this library; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
	02111-1307, USA.
*/

/*==============================================================================

  $Id: load_gt2.c,v 1.2 2005/03/30 19:09:35 realtech Exp $

  Graoumf tracker format (.GT2)

==============================================================================*/

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <ctype.h>
#include <stdio.h>
#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#include <string.h>

#include "mikmod_internals.h"

typedef struct GT_NOTE {
	UBYTE	note; /* 24-127, 48 is middle C-2. 0 for no note */
	UBYTE	inst; /* instrument, 1-255, 0 for none */
	UWORD	effect; /* 0 for no FX */
	UBYTE	vv; /* volume, 1-255, 0 for no volume */
} GT_NOTE;

/* general info chunk */
typedef struct GT2_CHUNK {
	UBYTE	magic[4]; /* must be 'GT2' */
	UBYTE	version; /* 01 = v0.7, 02=v0.726, 03=v0.731 */
	ULONG	chunk_size;
	CHAR	module_name[33]; /* 32 bytes in file */
	CHAR	comments_author[161]; /* 160 bytes in file */
	UBYTE	date_day;
	UBYTE	date_month;
	UWORD	date_year;
	CHAR	tracker_name[25]; /* 24 in file */
	UWORD	initial_speed;
	UWORD	initial_tempo;
	UWORD	initial_master_volume; /* 000 - fff */
	UWORD	num_voices; /* for the following panning section */
	UWORD	*voice_pannings; /* 000 - 800 - fff */
} GT2_CHUNK;

/* track volume chunk */
typedef struct TVOL_CHUNK {
	UBYTE	id[4]; /* must be TVOL */
	ULONG	chunk_size;
	UWORD	num_tracks; /* for the following array */
	UWORD	*track_volumes; /* 0000 - 1000 - FFFF */
} TVOL_CHUNK;

/* extra-comment chunk */
typedef struct XCOM_CHUNK {
	UBYTE	id[4]; /* must be XCOM */
	ULONG	chunk_size;
	ULONG	comment_len;
	CHAR	*comment; /* comment_len + 1 allocated */
} XCOM_CHUNK;

/* song chunk */
typedef struct SONG_CHUNK {
	UBYTE	id[4]; /* must be SONG */
	ULONG	chunk_size;
	UWORD	song_length;
	UWORD	song_repeat_point;
	UWORD	*patterns; /* pattern numbers */
} SONG_CHUNK;

/* pattern set chunk */
typedef struct PATS_CHUNK {
	UBYTE	id[4]; /* must be PATS */
	ULONG	chunk_size;
	UWORD	num_tracks; /* total number of tracks for the song */
	UWORD	num_patterns; /* number of patterns saved */
} PATS_CHUNK;

/* pattern chunk */
typedef struct PATD_CHUNK {
	UBYTE	id[4]; /* must be PATD */
	ULONG	chunk_size;
	UWORD	pattern_number;
	CHAR	pattern_name[17]; /* 16 in file */
	UWORD	codage_version; /* only 0 expected for now */
	/* version 0 (full pattern) */
	UWORD	num_lines;
	UWORD	num_tracks;
	GT_NOTE *notes; /* sizeof(GT_NOTE) * num_lines * num_tracks */
} PATD_CHUNK;

/* instrument set chunk */
typedef struct ORCH_CHUNK {
	UBYTE	id[4]; /* must be ORCH */
	ULONG	chunk_size;
	UWORD	num_instruments; /* number of instruments saved */
} ORCH_CHUNK;

typedef struct INST_NOTE {
	UBYTE 	samp_number;/* sample number for midi note */
	CHAR	tranp;		/* transposition for note */
} INST_NOTE;

/* instrument chunk */
typedef struct INST_CHUNK {
	UBYTE	id[4]; /* must be INST */
	ULONG	chunk_size;
	UWORD	instrument_number;
	CHAR	name[29]; /* 28 in file */
	UWORD	type; /* 0 = sample */
	UWORD	volume; /* volume, 0-255 */
	UWORD	auto_panning; /* autopanning, 000 - 800 - fff, -1 no autopanning */
	UWORD	volume_enveloppe_number;
	UWORD	tone_enveloppe_number;
	UWORD	pan_enveloppe_number;
	UBYTE	reserved[10];
	INST_NOTE	note[128];
} INST_CHUNK;

typedef struct SAMP_CHUNK {
	UBYTE	id[4]; /* must be SAMP */
	ULONG	chunk_size;
	UWORD	sample_number;
	CHAR	name[29]; /* 28 in file */
	UWORD	flags; /* bit0: 0 = mono, 1 = stereo  bit1: 0 normal loop, bit2: ping pong loop */
	UWORD	autopanning; /* 000 - 800 - fff */
	UWORD	num_bits; /* 8 or 16 */
	UWORD	rate; /* between 2000 and 65000 */
	ULONG	length; /* bytes */
	ULONG	loop_start; /* bytes */
	ULONG	loop_len; /* bytes */
	UWORD	volume; /* 0 - 255 */
	UWORD	finetune; /*  (-8..+7 -> -1..+7/8 halftone) */
	UWORD	codage; /* 0 */
	UBYTE	*data;
} SAMP_CHUNK;

typedef struct xENV_CHUNK {
	UBYTE	id[4]; /* must be VENV, TENV or PENV */
	ULONG	chunk_size;
	UWORD	envelope_number;
	CHAR	name[21]; /* 20 in file */
	UWORD	keyoff_offset;
	UBYTE	*data;	
} xENV_CHUNK;

typedef struct ENDC_CHUNK {
	UBYTE	id[4]; /* must be ENDC */
	ULONG	chunk_size;
	ULONG	total_module_size;
} ENDC_CHUNK;


typedef union GT_CHUNK
{
	UBYTE	id[4]; /* must be TVOL */
	GT2_CHUNK gt2;
	TVOL_CHUNK tvol;
	XCOM_CHUNK xcom;
	SONG_CHUNK song;
	PATS_CHUNK pats;
	PATD_CHUNK patd;
	ORCH_CHUNK orch;
	INST_CHUNK inst;
	SAMP_CHUNK samp;
	xENV_CHUNK xenv;
	ENDC_CHUNK endc;
} GT_CHUNK;

static GT_CHUNK *loadChunk(void)
{	
	GT_CHUNK *new_chunk = MikMod_malloc(sizeof(GT_CHUNK));

	/* the file chunk id only use 3 bytes, others 4 */
	_mm_read_UBYTES(new_chunk->id, 3, modreader);
	if (!	(new_chunk->id[0]=='G' &&
			new_chunk->id[1]=='T' &&
			new_chunk->id[2]=='2') 
		)
	{
		_mm_read_UBYTES(&new_chunk->id[3], 1, modreader);
	}
	else
	{
		new_chunk->id[3] = ' ';
	}

	printf(">> %c%c%c%c\n", new_chunk->id[0], new_chunk->id[1], new_chunk->id[2], new_chunk->id[3]);
	
	if (!memcmp(new_chunk, "GT2", 3)) {
		_mm_read_UBYTES(&new_chunk->gt2.version, 1, modreader);
		_mm_read_M_ULONGS(&new_chunk->gt2.chunk_size, 1, modreader);
		new_chunk->gt2.module_name[32] = 0;
		_mm_read_UBYTES(&new_chunk->gt2.module_name, 32, modreader);
		new_chunk->gt2.comments_author[160] = 0;
		_mm_read_UBYTES(&new_chunk->gt2.comments_author, 160, modreader);
		_mm_read_UBYTES(&new_chunk->gt2.date_day, 1, modreader);
		_mm_read_UBYTES(&new_chunk->gt2.date_month, 1, modreader);
		_mm_read_M_UWORDS(&new_chunk->gt2.date_year, 1, modreader);
		new_chunk->gt2.tracker_name[24] = 0;
		_mm_read_UBYTES(&new_chunk->gt2.tracker_name, 24, modreader);
		_mm_read_M_UWORDS(&new_chunk->gt2.initial_speed, 1, modreader);
		_mm_read_M_UWORDS(&new_chunk->gt2.initial_tempo, 1, modreader);
		_mm_read_M_UWORDS(&new_chunk->gt2.initial_master_volume, 1, modreader);
		_mm_read_M_UWORDS(&new_chunk->gt2.num_voices, 1, modreader);
		new_chunk->gt2.voice_pannings = MikMod_malloc(2*new_chunk->gt2.num_voices);
		_mm_read_M_UWORDS(new_chunk->gt2.voice_pannings, new_chunk->gt2.num_voices, modreader);
		return new_chunk;
	}

	if (!memcmp(new_chunk, "TVOL", 4)) { 
		new_chunk->tvol.chunk_size = _mm_read_M_ULONG(modreader);
		new_chunk->tvol.num_tracks = _mm_read_M_UWORD(modreader);
		new_chunk->tvol.track_volumes = MikMod_malloc(new_chunk->tvol.num_tracks * 2);
		_mm_read_M_UWORDS(new_chunk->tvol.track_volumes, new_chunk->tvol.num_tracks, modreader);
		return new_chunk;
	}
	
	if (!memcmp(new_chunk, "XCOM", 4)) { 
		new_chunk->xcom.chunk_size = _mm_read_M_ULONG(modreader);
		new_chunk->xcom.comment_len = _mm_read_M_ULONG(modreader);
		new_chunk->xcom.comment = MikMod_malloc(new_chunk->xcom.comment_len + 1);
		_mm_read_UBYTES(new_chunk->xcom.comment, new_chunk->xcom.comment_len, modreader);
		return new_chunk;
	}
	
	if (!memcmp(new_chunk, "SONG", 4)) { 
		new_chunk->song.chunk_size = _mm_read_M_ULONG(modreader);
		new_chunk->song.song_length = _mm_read_M_UWORD(modreader);
		new_chunk->song.song_repeat_point = _mm_read_M_UWORD(modreader);
		new_chunk->song.patterns = MikMod_malloc(2*new_chunk->song.song_length);
		_mm_read_M_UWORDS(new_chunk->song.patterns, new_chunk->song.song_length, modreader);
		return new_chunk;
	}
	
	if (!memcmp(new_chunk, "PATS", 4)) { 
		new_chunk->pats.chunk_size = _mm_read_M_ULONG(modreader);
		new_chunk->pats.num_tracks = _mm_read_M_UWORD(modreader);
		new_chunk->pats.num_patterns = _mm_read_M_UWORD(modreader);
		return new_chunk;
	}

	if (!memcmp(new_chunk, "PATD", 4)) {
		new_chunk->patd.chunk_size = _mm_read_M_ULONG(modreader);
		new_chunk->patd.pattern_number = _mm_read_M_UWORD(modreader);
		new_chunk->patd.pattern_name[16] = 0;
		_mm_read_UBYTES(new_chunk->patd.pattern_name, 16, modreader);
		new_chunk->patd.codage_version = _mm_read_M_UWORD(modreader);
		new_chunk->patd.num_lines = _mm_read_M_UWORD(modreader);
		new_chunk->patd.num_tracks = _mm_read_M_UWORD(modreader);
		new_chunk->patd.notes = MikMod_malloc(5 * 
										new_chunk->patd.num_lines * 
										new_chunk->patd.num_tracks);
		_mm_read_UBYTES(new_chunk->patd.notes, 
						new_chunk->patd.num_lines * new_chunk->patd.num_tracks * 5,
						modreader);
		return new_chunk;
	}
	
	if (!memcmp(new_chunk, "ORCH", 4)) { 
		new_chunk->orch.chunk_size = _mm_read_M_ULONG(modreader);
		new_chunk->orch.num_instruments = _mm_read_M_UWORD(modreader);
		return new_chunk;
	}
	if (!memcmp(new_chunk, "INST", 4)) { 
		return new_chunk;
	}
	if (!memcmp(new_chunk, "SAMP", 4)) { 
		return new_chunk;
	}
	if (!memcmp(new_chunk, "VENV", 4)) { 
		return new_chunk;
	}
	if (!memcmp(new_chunk, "TENV", 4)) { 
		return new_chunk;
	}
	if (!memcmp(new_chunk, "PENV", 4)) { 
		return new_chunk;
	}
	if (!memcmp(new_chunk, "ENDC", 4)) { 
		return new_chunk;
	}

	printf("?? %c%c%c%c\n", new_chunk->id[0], new_chunk->id[1], new_chunk->id[2], new_chunk->id[3]);
	
	MikMod_free(new_chunk);
	return NULL; // unknown chunk
}

static int GT2_Init(void)
{
	return 1;
}

static int GT2_Test(void)
{
	UBYTE magic[3];
	_mm_fseek(modreader, 0, SEEK_SET);

	_mm_read_UBYTES(magic, 3, modreader);

	if (magic[0] == 'G' && magic[1] == 'T' && magic[2] == '2') { return 1; }
	
	return 0;
}

static int GT2_Load(int curious)
{
	GT_CHUNK *tmp;
    (void)curious;

	_mm_fseek(modreader, 0, SEEK_SET);
	while (	(tmp = loadChunk() )) 
	{
		printf("%c%c%c%c\n", tmp->id[0], tmp->id[1], tmp->id[2], tmp->id[3]);

	}
	
	return 0;
}

static void GT2_Cleanup(void)
{
}

static CHAR *GT2_LoadTitle(void)
{
	CHAR title[33];
	_mm_fseek(modreader, 8, SEEK_SET);

	_mm_read_UBYTES(title, 32, modreader);
	title[32]=0;

	return (DupStr(title, 32, 1));
}


MIKMODAPI MLOADER load_gt2 = {
	NULL,
	"Graoumf Tracker 2 module",
	"Graoumf Tracker 2",
	GT2_Init,
	GT2_Test,
	GT2_Load,
	GT2_Cleanup,
	GT2_LoadTitle
};



