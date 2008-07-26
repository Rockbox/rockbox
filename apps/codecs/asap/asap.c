/*
 * asap.c - ASAP engine
 *
 * Copyright (C) 2005-2008  Piotr Fusik
 *
 * This file is part of ASAP (Another Slight Atari Player),
 * see http://asap.sourceforge.net
 *
 * ASAP is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * ASAP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ASAP; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
#include "codeclib.h"
#if !defined(JAVA) && !defined(CSHARP)
#include <string.h>
#endif

#include "asap_internal.h"
#if !defined(JAVA) && !defined(CSHARP)
#include "players.h"
#endif

#define memcpy ci->memcpy
#define memcmp ci->memcmp
#define memset ci->memset
#define strcpy ci->strcpy
#define strcmp ci->strcmp
#define strstr ci->strcasestr


#define CMR_BASS_TABLE_OFFSET  0x70f

CONST_LOOKUP(byte, cmr_bass_table) = {
	0x5C, 0x56, 0x50, 0x4D, 0x47, 0x44, 0x41, 0x3E,
	0x38, 0x35, (byte) 0x88, 0x7F, 0x79, 0x73, 0x6C, 0x67,
	0x60, 0x5A, 0x55, 0x51, 0x4C, 0x48, 0x43, 0x3F,
	0x3D, 0x39, 0x34, 0x33, 0x30, 0x2D, 0x2A, 0x28,
	0x25, 0x24, 0x21, 0x1F, 0x1E
};

ASAP_FUNC int ASAP_GetByte(ASAP_State PTR ast, int addr)
{
	switch (addr & 0xff0f) {
	case 0xd20a:
		return PokeySound_GetRandom(ast, addr);
	case 0xd20e:
		if ((addr & AST extra_pokey_mask) != 0)
			return 0xff;
		return AST irqst;
	case 0xd20f:
		return 0xff;
	case 0xd40b:
		return AST scanline_number >> 1;
	default:
		return dGetByte(addr);
	}
}

ASAP_FUNC void ASAP_PutByte(ASAP_State PTR ast, int addr, int data)
{
	if ((addr >> 8) == 0xd2) {
		if ((addr & (AST extra_pokey_mask + 0xf)) == 0xe) {
			AST irqst |= data ^ 0xff;
#define SET_TIMER_IRQ(ch) \
			if ((data & AST irqst & ch) != 0) { \
				if (AST timer##ch##_cycle == NEVER) { \
					int t = AST base_pokey.tick_cycle##ch; \
					while (t < AST cycle) \
						t += AST base_pokey.period_cycles##ch; \
					AST timer##ch##_cycle = t; \
					if (AST nearest_event_cycle > t) \
						AST nearest_event_cycle = t; \
				} \
			} \
			else \
				AST timer##ch##_cycle = NEVER;
			SET_TIMER_IRQ(1);
			SET_TIMER_IRQ(2);
			SET_TIMER_IRQ(4);
		}
		else
			PokeySound_PutByte(ast, addr, data);
	}
	else if ((addr & 0xff0f) == 0xd40a) {
		if (AST cycle <= AST next_scanline_cycle - 8)
			AST cycle = AST next_scanline_cycle - 8;
		else
			AST cycle = AST next_scanline_cycle + 106;
	}
	else
		dPutByte(addr, data);
}

#define MAX_SONGS  32

CONST_LOOKUP(int, perframe2fastplay) = { 312, 312 / 2, 312 / 3, 312 / 4 };

FILE_FUNC abool load_native(ASAP_State PTR ast, ASAP_ModuleInfo PTR module_info,
                            const byte ARRAY module, int module_len, ASAP_OBX player)
{
#if defined(JAVA) || defined(CSHARP)
	try
#endif
	{
		int player_last_byte;
		int block_len;
		if (UBYTE(module[0]) != 0xff || UBYTE(module[1]) != 0xff)
			return FALSE;
#ifdef JAVA
		try {
			player.read();
			player.read();
			MODULE_INFO player = player.read();
			MODULE_INFO player += player.read() << 8;
			player_last_byte = player.read();
			player_last_byte += player.read() << 8;
		} catch (IOException e) {
			throw new RuntimeException();
		}
#elif defined(CSHARP)
		player.ReadByte();
		player.ReadByte();
		MODULE_INFO player = player.ReadByte();
		MODULE_INFO player += player.ReadByte() << 8;
		player_last_byte = player.ReadByte();
		player_last_byte += player.ReadByte() << 8;
#else
		MODULE_INFO player = UBYTE(player[2]) + (UBYTE(player[3]) << 8);
		player_last_byte = UBYTE(player[4]) + (UBYTE(player[5]) << 8);
#endif
		MODULE_INFO music = UBYTE(module[2]) + (UBYTE(module[3]) << 8);
		if (MODULE_INFO music <= player_last_byte)
			return FALSE;
		block_len = UBYTE(module[4]) + (UBYTE(module[5]) << 8) + 1 - MODULE_INFO music;
		if (6 + block_len != module_len) {
			int info_addr;
			int info_len;
			if (MODULE_INFO type != 'r' || 11 + block_len > module_len)
				return FALSE;
			/* allow optional info for Raster Music Tracker */
			info_addr = UBYTE(module[6 + block_len]) + (UBYTE(module[7 + block_len]) << 8);
			if (info_addr != MODULE_INFO music + block_len)
				return FALSE;
			info_len = UBYTE(module[8 + block_len]) + (UBYTE(module[9 + block_len]) << 8) + 1 - info_addr;
			if (10 + block_len + info_len != module_len)
				return FALSE;
		}
		if (ast != NULL) {
			COPY_ARRAY(AST memory, MODULE_INFO music, module, 6, block_len);
#ifdef JAVA
			int addr = MODULE_INFO player;
			do {
				int i;
				try {
					i = player.read(AST memory, addr, player_last_byte + 1 - addr);
				} catch (IOException e) {
					throw new RuntimeException();
				}
				if (i <= 0)
					throw new RuntimeException();
				addr += i;
			} while (addr <= player_last_byte);
#elif defined(CSHARP)
			int addr = MODULE_INFO player;
			do {
				int i = player.Read(AST memory, addr, player_last_byte + 1 - addr);
				if (i <= 0)
					throw new Exception();
				addr += i;
			} while (addr <= player_last_byte);
#else
			COPY_ARRAY(AST memory, MODULE_INFO player, player, 6, player_last_byte + 1 - MODULE_INFO player);
#endif
		}
		return TRUE;
	}
#ifdef JAVA
	finally {
		try {
			player.close();
		} catch (IOException e) {
			throw new RuntimeException();
		}
	}
#elif defined(CSHARP)
	finally {
		player.Close();
	}
#endif
}

FILE_FUNC void set_song_duration(ASAP_ModuleInfo PTR module_info, int player_calls)
{
	MODULE_INFO durations[MODULE_INFO songs] = (int) (player_calls * MODULE_INFO fastplay * 114000.0 / 1773447);
	MODULE_INFO songs++;
}

#define SEEN_THIS_CALL  1
#define SEEN_BEFORE     2
#define SEEN_REPEAT     3

FILE_FUNC void parse_cmc_song(ASAP_ModuleInfo PTR module_info, const byte ARRAY module, int pos)
{
	int tempo = UBYTE(module[0x19]);
	int player_calls = 0;
	int rep_start_pos = 0;
	int rep_end_pos = 0;
	int rep_times = 0;
	NEW_ARRAY(byte, seen, 0x55);
	INIT_ARRAY(seen);
	while (pos >= 0 && pos < 0x55) {
		int p1;
		int p2;
		int p3;
		if (pos == rep_end_pos && rep_times > 0) {
			for (p1 = 0; p1 < 0x55; p1++)
				if (seen[p1] == SEEN_THIS_CALL || seen[p1] == SEEN_REPEAT)
					seen[p1] = 0;
			rep_times--;
			pos = rep_start_pos;
		}
		if (seen[pos] != 0) {
			if (seen[pos] != SEEN_THIS_CALL)
				MODULE_INFO loops[MODULE_INFO songs] = TRUE;
			break;
		}
		seen[pos] = SEEN_THIS_CALL;
		p1 = UBYTE(module[0x206 + pos]);
		p2 = UBYTE(module[0x25b + pos]);
		p3 = UBYTE(module[0x2b0 + pos]);
		if (p1 == 0xfe || p2 == 0xfe || p3 == 0xfe) {
			pos++;
			continue;
		}
		p1 >>= 4;
		if (p1 == 8)
			break;
		if (p1 == 9) {
			pos = p2;
			continue;
		}
		if (p1 == 0xa) {
			pos -= p2;
			continue;
		}
		if (p1 == 0xb) {
			pos += p2;
			continue;
		}
		if (p1 == 0xc) {
			tempo = p2;
			pos++;
			continue;
		}
		if (p1 == 0xd) {
			pos++;
			rep_start_pos = pos;
			rep_end_pos = pos + p2;
			rep_times = p3 - 1;
			continue;
		}
		if (p1 == 0xe) {
			MODULE_INFO loops[MODULE_INFO songs] = TRUE;
			break;
		}
		p2 = rep_times > 0 ? SEEN_REPEAT : SEEN_BEFORE;
		for (p1 = 0; p1 < 0x55; p1++)
			if (seen[p1] == SEEN_THIS_CALL)
				seen[p1] = (byte) p2;
		player_calls += tempo << 6;
		pos++;
	}
	set_song_duration(module_info, player_calls);
}

