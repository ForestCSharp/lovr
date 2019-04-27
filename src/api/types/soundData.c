#include "api.h"
#include "data/soundData.h"

static int l_lovrSoundDataGetBitDepth(lua_State* L) {
  SoundData* soundData = luax_checktype(L, 1, SoundData);
  lua_pushinteger(L, soundData->bitDepth);
  return 1;
}

static int l_lovrSoundDataGetChannelCount(lua_State* L) {
  SoundData* soundData = luax_checktype(L, 1, SoundData);
  lua_pushinteger(L, soundData->channelCount);
  return 1;
}

static int l_lovrSoundDataGetDuration(lua_State* L) {
  SoundData* soundData = luax_checktype(L, 1, SoundData);
  lua_pushnumber(L, soundData->samples / (f32) soundData->sampleRate);
  return 1;
}

static int l_lovrSoundDataGetSample(lua_State* L) {
  SoundData* soundData = luax_checktype(L, 1, SoundData);
  usize index = luax_checku32(L, 2);
  lua_pushnumber(L, lovrSoundDataGetSample(soundData, index));
  return 1;
}

static int l_lovrSoundDataGetSampleCount(lua_State* L) {
  SoundData* soundData = luax_checktype(L, 1, SoundData);
  lua_pushinteger(L, soundData->samples);
  return 1;
}

static int l_lovrSoundDataGetSampleRate(lua_State* L) {
  SoundData* soundData = luax_checktype(L, 1, SoundData);
  lua_pushinteger(L, soundData->sampleRate);
  return 1;
}

static int l_lovrSoundDataSetSample(lua_State* L) {
  SoundData* soundData = luax_checktype(L, 1, SoundData);
  usize index = luax_checku32(L, 2);
  f32 value = luax_checkfloat(L, 3);
  lovrSoundDataSetSample(soundData, index, value);
  return 0;
}

const luaL_Reg lovrSoundData[] = {
  { "getBitDepth", l_lovrSoundDataGetBitDepth },
  { "getChannelCount", l_lovrSoundDataGetChannelCount },
  { "getDuration", l_lovrSoundDataGetDuration },
  { "getSample", l_lovrSoundDataGetSample },
  { "getSampleCount", l_lovrSoundDataGetSampleCount },
  { "getSampleRate", l_lovrSoundDataGetSampleRate },
  { "setSample", l_lovrSoundDataSetSample },
  { NULL, NULL }
};
