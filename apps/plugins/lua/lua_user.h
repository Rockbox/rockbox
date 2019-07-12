#ifndef _LUA_USER_H_
#define _LUA_USER_H_

#define LUA_OOM(L) set_lua_OOM(L)

struct lua_OOM {
    lua_State * L;
    int         count;
};

int set_lua_OOM(lua_State * L);

struct lua_OOM* get_lua_OOM(void);
#endif
