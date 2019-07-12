#include "plugin.h"
#include "lstate.h"
#include LUA_USER_H


static struct lua_OOM l_oom = {0};

int set_lua_OOM(lua_State * L)
{
    l_oom.L = L;
    l_oom.count++;
    return 0;
}

struct lua_OOM *get_lua_OOM(void)
{
    return &l_oom;
}