FILE_FUNC abool parse_cmc(ASAP_State PTR ast, ASAP_ModuleInfo PTR module_info,
                          const byte ARRAY module, int module_len, abool cmr)
{
	int last_pos;
	int pos;
	if (module_len < 0x306)
		return FALSE;
	MODULE_INFO type = cmr ? 'z' : 'c';
	if (!load_native(ast, module_info, module, module_len, GET_OBX(cmc)))
		return FALSE;
	if (ast != NULL && cmr)
		COPY_ARRAY(AST memory, 0x500 + CMR_BASS_TABLE_OFFSET, cmr_bass_table, 0, sizeof(cmr_bass_table));
	/* auto-detect number of subsongs */
	last_pos = 0x54;
	while (--last_pos >= 0) {
		if (UBYTE(module[0x206 + last_pos]) < 0xb0
		 || UBYTE(module[0x25b + last_pos]) < 0x40
		 || UBYTE(module[0x2b0 + last_pos]) < 0x40)
			break;
	}
	MODULE_INFO songs = 0;
	parse_cmc_song(module_info, module, 0);
	for (pos = 0; pos < last_pos && MODULE_INFO songs < MAX_SONGS; pos++)
		if (UBYTE(module[0x206 + pos]) == 0x8f || UBYTE(module[0x206 + pos]) == 0xef)
			parse_cmc_song(module_info, module, pos + 1);
	return TRUE;
}

FILE_FUNC void parse_mpt_song(ASAP_ModuleInfo PTR module_info, const byte ARRAY module,
                              abool ARRAY global_seen, int song_len, int pos)
{
	int addr_to_offset = UBYTE(module[2]) + (UBYTE(module[3]) << 8) - 6;
	int tempo = UBYTE(module[0x1cf]);
	int player_calls = 0;
	NEW_ARRAY(byte, seen, 256);
	NEW_ARRAY(int, pattern_offset, 4);
	NEW_ARRAY(int, blank_rows, 4);
	NEW_ARRAY(int, blank_rows_counter, 4);
	INIT_ARRAY(seen);
	INIT_ARRAY(blank_rows);
	while (pos < song_len) {
		int i;
		int ch;
		int pattern_rows;
		if (seen[pos] != 0) {
			if (seen[pos] != SEEN_THIS_CALL)
				MODULE_INFO loops[MODULE_INFO songs] = TRUE;
			break;
		}
		seen[pos] = SEEN_THIS_CALL;
		global_seen[pos] = TRUE;
		i = UBYTE(module[0x1d0 + pos * 2]);
		if (i == 0xff) {
			pos = UBYTE(module[0x1d1 + pos * 2]);
			continue;
		}
		for (ch = 3; ch >= 0; ch--) {
			i = UBYTE(module[0x1c6 + ch]) + (UBYTE(module[0x1ca + ch]) << 8) - addr_to_offset;
			i = UBYTE(module[i + pos * 2]);
			if (i >= 0x40)
				break;
			i <<= 1;
			i = UBYTE(module[0x46 + i]) + (UBYTE(module[0x47 + i]) << 8);
			pattern_offset[ch] = i == 0 ? 0 : i - addr_to_offset;
			blank_rows_counter[ch] = 0;
		}
		if (ch >= 0)
			break;
		for (i = 0; i < song_len; i++)
			if (seen[i] == SEEN_THIS_CALL)
				seen[i] = SEEN_BEFORE;
		for (pattern_rows = UBYTE(module[0x1ce]); --pattern_rows >= 0; ) {
			for (ch = 3; ch >= 0; ch--) {
				if (pattern_offset[ch] == 0 || --blank_rows_counter[ch] >= 0)
					continue;
				for (;;) {
					i = UBYTE(module[pattern_offset[ch]++]);
					if (i < 0x40 || i == 0xfe)
						break;
					if (i < 0x80)
						continue;
					if (i < 0xc0) {
						blank_rows[ch] = i - 0x80;
						continue;
					}
					if (i < 0xd0)
						continue;
					if (i < 0xe0) {
						tempo = i - 0xcf;
						continue;
					}
					pattern_rows = 0;
				}
				blank_rows_counter[ch] = blank_rows[ch];
			}
			player_calls += tempo;
		}
		pos++;
	}
	if (player_calls > 0)
		set_song_duration(module_info, player_calls);
}

FILE_FUNC abool parse_mpt(ASAP_State PTR ast, ASAP_ModuleInfo PTR module_info,
                          const byte ARRAY module, int module_len)
{
	int track0_addr;
	int pos;
	int song_len;
	/* seen[i] == TRUE if the track position i has been processed */
	NEW_ARRAY(abool, global_seen, 256);
	if (module_len < 0x1d0)
		return FALSE;
	MODULE_INFO type = 'm';
	if (!load_native(ast, module_info, module, module_len, GET_OBX(mpt)))
		return FALSE;
	track0_addr = UBYTE(module[2]) + (UBYTE(module[3]) << 8) + 0x1ca;
	if (UBYTE(module[0x1c6]) + (UBYTE(module[0x1ca]) << 8) != track0_addr)
		return FALSE;
	/* Calculate the length of the first track. Address of the second track minus
	   address of the first track equals the length of the first track in bytes.
	   Divide by two to get number of track positions. */
	song_len = (UBYTE(module[0x1c7]) + (UBYTE(module[0x1cb]) << 8) - track0_addr) >> 1;
	if (song_len > 0xfe)
		return FALSE;
	INIT_ARRAY(global_seen);
	MODULE_INFO songs = 0;
	for (pos = 0; pos < song_len && MODULE_INFO songs < MAX_SONGS; pos++) {
		if (!global_seen[pos]) {
			MODULE_INFO song_pos[MODULE_INFO songs] = (byte) pos;
			parse_mpt_song(module_info, module, global_seen, song_len, pos);
		}
	}
	return MODULE_INFO songs != 0;
}

CONST_LOOKUP(byte, rmt_volume_silent) = { 16, 8, 4, 3, 2, 2, 2, 1, 1, 1, 1, 1, 1, 1, 1 };

FILE_FUNC int rmt_instrument_frames(const byte ARRAY module, int instrument, int volume, int volume_frame, abool extra_pokey)
{
	int addr_to_offset = UBYTE(module[2]) + (UBYTE(module[3]) << 8) - 6;
	int per_frame = module[0xc];
	int player_call;
	int player_calls;
	int index;
	int index_end;
	int index_loop;
	int volume_slide_depth;
	int volume_min;
	abool looping;
	int volume_slide;
	abool silent_loop;
	instrument = UBYTE(module[0xe]) + (UBYTE(module[0xf]) << 8) - addr_to_offset + (instrument << 1);
	if (module[instrument + 1] == 0)
		return 0;
	instrument = UBYTE(module[instrument]) + (UBYTE(module[instrument + 1]) << 8) - addr_to_offset;
	player_calls = player_call = volume_frame * per_frame;
	index = UBYTE(module[instrument]) + 1 + player_call * 3;
	index_end = UBYTE(module[instrument + 2]) + 3;
	index_loop = UBYTE(module[instrument + 3]);
	if (index_loop >= index_end)
		return 0; /* error */
	volume_slide_depth = UBYTE(module[instrument + 6]);
	volume_min = UBYTE(module[instrument + 7]);
	looping = index >= index_end;
	if (looping)
		index = (index - index_end) % (index_end - index_loop) + index_loop;
	else {
		do {
			int vol = module[instrument + index];
			if (extra_pokey)
				vol >>= 4;
			if ((vol & 0xf) >= rmt_volume_silent[volume])
				player_calls = player_call + 1;
			player_call++;
			index += 3;
		} while (index < index_end);
	}
	if (volume_slide_depth == 0)
		return player_calls / per_frame;
	volume_slide = 128;
	silent_loop = FALSE;
	for (;;) {
		int vol;
		if (index >= index_end) {
			if (silent_loop)
				break;
			silent_loop = TRUE;
			index = index_loop;
		}
		vol = module[instrument + index];
		if (extra_pokey)
			vol >>= 4;
		if ((vol & 0xf) >= rmt_volume_silent[volume]) {
			player_calls = player_call + 1;
			silent_loop = FALSE;
		}
		player_call++;
		index += 3;
		volume_slide -= volume_slide_depth;
		if (volume_slide < 0) {
			volume_slide += 256;
			if (--volume <= volume_min)
				break;
		}
	}
	return player_calls / per_frame;
}

