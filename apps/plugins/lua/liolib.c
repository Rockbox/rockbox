/*
** $Id: liolib.c,v 2.73.1.3 2008/01/18 17:47:43 roberto Exp $
** Standard I/O (and system) library
** See Copyright Notice in lua.h
*/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define liolib_c
#define LUA_LIB

#include "lua.h"

#include "lauxlib.h"
#include "lualib.h"
#include "rocklibc.h"



#define IO_INPUT    1
#define IO_OUTPUT   2


static const char *const fnames[] = {"input", "output"};


static int pushresult (lua_State *L, int i, const char *filename) {
  int en = errno;
  if (i) {
    lua_pushboolean(L, 1);
    return 1;
  }
  else {
    lua_pushnil(L);
    if (filename)
      lua_pushfstring(L, "%s: %s", filename, strerror(en));
    else
      lua_pushfstring(L, "%s", strerror(en));
    lua_pushinteger(L, 0);
    return 3;
  }
}


static void fileerror (lua_State *L, int arg, const char *filename) {
  lua_pushfstring(L, "%s: %s", filename, strerror(errno));
  luaL_argerror(L, arg, lua_tostring(L, -1));
}


static int io_type (lua_State *L) {
  void *ud;
  luaL_checkany(L, 1);
  ud = lua_touserdata(L, 1);
  lua_getfield(L, LUA_REGISTRYINDEX, LUA_FILEHANDLE);
  if (ud == NULL || !lua_getmetatable(L, 1) || !lua_rawequal(L, -2, -1))
    lua_pushnil(L);  /* not a file */
  else if (*((int *)ud) < 0)
    lua_pushliteral(L, "closed file");
  else
    lua_pushliteral(L, "file");
  return 1;
}


static int* tofile (lua_State *L) {
  int *f = (int*) luaL_checkudata(L, 1, LUA_FILEHANDLE);
  if (*f < 0)
    luaL_error(L, "attempt to use a closed file");
  return f;
}



/*
** When creating file handles, always creates a `closed' file handle
** before opening the actual file; so, if there is a memory error, the
** file is not left opened.
*/
static int* newfile (lua_State *L) {
  int *pf = (int *)lua_newuserdata(L, sizeof(int));
  *pf = -1;  /* file handle is currently `closed' */
  luaL_getmetatable(L, LUA_FILEHANDLE);
  lua_setmetatable(L, -2);
  return pf;
}


/*
** function to close regular files
*/
static int io_fclose (lua_State *L) {
  int *p = tofile(L);
  int ok = (rb->close(*p) == 0);
  *p = -1;
  return pushresult(L, ok, NULL);
}


static inline int aux_close (lua_State *L) {
  return io_fclose(L);
}


static int io_close (lua_State *L) {
  if (lua_isnone(L, 1))
    lua_rawgeti(L, LUA_ENVIRONINDEX, IO_OUTPUT);
  tofile(L);  /* make sure argument is a file */
  return aux_close(L);
}


static int io_gc (lua_State *L) {
  int f = *(int*) luaL_checkudata(L, 1, LUA_FILEHANDLE);
  /* ignore closed files */
  if (f >= 0)
    aux_close(L);
  return 0;
}


static int io_tostring (lua_State *L) {
  int f = *(int*) luaL_checkudata(L, 1, LUA_FILEHANDLE);
  if (f < 0)
    lua_pushliteral(L, "file (closed)");
  else
    lua_pushfstring(L, "file (%d)", f);
  return 1;
}


static int io_open (lua_State *L) {
  const char *filename = luaL_checkstring(L, 1);
  const char *mode = luaL_optstring(L, 2, "r");
  int *pf = newfile(L);
  int flags = 0;
  if(*(mode+1) == '+') {
    flags = O_RDWR;
    switch(*mode) {
        case 'w':
            flags |= O_TRUNC; break;
        case 'a':
            flags |= O_APPEND; break;
    }
  }
  else {
    switch(*mode) {
        case 'r':
            flags = O_RDONLY; break;
        case 'w':
            flags = O_WRONLY | O_TRUNC; break;
        case 'a':
            flags = O_WRONLY | O_APPEND; break;
    }
  }
  if((*mode == 'w' || *mode == 'a') && !rb->file_exists(filename))
    flags |= O_CREAT;
  *pf = rb->open(filename, flags);
  return (*pf < 0) ? pushresult(L, 0, filename) : 1;
}


static int* getiofile (lua_State *L, int findex) {
  int *f;
  lua_rawgeti(L, LUA_ENVIRONINDEX, findex);
  f = (int *)lua_touserdata(L, -1);
  if (f == NULL || *f < 0)
    luaL_error(L, "standard %s file is closed", fnames[findex - 1]);
  return f;
}


