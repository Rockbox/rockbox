/*
** $Id: lauxlib.c,v 1.159.1.3 2008/01/21 13:20:51 roberto Exp $
** Auxiliary functions for building Lua libraries
** See Copyright Notice in lua.h
*/


#include <ctype.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lstring.h" /* ROCKLUA ADDED */

/* This file uses only the official API of Lua.
** Any function declared here could be written as an application function.
** Note ** luaS_newlloc breaks this guarantee ROCKLUA ADDED
*/

#define lauxlib_c
#define LUA_LIB

#include "lua.h"

#include "lauxlib.h"


#define FREELIST_REF	0	/* free list of references */


/* convert a stack index to positive */
#define abs_index(L, i)		((i) > 0 || (i) <= LUA_REGISTRYINDEX ? (i) : \
					lua_gettop(L) + (i) + 1)

#ifndef LUA_OOM
  #define LUA_OOM(L) {}
#endif
/*
** {======================================================
** Error-report functions
** =======================================================
*/


LUALIB_API int luaL_argerror (lua_State *L, int narg, const char *extramsg) {
  lua_Debug ar;
  if (!lua_getstack(L, 0, &ar))  /* no stack frame? */
    return luaL_error(L, "bad argument #%d (%s)", narg, extramsg);
  lua_getinfo(L, "n", &ar);
  if (strcmp(ar.namewhat, "method") == 0) {
    narg--;  /* do not count `self' */
    if (narg == 0)  /* error is in the self argument itself? */
      return luaL_error(L, "calling " LUA_QS " on bad self (%s)",
                           ar.name, extramsg);
  }
  if (ar.name == NULL)
    ar.name = "?";
  return luaL_error(L, "bad argument #%d to " LUA_QS " (%s)",
                        narg, ar.name, extramsg);
}


LUALIB_API int luaL_typerror (lua_State *L, int narg, const char *tname) {
  const char *msg = lua_pushfstring(L, "%s expected, got %s",
                                    tname, luaL_typename(L, narg));
  return luaL_argerror(L, narg, msg);
}


static void tag_error (lua_State *L, int narg, int tag) {
  luaL_typerror(L, narg, lua_typename(L, tag));
}


LUALIB_API void luaL_where (lua_State *L, int level) {
  lua_Debug ar;
  if (lua_getstack(L, level, &ar)) {  /* check function at level */
    lua_getinfo(L, "Sl", &ar);  /* get info about it */
    if (ar.currentline > 0) {  /* is there info? */
      lua_pushfstring(L, "%s:%d: ", ar.short_src, ar.currentline);
      return;
    }
  }
  lua_pushliteral(L, "");  /* else, no information available... */
}


LUALIB_API int luaL_error (lua_State *L, const char *fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  luaL_where(L, 1);
  lua_pushvfstring(L, fmt, argp);
  va_end(argp);
  lua_concat(L, 2);
  return lua_error(L);
}

/* }====================================================== */


LUALIB_API int luaL_checkoption (lua_State *L, int narg, const char *def,
                                 const char *const lst[]) {
  const char *name = (def) ? luaL_optstring(L, narg, def) :
                             luaL_checkstring(L, narg);
  int i;
  for (i=0; lst[i]; i++)
    if (strcmp(lst[i], name) == 0)
      return i;
  return luaL_argerror(L, narg,
                       lua_pushfstring(L, "invalid option " LUA_QS, name));
}


LUALIB_API int luaL_newmetatable (lua_State *L, const char *tname) {
  lua_getfield(L, LUA_REGISTRYINDEX, tname);  /* get registry.name */
  if (!lua_isnil(L, -1))  /* name already in use? */
    return 0;  /* leave previous value on top, but return 0 */
  lua_pop(L, 1);
  lua_newtable(L);  /* create metatable */
  lua_pushvalue(L, -1);
  lua_setfield(L, LUA_REGISTRYINDEX, tname);  /* registry.name = metatable */
  return 1;
}


