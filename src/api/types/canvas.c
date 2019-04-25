#include "api.h"
#include "api/graphics.h"
#include "graphics/canvas.h"
#include "graphics/graphics.h"
#include "lib/err.h"

Texture* luax_checktexture(lua_State* L, int index) {
  Canvas* canvas = luax_totype(L, index, Canvas);
  if (canvas) {
    const Attachment* attachment = lovrCanvasGetAttachments(canvas, NULL);
    return attachment->texture;
  } else {
    return luax_checktype(L, index, Texture);
  }
}

static int luax_checkattachment(lua_State* L, int index, Attachment* attachment) {
  if (lua_istable(L, index)) {
    lua_rawgeti(L, index, 1);
    attachment->texture = luax_checktype(L, -1, Texture);
    lua_pop(L, 1);

    lua_rawgeti(L, index, 2);
    attachment->slice = luax_optu32(L, -1, 1) - 1; // TODO handle 0 better
    lua_pop(L, 1);

    lua_rawgeti(L, index, 3);
    attachment->level = luax_optmipmap(L, -1, attachment->texture);
    lua_pop(L, 1);

    index++;
  } else {
    attachment->texture = luax_checktype(L, index++, Texture);
    attachment->slice = lua_type(L, index) == LUA_TNUMBER ? luax_checku32(L, index++) : 0; // TODO handle 0 better
    attachment->level = lua_type(L, index) == LUA_TNUMBER ? luax_optmipmap(L, index++, attachment->texture) : 0;
  }
  return index;
}

void luax_readattachments(lua_State* L, int index, Attachment* attachments, u32* count) {
  bool table = lua_istable(L, index);
  int top = table ? -1 : lua_gettop(L);
  usize n;

  if (table) {
    n = lua_objlen(L, index);
    n = MIN(n, 3 * MAX_CANVAS_ATTACHMENTS);
    for (usize i = 0; i < n; i++) {
      lua_rawgeti(L, index, i + 1);
    }
    index = -n;
  }

  for (*count = 0; *count < MAX_CANVAS_ATTACHMENTS && index <= top; (*count)++) {
    index = luax_checkattachment(L, index, attachments + *count);
  }

  if (table) {
    lua_pop(L, n);
  }
}

static int l_lovrCanvasNewTextureData(lua_State* L) {
  Canvas* canvas = luax_checktype(L, 1, Canvas);
  u32 index = luax_optu32(L, 2, 1);
  u32 count;
  lovrCanvasGetAttachments(canvas, &count);
  lovrAssert(index - 1 < count, "Can not create a TextureData from Texture #%d of Canvas (it only has %d texture%s)", index, count, count > 1 ? "s" : "");
  TextureData* textureData = lovrCanvasNewTextureData(canvas, index);
  luax_pushobject(L, textureData);
  lovrRelease(TextureData, textureData);
  return 1;
}

static int l_lovrCanvasRenderTo(lua_State* L) {
  Canvas* canvas = luax_checktype(L, 1, Canvas);
  luaL_checktype(L, 2, LUA_TFUNCTION);
  int argumentCount = lua_gettop(L) - 2;
  Canvas* old = lovrGraphicsGetCanvas();
  lovrGraphicsSetCanvas(canvas);
  lua_call(L, argumentCount, 0);
  lovrGraphicsSetCanvas(old);
  return 0;
}

static int l_lovrCanvasGetTexture(lua_State* L) {
  Canvas* canvas = luax_checktype(L, 1, Canvas);
  u32 count;
  const Attachment* attachments = lovrCanvasGetAttachments(canvas, &count);
  for (u32 i = 0; i < count; i++) {
    luax_pushobject(L, attachments[i].texture);
  }
  return count;
}

static int l_lovrCanvasSetTexture(lua_State* L) {
  Canvas* canvas = luax_checktype(L, 1, Canvas);
  Attachment attachments[MAX_CANVAS_ATTACHMENTS];
  u32 count;
  luax_readattachments(L, 2, attachments, &count);
  lovrCanvasSetAttachments(canvas, attachments, count);
  return 0;
}

static int l_lovrCanvasGetWidth(lua_State* L) {
  Canvas* canvas = luax_checktype(L, 1, Canvas);
  lua_pushinteger(L, lovrCanvasGetWidth(canvas));
  return 1;
}

static int l_lovrCanvasGetHeight(lua_State* L) {
  Canvas* canvas = luax_checktype(L, 1, Canvas);
  lua_pushinteger(L, lovrCanvasGetHeight(canvas));
  return 1;
}

static int l_lovrCanvasGetDimensions(lua_State* L) {
  Canvas* canvas = luax_checktype(L, 1, Canvas);
  lua_pushinteger(L, lovrCanvasGetWidth(canvas));
  lua_pushinteger(L, lovrCanvasGetHeight(canvas));
  return 2;
}

static int l_lovrCanvasGetDepthTexture(lua_State* L) {
  Canvas* canvas = luax_checktype(L, 1, Canvas);
  Texture* texture = lovrCanvasGetDepthTexture(canvas);
  luax_pushobject(L, texture);
  return 1;
}

static int l_lovrCanvasGetMSAA(lua_State* L) {
  Canvas* canvas = luax_checktype(L, 1, Canvas);
  u32 msaa = lovrCanvasGetMSAA(canvas);
  lua_pushinteger(L, msaa);
  return 1;
}

static int l_lovrCanvasIsStereo(lua_State* L) {
  Canvas* canvas = luax_checktype(L, 1, Canvas);
  bool stereo = lovrCanvasIsStereo(canvas);
  lua_pushboolean(L, stereo);
  return 1;
}

const luaL_Reg lovrCanvas[] = {
  { "newTextureData", l_lovrCanvasNewTextureData },
  { "renderTo", l_lovrCanvasRenderTo },
  { "getTexture", l_lovrCanvasGetTexture },
  { "setTexture", l_lovrCanvasSetTexture },
  { "getWidth", l_lovrCanvasGetWidth },
  { "getHeight", l_lovrCanvasGetHeight },
  { "getDimensions", l_lovrCanvasGetDimensions },
  { "getDepthTexture", l_lovrCanvasGetDepthTexture },
  { "getMSAA", l_lovrCanvasGetMSAA },
  { "isStereo", l_lovrCanvasIsStereo },
  { NULL, NULL }
};
