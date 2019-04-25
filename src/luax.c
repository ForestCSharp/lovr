#include "luax.h"
#include "platform.h"
#include "lib/sds/sds.h"
#include <stdlib.h>
#include <math.h>

static int luax_meta__tostring(lua_State* L) {
  lua_getfield(L, -1, "name");
  return 1;
}

static int luax_meta__gc(lua_State* L) {
  lovrGenericRelease(*(Ref**) lua_touserdata(L, 1));
  return 0;
}

static int luax_module__gc(lua_State* L) {
  lua_getfield(L, LUA_REGISTRYINDEX, "_lovrmodules");
  for (int i = luax_len(L, 2); i >= 1; i--) {
    lua_rawgeti(L, 2, i);
    luax_destructor destructor = (luax_destructor) lua_tocfunction(L, -1);
    destructor();
    lua_pop(L, 1);
  }
  return 0;
}

// A version of print that uses lovrLog, for platforms that need it (Android)
int luax_print(lua_State* L) {
  sds str = sdsempty();
  int n = lua_gettop(L);  /* number of arguments */
  int i;

  lua_getglobal(L, "tostring");
  for (i=1; i<=n; i++) {
    const char *s;
    lua_pushvalue(L, -1);  /* function to be called */
    lua_pushvalue(L, i);   /* value to print */
    lua_call(L, 1, 1);
    s = lua_tostring(L, -1);  /* get result */
    if (s == NULL)
      return luaL_error(L, LUA_QL("tostring") " must return a string to "
                           LUA_QL("print"));
    if (i>1) str = sdscat(str, "\t");
    str = sdscat(str, s);
    lua_pop(L, 1);  /* pop result */
  }
  lovrLog("%s", str);

  sdsfree(str);
  return 0;
}

void luax_setmainthread(lua_State *L) {
#if LUA_VERSION_NUM < 502
  lua_pushthread(L);
  lua_rawseti(L, LUA_REGISTRYINDEX, LUA_RIDX_MAINTHREAD);
#endif
}

void luax_atexit(lua_State* L, luax_destructor destructor) {
  lua_getfield(L, LUA_REGISTRYINDEX, "_lovrmodules");

  if (lua_isnil(L, -1)) {
    lua_newtable(L);
    lua_replace(L, -2);

    // Userdata sentinel since tables don't have __gc (yet)
    lua_newuserdata(L, sizeof(void*));
    lua_createtable(L, 0, 1);
    lua_pushcfunction(L, luax_module__gc);
    lua_setfield(L, -2, "__gc");
    lua_setmetatable(L, -2);
    lua_setfield(L, -2, "");

    // Write to the registry
    lua_pushvalue(L, -1);
    lua_setfield(L, LUA_REGISTRYINDEX, "_lovrmodules");
  }

  int length = luax_len(L, -1);
  lua_pushcfunction(L, (lua_CFunction) destructor);
  lua_rawseti(L, -2, length + 1);
  lua_pop(L, 1);
}

void luax_registerloader(lua_State* L, lua_CFunction loader, int index) {
  lua_getglobal(L, "table");
  lua_getfield(L, -1, "insert");
  lua_getglobal(L, "package");
  lua_getfield(L, -1, "loaders");
  lua_remove(L, -2);
  if (lua_istable(L, -1)) {
    lua_pushinteger(L, index);
    lua_pushcfunction(L, loader);
    lua_call(L, 3, 0);
  }
  lua_pop(L, 1);
}

void _luax_registertype(lua_State* L, const char* name, const luaL_Reg* functions) {

  // Push metatable
  luaL_newmetatable(L, name);
  lua_getmetatable(L, -1);

  // m.__index = m
  lua_pushvalue(L, -1);
  lua_setfield(L, -1, "__index");

  // m.__gc = gc
  lua_pushcfunction(L, luax_meta__gc);
  lua_setfield(L, -2, "__gc");

  // m.name = name
  lua_pushstring(L, name);
  lua_setfield(L, -2, "name");

  // m.__tostring
  lua_pushcfunction(L, luax_meta__tostring);
  lua_setfield(L, -2, "__tostring");

  // Register class functions
  if (functions) {
    luaL_register(L, NULL, functions);
  }

  // Pop metatable
  lua_pop(L, 1);
}

void _luax_extendtype(lua_State* L, const char* name, const luaL_Reg* baseFunctions, const luaL_Reg* functions) {
  _luax_registertype(L, name, functions);
  luaL_getmetatable(L, name);
  luaL_register(L, NULL, baseFunctions);
  lua_pop(L, 1);
}

void* _luax_totype(lua_State* L, int index, Type type) {
  void** p = lua_touserdata(L, index);

  if (p) {
    Ref* object = *(Ref**) p;
    if (object->type == type || lovrTypeInfo[object->type].super == type) {
      return object;
    }
  }

  return NULL;
}

