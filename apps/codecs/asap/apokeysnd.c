/*
 * apokeysnd.c - another POKEY sound emulator
 *
 * Copyright (C) 2007-2008  Piotr Fusik
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

#define memset ci->memset
#define ULTRASOUND_CYCLES  112

#define MUTE_FREQUENCY     1
#define MUTE_INIT          2
#define MUTE_USER          4

CONST_LOOKUP(byte, poly4_lookup) =
	{ 0, 0, 0, 0, 1, 1, 1, 0, 1, 1, 0, 0, 1, 0, 1 };
CONST_LOOKUP(byte, poly5_lookup) =
	{ 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 1, 0, 0, 0, 1,
	  0, 1, 0, 1, 1, 1, 1, 0, 1, 1, 0, 1, 0, 0, 1, 1 };

FILE_FUNC void init_state(PokeyState PTR pst)
{
	PST audctl = 0;
	PST init = FALSE;
	PST poly_index = 15 * 31 * 131071;
	PST div_cycles = 28;
	PST mute1 = MUTE_FREQUENCY | MUTE_USER;
	PST mute2 = MUTE_FREQUENCY | MUTE_USER;
	PST mute3 = MUTE_FREQUENCY | MUTE_USER;
	PST mute4 = MUTE_FREQUENCY | MUTE_USER;
	PST audf1 = 0;
	PST audf2 = 0;
	PST audf3 = 0;
	PST audf4 = 0;
	PST audc1 = 0;
	PST audc2 = 0;
	PST audc3 = 0;
	PST audc4 = 0;
	PST tick_cycle1 = NEVER;
	PST tick_cycle2 = NEVER;
	PST tick_cycle3 = NEVER;
	PST tick_cycle4 = NEVER;
	PST period_cycles1 = 28;
	PST period_cycles2 = 28;
	PST period_cycles3 = 28;
	PST period_cycles4 = 28;
	PST reload_cycles1 = 28;
	PST reload_cycles3 = 28;
	PST out1 = 0;
	PST out2 = 0;
	PST out3 = 0;
	PST out4 = 0;
	PST delta1 = 0;
	PST delta2 = 0;
	PST delta3 = 0;
	PST delta4 = 0;
	PST skctl = 3;
	ZERO_ARRAY(PST delta_buffer);
}

ASAP_FUNC void PokeySound_Initialize(ASAP_State PTR ast)
{
	int i;
	int reg;
	reg = 0x1ff;
	for (i = 0; i < 511; i++) {
		reg = ((((reg >> 5) ^ reg) & 1) << 8) + (reg >> 1);
		AST poly9_lookup[i] = (byte) reg;
	}
	reg = 0x1ffff;
	for (i = 0; i < 16385; i++) {
		reg = ((((reg >> 5) ^ reg) & 0xff) << 9) + (reg >> 8);
		AST poly17_lookup[i] = (byte) (reg >> 1);
	}
	AST sample_offset = 0;
	AST sample_index = 0;
	AST samples = 0;
	AST iir_acc_left = 0;
	AST iir_acc_right = 0;
	init_state(ADDRESSOF AST base_pokey);
	init_state(ADDRESSOF AST extra_pokey);
}

#define CYCLE_TO_SAMPLE(cycle)  (((cycle) * ASAP_SAMPLE_RATE + AST sample_offset) / ASAP_MAIN_CLOCK)

#define DO_TICK(ch) \
	if (PST init) { \
		switch (PST audc##ch >> 4) { \
		case 10: \
		case 14: \
			PST out##ch ^= 1; \
			PST delta_buffer[CYCLE_TO_SAMPLE(cycle)] += PST delta##ch = -PST delta##ch; \
			break; \
		default: \
			break; \
		} \
	} \
	else { \
		int poly = cycle + PST poly_index - (ch - 1); \
		int newout = PST out##ch; \
		switch (PST audc##ch >> 4) { \
		case 0: \
			if (poly5_lookup[poly % 31] != 0) { \
				if ((PST audctl & 0x80) != 0) \
					newout = AST poly9_lookup[poly % 511] & 1; \
				else { \
					poly %= 131071; \
					newout = (AST poly17_lookup[poly >> 3] >> (poly & 7)) & 1; \
				} \
			} \
			break; \
		case 2: \
		case 6: \
			newout ^= poly5_lookup[poly % 31]; \
			break; \
		case 4: \
			if (poly5_lookup[poly % 31] != 0) \
				newout = poly4_lookup[poly % 15]; \
			break; \
		case 8: \
			if ((PST audctl & 0x80) != 0) \
				newout = AST poly9_lookup[poly % 511] & 1; \
			else { \
				poly %= 131071; \
				newout = (AST poly17_lookup[poly >> 3] >> (poly & 7)) & 1; \
			} \
			break; \
		case 10: \
		case 14: \
			newout ^= 1; \
			break; \
		case 12: \
			newout = poly4_lookup[poly % 15]; \
			break; \
		default: \
			break; \
		} \
		if (newout != PST out##ch) { \
			PST out##ch = newout; \
			PST delta_buffer[CYCLE_TO_SAMPLE(cycle)] += PST delta##ch = -PST delta##ch; \
		} \
	}

FILE_FUNC void generate(ASAP_State PTR ast, PokeyState PTR pst, int current_cycle)
{
	for (;;) {
		int cycle = current_cycle;
		if (cycle > PST tick_cycle1)
			cycle = PST tick_cycle1;
		if (cycle > PST tick_cycle2)
			cycle = PST tick_cycle2;
		if (cycle > PST tick_cycle3)
			cycle = PST tick_cycle3;
		if (cycle > PST tick_cycle4)
			cycle = PST tick_cycle4;
		if (cycle == current_cycle)
			break;
		if (cycle == PST tick_cycle3) {
			PST tick_cycle3 += PST period_cycles3;
			if ((PST audctl & 4) != 0 && PST delta1 > 0 && PST mute1 == 0)
				PST delta_buffer[CYCLE_TO_SAMPLE(cycle)] += PST delta1 = -PST delta1;
			DO_TICK(3);
		}
		if (cycle == PST tick_cycle4) {
			PST tick_cycle4 += PST period_cycles4;
			if ((PST audctl & 8) != 0)
				PST tick_cycle3 = cycle + PST reload_cycles3;
			if ((PST audctl & 2) != 0 && PST delta2 > 0 && PST mute2 == 0)
				PST delta_buffer[CYCLE_TO_SAMPLE(cycle)] += PST delta2 = -PST delta2;
			DO_TICK(4);
		}
		if (cycle == PST tick_cycle1) {
			PST tick_cycle1 += PST period_cycles1;
			if ((PST skctl & 0x88) == 8)
				PST tick_cycle2 = cycle + PST period_cycles2;
			DO_TICK(1);
		}
		if (cycle == PST tick_cycle2) {
			PST tick_cycle2 += PST period_cycles2;
			if ((PST audctl & 0x10) != 0)
				PST tick_cycle1 = cycle + PST reload_cycles1;
			else if ((PST skctl & 8) != 0)
				PST tick_cycle1 = cycle + PST period_cycles1;
			DO_TICK(2);
		}
	}
}

#define MUTE_CHANNEL(ch, cond, mask) \
	if (cond) { \
		PST mute##ch |= mask; \
		PST tick_cycle##ch = NEVER; \
	} \
	else { \
		PST mute##ch &= ~mask; \
		if (PST tick_cycle##ch == NEVER && PST mute##ch == 0) \
			PST tick_cycle##ch = AST cycle; \
	}

#define DO_ULTRASOUND(ch) \
	MUTE_CHANNEL(ch, PST period_cycles##ch <= ULTRASOUND_CYCLES && (PST audc##ch >> 4 == 10 || PST audc##ch >> 4 == 14), MUTE_FREQUENCY)

#define DO_AUDC(ch) \
	if (data == PST audc##ch) \
		break; \
	generate(ast, pst, AST cycle); \
	PST audc##ch = data; \
	if ((data & 0x10) != 0) { \
		data &= 0xf; \
		if ((PST mute##ch & MUTE_USER) == 0) \
			PST delta_buffer[CYCLE_TO_SAMPLE(AST cycle)] \
				+= PST delta##ch > 0 ? data - PST delta##ch : data; \
		PST delta##ch = data; \
	} \
	else { \
		data &= 0xf; \
		DO_ULTRASOUND(ch); \
		if (PST delta##ch > 0) { \
			if ((PST mute##ch & MUTE_USER) == 0) \
				PST delta_buffer[CYCLE_TO_SAMPLE(AST cycle)] \
					+= data - PST delta##ch; \
			PST delta##ch = data; \
		} \
		else \
			PST delta##ch = -data; \
	} \
	break;

#define DO_INIT(ch, cond) \
	MUTE_CHANNEL(ch, PST init && cond, MUTE_INIT)

ASAP_FUNC void PokeySound_PutByte(ASAP_State PTR ast, int addr, int data)
{
	PokeyState PTR pst = (addr & AST extra_pokey_mask) != 0
		? ADDRESSOF AST extra_pokey : ADDRESSOF AST base_pokey;
	switch (addr & 0xf) {
	case 0x00:
		if (data == PST audf1)
			break;
		generate(ast, pst, AST cycle);
		PST audf1 = data;
		switch (PST audctl & 0x50) {
		case 0x00:
			PST period_cycles1 = PST div_cycles * (data + 1);
			break;
		case 0x10:
			PST period_cycles2 = PST div_cycles * (data + 256 * PST audf2 + 1);
			PST reload_cycles1 = PST div_cycles * (data + 1);
			DO_ULTRASOUND(2);
			break;
		case 0x40:
			PST period_cycles1 = data + 4;
			break;
		case 0x50:
			PST period_cycles2 = data + 256 * PST audf2 + 7;
			PST reload_cycles1 = data + 4;
			DO_ULTRASOUND(2);
			break;
		}
		DO_ULTRASOUND(1);
		break;
	case 0x01:
		DO_AUDC(1)
	case 0x02:
		if (data == PST audf2)
			break;
		generate(ast, pst, AST cycle);
		PST audf2 = data;
		switch (PST audctl & 0x50) {
		case 0x00:
		case 0x40:
			PST period_cycles2 = PST div_cycles * (data + 1);
			break;
		case 0x10:
			PST period_cycles2 = PST div_cycles * (PST audf1 + 256 * data + 1);
			break;
		case 0x50:
			PST period_cycles2 = PST audf1 + 256 * data + 7;
			break;
		}
		DO_ULTRASOUND(2);
		break;
	case 0x03:
		DO_AUDC(2)
	case 0x04:
		if (data == PST audf3)
			break;
		generate(ast, pst, AST cycle);
		PST audf3 = data;
		switch (PST audctl & 0x28) {
		case 0x00:
			PST period_cycles3 = PST div_cycles * (data + 1);
			break;
		case 0x08:
			PST period_cycles4 = PST div_cycles * (data + 256 * PST audf4 + 1);
			PST reload_cycles3 = PST div_cycles * (data + 1);
			DO_ULTRASOUND(4);
			break;
		case 0x20:
			PST period_cycles3 = data + 4;
			break;
		case 0x28:
			PST period_cycles4 = data + 256 * PST audf4 + 7;
			PST reload_cycles3 = data + 4;
			DO_ULTRASOUND(4);
			break;
		}
		DO_ULTRASOUND(3);
		break;
	case 0x05:
		DO_AUDC(3)
	case 0x06:
		if (data == PST audf4)
			break;
		generate(ast, pst, AST cycle);
		PST audf4 = data;
		switch (PST audctl & 0x28) {
		case 0x00:
		case 0x20:
			PST period_cycles4 = PST div_cycles * (data + 1);
			break;
		case 0x08:
			PST period_cycles4 = PST div_cycles * (PST audf3 + 256 * data + 1);
			break;
		case 0x28:
			PST period_cycles4 = PST audf3 + 256 * data + 7;
			break;
		}
		DO_ULTRASOUND(4);
		break;
	case 0x07:
		DO_AUDC(4)
	case 0x08:
		if (data == PST audctl)
			break;
		generate(ast, pst, AST cycle);
		PST audctl = data;
		PST div_cycles = ((data & 1) != 0) ? 114 : 28;
		/* TODO: tick_cycles */
		switch (data & 0x50) {
		case 0x00:
			PST period_cycles1 = PST div_cycles * (PST audf1 + 1);
			PST period_cycles2 = PST div_cycles * (PST audf2 + 1);
			break;
		case 0x10:
			PST period_cycles1 = PST div_cycles * 256;
			PST period_cycles2 = PST div_cycles * (PST audf1 + 256 * PST audf2 + 1);
			PST reload_cycles1 = PST div_cycles * (PST audf1 + 1);
			break;
		case 0x40:
			PST period_cycles1 = PST audf1 + 4;
			PST period_cycles2 = PST div_cycles * (PST audf2 + 1);
			break;
		case 0x50:
			PST period_cycles1 = 256;
			PST period_cycles2 = PST audf1 + 256 * PST audf2 + 7;
			PST reload_cycles1 = PST audf1 + 4;
			break;
		}
		DO_ULTRASOUND(1);
		DO_ULTRASOUND(2);
		switch (data & 0x28) {
		case 0x00:
			PST period_cycles3 = PST div_cycles * (PST audf3 + 1);
			PST period_cycles4 = PST div_cycles * (PST audf4 + 1);
			break;
		case 0x08:
			PST period_cycles3 = PST div_cycles * 256;
			PST period_cycles4 = PST div_cycles * (PST audf3 + 256 * PST audf4 + 1);
			PST reload_cycles3 = PST div_cycles * (PST audf3 + 1);
			break;
		case 0x20:
			PST period_cycles3 = PST audf3 + 4;
			PST period_cycles4 = PST div_cycles * (PST audf4 + 1);
			break;
		case 0x28:
			PST period_cycles3 = 256;
			PST period_cycles4 = PST audf3 + 256 * PST audf4 + 7;
			PST reload_cycles3 = PST audf3 + 4;
			break;
		}
		DO_ULTRASOUND(3);
		DO_ULTRASOUND(4);
		break;
	case 0x09:
		/* TODO: STIMER */
		break;
	case 0x0f:
		PST skctl = data;
		PST init = ((data & 3) == 0);
		DO_INIT(1, (PST audctl & 0x40) == 0);
		DO_INIT(2, (PST audctl & 0x50) != 0x50);
		DO_INIT(3, (PST audctl & 0x20) == 0);
		DO_INIT(4, (PST audctl & 0x28) != 0x28);
		break;
	default:
		break;
	}
}

