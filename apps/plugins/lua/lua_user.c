#include "plugin.h"
#include "lua.h"
#include "lua_user.h"

/* (Ab)using the lua_unlock call to allow yielding in lua */

void LuaLock(lua_State * L)
{
    (void) L;
}

void LuaUnlock(lua_State * L)
{
    /* safe to yield to other threads here */
    (void) L;
    static unsigned long last_yield = 0;
    if  (TIME_AFTER(*rb->current_tick, last_yield + (HZ / 20)))
    {
        rb->yield();
        last_yield = *rb->current_tick;
    }
}