void* _luax_checktype(lua_State* L, int index, Type type, const char* debug) {
  void* object = _luax_totype(L, index, type);

  if (!object) {
    luaL_typerror(L, index, debug);
  }

  return object;
}

// Registers the userdata on the top of the stack in the registry.
void luax_pushobject(lua_State* L, void* object) {
  if (!object) {
    lua_pushnil(L);
    return;
  }

  lua_getfield(L, LUA_REGISTRYINDEX, "_lovrobjects");

  // Create the registry if it doesn't exist yet
  if (lua_isnil(L, -1)) {
    lua_newtable(L);
    lua_replace(L, -2);

    // Create the metatable
    lua_newtable(L);

    // __mode = v
    lua_pushliteral(L, "v");
    lua_setfield(L, -2, "__mode");

    // Set the metatable
    lua_setmetatable(L, -2);

    // Write the table to the registry
    lua_pushvalue(L, -1);
    lua_setfield(L, LUA_REGISTRYINDEX, "_lovrobjects");
  }

  lua_pushlightuserdata(L, object);
  lua_gettable(L, -2);

  if (lua_isnil(L, -1)) {
    lua_pop(L, 1);
  } else {
    lua_remove(L, -2);
    return;
  }

  // Allocate userdata
  void** u = (void**) lua_newuserdata(L, sizeof(void**));
  luaL_getmetatable(L, lovrTypeInfo[((Ref*) object)->type].name);
  lua_setmetatable(L, -2);
  lovrRetain(object);
  *u = object;

  // Write to registry and remove registry, leaving userdata on stack
  lua_pushlightuserdata(L, object);
  lua_pushvalue(L, -2);
  lua_settable(L, -4);
  lua_remove(L, -2);
}

u32 luax_checku32(lua_State* L, int index) {
  lua_Number n = lua_tonumber(L, index);
  if (n != 0. || lua_isnumber(L, index)) {
    if (!(n >= 0. && n < UINT32_MAX)) {
      const char* message = lua_pushfstring(L, "number between %d and %f expected, got %f", 0, nextafter(UINT32_MAX - 1, 0.), n);
      luaL_argerror(L, index, message);
      return 0;
    }
    return (u32) n;
  }
  luaL_typerror(L, index, lua_typename(L, LUA_TNUMBER));
  return 0;
}

void luax_vthrow(lua_State* L, const char* format, va_list args) {
  lua_pushvfstring(L, format, args);
  lua_error(L);
}

// An implementation of luaL_traceback for Lua 5.1
void luax_traceback(lua_State* L, lua_State* T, const char* message, int level) {
  if (!lua_checkstack(L, 5)) {
    return;
  }
  lua_getglobal(L, "debug");
  if (!lua_istable(L, -1)) {
    lua_pop(L, 1);
    return;
  }
  lua_getfield(L, -1, "traceback");
  if (!lua_isfunction(L, -1)) {
    lua_pop(L, 2);
    return;
  }
  lua_remove(L, -2); // Pop debug object
  lua_pushthread(T);
  lua_pushstring(L, message);
  lua_pushinteger(L, level);
  lua_call(L, 3, 1);
}

int luax_getstack(lua_State *L) {
  luax_traceback(L, L, lua_tostring(L, 1), 2);
  return 1;
}

void luax_pushconf(lua_State* L) {
  lua_getfield(L, LUA_REGISTRYINDEX, "_lovrconf");
}

int luax_setconf(lua_State* L) {
  luax_pushconf(L);
  lovrAssert(lua_isnil(L, -1), "Unable to set lovr.conf multiple times");
  lua_pop(L, 1);
  lua_setfield(L, LUA_REGISTRYINDEX, "_lovrconf");
  return 0;
}

void luax_readcolor(lua_State* L, int index, Color* color) {
  color->r = color->g = color->b = color->a = 1.f;

  if (lua_istable(L, 1)) {
    for (int i = 1; i <= 4; i++) {
      lua_rawgeti(L, 1, i);
    }
    color->r = luax_checkfloat(L, -4);
    color->g = luax_checkfloat(L, -3);
    color->b = luax_checkfloat(L, -2);
    color->a = luax_optfloat(L, -1, 1.);
    lua_pop(L, 4);
  } else if (lua_gettop(L) >= index + 2) {
    color->r = luax_checkfloat(L, index);
    color->g = luax_checkfloat(L, index + 1);
    color->b = luax_checkfloat(L, index + 2);
    color->a = luax_optfloat(L, index + 3, 1.);
  } else if (lua_gettop(L) == index) {
    uint32_t x = luaL_checkinteger(L, index);
    color->r = ((x >> 16) & 0xff) / 255.f;
    color->g = ((x >> 8) & 0xff) / 255.f;
    color->b = ((x >> 0) & 0xff) / 255.f;
    color->a = 1.f;
  } else {
    luaL_error(L, "Invalid color, expected a hexcode, 3 numbers, 4 numbers, or a table");
  }
}
