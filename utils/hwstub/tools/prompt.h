/* Copyright (C) 2012-2015 Papavasileiou Dimitris
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef _PROMPT_H_
#define _PROMPT_H_

#include <lualib.h>
#include <lauxlib.h>

#define HAVE_LIBREADLINE
#define HAVE_READLINE_HISTORY
#define HAVE_IOCTL
#define COMPLETE_KEYWORDS
#define COMPLETE_MODULES
#define COMPLETE_TABLE_KEYS
#define COMPLETE_METATABLE_KEYS
#define COMPLETE_FILE_NAMES

#define LUAP_VERSION "0.6"

void luap_setprompts(lua_State *L, const char *single, const char *multi);
void luap_sethistory(lua_State *L, const char *file);
void luap_setname(lua_State *L, const char *name);
void luap_setcolor(lua_State *L, int enable);

void luap_getprompts(lua_State *L, const char **single, const char **multi);
void luap_gethistory(lua_State *L, const char **file);
void luap_getcolor(lua_State *L, int *enabled);
void luap_getname(lua_State *L, const char **name);

void luap_enter(lua_State *L, bool *terminate);
char *luap_describe (lua_State *L, int index);
int luap_call (lua_State *L, int n);

#endif
