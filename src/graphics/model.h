#include "util.h"
#include "lib/maf.h"
#include "types.h"

#pragma once

struct Animator;
struct Buffer;
struct Material;
struct Mesh;
struct ModelData;
struct Texture;

typedef struct {
  Ref ref;
  struct ModelData* data;
  struct Animator* animator;
  struct Buffer** buffers;
  struct Mesh** meshes;
  struct Texture** textures;
  struct Material** materials;
  struct Material* userMaterial;
  f32* globalNodeTransforms;
} Model;

Model* lovrModelInit(Model* model, struct ModelData* data);
#define lovrModelCreate(...) lovrModelInit(lovrAlloc(Model), __VA_ARGS__)
void lovrModelDestroy(void* ref);
void lovrModelDraw(Model* model, mat4 transform, u32 instances);
struct Animator* lovrModelGetAnimator(Model* model);
void lovrModelSetAnimator(Model* model, struct Animator* animator);
struct Material* lovrModelGetMaterial(Model* model);
void lovrModelSetMaterial(Model* model, struct Material* material);
void lovrModelGetAABB(Model* model, f32 aabb[6]);