FILE_FUNC void parse_rmt_song(ASAP_ModuleInfo PTR module_info, const byte ARRAY module,
                              abool ARRAY global_seen, int song_len, int pos_shift, int pos)
{
	int ch;
	int addr_to_offset = UBYTE(module[2]) + (UBYTE(module[3]) << 8) - 6;
	int tempo = UBYTE(module[0xb]);
	int frames = 0;
	int song_offset = UBYTE(module[0x14]) + (UBYTE(module[0x15]) << 8) - addr_to_offset;
	int pattern_lo_offset = UBYTE(module[0x10]) + (UBYTE(module[0x11]) << 8) - addr_to_offset;
	int pattern_hi_offset = UBYTE(module[0x12]) + (UBYTE(module[0x13]) << 8) - addr_to_offset;
	int instrument_frames;
	NEW_ARRAY(byte, seen, 256);
	NEW_ARRAY(int, pattern_begin, 8);
	NEW_ARRAY(int, pattern_offset, 8);
	NEW_ARRAY(int, blank_rows, 8);
	NEW_ARRAY(int, instrument_no, 8);
	NEW_ARRAY(int, instrument_frame, 8);
	NEW_ARRAY(int, volume_value, 8);
	NEW_ARRAY(int, volume_frame, 8);
	INIT_ARRAY(seen);
	INIT_ARRAY(instrument_no);
	INIT_ARRAY(instrument_frame);
	INIT_ARRAY(volume_value);
	INIT_ARRAY(volume_frame);
	while (pos < song_len) {
		int i;
		int pattern_rows;
		if (seen[pos] != 0) {
			if (seen[pos] != SEEN_THIS_CALL)
				MODULE_INFO loops[MODULE_INFO songs] = TRUE;
			break;
		}
		seen[pos] = SEEN_THIS_CALL;
		global_seen[pos] = TRUE;
		if (UBYTE(module[song_offset + (pos << pos_shift)]) == 0xfe) {
			pos = UBYTE(module[song_offset + (pos << pos_shift) + 1]);
			continue;
		}
		for (ch = 0; ch < 1 << pos_shift; ch++) {
			i = UBYTE(module[song_offset + (pos << pos_shift) + ch]);
			if (i == 0xff)
				blank_rows[ch] = 256;
			else {
				pattern_offset[ch] = pattern_begin[ch] = UBYTE(module[pattern_lo_offset + i])
					+ (UBYTE(module[pattern_hi_offset + i]) << 8) - addr_to_offset;
				blank_rows[ch] = 0;
			}
		}
		for (i = 0; i < song_len; i++)
			if (seen[i] == SEEN_THIS_CALL)
				seen[i] = SEEN_BEFORE;
		for (pattern_rows = UBYTE(module[0xa]); --pattern_rows >= 0; ) {
			for (ch = 0; ch < 1 << pos_shift; ch++) {
				if (--blank_rows[ch] > 0)
					continue;
				for (;;) {
					i = UBYTE(module[pattern_offset[ch]++]);
					if ((i & 0x3f) < 62) {
						i += UBYTE(module[pattern_offset[ch]++]) << 8;
						if ((i & 0x3f) != 61) {
							instrument_no[ch] = i >> 10;
							instrument_frame[ch] = frames;
						}
						volume_value[ch] = (i >> 6) & 0xf;
						volume_frame[ch] = frames;
						break;
					}
					if (i == 62) {
						blank_rows[ch] = UBYTE(module[pattern_offset[ch]++]);
						break;
					}
					if ((i & 0x3f) == 62) {
						blank_rows[ch] = i >> 6;
						break;
					}
					if ((i & 0xbf) == 63) {
						tempo = UBYTE(module[pattern_offset[ch]++]);
						continue;
					}
					if (i == 0xbf) {
						pattern_offset[ch] = pattern_begin[ch] + UBYTE(module[pattern_offset[ch]]);
						continue;
					}
					/* assert(i == 0xff); */
					pattern_rows = -1;
					break;
				}
				if (pattern_rows < 0)
					break;
			}
			if (pattern_rows >= 0)
				frames += tempo;
		}
		pos++;
	}
	instrument_frames = 0;
	for (ch = 0; ch < 1 << pos_shift; ch++) {
		int frame = instrument_frame[ch];
		frame += rmt_instrument_frames(module, instrument_no[ch], volume_value[ch], volume_frame[ch] - frame, ch >= 4);
		if (instrument_frames < frame)
			instrument_frames = frame;
	}
	if (frames > instrument_frames) {
		if (frames - instrument_frames > 100)
			MODULE_INFO loops[MODULE_INFO songs] = FALSE;
		frames = instrument_frames;
	}
	if (frames > 0)
		set_song_duration(module_info, frames);
}

FILE_FUNC abool parse_rmt(ASAP_State PTR ast, ASAP_ModuleInfo PTR module_info,
                          const byte ARRAY module, int module_len)
{
	int per_frame;
	int pos_shift;
	int song_len;
	int pos;
	NEW_ARRAY(abool, global_seen, 256);
	if (module_len < 0x30 || module[6] != 'R' || module[7] != 'M'
	 || module[8] != 'T' || module[0xd] != 1)
		return FALSE;
	switch ((char) module[9]) {
	case '4':
		pos_shift = 2;
		break;
	case '8':
		MODULE_INFO channels = 2;
		pos_shift = 3;
		break;
	default:
		return FALSE;
	}
	per_frame = module[0xc];
	if (per_frame < 1 || per_frame > 4)
		return FALSE;
	MODULE_INFO type = 'r';
	if (!load_native(ast, module_info, module, module_len,
		MODULE_INFO channels == 2 ? GET_OBX(rmt8) : GET_OBX(rmt4)))
		return FALSE;
	song_len = UBYTE(module[4]) + (UBYTE(module[5]) << 8) + 1
		- UBYTE(module[0x14]) - (UBYTE(module[0x15]) << 8);
	if (pos_shift == 3 && (song_len & 4) != 0
	 && UBYTE(module[6 + UBYTE(module[4]) + (UBYTE(module[5]) << 8)
		- UBYTE(module[2]) - (UBYTE(module[3]) << 8) - 3]) == 0xfe)
		song_len += 4;
	song_len >>= pos_shift;
	if (song_len >= 0x100)
		return FALSE;
	INIT_ARRAY(global_seen);
	MODULE_INFO songs = 0;
	for (pos = 0; pos < song_len && MODULE_INFO songs < MAX_SONGS; pos++) {
		if (!global_seen[pos]) {
			MODULE_INFO song_pos[MODULE_INFO songs] = (byte) pos;
			parse_rmt_song(module_info, module, global_seen, song_len, pos_shift, pos);
		}
	}
	/* must set fastplay after song durations calculations, so they assume 312 */
	MODULE_INFO fastplay = perframe2fastplay[per_frame - 1];
	MODULE_INFO player = 0x600;
	return MODULE_INFO songs != 0;
}

FILE_FUNC void parse_tmc_song(ASAP_ModuleInfo PTR module_info, const byte ARRAY module, int pos)
{
	int addr_to_offset = UBYTE(module[2]) + (UBYTE(module[3]) << 8) - 6;
	int tempo = UBYTE(module[0x24]) + 1;
	int frames = 0;
	NEW_ARRAY(int, pattern_offset, 8);
	NEW_ARRAY(int, blank_rows, 8);
	while (UBYTE(module[0x1a6 + 15 + pos]) < 0x80) {
		int ch;
		int pattern_rows;
		for (ch = 7; ch >= 0; ch--) {
			int pat = UBYTE(module[0x1a6 + 15 + pos - 2 * ch]);
			pattern_offset[ch] = UBYTE(module[0xa6 + pat]) + (UBYTE(module[0x126 + pat]) << 8) - addr_to_offset;
			blank_rows[ch] = 0;
		}
		for (pattern_rows = 64; --pattern_rows >= 0; ) {
			for (ch = 7; ch >= 0; ch--) {
				if (--blank_rows[ch] >= 0)
					continue;
				for (;;) {
					int i = UBYTE(module[pattern_offset[ch]++]);
					if (i < 0x40) {
						pattern_offset[ch]++;
						break;
					}
					if (i == 0x40) {
						i = UBYTE(module[pattern_offset[ch]++]);
						if ((i & 0x7f) == 0)
							pattern_rows = 0;
						else
							tempo = (i & 0x7f) + 1;
						if (i >= 0x80)
							pattern_offset[ch]++;
						break;
					}
					if (i < 0x80) {
						i = module[pattern_offset[ch]++] & 0x7f;
						if (i == 0)
							pattern_rows = 0;
						else
							tempo = i + 1;
						pattern_offset[ch]++;
						break;
					}
					if (i < 0xc0)
						continue;
					blank_rows[ch] = i - 0xbf;
					break;
				}
			}
			frames += tempo;
		}
		pos += 16;
	}
	if (UBYTE(module[0x1a6 + 14 + pos]) < 0x80)
		MODULE_INFO loops[MODULE_INFO songs] = TRUE;
	set_song_duration(module_info, frames);
}

FILE_FUNC abool parse_tmc(ASAP_State PTR ast, ASAP_ModuleInfo PTR module_info,
                          const byte ARRAY module, int module_len)
{
	int i;
	int last_pos;
	if (module_len < 0x1d0)
		return FALSE;
	MODULE_INFO type = 't';
	if (!load_native(ast, module_info, module, module_len, GET_OBX(tmc)))
		return FALSE;
	MODULE_INFO channels = 2;
	i = 0;
	/* find first instrument */
	while (module[0x66 + i] == 0) {
		if (++i >= 64)
			return FALSE; /* no instrument */
	}
	last_pos = (UBYTE(module[0x66 + i]) << 8) + UBYTE(module[0x26 + i])
		- UBYTE(module[2]) - (UBYTE(module[3]) << 8) - 0x1b0;
	if (0x1b5 + last_pos >= module_len)
		return FALSE;
	/* skip trailing jumps */
	do {
		if (last_pos <= 0)
			return FALSE; /* no pattern to play */
		last_pos -= 16;
	} while (UBYTE(module[0x1b5 + last_pos]) >= 0x80);
	MODULE_INFO songs = 0;
	parse_tmc_song(module_info, module, 0);
	for (i = 0; i < last_pos && MODULE_INFO songs < MAX_SONGS; i += 16)
		if (UBYTE(module[0x1b5 + i]) >= 0x80)
			parse_tmc_song(module_info, module, i + 16);
	/* must set fastplay after song durations calculations, so they assume 312 */
	i = module[0x25];
	if (i < 1 || i > 4)
		return FALSE;
	if (ast != NULL)
		AST tmc_per_frame = module[0x25];
	MODULE_INFO fastplay = perframe2fastplay[i - 1];
	return TRUE;
}