LUALIB_API void *luaL_checkudata (lua_State *L, int ud, const char *tname) {
  void *p = lua_touserdata(L, ud);
  if (p != NULL) {  /* value is a userdata? */
    if (lua_getmetatable(L, ud)) {  /* does it have a metatable? */
      lua_getfield(L, LUA_REGISTRYINDEX, tname);  /* get correct metatable */
      if (lua_rawequal(L, -1, -2)) {  /* does it have the correct mt? */
        lua_pop(L, 2);  /* remove both metatables */
        return p;
      }
    }
  }
  luaL_typerror(L, ud, tname);  /* else error */
  return NULL;  /* to avoid warnings */
}


LUALIB_API void luaL_checkstack (lua_State *L, int space, const char *mes) {
  if (!lua_checkstack(L, space))
    luaL_error(L, "stack overflow (%s)", mes);
}


LUALIB_API void luaL_checktype (lua_State *L, int narg, int t) {
  if (lua_type(L, narg) != t)
    tag_error(L, narg, t);
}


LUALIB_API void luaL_checkany (lua_State *L, int narg) {
  if (lua_type(L, narg) == LUA_TNONE)
    luaL_argerror(L, narg, "value expected");
}


LUALIB_API const char *luaL_checklstring (lua_State *L, int narg, size_t *len) {
  const char *s = lua_tolstring(L, narg, len);
  if (!s) tag_error(L, narg, LUA_TSTRING);
  return s;
}


LUALIB_API const char *luaL_optlstring (lua_State *L, int narg,
                                        const char *def, size_t *len) {
  if (lua_isnoneornil(L, narg)) {
    if (len)
      *len = (def ? strlen(def) : 0);
    return def;
  }
  else return luaL_checklstring(L, narg, len);
}


LUALIB_API lua_Number luaL_checknumber (lua_State *L, int narg) {
  lua_Number d = lua_tonumber(L, narg);
  if (d == 0 && !lua_isnumber(L, narg))  /* avoid extra test when d is not 0 */
    tag_error(L, narg, LUA_TNUMBER);
  return d;
}


LUALIB_API lua_Number luaL_optnumber (lua_State *L, int narg, lua_Number def) {
  return luaL_opt(L, luaL_checknumber, narg, def);
}


LUALIB_API lua_Integer luaL_checkinteger (lua_State *L, int narg) {
  lua_Integer d = lua_tointeger(L, narg);
  if (d == 0 && !lua_isnumber(L, narg))  /* avoid extra test when d is not 0 */
    tag_error(L, narg, LUA_TNUMBER);
  return d;
}


LUALIB_API lua_Integer luaL_optinteger (lua_State *L, int narg,
                                                      lua_Integer def) {
  return luaL_opt(L, luaL_checkinteger, narg, def);
}

/* ROCKLUA ADDED */
LUALIB_API int luaL_checkboolean (lua_State *L, int narg) {
  int b = lua_toboolean(L, narg);
  if( b == 0 && !lua_isboolean(L, narg))
    tag_error(L, narg, LUA_TBOOLEAN);
  return b;
}

/* ROCKLUA ADDED */
LUALIB_API int luaL_optboolean (lua_State *L, int narg, int def) {
  return luaL_opt(L, luaL_checkboolean, narg, def);
}


LUALIB_API int luaL_getmetafield (lua_State *L, int obj, const char *event) {
  if (!lua_getmetatable(L, obj))  /* no metatable? */
    return 0;
  lua_pushstring(L, event);
  lua_rawget(L, -2);
  if (lua_isnil(L, -1)) {
    lua_pop(L, 2);  /* remove metatable and metafield */
    return 0;
  }
  else {
    lua_remove(L, -2);  /* remove only metatable */
    return 1;
  }
}