static int g_iofile (lua_State *L, int f, int flags) {
  if (!lua_isnoneornil(L, 1)) {
    const char *filename = lua_tostring(L, 1);
    if (filename) {
      int *pf = newfile(L);
      *pf = rb->open(filename, flags);
      if (*pf < 0)
        fileerror(L, 1, filename);
    }
    else {
      tofile(L);  /* check that it's a valid file handle */
      lua_pushvalue(L, 1);
    }
    lua_rawseti(L, LUA_ENVIRONINDEX, f);
  }
  /* return current value */
  lua_rawgeti(L, LUA_ENVIRONINDEX, f);
  return 1;
}


static int io_input (lua_State *L) {
  return g_iofile(L, IO_INPUT, O_RDONLY);
}


static int io_output (lua_State *L) {
  return g_iofile(L, IO_OUTPUT, O_WRONLY);
}


static int io_readline (lua_State *L);


static void aux_lines (lua_State *L, int idx, int toclose) {
  lua_pushvalue(L, idx);
  lua_pushboolean(L, toclose);  /* close/not close file when finished */
  lua_pushcclosure(L, io_readline, 2);
}


static int f_lines (lua_State *L) {
  tofile(L);  /* check that it's a valid file handle */
  aux_lines(L, 1, 0);
  return 1;
}


static int io_lines (lua_State *L) {
  if (lua_isnoneornil(L, 1)) {  /* no arguments? */
    /* will iterate over default input */
    lua_rawgeti(L, LUA_ENVIRONINDEX, IO_INPUT);
    return f_lines(L);
  }
  else {
    const char *filename = luaL_checkstring(L, 1);
    int *pf = newfile(L);
    *pf = rb->open(filename, O_RDONLY);
    if (*pf < 0)
      fileerror(L, 1, filename);
    aux_lines(L, lua_gettop(L), 1);
    return 1;
  }
}


/*
** {======================================================
** READ
** =======================================================
*/

static int read_number (lua_State *L, int *f) {
  lua_Number d;
  if (PREFIX(fscanf)(*f, LUA_NUMBER_SCAN, &d) == 1) {
    lua_pushnumber(L, d);
    return 1;
  }
  else return 0;  /* read fails */
}


static int test_eof (lua_State *L, int *f) {
  ssize_t s = rb->lseek(*f, 0, SEEK_CUR);
  lua_pushlstring(L, NULL, 0);
  return s != rb->filesize(*f);
}


/* Rockbox already defines read_line() */
static int _read_line (lua_State *L, int *f) {
  luaL_Buffer b;
  luaL_buffinit(L, &b);
  for (;;) {
    size_t l;
    size_t r;
    char *p = luaL_prepbuffer(&b);
    r = rb->read_line(*f, p, LUAL_BUFFERSIZE);
    l = strlen(p);
    if (l == 0 || p[l-1] != '\n')
      luaL_addsize(&b, l);
    else {
      luaL_addsize(&b, l - 1);  /* do not include `eol' */
      luaL_pushresult(&b);  /* close buffer */
      return 1;  /* read at least an `eol' */
    }
    if (r < LUAL_BUFFERSIZE) {  /* eof? */
      luaL_pushresult(&b);  /* close buffer */
      return (lua_objlen(L, -1) > 0);  /* check whether read something */
    }
  }
}


static int read_chars (lua_State *L, int *f, size_t n) {
  size_t rlen;  /* how much to read */
  size_t nr;  /* number of chars actually read */
  luaL_Buffer b;
  luaL_buffinit(L, &b);
  rlen = LUAL_BUFFERSIZE;  /* try to read that much each time */
  do {
    char *p = luaL_prepbuffer(&b);
    if (rlen > n) rlen = n;  /* cannot read more than asked */
    nr = rb->read(*f, p, rlen);
    luaL_addsize(&b, nr);
    n -= nr;  /* still have to read `n' chars */
  } while (n > 0 && nr == rlen);  /* until end of count or eof */
  luaL_pushresult(&b);  /* close buffer */
  return (n == 0 || lua_objlen(L, -1) > 0);
}