FILE_FUNC void parse_tm2_song(ASAP_ModuleInfo PTR module_info, const byte ARRAY module, int pos)
{
	int addr_to_offset = UBYTE(module[2]) + (UBYTE(module[3]) << 8) - 6;
	int tempo = UBYTE(module[0x24]) + 1;
	int player_calls = 0;
	NEW_ARRAY(int, pattern_offset, 8);
	NEW_ARRAY(int, blank_rows, 8);
	for (;;) {
		int ch;
		int pattern_rows = UBYTE(module[0x386 + 16 + pos]);
		if (pattern_rows == 0)
			break;
		if (pattern_rows >= 0x80) {
			MODULE_INFO loops[MODULE_INFO songs] = TRUE;
			break;
		}
		for (ch = 7; ch >= 0; ch--) {
			int pat = UBYTE(module[0x386 + 15 + pos - 2 * ch]);
			pattern_offset[ch] = UBYTE(module[0x106 + pat]) + (UBYTE(module[0x206 + pat]) << 8) - addr_to_offset;
			blank_rows[ch] = 0;
		}
		while (--pattern_rows >= 0) {
			for (ch = 7; ch >= 0; ch--) {
				if (--blank_rows[ch] >= 0)
					continue;
				for (;;) {
					int i = UBYTE(module[pattern_offset[ch]++]);
					if (i == 0) {
						pattern_offset[ch]++;
						break;
					}
					if (i < 0x40) {
						if (UBYTE(module[pattern_offset[ch]++]) >= 0x80)
							pattern_offset[ch]++;
						break;
					}
					if (i < 0x80) {
						pattern_offset[ch]++;
						break;
					}
					if (i == 0x80) {
						blank_rows[ch] = UBYTE(module[pattern_offset[ch]++]);
						break;
					}
					if (i < 0xc0)
						break;
					if (i < 0xd0) {
						tempo = i - 0xbf;
						continue;
					}
					if (i < 0xe0) {
						pattern_offset[ch]++;
						break;
					}
					if (i < 0xf0) {
						pattern_offset[ch] += 2;
						break;
					}
					if (i < 0xff) {
						blank_rows[ch] = i - 0xf0;
						break;
					}
					blank_rows[ch] = 64;
					break;
				}
			}
			player_calls += tempo;
		}
		pos += 17;
	}
	set_song_duration(module_info, player_calls);
}

FILE_FUNC abool parse_tm2(ASAP_State PTR ast, ASAP_ModuleInfo PTR module_info,
                          const byte ARRAY module, int module_len)
{
	int i;
	int last_pos;
	int c;
	if (module_len < 0x3a4)
		return FALSE;
	MODULE_INFO type = 'T';
	if (!load_native(ast, module_info, module, module_len, GET_OBX(tm2)))
		return FALSE;
	i = module[0x25];
	if (i < 1 || i > 4)
		return FALSE;
	MODULE_INFO fastplay = perframe2fastplay[i - 1];
	MODULE_INFO player = 0x500;
	if (module[0x1f] != 0)
		MODULE_INFO channels = 2;
	last_pos = 0xffff;
	for (i = 0; i < 0x80; i++) {
		int instr_addr = UBYTE(module[0x86 + i]) + (UBYTE(module[0x306 + i]) << 8);
		if (instr_addr != 0 && instr_addr < last_pos)
			last_pos = instr_addr;
	}
	for (i = 0; i < 0x100; i++) {
		int pattern_addr = UBYTE(module[0x106 + i]) + (UBYTE(module[0x206 + i]) << 8);
		if (pattern_addr != 0 && pattern_addr < last_pos)
			last_pos = pattern_addr;
	}
	last_pos -= UBYTE(module[2]) + (UBYTE(module[3]) << 8) + 0x380;
	if (0x386 + last_pos >= module_len)
		return FALSE;
	/* skip trailing stop/jump commands */
	do {
		if (last_pos <= 0)
			return FALSE;
		last_pos -= 17;
		c = UBYTE(module[0x386 + 16 + last_pos]);
	} while (c == 0 || c >= 0x80);
	MODULE_INFO songs = 0;
	parse_tm2_song(module_info, module, 0);
	for (i = 0; i < last_pos && MODULE_INFO songs < MAX_SONGS; i += 17) {
		c = UBYTE(module[0x386 + 16 + i]);
		if (c == 0 || c >= 0x80)
			parse_tm2_song(module_info, module, i + 17);
	}
	return TRUE;
}

#if !defined(JAVA) && !defined(CSHARP)

static abool parse_hex(int *retval, const char *p)
{
	int r = 0;
	do {
		char c = *p;
		if (r > 0xfff)
			return FALSE;
		r <<= 4;
		if (c >= '0' && c <= '9')
			r += c - '0';
		else if (c >= 'A' && c <= 'F')
			r += c - 'A' + 10;
		else if (c >= 'a' && c <= 'f')
			r += c - 'a' + 10;
		else
			return FALSE;
	} while (*++p != '\0');
	*retval = r;
	return TRUE;
}

static abool parse_dec(int *retval, const char *p, int minval, int maxval)
{
	int r = 0;
	do {
		char c = *p;
		if (c >= '0' && c <= '9')
			r = 10 * r + c - '0';
		else
			return FALSE;
		if (r > maxval)
			return FALSE;
	} while (*++p != '\0');
	if (r < minval)
		return FALSE;
	*retval = r;
	return TRUE;
}

static abool parse_text(char *retval, const char *p)
{
	int i;
	if (*p != '"')
		return FALSE;
	p++;
	if (p[0] == '<' && p[1] == '?' && p[2] == '>' && p[3] == '"')
		return TRUE;
	i = 0;
	while (*p != '"') {
		if (i >= 127)
			return FALSE;
		if (*p == '\0')
			return FALSE;
		retval[i++] = *p++;
	}
	retval[i] = '\0';
	return TRUE;
}

int ASAP_ParseDuration(const char *s)
{
	int r;
	if (*s < '0' || *s > '9')
		return -1;
	r = *s++ - '0';
	if (*s >= '0' && *s <= '9')
		r = 10 * r + *s++ - '0';
	if (*s == ':') {
		s++;
		if (*s < '0' || *s > '5')
			return -1;
		r = 60 * r + (*s++ - '0') * 10;
		if (*s < '0' || *s > '9')
			return -1;
		r += *s++ - '0';
	}
	r *= 1000;
	if (*s != '.')
		return r;
	s++;
	if (*s < '0' || *s > '9')
		return r;
	r += 100 * (*s++ - '0');
	if (*s < '0' || *s > '9')
		return r;
	r += 10 * (*s++ - '0');
	if (*s < '0' || *s > '9')
		return r;
	r += *s - '0';
	return r;
}

static char *two_digits(char *s, int x)
{
	s[0] = '0' + x / 10;
	s[1] = '0' + x % 10;
	return s + 2;
}

void ASAP_DurationToString(char *s, int duration)
{
	if (duration >= 0) {
		int seconds = duration / 1000;
		int minutes = seconds / 60;
		s = two_digits(s, minutes);
		*s++ = ':';
		s = two_digits(s, seconds % 60);
		duration %= 1000;
		if (duration != 0) {
			*s++ = '.';
			s = two_digits(s, duration / 10);
			duration %= 10;
			if (duration != 0)
				*s++ = '0' + duration;
		}
	}
	*s = '\0';
}

#endif /* !defined(JAVA) && !defined(CSHARP) */