LUALIB_API int luaL_callmeta (lua_State *L, int obj, const char *event) {
  obj = abs_index(L, obj);
  if (!luaL_getmetafield(L, obj, event))  /* no metafield? */
    return 0;
  lua_pushvalue(L, obj);
  lua_call(L, 1, 1);
  return 1;
}


 /* ROCKLUA ADDED */
static int libsize_storenames (lua_State *L, const char* libname, const luaL_Reg *l) {
  int size = 0;
  if (libname)
    luaS_newlloc(L, libname, TSTR_INBIN);
  for (; l->name; l++) {
    size++;
    luaS_newlloc(L, l->name, TSTR_INBIN);
  }
  return size;
}


#if 0
LUALIB_API void (luaL_register) (lua_State *L, const char *libname,
                                const luaL_Reg *l) {
  luaI_openlib(L, libname, l, 0);
}
#else /* late bind cfunction tables to save ram */
static int latebind_func_index(lua_State *L)
{
  const luaL_Reg *llib;
  /* name @ top of stack 1(basetable;name)-1*/
  const char *name = lua_tostring(L, -1);

  lua_pushstring (L, "__latebind");/* basetable;name;__latebind;*/
  if(lua_istable(L, -3))
    lua_rawget (L, -3);/* basetable;name;__latebind(t);*/

  luaL_argcheck(L, lua_istable(L, -3) && lua_istable(L, -1), 1,
                "__latebind table expected");
  /* (btable;name;__latebind(t)) */

  /* lookup late bound func(s), lua_objlen allows check of multiple luaL_Reg */
  for(int i = lua_objlen(L, -1); i > 0; i--) {
    lua_rawgeti (L, -1, i);/* btable;name;__lb(t);llib*/
    llib = (const luaL_Reg *) lua_touserdata (L, -1);
    lua_pop(L, 1); /* (btable;name;__lb(t);llib) -> (btable;name;__lb(t)) */

    if(!llib)
      continue;

    for (; llib->name; llib++) {
      if(!name || strcmp(name, llib->name) == 0) {
        /* don't overwrite existing keys within basetable */
        lua_pushstring(L, llib->name); /* (btable;name;__lb(t);llib->name) */
        lua_rawget(L, -4); /* (btable;name;__lb(t);btable[func]?) */

        if(lua_isnil(L, -1)) {
          /* if function is used it is added to the base table */
          lua_pop(L, 1);
          lua_pushcclosure(L, llib->func, 0); /* (btable;name;__lb(t);llib->func) */
        }

        if(!name) /* nil name binds all functions in table immediately */
          lua_setfield(L, -4, llib->name); /* (btable;name;mtable) */
        else {
          lua_pushvalue(L, -1); /* dupe closure or existing func */
          /* (btable;name;__lb(t);llib->func;llib->func) */
          lua_setfield(L, -5, llib->name); /* (btable;name;__lb(t);llib->func) */
          /* returns the closure */
          return 1;
        }
      }
    }
  }

  lua_pop(L, 2); /* (btable;name;__lb(t)) -> (btable) */
  if(!name) {
    lua_pushnil(L); /* remove metatable (btable;name;__lb(t);nil)*/
    lua_setmetatable(L, -2);
    lua_pushnil(L); /* remove __latebind table*/
    lua_setfield (L, -2, "__latebind");
  }

  return 0;
}


