#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

void lovrInit(lua_State* L, int argc, char** argv);
void lovrDestroy(int exitCode);
void lovrRun(lua_State* L);
