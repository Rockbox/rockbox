/*
 * asap_internal.h - private interface of the ASAP engine
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

#ifndef _ASAP_INTERNAL_H_
#define _ASAP_INTERNAL_H_

#if !defined(JAVA) && !defined(CSHARP)

#include "asap.h"

#define CONST_LOOKUP(type, name) \
                                static const type name[]
#define FILE_FUNC               static
#define ASAP_FUNC
#define PTR                     *
#define ADDRESSOF               &
#define ARRAY                   *
#define VOIDPTR                 void *
#define UBYTE(data)             (data)
#define SBYTE(data)             (signed char) (data)
#define STRING                  const char *
#define ZERO_ARRAY(array)       memset(array, 0, sizeof(array))
#define COPY_ARRAY(dest, dest_offset, src, src_offset, len) \
                                memcpy(dest + dest_offset, src + src_offset, len)
#define NEW_ARRAY(type, name, size) \
                                type name[size]
#define INIT_ARRAY(array)       memset(array, 0, sizeof(array))

#define AST                     ast->
#define PST                     pst->
#define MODULE_INFO             module_info->
#define ASAP_OBX                const byte *
#define GET_OBX(name)           name##_obx

int ASAP_GetByte(ASAP_State *ast, int addr);
void ASAP_PutByte(ASAP_State *ast, int addr, int data);

void Cpu_RunScanlines(ASAP_State *ast, int scanlines);

void PokeySound_Initialize(ASAP_State *ast);
void PokeySound_StartFrame(ASAP_State *ast);
void PokeySound_PutByte(ASAP_State *ast, int addr, int data);
int PokeySound_GetRandom(ASAP_State *ast, int addr);
void PokeySound_EndFrame(ASAP_State *ast, int cycle_limit);
int PokeySound_Generate(ASAP_State *ast, byte buffer[], int buffer_offset, int blocks, ASAP_SampleFormat format);
abool PokeySound_IsSilent(const PokeyState *pst);
void PokeySound_Mute(const ASAP_State *ast, PokeyState *pst, int mask);

#ifdef ASAPSCAN
abool call_6502_player(ASAP_State *ast);
extern abool cpu_trace;
void print_cpu_state(const ASAP_State *ast, int pc, int a, int x, int y, int s, int nz, int vdi, int c);
#endif

#endif /* !defined(JAVA) && !defined(CSHARP) */

#define ASAP_MAIN_CLOCK         1773447

#define V_FLAG                  0x40
#define D_FLAG                  0x08
#define I_FLAG                  0x04
#define Z_FLAG                  0x02

#define NEVER                   0x800000

#define dGetByte(addr)          UBYTE(AST memory[addr])
#define dPutByte(addr, data)    AST memory[addr] = (byte) (data)
#define dGetWord(addr)          (dGetByte(addr) + (dGetByte((addr) + 1) << 8))
#define GetByte(addr)           (((addr) & 0xf900) == 0xd000 ? ASAP_GetByte(ast, addr) : dGetByte(addr))
#define PutByte(addr, data)     do { if (((addr) & 0xf900) == 0xd000) ASAP_PutByte(ast, addr, data); else dPutByte(addr, data); } while (FALSE)
#define RMW_GetByte(dest, addr) do { if (((addr) >> 8) == 0xd2) { dest = ASAP_GetByte(ast, addr); AST cycle--; ASAP_PutByte(ast, addr, dest); AST cycle++; } else dest = dGetByte(addr); } while (FALSE)

#endif /* _ASAP_INTERNAL_H_ */