static int latebind_func_pairs(lua_State *L)
{
  /* basetable @ top of stack 1(basetable)-1 */
  luaL_argcheck(L, lua_istable(L, 1), 1, "table expected");

#if 0
  lua_getglobal(L, "next"); /* function to be called / returned (btable;next) */
  lua_createtable(L, 0, 15); /* btable;next;newtable; */
  lua_pushnil(L); /* nil name retrieves all unbound latebound functions */
  lua_pushnil(L); /* first key */
#else
  /* this way is more RAM efficient in testing */
  if(luaL_dostring(L, "return next, {}, nil, nil")!= 0)
    lua_error(L);
#endif
  /* (btable;next;ntable;nil;nil) */
  /* clone base table */
  while(lua_next(L, 1) > 0) {
    /* (btable;next;ntable;nil;k;v) */
    lua_pushvalue(L, -2); /* dupe key Stk = (..;k;v -> ..k;v;k)*/
    lua_insert(L, -2); /* Stk = (..k;k;v) */
    lua_rawset(L, -5); /* btable;next;ntable;nil;k */
  }
  /* fill the new table with all the latebound functions */
  /* nil name retrieves all unbound latebound functions */
  latebind_func_index(L);/* (btable;next;ntable;nil) -> (btable;next;ntable) */
  lua_pushnil(L); /*nil initial key for next*/
  /* stack = (btable;next;ntable;nil) */
  return 3; /*(next,ntable,nil)*/
}


LUALIB_API void (luaL_register) (lua_State *L, const char *libname,
                                const luaL_Reg *l) {
  if(!libname)
  {
    /* if there is no libname register normally */
    luaI_openlib(L, libname, l, 0);
    return;
  }

  /* store empty table instead of passed luaL_Reg table */
  static const struct luaL_reg late_lib [] =
  {
    {NULL, NULL}
  };

  static const struct luaL_reg late_meta [] =
  {
    {"__index", latebind_func_index},
    {"__pairs", latebind_func_pairs},
    {"__call", latebind_func_index}, /* allows t("func") -- nil binds all */
    {NULL, NULL}
  };

  libsize_storenames(L, libname, l); /* store func names */
  luaI_openlib(L, libname, late_lib, 0); /* basetable; */

  luaL_findtable(L, -1,"__latebind", 0); /* create table if doesn't exist */
  /* basetable;__latebind(t); */
  /* save pointer to real luaL_reg */
  lua_pushlightuserdata (L, (void *) l); /*basetable;__lb(t);userdata;*/
  lua_rawseti(L, -2, lua_objlen(L, -2) + 1); /*lb(t)[n] = userdata */

  lua_pop(L, 1);/* (basetable;__latebind(t)) -> (basetable) */

  if(luaL_newmetatable(L, "META_LATEBIND"))
  {
    luaI_openlib(L, NULL, late_meta, 0); /*basetable;metatable*/
    lua_pushvalue(L, -1); /* dupe the metatable (basetable;mt;mt) */
    lua_setfield (L, -2, "__metatable"); /* metatable[__metatable] = metatable */
  }

  lua_setmetatable(L, -2); /* (basetable;mt) -> (basetable+mtable) */
  /* base table is top of stack (basetable) */
}
#endif

#if 0
static int libsize (const luaL_Reg *l) {
  int size = 0;
  for (; l->name; l++) size++;
  return size;
}
#endif

LUALIB_API void luaI_openlib (lua_State *L, const char *libname,
                              const luaL_Reg *l, int nup) {
  int size = libsize_storenames(L, libname, l);
  if (libname) {
    /* check whether lib already exists */
    luaL_findtable(L, LUA_REGISTRYINDEX, "_LOADED", 1);
    lua_getfield(L, -1, libname);  /* get _LOADED[libname] */
    if (!lua_istable(L, -1)) {  /* not found? */
      lua_pop(L, 1);  /* remove previous result */
      /* try global variable (and create one if it does not exist) */
      if (luaL_findtable(L, LUA_GLOBALSINDEX, libname, size) != NULL)
        luaL_error(L, "name conflict for module " LUA_QS, libname);
      lua_pushvalue(L, -1);
      lua_setfield(L, -3, libname);  /* _LOADED[libname] = new table */
    }
    lua_remove(L, -2);  /* remove _LOADED table */
    lua_insert(L, -(nup+1));  /* move library table to below upvalues */
  }
  for (; l->name; l++) {
    int i;
    for (i=0; i<nup; i++)  /* copy upvalues to the top */
      lua_pushvalue(L, -nup);
    lua_pushcclosure(L, l->func, nup);
    lua_setfield(L, -(nup+2), l->name);
  }
  lua_pop(L, nup);  /* remove upvalues */
}