ASAP_FUNC int PokeySound_GetRandom(ASAP_State PTR ast, int addr)
{
	PokeyState PTR pst = (addr & AST extra_pokey_mask) != 0
		? ADDRESSOF AST extra_pokey : ADDRESSOF AST base_pokey;
	int i;
	if (PST init)
		return 0xff;
	i = AST cycle + PST poly_index;
	if ((PST audctl & 0x80) != 0)
		return AST poly9_lookup[i % 511];
	else {
		int j;
		i %= 131071;
		j = i >> 3;
		i &= 7;
		return ((AST poly17_lookup[j] >> i) + (AST poly17_lookup[j + 1] << (8 - i))) & 0xff;
	}
}

FILE_FUNC void end_frame(ASAP_State PTR ast, PokeyState PTR pst, int cycle_limit)
{
	int m;
	generate(ast, pst, cycle_limit);
	PST poly_index += cycle_limit;
	m = ((PST audctl & 0x80) != 0) ? 15 * 31 * 511 : 15 * 31 * 131071;
	if (PST poly_index >= 2 * m)
		PST poly_index -= m;
	if (PST tick_cycle1 != NEVER)
		PST tick_cycle1 -= cycle_limit;
	if (PST tick_cycle2 != NEVER)
		PST tick_cycle2 -= cycle_limit;
	if (PST tick_cycle3 != NEVER)
		PST tick_cycle3 -= cycle_limit;
	if (PST tick_cycle4 != NEVER)
		PST tick_cycle4 -= cycle_limit;
}

