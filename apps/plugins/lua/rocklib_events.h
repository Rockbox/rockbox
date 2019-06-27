#define LUA_ROCKEVENTSNAME "rockevents"
int luaopen_rockevents (lua_State *L);

/* in rockaux.c */
int get_plugin_action(int timeout, bool with_remote);