/*
** {======================================================
** getn-setn: size for arrays
** =======================================================
*/

#if defined(LUA_COMPAT_GETN)

static int checkint (lua_State *L, int topop) {
  int n = (lua_type(L, -1) == LUA_TNUMBER) ? lua_tointeger(L, -1) : -1;
  lua_pop(L, topop);
  return n;
}


static void getsizes (lua_State *L) {
  lua_getfield(L, LUA_REGISTRYINDEX, "LUA_SIZES");
  if (lua_isnil(L, -1)) {  /* no `size' table? */
    lua_pop(L, 1);  /* remove nil */
    lua_newtable(L);  /* create it */
    lua_pushvalue(L, -1);  /* `size' will be its own metatable */
    lua_setmetatable(L, -2);
    lua_pushliteral(L, "kv");
    lua_setfield(L, -2, "__mode");  /* metatable(N).__mode = "kv" */
    lua_pushvalue(L, -1);
    lua_setfield(L, LUA_REGISTRYINDEX, "LUA_SIZES");  /* store in register */
  }
}


LUALIB_API void luaL_setn (lua_State *L, int t, int n) {
  t = abs_index(L, t);
  lua_pushliteral(L, "n");
  lua_rawget(L, t);
  if (checkint(L, 1) >= 0) {  /* is there a numeric field `n'? */
    lua_pushliteral(L, "n");  /* use it */
    lua_pushinteger(L, n);
    lua_rawset(L, t);
  }
  else {  /* use `sizes' */
    getsizes(L);
    lua_pushvalue(L, t);
    lua_pushinteger(L, n);
    lua_rawset(L, -3);  /* sizes[t] = n */
    lua_pop(L, 1);  /* remove `sizes' */
  }
}


LUALIB_API int luaL_getn (lua_State *L, int t) {
  int n;
  t = abs_index(L, t);
  lua_pushliteral(L, "n");  /* try t.n */
  lua_rawget(L, t);
  if ((n = checkint(L, 1)) >= 0) return n;
  getsizes(L);  /* else try sizes[t] */
  lua_pushvalue(L, t);
  lua_rawget(L, -2);
  if ((n = checkint(L, 2)) >= 0) return n;
  return (int)lua_objlen(L, t);
}

#endif

/* }====================================================== */



LUALIB_API const char *luaL_gsub (lua_State *L, const char *s, const char *p,
                                                               const char *r) {
  const char *wild;
  size_t l = strlen(p);
  luaL_Buffer b;
  luaL_buffinit(L, &b);
  while ((wild = strstr(s, p)) != NULL) {
    luaL_addlstring(&b, s, wild - s);  /* push prefix */
    luaL_addstring(&b, r);  /* push replacement in place of pattern */
    s = wild + l;  /* continue after `p' */
  }
  luaL_addstring(&b, s);  /* push last suffix */
  luaL_pushresult(&b);
  return lua_tostring(L, -1);
}


LUALIB_API const char *luaL_findtable (lua_State *L, int idx,
                                       const char *fname, int szhint) {
  const char *e;
  lua_pushvalue(L, idx);
  do {
    e = strchr(fname, '.');
    if (e == NULL) e = fname + strlen(fname);
    lua_pushlstring(L, fname, e - fname);
    lua_rawget(L, -2);
    if (lua_isnil(L, -1)) {  /* no such field? */
      lua_pop(L, 1);  /* remove this nil */
      lua_createtable(L, 0, (*e == '.' ? 1 : szhint)); /* new table for field */
      lua_pushlstring(L, fname, e - fname);
      lua_pushvalue(L, -2);
      lua_settable(L, -4);  /* set new table into field */
    }
    else if (!lua_istable(L, -1)) {  /* field has a non-table value? */
      lua_pop(L, 2);  /* remove table and value */
      return fname;  /* return problematic part of the name */
    }
    lua_remove(L, -2);  /* remove previous table */
    fname = e + 1;
  } while (*e == '.');
  return NULL;
}



