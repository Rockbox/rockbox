  #define lua_lock(L) LuaLock(L)
  #define lua_unlock(L) LuaUnlock(L)

  void LuaLock(lua_State * L);
  void LuaUnlock(lua_State * L);
