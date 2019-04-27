#include "api.h"
#include "data/textureData.h"

static int l_lovrTextureDataEncode(lua_State* L) {
  TextureData* textureData = luax_checktype(L, 1, TextureData);
  const char* filename = luaL_checkstring(L, 2);
  bool success = lovrTextureDataEncode(textureData, filename);
  lua_pushboolean(L, success);
  return 1;
}

static int l_lovrTextureDataGetWidth(lua_State* L) {
  TextureData* textureData = luax_checktype(L, 1, TextureData);
  lua_pushinteger(L, textureData->width);
  return 1;
}

static int l_lovrTextureDataGetHeight(lua_State* L) {
  TextureData* textureData = luax_checktype(L, 1, TextureData);
  lua_pushinteger(L, textureData->height);
  return 1;
}

static int l_lovrTextureDataGetDimensions(lua_State* L) {
  TextureData* textureData = luax_checktype(L, 1, TextureData);
  lua_pushinteger(L, textureData->width);
  lua_pushinteger(L, textureData->height);
  return 2;
}

static int l_lovrTextureDataGetFormat(lua_State* L) {
  TextureData* textureData = luax_checktype(L, 1, TextureData);
  lua_pushstring(L, TextureFormats[textureData->format]);
  return 1;
}

static int l_lovrTextureDataGetPixel(lua_State* L) {
  TextureData* textureData = luax_checktype(L, 1, TextureData);
  u32 x = luax_checku32(L, 2);
  u32 y = luax_checku32(L, 3);
  Color color = lovrTextureDataGetPixel(textureData, x, y);
  lua_pushnumber(L, color.r);
  lua_pushnumber(L, color.g);
  lua_pushnumber(L, color.b);
  lua_pushnumber(L, color.a);
  return 4;
}

static int l_lovrTextureDataSetPixel(lua_State* L) {
  TextureData* textureData = luax_checktype(L, 1, TextureData);
  u32 x = luax_checku32(L, 2);
  u32 y = luax_checku32(L, 3);
  Color color = {
    luax_optf32(L, 4, 1.f),
    luax_optf32(L, 5, 1.f),
    luax_optf32(L, 6, 1.f),
    luax_optf32(L, 7, 1.f)
  };
  lovrTextureDataSetPixel(textureData, x, y, color);
  return 0;
}

const luaL_Reg lovrTextureData[] = {
  { "encode", l_lovrTextureDataEncode },
  { "getWidth", l_lovrTextureDataGetWidth },
  { "getHeight", l_lovrTextureDataGetHeight },
  { "getDimensions", l_lovrTextureDataGetDimensions },
  { "getFormat", l_lovrTextureDataGetFormat },
  { "getPixel", l_lovrTextureDataGetPixel },
  { "setPixel", l_lovrTextureDataSetPixel },
  { NULL, NULL }
};