/*
** {======================================================
** Generic Buffer manipulation
** =======================================================
*/


#define bufflen(B)	((B)->p - (B)->buffer)
#define bufffree(B)	((size_t)(LUAL_BUFFERSIZE - bufflen(B)))

#define LIMIT	(LUA_MINSTACK/2)


static int emptybuffer (luaL_Buffer *B) {
  size_t l = bufflen(B);
  if (l == 0) return 0;  /* put nothing on stack */
  else {
    lua_pushlstring(B->L, B->buffer, l);
    B->p = B->buffer;
    B->lvl++;
    return 1;
  }
}


static void adjuststack (luaL_Buffer *B) {
  if (B->lvl > 1) {
    lua_State *L = B->L;
    int toget = 1;  /* number of levels to concat */
    size_t toplen = lua_strlen(L, -1);
    do {
      size_t l = lua_strlen(L, -(toget+1));
      if (B->lvl - toget + 1 >= LIMIT || toplen > l) {
        toplen += l;
        toget++;
      }
      else break;
    } while (toget < B->lvl);
    lua_concat(L, toget);
    B->lvl = B->lvl - toget + 1;
  }
}


LUALIB_API char *luaL_prepbuffer (luaL_Buffer *B) {
  if (emptybuffer(B))
    adjuststack(B);
  return B->buffer;
}


LUALIB_API void luaL_addlstring (luaL_Buffer *B, const char *s, size_t l) {
  while (l--)
    luaL_addchar(B, *s++);
}


LUALIB_API void luaL_addstring (luaL_Buffer *B, const char *s) {
  luaL_addlstring(B, s, strlen(s));
}


LUALIB_API void luaL_pushresult (luaL_Buffer *B) {
  emptybuffer(B);
  lua_concat(B->L, B->lvl);
  B->lvl = 1;
}


LUALIB_API void luaL_addvalue (luaL_Buffer *B) {
  lua_State *L = B->L;
  size_t vl;
  const char *s = lua_tolstring(L, -1, &vl);
  if (vl <= bufffree(B)) {  /* fit into buffer? */
    memcpy(B->p, s, vl);  /* put it there */
    B->p += vl;
    lua_pop(L, 1);  /* remove from stack */
  }
  else {
    if (emptybuffer(B))
      lua_insert(L, -2);  /* put buffer before new value */
    B->lvl++;  /* add new value into B stack */
    adjuststack(B);
  }
}


LUALIB_API void luaL_buffinit (lua_State *L, luaL_Buffer *B) {
  B->L = L;
  B->p = B->buffer;
  B->lvl = 0;
}

/* }====================================================== */


LUALIB_API int luaL_ref (lua_State *L, int t) {
  int ref;
  t = abs_index(L, t);
  if (lua_isnil(L, -1)) {
    lua_pop(L, 1);  /* remove from stack */
    return LUA_REFNIL;  /* `nil' has a unique fixed reference */
  }
  lua_rawgeti(L, t, FREELIST_REF);  /* get first free element */
  ref = (int)lua_tointeger(L, -1);  /* ref = t[FREELIST_REF] */
  lua_pop(L, 1);  /* remove it from stack */
  if (ref != 0) {  /* any free element? */
    lua_rawgeti(L, t, ref);  /* remove it from list */
    lua_rawseti(L, t, FREELIST_REF);  /* (t[FREELIST_REF] = t[ref]) */
  }
  else {  /* no free elements */
    ref = (int)lua_objlen(L, t);
    ref++;  /* create new reference */
  }
  lua_rawseti(L, t, ref);
  return ref;
}


