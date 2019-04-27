#include "api.h"
#include "api/graphics.h"
#include "data/textureData.h"
#include "graphics/texture.h"
#include "lib/err.h"

u32 luax_optmipmap(lua_State* L, int index, Texture* texture) {
  u32 mipmap = luaL_optinteger(L, index, 1);
  lovrAssert(mipmap >= 1 && mipmap <= lovrTextureGetMipmapCount(texture), "Invalid mipmap %d\n", mipmap);
  return mipmap - 1;
}

static int l_lovrTextureGetDepth(lua_State* L) {
  Texture* texture = luax_checktype(L, 1, Texture);
  lua_pushnumber(L, lovrTextureGetDepth(texture, luax_optmipmap(L, 2, texture)));
  return 1;
}

static int l_lovrTextureGetDimensions(lua_State* L) {
  Texture* texture = luax_checktype(L, 1, Texture);
  u32 mipmap = luax_optmipmap(L, 2, texture);
  lua_pushinteger(L, lovrTextureGetWidth(texture, mipmap));
  lua_pushinteger(L, lovrTextureGetHeight(texture, mipmap));
  if (lovrTextureGetType(texture) != TEXTURE_2D) {
    lua_pushinteger(L, lovrTextureGetDepth(texture, mipmap));
    return 3;
  }
  return 2;
}

static int l_lovrTextureGetFilter(lua_State* L) {
  Texture* texture = luax_checktype(L, 1, Texture);
  TextureFilter filter = lovrTextureGetFilter(texture);
  lua_pushstring(L, FilterModes[filter.mode]);
  if (filter.mode == FILTER_ANISOTROPIC) {
    lua_pushnumber(L, filter.anisotropy);
    return 2;
  }
  return 1;
}

static int l_lovrTextureGetFormat(lua_State* L) {
  Texture* texture = luax_checktype(L, 1, Texture);
  lua_pushstring(L, TextureFormats[lovrTextureGetFormat(texture)]);
  return 1;
}

static int l_lovrTextureGetHeight(lua_State* L) {
  Texture* texture = luax_checktype(L, 1, Texture);
  lua_pushnumber(L, lovrTextureGetHeight(texture, luax_optmipmap(L, 2, texture)));
  return 1;
}

static int l_lovrTextureGetMipmapCount(lua_State* L) {
  Texture* texture = luax_checktype(L, 1, Texture);
  lua_pushinteger(L, lovrTextureGetMipmapCount(texture));
  return 1;
}

static int l_lovrTextureGetType(lua_State* L) {
  Texture* texture = luax_checktype(L, 1, Texture);
  lua_pushstring(L, TextureTypes[lovrTextureGetType(texture)]);
  return 1;
}

static int l_lovrTextureGetWidth(lua_State* L) {
  Texture* texture = luax_checktype(L, 1, Texture);
  lua_pushnumber(L, lovrTextureGetWidth(texture, luax_optmipmap(L, 2, texture)));
  return 1;
}

static int l_lovrTextureGetWrap(lua_State* L) {
  Texture* texture = luax_checktype(L, 1, Texture);
  TextureWrap wrap = lovrTextureGetWrap(texture);
  lua_pushstring(L, WrapModes[wrap.s]);
  lua_pushstring(L, WrapModes[wrap.t]);
  if (lovrTextureGetType(texture) == TEXTURE_CUBE) {
    lua_pushstring(L, WrapModes[wrap.r]);
    return 3;
  }
  return 2;
}

static int l_lovrTextureReplacePixels(lua_State* L) {
  Texture* texture = luax_checktype(L, 1, Texture);
  TextureData* textureData = luax_checktype(L, 2, TextureData);
  u32 x = luax_optu32(L, 3, 0);
  u32 y = luax_optu32(L, 4, 0);
  u32 slice = luax_optu32(L, 5, 1);
  u32 mipmap = luax_optu32(L, 6, 1);
  lovrTextureReplacePixels(texture, textureData, x, y, slice - 1, mipmap - 1);
  return 0;
}

static int l_lovrTextureSetFilter(lua_State* L) {
  Texture* texture = luax_checktype(L, 1, Texture);
  FilterMode mode = luaL_checkoption(L, 2, NULL, FilterModes);
  f32 anisotropy = luax_optfloat(L, 3, 1.f);
  TextureFilter filter = { .mode = mode, .anisotropy = anisotropy };
  lovrTextureSetFilter(texture, filter);
  return 0;
}

static int l_lovrTextureSetWrap(lua_State* L) {
  Texture* texture = luax_checktype(L, 1, Texture);
  TextureWrap wrap;
  wrap.s = luaL_checkoption(L, 2, NULL, WrapModes);
  wrap.t = luaL_checkoption(L, 3, luaL_checkstring(L, 2), WrapModes);
  wrap.r = luaL_checkoption(L, 4, luaL_checkstring(L, 2), WrapModes);
  lovrTextureSetWrap(texture, wrap);
  return 0;
}

const luaL_Reg lovrTexture[] = {
  { "getDepth", l_lovrTextureGetDepth },
  { "getDimensions", l_lovrTextureGetDimensions },
  { "getFilter", l_lovrTextureGetFilter },
  { "getFormat", l_lovrTextureGetFormat },
  { "getHeight", l_lovrTextureGetHeight },
  { "getMipmapCount", l_lovrTextureGetMipmapCount },
  { "getType", l_lovrTextureGetType },
  { "getWidth", l_lovrTextureGetWidth },
  { "getWrap", l_lovrTextureGetWrap },
  { "replacePixels", l_lovrTextureReplacePixels },
  { "setFilter", l_lovrTextureSetFilter },
  { "setWrap", l_lovrTextureSetWrap },
  { NULL, NULL }
};