FILE_FUNC abool parse_sap_header(ASAP_ModuleInfo PTR module_info,
                                 const byte ARRAY module, int module_len)
{
	int module_index = 0;
	abool sap_signature = FALSE;
	int duration_index = 0;
	for (;;) {
		NEW_ARRAY(char, line, 256);
		int i;
#if !defined(JAVA) && !defined(CSHARP)
		char *p;
#endif
		if (module_index + 8 >= module_len)
			return FALSE;
		if (UBYTE(module[module_index]) == 0xff)
			break;
		i = 0;
		while (module[module_index] != 0x0d) {
			line[i++] = (char) module[module_index++];
			if (module_index >= module_len || (unsigned)i >= sizeof(line) - 1)
				return FALSE;
		}
		if (++module_index >= module_len || module[module_index++] != 0x0a)
			return FALSE;

#ifdef JAVA
		String tag = new String(line, 0, i);
		String arg = null;
		i = tag.indexOf(' ');
		if (i >= 0) {
			arg = tag.substring(i + 1);
			tag = tag.substring(0, i);
		}
#define TAG_IS(t)               tag.equals(t)
#define CHAR_ARG                arg.charAt(0)
#define SET_HEX(v)              v = Integer.parseInt(arg, 16)
#define SET_DEC(v, min, max)    do { v = Integer.parseInt(arg); if (v < min || v > max) return FALSE; } while (FALSE)
#define SET_TEXT(v)             v = arg.substring(1, arg.length() - 1)
#define DURATION_ARG            parseDuration(arg)
#define ARG_CONTAINS(t)         (arg.indexOf(t) >= 0)
#elif defined(CSHARP)
		string tag = new string(line, 0, i);
		string arg = null;
		i = tag.IndexOf(' ');
		if (i >= 0) {
			arg = tag.Substring(i + 1);
			tag = tag.Substring(0, i);
		}
#define TAG_IS(t)               tag == t
#define CHAR_ARG                arg[0]
#define SET_HEX(v)              v = int.Parse(arg, System.Globalization.NumberStyles.HexNumber)
#define SET_DEC(v, min, max)    do { v = int.Parse(arg); if (v < min || v > max) return FALSE; } while (FALSE)
#define SET_TEXT(v)             v = arg.Substring(1, arg.Length - 1)
#define DURATION_ARG            ParseDuration(arg)
#define ARG_CONTAINS(t)         (arg.IndexOf(t) >= 0)
#else
		line[i] = '\0';
		for (p = line; *p != '\0'; p++) {
			if (*p == ' ') {
				*p++ = '\0';
				break;
			}
		}
#define TAG_IS(t)               (strcmp(line, t) == 0)
#define CHAR_ARG                *p
#define SET_HEX(v)              do { if (!parse_hex(&v, p)) return FALSE; } while (FALSE)
#define SET_DEC(v, min, max)    do { if (!parse_dec(&v, p, min, max)) return FALSE; } while (FALSE)
#define SET_TEXT(v)             do { if (!parse_text(v, p)) return FALSE; } while (FALSE)
#define DURATION_ARG            ASAP_ParseDuration(p)
#define ARG_CONTAINS(t)         (strstr(p, t) != NULL)
#endif

		if (TAG_IS("SAP"))
			sap_signature = TRUE;
		if (!sap_signature)
			return FALSE;
		if (TAG_IS("AUTHOR"))
			SET_TEXT(MODULE_INFO author);
		else if (TAG_IS("NAME"))
			SET_TEXT(MODULE_INFO name);
		else if (TAG_IS("DATE"))
			SET_TEXT(MODULE_INFO date);
		else if (TAG_IS("SONGS"))
			SET_DEC(MODULE_INFO songs, 1, MAX_SONGS);
		else if (TAG_IS("DEFSONG"))
			SET_DEC(MODULE_INFO default_song, 0, MAX_SONGS - 1);
		else if (TAG_IS("STEREO"))
			MODULE_INFO channels = 2;
		else if (TAG_IS("TIME")) {
			int duration = DURATION_ARG;
			if (duration < 0 || duration_index >= MAX_SONGS)
				return FALSE;
			MODULE_INFO durations[duration_index] = duration;
			if (ARG_CONTAINS("LOOP"))
				MODULE_INFO loops[duration_index] = TRUE;
			duration_index++;
		}
		else if (TAG_IS("TYPE"))
			MODULE_INFO type = CHAR_ARG;
		else if (TAG_IS("FASTPLAY"))
			SET_DEC(MODULE_INFO fastplay, 1, 312);
		else if (TAG_IS("MUSIC"))
			SET_HEX(MODULE_INFO music);
		else if (TAG_IS("INIT"))
			SET_HEX(MODULE_INFO init);
		else if (TAG_IS("PLAYER"))
			SET_HEX(MODULE_INFO player);
	}
	if (MODULE_INFO default_song >= MODULE_INFO songs)
		return FALSE;
	switch (MODULE_INFO type) {
	case 'B':
	case 'D':
		if (MODULE_INFO player < 0 || MODULE_INFO init < 0)
			return FALSE;
		break;
	case 'C':
		if (MODULE_INFO player < 0 || MODULE_INFO music < 0)
			return FALSE;
		break;
	case 'S':
		if (MODULE_INFO init < 0)
			return FALSE;
		MODULE_INFO fastplay = 78;
		break;
	default:
		return FALSE;
	}
	if (UBYTE(module[module_index + 1]) != 0xff)
		return FALSE;
	MODULE_INFO header_len = module_index;
	return TRUE;
}

FILE_FUNC abool parse_sap(ASAP_State PTR ast, ASAP_ModuleInfo PTR module_info,
                          const byte ARRAY module, int module_len)
{
	int module_index;
	if (!parse_sap_header(module_info, module, module_len))
		return FALSE;
	if (ast == NULL)
		return TRUE;
	ZERO_ARRAY(AST memory);
	module_index = MODULE_INFO header_len + 2;
	while (module_index + 5 <= module_len) {
		int start_addr = UBYTE(module[module_index]) + (UBYTE(module[module_index + 1]) << 8);
		int block_len = UBYTE(module[module_index + 2]) + (UBYTE(module[module_index + 3]) << 8) + 1 - start_addr;
		if (block_len <= 0 || module_index + block_len > module_len)
			return FALSE;
		module_index += 4;
		COPY_ARRAY(AST memory, start_addr, module, module_index, block_len);
		module_index += block_len;
		if (module_index == module_len)
			return TRUE;
		if (module_index + 7 <= module_len
		 && UBYTE(module[module_index]) == 0xff && UBYTE(module[module_index + 1]) == 0xff)
			module_index += 2;
	}
	return FALSE;
}

#define ASAP_EXT(c1, c2, c3) (((c1) + ((c2) << 8) + ((c3) << 16)) | 0x202020)

FILE_FUNC int get_packed_ext(STRING filename)
{
#ifdef JAVA
	int i = filename.length();
	int ext = 0;
	while (--i > 0) {
		if (filename.charAt(i) == '.')
			return ext | 0x202020;
		ext = (ext << 8) + filename.charAt(i);
	}
	return 0;
#elif defined(CSHARP)
	int i = filename.Length;
	int ext = 0;
	while (--i > 0) {
		if (filename[i] == '.')
			return ext | 0x202020;
		ext = (ext << 8) + filename[i];
	}
	return 0;
#else
	const char *p;
	int ext;
	for (p = filename; *p != '\0'; p++);
	ext = 0;
	for (;;) {
		if (--p <= filename || *p <= ' ')
			return 0; /* no filename extension or invalid character */
		if (*p == '.')
			return ext | 0x202020;
		ext = (ext << 8) + (*p & 0xff);
	}
#endif
}

FILE_FUNC abool is_our_ext(int ext)
{
	switch (ext) {
	case ASAP_EXT('C', 'M', 'C'):
	case ASAP_EXT('C', 'M', 'R'):
	case ASAP_EXT('D', 'M', 'C'):
	case ASAP_EXT('M', 'P', 'D'):
	case ASAP_EXT('M', 'P', 'T'):
	case ASAP_EXT('R', 'M', 'T'):
	case ASAP_EXT('S', 'A', 'P'):
	case ASAP_EXT('T', 'M', '2'):
	case ASAP_EXT('T', 'M', '8'):
	case ASAP_EXT('T', 'M', 'C'):
		return TRUE;
	default:
		return FALSE;
	}
}

ASAP_FUNC abool ASAP_IsOurFile(STRING filename)
{
	int ext = get_packed_ext(filename);
	return is_our_ext(ext);
}

ASAP_FUNC abool ASAP_IsOurExt(STRING ext)
{
#ifdef JAVA
	return ext.length() == 3
		&& is_our_ext(ASAP_EXT(ext.charAt(0), ext.charAt(1), ext.charAt(2)));
#else
	return ext[0] > ' ' && ext[1] > ' ' && ext[2] > ' ' && ext[3] == '\0'
		&& is_our_ext(ASAP_EXT(ext[0], ext[1], ext[2]));
#endif
}

FILE_FUNC abool parse_file(ASAP_State PTR ast, ASAP_ModuleInfo PTR module_info,
                           STRING filename, const byte ARRAY module, int module_len)
{
	int i;
#ifdef JAVA
	int basename = 0;
	int ext = -1;
	for (i = 0; i < filename.length(); i++) {
		int c = filename.charAt(i);
		if (c == '/' || c == '\\')
			basename = i + 1;
		else if (c == '.')
			ext = i;
	}
	if (ext < 0)
		ext = i;
	module_info.author = "";
	module_info.name = filename.substring(basename, ext);
	module_info.date = "";
#elif defined(CSHARP)
	int basename = 0;
	int ext = -1;
	for (i = 0; i < filename.Length; i++) {
		int c = filename[i];
		if (c == '/' || c == '\\')
			basename = i + 1;
		else if (c == '.')
			ext = i;
	}
	if (ext < 0)
		ext = i;
	module_info.author = string.Empty;
	module_info.name = filename.Substring(basename, ext - basename);
	module_info.date = string.Empty;
#else
	const char *p;
	const char *basename = filename;
	const char *ext = NULL;
	for (p = filename; *p != '\0'; p++) {
		if (*p == '/' || *p == '\\')
			basename = p + 1;
		else if (*p == '.')
			ext = p;
	}
	if (ext == NULL)
		ext = p;
	module_info->author[0] = '\0';
	i = ext - basename;
	memcpy(module_info->name, basename, i);
	module_info->name[i] = '\0';
	module_info->date[0] = '\0';
#endif
	MODULE_INFO channels = 1;
	MODULE_INFO songs = 1;
	MODULE_INFO default_song = 0;
	for (i = 0; i < MAX_SONGS; i++) {
		MODULE_INFO durations[i] = -1;
		MODULE_INFO loops[i] = FALSE;
	}
	MODULE_INFO type = '?';
	MODULE_INFO fastplay = 312;
	MODULE_INFO music = -1;
	MODULE_INFO init = -1;
	MODULE_INFO player = -1;
	switch (get_packed_ext(filename)) {
	case ASAP_EXT('C', 'M', 'C'):
		return parse_cmc(ast, module_info, module, module_len, FALSE);
	case ASAP_EXT('C', 'M', 'R'):
		return parse_cmc(ast, module_info, module, module_len, TRUE);
	case ASAP_EXT('D', 'M', 'C'):
		MODULE_INFO fastplay = 156;
		return parse_cmc(ast, module_info, module, module_len, FALSE);
	case ASAP_EXT('M', 'P', 'D'):
		MODULE_INFO fastplay = 156;
		return parse_mpt(ast, module_info, module, module_len);
	case ASAP_EXT('M', 'P', 'T'):
		return parse_mpt(ast, module_info, module, module_len);
	case ASAP_EXT('R', 'M', 'T'):
		return parse_rmt(ast, module_info, module, module_len);
	case ASAP_EXT('S', 'A', 'P'):
		return parse_sap(ast, module_info, module, module_len);
	case ASAP_EXT('T', 'M', '2'):
		return parse_tm2(ast, module_info, module, module_len);
	case ASAP_EXT('T', 'M', '8'):
	case ASAP_EXT('T', 'M', 'C'):
		return parse_tmc(ast, module_info, module, module_len);
	default:
		return FALSE;
	}
}