static int g_read (lua_State *L, int *f, int first) {
  int nargs = lua_gettop(L) - 1;
  int success;
  int n;
  if (nargs == 0) {  /* no arguments? */
    success = _read_line(L, f);
    n = first+1;  /* to return 1 result */
  }
  else {  /* ensure stack space for all results and for auxlib's buffer */
    luaL_checkstack(L, nargs+LUA_MINSTACK, "too many arguments");
    success = 1;
    for (n = first; nargs-- && success; n++) {
      if (lua_type(L, n) == LUA_TNUMBER) {
        size_t l = (size_t)lua_tointeger(L, n);
        success = (l == 0) ? test_eof(L, f) : read_chars(L, f, l);
      }
      else {
        const char *p = lua_tostring(L, n);
        luaL_argcheck(L, p && p[0] == '*', n, "invalid option");
        switch (p[1]) {
          case 'n':  /* number */
            success = read_number(L, f);
            break;
          case 'l':  /* line */
            success = _read_line(L, f);
            break;
          case 'a':  /* file */
            read_chars(L, f, ~((size_t)0));  /* read MAX_SIZE_T chars */
            success = 1; /* always success */
            break;
          default:
            return luaL_argerror(L, n, "invalid format");
        }
      }
    }
  }
  if (!success) {
    lua_pop(L, 1);  /* remove last result */
    lua_pushnil(L);  /* push nil instead */
  }
  return n - first;
}


static int io_read (lua_State *L) {
  return g_read(L, getiofile(L, IO_INPUT), 1);
}


static int f_read (lua_State *L) {
  return g_read(L, tofile(L), 2);
}


static int io_readline (lua_State *L) {
  int *f = (int *) lua_touserdata(L, lua_upvalueindex(1));
  int sucess;
  if (*f < 0)  /* file is already closed? */
    luaL_error(L, "file is already closed");
  sucess = _read_line(L, f);
  if (sucess) return 1;
  else {  /* EOF */
    if (lua_toboolean(L, lua_upvalueindex(2))) {  /* generator created file? */
      lua_settop(L, 0);
      lua_pushvalue(L, lua_upvalueindex(1));
      aux_close(L);  /* close it */
    }
    return 0;
  }
}

/* }====================================================== */


static int g_write (lua_State *L, int *f, int arg) {
  int nargs = lua_gettop(L) - 1;
  int status = 1;
  for (; nargs--; arg++) {
    if (lua_type(L, arg) == LUA_TNUMBER) {
      /* optimization: could be done exactly as for strings */
      status = status &&
          rb->fdprintf(*f, LUA_NUMBER_FMT, lua_tonumber(L, arg)) > 0;
    }
    else {
      size_t l;
      const char *s = luaL_checklstring(L, arg, &l);
      status = status && (rb->write(*f, s, l) == (ssize_t)l);
    }
  }
  return pushresult(L, status, NULL);
}


static int io_write (lua_State *L) {
  return g_write(L, getiofile(L, IO_OUTPUT), 1);
}


static int f_write (lua_State *L) {
  return g_write(L, tofile(L), 2);
}


static int f_seek (lua_State *L) {
  static const int mode[] = {SEEK_SET, SEEK_CUR, SEEK_END};
  static const char *const modenames[] = {"set", "cur", "end", NULL};
  int f = *tofile(L);
  int op = luaL_checkoption(L, 2, "cur", modenames);
  long offset = luaL_optlong(L, 3, 0);
  op = rb->lseek(f, offset, mode[op]);
  if (op)
    return pushresult(L, 0, NULL);  /* error */
  else {
    lua_pushinteger(L, rb->lseek(f, 0, SEEK_CUR));
    return 1;
  }
}


static const luaL_Reg iolib[] = {
  {"close", io_close},
  {"input", io_input},
  {"lines", io_lines},
  {"open", io_open},
  {"output", io_output},
  {"read", io_read},
  {"type", io_type},
  {"write", io_write},
  {NULL, NULL}
};


static const luaL_Reg flib[] = {
  {"close", io_close},
  {"lines", f_lines},
  {"read", f_read},
  {"seek", f_seek},
  {"write", f_write},
  {"__gc", io_gc},
  {"__tostring", io_tostring},
  {NULL, NULL}
};


static void createmeta (lua_State *L) {
  luaL_newmetatable(L, LUA_FILEHANDLE);  /* create metatable for file handles */
  lua_pushvalue(L, -1);  /* push metatable */
  lua_setfield(L, -2, "__index");  /* metatable.__index = metatable */
  luaL_register(L, NULL, flib);  /* file methods */
}


LUALIB_API int luaopen_io (lua_State *L) {
  createmeta(L);
  lua_replace(L, LUA_ENVIRONINDEX);
  /* open library */
  luaL_register(L, LUA_IOLIBNAME, iolib);
  /* create (and set) default files */
  lua_pop(L, 1);  /* pop environment for default files */
  return 1;
}