LUALIB_API void luaL_unref (lua_State *L, int t, int ref) {
  if (ref >= 0) {
    t = abs_index(L, t);
    lua_rawgeti(L, t, FREELIST_REF);
    lua_rawseti(L, t, ref);  /* t[ref] = t[FREELIST_REF] */
    lua_pushinteger(L, ref);
    lua_rawseti(L, t, FREELIST_REF);  /* t[FREELIST_REF] = ref */
  }
}



/*
** {======================================================
** Load functions
** =======================================================
*/

typedef struct LoadF {
  int extraline;
  int f;
  char buff[LUAL_BUFFERSIZE];
} LoadF;

static const char *getF (lua_State *L, void *ud, size_t *size) {
  LoadF *lf = (LoadF *)ud;
  (void)L;
  if (lf->extraline) {
    lf->extraline = 0;
    *size = 1;
    return "\n";
  }
  *size = rb->read(lf->f, lf->buff, LUAL_BUFFERSIZE);
  return (*size > 0) ? lf->buff : NULL;
}


static int errfile (lua_State *L, const char *what, int fnameindex) {
  const char *serr = strerror(errno);
  const char *filename = lua_tostring(L, fnameindex) + 1;
  lua_pushfstring(L, "cannot %s %s: %s", what, filename, serr);
  lua_remove(L, fnameindex);
  return LUA_ERRFILE;
}

LUALIB_API int luaL_loadfile (lua_State *L, const char *filename) {
  LoadF lf;
  int status;
  int fnameindex = lua_gettop(L) + 1;  /* index of filename on the stack */
  lf.extraline = 0;
  lf.f = rb->open(filename, O_RDONLY);
  lua_pushfstring(L, "@%s", filename);
  if (lf.f < 0) return errfile(L, "open", fnameindex);
  status = lua_load(L, getF, &lf, lua_tostring(L, -1));
  rb->close(lf.f);
  lua_remove(L, fnameindex);
  return status;
}


typedef struct LoadS {
  const char *s;
  size_t size;
} LoadS;


static const char *getS (lua_State *L, void *ud, size_t *size) {
  LoadS *ls = (LoadS *)ud;
  (void)L;
  if (ls->size == 0) return NULL;
  *size = ls->size;
  ls->size = 0;
  return ls->s;
}


LUALIB_API int luaL_loadbuffer (lua_State *L, const char *buff, size_t size,
                                const char *name) {
  LoadS ls;
  ls.s = buff;
  ls.size = size;
  return lua_load(L, getS, &ls, name);
}


LUALIB_API int (luaL_loadstring) (lua_State *L, const char *s) {
  return luaL_loadbuffer(L, s, strlen(s), s);
}



/* }====================================================== */


static void *l_alloc (void *ud, void *ptr, size_t osize, size_t nsize) {
  (void) osize;
  lua_State *L = (lua_State *)ud;
  void *nptr;

  if (nsize == 0) {
    free(ptr);
    return NULL;
  }

  nptr = realloc(ptr, nsize);
  if (nptr == NULL) {
    if(L != NULL)
    {
      luaC_fullgc(L); /* emergency full collection. */
      nptr = realloc(ptr, nsize); /* try allocation again */
    }

    if (nptr == NULL) {
      LUA_OOM(L); /* if defined.. signal OOM condition */
      nptr = realloc(ptr, nsize); /* try allocation again */
    }
  }

  return nptr;
}


static int panic (lua_State *L) {
    DEBUGF("PANIC: unprotected error in call to Lua API (%s)\n",
        lua_tostring(L, -1));
    rb->splashf(5 * HZ, "PANIC: unprotected error in call to Lua API (%s)",
        lua_tostring(L, -1));

    return 0;
}


LUALIB_API lua_State *luaL_newstate (void) {
  lua_State *L = lua_newstate(l_alloc, NULL);
  if (L){
    lua_setallocf(L, l_alloc, L); /* allocator needs lua_State. */
    lua_atpanic(L, &panic);
  }
  return L;
}