ASAP_FUNC abool ASAP_GetModuleInfo(ASAP_ModuleInfo PTR module_info, STRING filename,
                                   const byte ARRAY module, int module_len)
{
	return parse_file(NULL, module_info, filename, module, module_len);
}

ASAP_FUNC abool ASAP_Load(ASAP_State PTR ast, STRING filename,
                          const byte ARRAY module, int module_len)
{
	AST silence_cycles = 0;
	return parse_file(ast, ADDRESSOF AST module_info, filename, module, module_len);
}

ASAP_FUNC void ASAP_DetectSilence(ASAP_State PTR ast, int seconds)
{
	AST silence_cycles = seconds * ASAP_MAIN_CLOCK;
}

FILE_FUNC void call_6502(ASAP_State PTR ast, int addr, int max_scanlines)
{
	AST cpu_pc = addr;
	/* put a CIM at 0xd20a and a return address on stack */
	dPutByte(0xd20a, 0xd2);
	dPutByte(0x01fe, 0x09);
	dPutByte(0x01ff, 0xd2);
	AST cpu_s = 0xfd;
	Cpu_RunScanlines(ast, max_scanlines);
}

/* 50 Atari frames for the initialization routine - some SAPs are self-extracting. */
#define SCANLINES_FOR_INIT  (50 * 312)

FILE_FUNC void call_6502_init(ASAP_State PTR ast, int addr, int a, int x, int y)
{
	AST cpu_a = a & 0xff;
	AST cpu_x = x & 0xff;
	AST cpu_y = y & 0xff;
	call_6502(ast, addr, SCANLINES_FOR_INIT);
}

ASAP_FUNC void ASAP_PlaySong(ASAP_State PTR ast, int song, int duration)
{
	AST current_song = song;
	AST current_duration = duration;
	AST blocks_played = 0;
	AST silence_cycles_counter = AST silence_cycles;
	AST extra_pokey_mask = AST module_info.channels > 1 ? 0x10 : 0;
	PokeySound_Initialize(ast);
	AST cycle = 0;
	AST cpu_nz = 0;
	AST cpu_c = 0;
	AST cpu_vdi = 0;
	AST scanline_number = 0;
	AST next_scanline_cycle = 0;
	AST timer1_cycle = NEVER;
	AST timer2_cycle = NEVER;
	AST timer4_cycle = NEVER;
	AST irqst = 0xff;
	switch (AST module_info.type) {
	case 'B':
		call_6502_init(ast, AST module_info.init, song, 0, 0);
		break;
	case 'C':
	case 'c':
	case 'z':
		call_6502_init(ast, AST module_info.player + 3, 0x70, AST module_info.music, AST module_info.music >> 8);
		call_6502_init(ast, AST module_info.player + 3, 0x00, song, 0);
		break;
	case 'D':
	case 'S':
		AST cpu_a = song;
		AST cpu_x = 0x00;
		AST cpu_y = 0x00;
		AST cpu_s = 0xff;
		AST cpu_pc = AST module_info.init;
		break;
	case 'm':
		call_6502_init(ast, AST module_info.player, 0x00, AST module_info.music >> 8, AST module_info.music);
		call_6502_init(ast, AST module_info.player, 0x02, AST module_info.song_pos[song], 0);
		break;
	case 'r':
		call_6502_init(ast, AST module_info.player, AST module_info.song_pos[song], AST module_info.music, AST module_info.music >> 8);
		break;
	case 't':
	case 'T':
		call_6502_init(ast, AST module_info.player, 0x70, AST module_info.music >> 8, AST module_info.music);
		call_6502_init(ast, AST module_info.player, 0x00, song, 0);
		AST tmc_per_frame_counter = 1;
		break;
	}
	ASAP_MutePokeyChannels(ast, 0);
}

ASAP_FUNC void ASAP_MutePokeyChannels(ASAP_State PTR ast, int mask)
{
	PokeySound_Mute(ast, ADDRESSOF AST base_pokey, mask);
	PokeySound_Mute(ast, ADDRESSOF AST extra_pokey, mask >> 4);
}

ASAP_FUNC abool call_6502_player(ASAP_State PTR ast)
{
	int s;
	PokeySound_StartFrame(ast);
	switch (AST module_info.type) {
	case 'B':
		call_6502(ast, AST module_info.player, AST module_info.fastplay);
		break;
	case 'C':
	case 'c':
	case 'z':
		call_6502(ast, AST module_info.player + 6, AST module_info.fastplay);
		break;
	case 'D':
		s = AST cpu_s;
#define PUSH_ON_6502_STACK(x)  dPutByte(0x100 + s, x); s = (s - 1) & 0xff
#define RETURN_FROM_PLAYER_ADDR  0xd200
		/* save 6502 state on 6502 stack */
		PUSH_ON_6502_STACK(AST cpu_pc >> 8);
		PUSH_ON_6502_STACK(AST cpu_pc & 0xff);
		PUSH_ON_6502_STACK(((AST cpu_nz | (AST cpu_nz >> 1)) & 0x80) + AST cpu_vdi + \
			((AST cpu_nz & 0xff) == 0 ? Z_FLAG : 0) + AST cpu_c + 0x20);
		PUSH_ON_6502_STACK(AST cpu_a);
		PUSH_ON_6502_STACK(AST cpu_x);
		PUSH_ON_6502_STACK(AST cpu_y);
		/* RTS will jump to 6502 code that restores the state */
		PUSH_ON_6502_STACK((RETURN_FROM_PLAYER_ADDR - 1) >> 8);
		PUSH_ON_6502_STACK((RETURN_FROM_PLAYER_ADDR - 1) & 0xff);
		AST cpu_s = s;
		dPutByte(RETURN_FROM_PLAYER_ADDR, 0x68);     /* PLA */
		dPutByte(RETURN_FROM_PLAYER_ADDR + 1, 0xa8); /* TAY */
		dPutByte(RETURN_FROM_PLAYER_ADDR + 2, 0x68); /* PLA */
		dPutByte(RETURN_FROM_PLAYER_ADDR + 3, 0xaa); /* TAX */
		dPutByte(RETURN_FROM_PLAYER_ADDR + 4, 0x68); /* PLA */
		dPutByte(RETURN_FROM_PLAYER_ADDR + 5, 0x40); /* RTI */
		AST cpu_pc = AST module_info.player;
		Cpu_RunScanlines(ast, AST module_info.fastplay);
		break;
	case 'S':
		Cpu_RunScanlines(ast, AST module_info.fastplay);
		{
			int i = dGetByte(0x45) - 1;
			dPutByte(0x45, i);
			if (i == 0)
				dPutByte(0xb07b, dGetByte(0xb07b) + 1);
		}
		break;
	case 'm':
	case 'r':
	case 'T':
		call_6502(ast, AST module_info.player + 3, AST module_info.fastplay);
		break;
	case 't':
		if (--AST tmc_per_frame_counter <= 0) {
			AST tmc_per_frame_counter = AST tmc_per_frame;
			call_6502(ast, AST module_info.player + 3, AST module_info.fastplay);
		}
		else
			call_6502(ast, AST module_info.player + 6, AST module_info.fastplay);
		break;
	}
	PokeySound_EndFrame(ast, AST module_info.fastplay * 114);
	if (AST silence_cycles > 0) {
		if (PokeySound_IsSilent(ADDRESSOF AST base_pokey)
		 && PokeySound_IsSilent(ADDRESSOF AST extra_pokey)) {
			AST silence_cycles_counter -= AST module_info.fastplay * 114;
			if (AST silence_cycles_counter <= 0)
				return FALSE;
		}
		else
			AST silence_cycles_counter = AST silence_cycles;
	}
	return TRUE;
}

FILE_FUNC int milliseconds_to_blocks(int milliseconds)
{
	return milliseconds * (ASAP_SAMPLE_RATE / 100) / 10;
}