ASAP_FUNC void PokeySound_StartFrame(ASAP_State PTR ast)
{
	ZERO_ARRAY(AST base_pokey.delta_buffer);
	if (AST extra_pokey_mask != 0)
		ZERO_ARRAY(AST extra_pokey.delta_buffer);
}

ASAP_FUNC void PokeySound_EndFrame(ASAP_State PTR ast, int current_cycle)
{
	end_frame(ast, ADDRESSOF AST base_pokey, current_cycle);
	if (AST extra_pokey_mask != 0)
		end_frame(ast, ADDRESSOF AST extra_pokey, current_cycle);
	AST sample_offset += current_cycle * ASAP_SAMPLE_RATE;
	AST sample_index = 0;
	AST samples = AST sample_offset / ASAP_MAIN_CLOCK;
	AST sample_offset %= ASAP_MAIN_CLOCK;
}

ASAP_FUNC int PokeySound_Generate(ASAP_State PTR ast, byte ARRAY buffer, int buffer_offset, int blocks, ASAP_SampleFormat format)
{
	int i = AST sample_index;
	int samples = AST samples;
	int acc_left = AST iir_acc_left;
	int acc_right = AST iir_acc_right;
	if (blocks < samples - i)
		samples = i + blocks;
	else
		blocks = samples - i;
	for (; i < samples; i++) {
		int sample;
		acc_left += (AST base_pokey.delta_buffer[i] << 20) - (acc_left * 3 >> 10);
		sample = acc_left >> 10;
#define STORE_SAMPLE \
		if (sample < -32767) \
			sample = -32767; \
		else if (sample > 32767) \
			sample = 32767; \
		switch (format) { \
		case ASAP_FORMAT_U8: \
			buffer[buffer_offset++] = (byte) ((sample >> 8) + 128); \
			break; \
		case ASAP_FORMAT_S16_LE: \
			buffer[buffer_offset++] = (byte) sample; \
			buffer[buffer_offset++] = (byte) (sample >> 8); \
			break; \
		case ASAP_FORMAT_S16_BE: \
			buffer[buffer_offset++] = (byte) (sample >> 8); \
			buffer[buffer_offset++] = (byte) sample; \
			break; \
		}
		STORE_SAMPLE;
		if (AST extra_pokey_mask != 0) {
			acc_right += (AST extra_pokey.delta_buffer[i] << 20) - (acc_right * 3 >> 10);
			sample = acc_right >> 10;
			STORE_SAMPLE;
		}
	}
	if (i == AST samples) {
		acc_left += AST base_pokey.delta_buffer[i] << 20;
		acc_right += AST extra_pokey.delta_buffer[i] << 20;
	}
	AST sample_index = i;
	AST iir_acc_left = acc_left;
	AST iir_acc_right = acc_right;
	return blocks;
}

ASAP_FUNC abool PokeySound_IsSilent(const PokeyState PTR pst)
{
	return ((PST audc1 | PST audc2 | PST audc3 | PST audc4) & 0xf) == 0;
}

ASAP_FUNC void PokeySound_Mute(const ASAP_State PTR ast, PokeyState PTR pst, int mask)
{
	MUTE_CHANNEL(1, (mask & 1) != 0, MUTE_USER);
	MUTE_CHANNEL(2, (mask & 2) != 0, MUTE_USER);
	MUTE_CHANNEL(3, (mask & 4) != 0, MUTE_USER);
	MUTE_CHANNEL(4, (mask & 8) != 0, MUTE_USER);
}
