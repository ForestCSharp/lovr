#include "data/rasterizer.h"
#include "types.h"
#include "lib/map/map.h"
#include <stdint.h>
#include <stdbool.h>

#pragma once

struct Rasterizer;
struct Texture;

typedef map_t(Glyph) map_glyph_t;

typedef enum {
  ALIGN_LEFT,
  ALIGN_CENTER,
  ALIGN_RIGHT
} HorizontalAlign;

typedef enum {
  ALIGN_TOP,
  ALIGN_MIDDLE,
  ALIGN_BOTTOM
} VerticalAlign;

typedef struct {
  u32 x;
  u32 y;
  u32 width;
  u32 height;
  u32 rowHeight;
  u32 padding;
  map_glyph_t glyphs;
} FontAtlas;

typedef struct Font {
  Ref ref;
  struct Rasterizer* rasterizer;
  struct Texture* texture;
  FontAtlas atlas;
  map_int_t kerning;
  f32 lineHeight;
  f32 pixelDensity;
  bool flip;
} Font;

Font* lovrFontInit(Font* font, struct Rasterizer* rasterizer);
#define lovrFontCreate(...) lovrFontInit(lovrAlloc(Font), __VA_ARGS__)
void lovrFontDestroy(void* ref);
struct Rasterizer* lovrFontGetRasterizer(Font* font);
void lovrFontRender(Font* font, const char* str, size_t length, float wrap, HorizontalAlign halign, float* vertices, uint16_t* indices, uint16_t baseVertex);
void lovrFontMeasure(Font* font, const char* string, size_t length, float wrap, float* width, uint32_t* lineCount, uint32_t* glyphCount);
float lovrFontGetHeight(Font* font);
float lovrFontGetAscent(Font* font);
float lovrFontGetDescent(Font* font);
float lovrFontGetBaseline(Font* font);
float lovrFontGetLineHeight(Font* font);
void lovrFontSetLineHeight(Font* font, float lineHeight);
bool lovrFontIsFlipEnabled(Font* font);
void lovrFontSetFlipEnabled(Font* font, bool flip);
int lovrFontGetKerning(Font* font, unsigned int a, unsigned int b);
float lovrFontGetPixelDensity(Font* font);
void lovrFontSetPixelDensity(Font* font, float pixelDensity);
Glyph* lovrFontGetGlyph(Font* font, uint32_t codepoint);
void lovrFontAddGlyph(Font* font, Glyph* glyph);
void lovrFontExpandTexture(Font* font);
void lovrFontCreateTexture(Font* font);