ASAP_FUNC void ASAP_Seek(ASAP_State PTR ast, int position)
{
	int block = milliseconds_to_blocks(position);
	if (block < AST blocks_played)
		ASAP_PlaySong(ast, AST current_song, AST current_duration);
	while (AST blocks_played + AST samples - AST sample_index < block) {
		AST blocks_played += AST samples - AST sample_index;
		call_6502_player(ast);
	}
	AST sample_index += block - AST blocks_played;
	AST blocks_played = block;
}

ASAP_FUNC int ASAP_Generate(ASAP_State PTR ast, VOIDPTR buffer, int buffer_len,
                            ASAP_SampleFormat format)
{
	int block_shift;
	int buffer_blocks;
	int block;
	if (AST silence_cycles > 0 && AST silence_cycles_counter <= 0)
		return 0;
	block_shift = (AST module_info.channels - 1) + (format != ASAP_FORMAT_U8 ? 1 : 0);
	buffer_blocks = buffer_len >> block_shift;
	if (AST current_duration > 0) {
		int total_blocks = milliseconds_to_blocks(AST current_duration);
		if (buffer_blocks > total_blocks - AST blocks_played)
			buffer_blocks = total_blocks - AST blocks_played;
	}
	block = 0;
	do {
		int blocks = PokeySound_Generate(ast, buffer, block << block_shift, buffer_blocks - block, format);
		AST blocks_played += blocks;
		block += blocks;
	} while (block < buffer_blocks && call_6502_player(ast));
	return block << block_shift;
}

#if !defined(JAVA) && !defined(CSHARP)

abool ASAP_ChangeExt(char *filename, const char *ext)
{
	char *dest = NULL;
	while (*filename != '\0') {
		if (*filename == '/' || *filename == '\\')
			dest = NULL;
		else if (*filename == '.')
			dest = filename + 1;
		filename++;
	}
	if (dest == NULL)
		return FALSE;
	strcpy(dest, ext);
	return TRUE;
}

abool ASAP_CanSetModuleInfo(const char *filename)
{
	int ext = get_packed_ext(filename);
	return ext == ASAP_EXT('S', 'A', 'P');
}

static byte *put_string(byte *dest, const char *str)
{
	while (*str != '\0')
		*dest++ = *str++;
	return dest;
}

static byte *put_dec(byte *dest, int value)
{
	if (value >= 10) {
		dest = put_dec(dest, value / 10);
		value %= 10;
	}
	*dest++ = '0' + value;
	return dest;
}

static byte *put_text_tag(byte *dest, const char *tag, const char *value)
{
	dest = put_string(dest, tag);
	*dest++ = ' ';
	*dest++ = '"';
	if (*value == '\0')
		value = "<?>";
	while (*value != '\0') {
		if (*value < ' ' || *value > 'z' || *value == '"' || *value == '`')
			return NULL;
		*dest++ = *value++;
	}
	*dest++ = '"';
	*dest++ = '\r';
	*dest++ = '\n';
	return dest;
}

static byte *put_hex_tag(byte *dest, const char *tag, int value)
{
	int i;
	if (value < 0)
		return dest;
	dest = put_string(dest, tag);
	*dest++ = ' ';
	for (i = 12; i >= 0; i -= 4) {
		int digit = (value >> i) & 0xf;
		*dest++ = (byte) (digit + (digit < 10 ? '0' : 'A' - 10));
	}
	*dest++ = '\r';
	*dest++ = '\n';
	return dest;
}

static byte *put_dec_tag(byte *dest, const char *tag, int value)
{
	dest = put_string(dest, tag);
	*dest++ = ' ';
	dest = put_dec(dest, value);
	*dest++ = '\r';
	*dest++ = '\n';
	return dest;
}

static byte *start_sap_header(byte *dest, const ASAP_ModuleInfo *module_info)
{
	dest = put_string(dest, "SAP\r\n");
	dest = put_text_tag(dest, "AUTHOR", module_info->author);
	if (dest == NULL)
		return NULL;
	dest = put_text_tag(dest, "NAME", module_info->name);
	if (dest == NULL)
		return NULL;
	dest = put_text_tag(dest, "DATE", module_info->date);
	if (dest == NULL)
		return NULL;
	if (module_info->songs > 1) {
		dest = put_dec_tag(dest, "SONGS", module_info->songs);
		if (module_info->default_song > 0)
			dest = put_dec_tag(dest, "DEFSONG", module_info->default_song);
	}
	if (module_info->channels > 1)
		dest = put_string(dest, "STEREO\r\n");
	return dest;
}

static byte *put_durations(byte *dest, const ASAP_ModuleInfo *module_info)
{
	int song;
	for (song = 0; song < module_info->songs; song++) {
		if (module_info->durations[song] < 0)
			break;
		dest = put_string(dest, "TIME ");
		ASAP_DurationToString((char *) dest, module_info->durations[song]);
		while (*dest != '\0')
			dest++;
		if (module_info->loops[song])
			dest = put_string(dest, " LOOP");
		*dest++ = '\r';
		*dest++ = '\n';
	}
	return dest;
}

static byte *put_sap_header(byte *dest, const ASAP_ModuleInfo *module_info, char type, int music, int init, int player)
{
	dest = start_sap_header(dest, module_info);
	if (dest == NULL)
		return NULL;
	dest = put_string(dest, "TYPE ");
	*dest++ = type;
	*dest++ = '\r';
	*dest++ = '\n';
	if (module_info->fastplay != 312)
		dest = put_dec_tag(dest, "FASTPLAY", module_info->fastplay);
	dest = put_hex_tag(dest, "MUSIC", music);
	dest = put_hex_tag(dest, "INIT", init);
	dest = put_hex_tag(dest, "PLAYER", player);
	dest = put_durations(dest, module_info);
	return dest;
}

int ASAP_SetModuleInfo(const ASAP_ModuleInfo *module_info, const byte ARRAY module,
                       int module_len, byte ARRAY out_module)
{
	byte *dest;
	int i;
	if (memcmp(module, "SAP\r\n", 5) != 0)
		return -1;
	dest = start_sap_header(out_module, module_info);
	if (dest == NULL)
		return -1;
	i = 5;
	while (i < module_len && module[i] != 0xff) {
		if (memcmp(module + i, "AUTHOR ", 7) == 0
		 || memcmp(module + i, "NAME ", 5) == 0
		 || memcmp(module + i, "DATE ", 5) == 0
		 || memcmp(module + i, "SONGS ", 6) == 0
		 || memcmp(module + i, "DEFSONG ", 8) == 0
		 || memcmp(module + i, "STEREO", 6) == 0
		 || memcmp(module + i, "TIME ", 5) == 0) {
			while (i < module_len && module[i++] != 0x0a);
		}
		else {
			int b;
			do {
				b = module[i++];
				*dest++ = b;
			} while (i < module_len && b != 0x0a);
		}
	}
	dest = put_durations(dest, module_info);
	module_len -= i;
	memcpy(dest, module + i, module_len);
	dest += module_len;
	return dest - out_module;
}

#define RMT_INIT  0x0c80
#define TM2_INIT  0x1080

const char *ASAP_CanConvert(const char *filename, const ASAP_ModuleInfo *module_info,
                            const byte ARRAY module, int module_len)
{
  (void)filename;
	switch (module_info->type) {
	case 'B':
		if (module_info->init == 0x4f3 || module_info->init == 0xf4f3 || module_info->init == 0x4ef)
			return module_info->fastplay == 156 ? "mpd" : "mpt";
		if (module_info->init == RMT_INIT)
			return "rmt";
		if ((module_info->init == 0x4f5 || module_info->init == 0xf4f5 || module_info->init == 0x4f2)
		 || ((module_info->init == 0x4e7 || module_info->init == 0xf4e7 || module_info->init == 0x4e4) && module_info->fastplay == 156)
		 || ((module_info->init == 0x4e5 || module_info->init == 0xf4e5 || module_info->init == 0x4e2) && (module_info->fastplay == 104 || module_info->fastplay == 78)))
			return "tmc";
		if (module_info->init == TM2_INIT)
			return "tm2";
		break;
	case 'C':
		if (module_info->player == 0x500 || module_info->player == 0xf500) {
			if (module_info->fastplay == 156)
				return "dmc";
			return module[module_len - 170] == 0x1e ? "cmr" : "cmc";
		}
		break;
	case 'c':
	case 'z':
	case 'm':
	case 'r':
	case 't':
	case 'T':
		return "sap";
	default:
		break;
	}
	return NULL;
}

