/*
** $Id$
** String table (keep all strings handled by Lua)
** See Copyright Notice in lua.h
*/

#ifndef lstring_h
#define lstring_h


#include "lgc.h"
#include "lobject.h"
#include "lstate.h"

/* ROCKLUA ADDED */
#define TSTR_INBLOB    0 /* string will be allocated at end of tstring struct */
#define TSTR_INBIN     1 /* string is static within binary, pointer stored    */
#define TSTR_FIXED     2 /* string won't be collected for duration of L state */
#define TSTR_CHKSZ     4 /* luaS_newllocstr shall determine size of string    */
#define TSTR_ISLIT     8 | TSTR_INBIN /* literal string static within binary  */
#define sizetstring(t, l) (sizeof(union TString) + (testbits((t), TSTR_INBIN) ? \
                                   sizeof(const char **) : ((l)+1)*sizeof(char)))

#define sizeudata(u)	(sizeof(union Udata)+(u)->len)

#define luaS_new(L, s)  (luaS_newllocstr(L, s, 0, TSTR_INBLOB | TSTR_CHKSZ))
#define luaS_newlstr(L, s, len) (luaS_newllocstr(L, s, len, TSTR_INBLOB))
#define luaS_newlloc(L, s, t)   (luaS_newllocstr(L, s, 0, ((t) | TSTR_CHKSZ)))
#define luaS_newliteral(L, s)   (luaS_newllocstr(L, "" s, \
                                 (sizeof(s)/sizeof(char))-1, TSTR_ISLIT))

#define luaS_fix(s)	l_setbit((s)->tsv.marked, FIXEDBIT)

LUAI_FUNC void luaS_resize (lua_State *L, int newsize);
LUAI_FUNC Udata *luaS_newudata (lua_State *L, size_t s, Table *e);
/* ROCKLUA ADDED */
LUAI_FUNC TString *luaS_newllocstr (lua_State *L,
                                    const char *str, size_t l, char type);

#endif