int ASAP_Convert(const char *filename, const ASAP_ModuleInfo *module_info,
                 const byte ARRAY module, int module_len, byte ARRAY out_module)
{
    (void) filename;
	int out_len;
	byte *dest;
	int addr;
	int player;
	static const int tmc_player[4] = { 3, -9, -10, -10 };
	static const int tmc_init[4] = { -14, -16, -17, -17 };
	switch (module_info->type) {
	case 'B':
	case 'C':
		out_len = module[module_info->header_len + 4] + (module[module_info->header_len + 5] << 8)
		        - module[module_info->header_len + 2] - (module[module_info->header_len + 3] << 8) + 7;
		if (out_len < 7 || module_info->header_len + out_len >= module_len)
			return -1;
		memcpy(out_module, module + module_info->header_len, out_len);
		return out_len;
	case 'c':
	case 'z':
		dest = put_sap_header(out_module, module_info, 'C', module_info->music, -1, module_info->player);
		if (dest == NULL)
			return -1;
		memcpy(dest, module, module_len);
		dest += module_len;
		memcpy(dest, cmc_obx + 2, sizeof(cmc_obx) - 2);
		if (module_info->type == 'z')
			memcpy(dest + 4 + CMR_BASS_TABLE_OFFSET, cmr_bass_table, sizeof(cmr_bass_table));
		dest += sizeof(cmc_obx) - 2;
		return dest - out_module;
	case 'm':
		if (module_info->songs != 1) {
			addr = module_info->player - 17 - module_info->songs;
			dest = put_sap_header(out_module, module_info, 'B', -1, module_info->player - 17, module_info->player + 3);
		}
		else {
			addr = module_info->player - 13;
			dest = put_sap_header(out_module, module_info, 'B', -1, addr, module_info->player + 3);
		}
		if (dest == NULL)
			return -1;
		memcpy(dest, module, module_len);
		dest += module_len;
		*dest++ = (byte) addr;
		*dest++ = (byte) (addr >> 8);
		*dest++ = mpt_obx[4];
		*dest++ = mpt_obx[5];
		if (module_info->songs != 1) {
			memcpy(dest, module_info->song_pos, module_info->songs);
			dest += module_info->songs;
			*dest++ = 0x48; /* pha */
		}
		*dest++ = 0xa0; /* ldy #<music */
		*dest++ = (byte) module_info->music;
		*dest++ = 0xa2; /* ldx #>music */
		*dest++ = (byte) (module_info->music >> 8);
		*dest++ = 0xa9; /* lda #0 */
		*dest++ = 0;
		*dest++ = 0x20; /* jsr player */
		*dest++ = (byte) module_info->player;
		*dest++ = (byte) (module_info->player >> 8);
		if (module_info->songs != 1) {
			*dest++ = 0x68; /* pla */
			*dest++ = 0xa8; /* tay */
			*dest++ = 0xbe; /* ldx song2pos,y */
			*dest++ = (byte) addr;
			*dest++ = (byte) (addr >> 8);
		}
		else {
			*dest++ = 0xa2; /* ldx #0 */
			*dest++ = 0;
		}
		*dest++ = 0xa9; /* lda #2 */
		*dest++ = 2;
		memcpy(dest, mpt_obx + 6, sizeof(mpt_obx) - 6);
		dest += sizeof(mpt_obx) - 6;
		return dest - out_module;
	case 'r':
		dest = put_sap_header(out_module, module_info, 'B', -1, RMT_INIT, module_info->player + 3);
		if (dest == NULL)
			return -1;
		memcpy(dest, module, module_len);
		dest += module_len;
		*dest++ = (byte) RMT_INIT;
		*dest++ = (byte) (RMT_INIT >> 8);
		if (module_info->songs != 1) {
			addr = RMT_INIT + 10 + module_info->songs;
			*dest++ = (byte) addr;
			*dest++ = (byte) (addr >> 8);
			*dest++ = 0xa8; /* tay */
			*dest++ = 0xb9; /* lda song2pos,y */
			*dest++ = (byte) (RMT_INIT + 11);
			*dest++ = (byte) ((RMT_INIT + 11) >> 8);
		}
		else {
			*dest++ = (byte) (RMT_INIT + 8);
			*dest++ = (byte) ((RMT_INIT + 8) >> 8);
			*dest++ = 0xa9; /* lda #0 */
			*dest++ = 0;
		}
		*dest++ = 0xa2; /* ldx #<music */
		*dest++ = (byte) module_info->music;
		*dest++ = 0xa0; /* ldy #>music */
		*dest++ = (byte) (module_info->music >> 8);
		*dest++ = 0x4c; /* jmp player */
		*dest++ = (byte) module_info->player;
		*dest++ = (byte) (module_info->player >> 8);
		if (module_info->songs != 1) {
			memcpy(dest, module_info->song_pos, module_info->songs);
			dest += module_info->songs;
		}
		if (module_info->channels == 1) {
			memcpy(dest, rmt4_obx + 2, sizeof(rmt4_obx) - 2);
			dest += sizeof(rmt4_obx) - 2;
		}
		else {
			memcpy(dest, rmt8_obx + 2, sizeof(rmt8_obx) - 2);
			dest += sizeof(rmt8_obx) - 2;
		}
		return dest - out_module;
	case 't':
		player = module_info->player + tmc_player[module[0x25] - 1];
		addr = player + tmc_init[module[0x25] - 1];
		if (module_info->songs != 1)
			addr -= 3;
		dest = put_sap_header(out_module, module_info, 'B', -1, addr, player);
		if (dest == NULL)
			return -1;
		memcpy(dest, module, module_len);
		dest += module_len;
		*dest++ = (byte) addr;
		*dest++ = (byte) (addr >> 8);
		*dest++ = tmc_obx[4];
		*dest++ = tmc_obx[5];
		if (module_info->songs != 1)
			*dest++ = 0x48; /* pha */
		*dest++ = 0xa0; /* ldy #<music */
		*dest++ = (byte) module_info->music;
		*dest++ = 0xa2; /* ldx #>music */
		*dest++ = (byte) (module_info->music >> 8);
		*dest++ = 0xa9; /* lda #$70 */
		*dest++ = 0x70;
		*dest++ = 0x20; /* jsr player */
		*dest++ = (byte) module_info->player;
		*dest++ = (byte) (module_info->player >> 8);
		if (module_info->songs != 1) {
			*dest++ = 0x68; /* pla */
			*dest++ = 0xaa; /* tax */
			*dest++ = 0xa9; /* lda #0 */
			*dest++ = 0;
		}
		else {
			*dest++ = 0xa9; /* lda #$60 */
			*dest++ = 0x60;
		}
		switch (module[0x25]) {
		case 2:
			*dest++ = 0x06; /* asl 0 */
			*dest++ = 0;
			*dest++ = 0x4c; /* jmp player */
			*dest++ = (byte) module_info->player;
			*dest++ = (byte) (module_info->player >> 8);
			*dest++ = 0xa5; /* lda 0 */
			*dest++ = 0;
			*dest++ = 0xe6; /* inc 0 */
			*dest++ = 0;
			*dest++ = 0x4a; /* lsr @ */
			*dest++ = 0x90; /* bcc player+3 */
			*dest++ = 5;
			*dest++ = 0xb0; /* bcs player+6 */
			*dest++ = 6;
			break;
		case 3:
		case 4:
			*dest++ = 0xa0; /* ldy #1 */
			*dest++ = 1;
			*dest++ = 0x84; /* sty 0 */
			*dest++ = 0;
			*dest++ = 0xd0; /* bne player */
			*dest++ = 10;
			*dest++ = 0xc6; /* dec 0 */
			*dest++ = 0;
			*dest++ = 0xd0; /* bne player+6 */
			*dest++ = 12;
			*dest++ = 0xa0; /* ldy #3 */
			*dest++ = module[0x25];
			*dest++ = 0x84; /* sty 0 */
			*dest++ = 0;
			*dest++ = 0xd0; /* bne player+3 */
			*dest++ = 3;
			break;
		default:
			break;
		}
		memcpy(dest, tmc_obx + 6, sizeof(tmc_obx) - 6);
		dest += sizeof(tmc_obx) - 6;
		return dest - out_module;
	case 'T':
		dest = put_sap_header(out_module, module_info, 'B', -1, TM2_INIT, module_info->player + 3);
		if (dest == NULL)
			return -1;
		memcpy(dest, module, module_len);
		dest += module_len;
		*dest++ = (byte) TM2_INIT;
		*dest++ = (byte) (TM2_INIT >> 8);
		if (module_info->songs != 1) {
			*dest++ = (byte) (TM2_INIT + 16);
			*dest++ = (byte) ((TM2_INIT + 16) >> 8);
			*dest++ = 0x48; /* pha */
		}
		else {
			*dest++ = (byte) (TM2_INIT + 14);
			*dest++ = (byte) ((TM2_INIT + 14) >> 8);
		}
		*dest++ = 0xa0; /* ldy #<music */
		*dest++ = (byte) module_info->music;
		*dest++ = 0xa2; /* ldx #>music */
		*dest++ = (byte) (module_info->music >> 8);
		*dest++ = 0xa9; /* lda #$70 */
		*dest++ = 0x70;
		*dest++ = 0x20; /* jsr player */
		*dest++ = (byte) module_info->player;
		*dest++ = (byte) (module_info->player >> 8);
		if (module_info->songs != 1) {
			*dest++ = 0x68; /* pla */
			*dest++ = 0xaa; /* tax */
			*dest++ = 0xa9; /* lda #0 */
			*dest++ = 0;
		}
		else {
			*dest++ = 0xa9; /* lda #0 */
			*dest++ = 0;
			*dest++ = 0xaa; /* tax */
		}
		*dest++ = 0x4c; /* jmp player */
		*dest++ = (byte) module_info->player;
		*dest++ = (byte) (module_info->player >> 8);
		memcpy(dest, tm2_obx + 2, sizeof(tm2_obx) - 2);
		dest += sizeof(tm2_obx) - 2;
		return dest - out_module;
	default:
		return -1;
	}
}

#endif /* !defined(JAVA) && !defined(CSHARP) */
