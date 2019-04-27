#include "data/modelData.h"
#include "data/blob.h"
#include "data/textureData.h"
#include "filesystem/filesystem.h"
#include "lib/err.h"
#include "lib/maf.h"
#include "lib/map/map.h"
#include "lib/vec/vec.h"
#include <stdio.h>
#include <ctype.h>

typedef vec_t(ModelMaterial) vec_material_t;

typedef struct {
  i32 material;
  i32 start;
  i32 count;
} objGroup;

typedef vec_t(objGroup) vec_group_t;

#define STARTS_WITH(a, b) !strncmp(a, b, strlen(b))

static void parseMtl(char* path, vec_void_t* textures, vec_material_t* materials, map_int_t* names, char* base) {
  size_t length = 0;
  char* data = lovrFilesystemRead(path, -1, &length);
  lovrAssert(data && length > 0, "Unable to read mtl from '%s'", path);
  char* s = data;

  while (length > 0) {
    i32 lineLength = 0;

    if (STARTS_WITH(s, "newmtl ")) {
      char name[128];
      bool hasName = sscanf(s + 7, "%s\n%n", name, &lineLength);
      lovrAssert(hasName, "Bad OBJ: Expected a material name");
      map_set(names, name, materials->length);
      vec_push(materials, ((ModelMaterial) {
        .scalars[SCALAR_METALNESS] = 1.f,
        .scalars[SCALAR_ROUGHNESS] = 1.f,
        .colors[COLOR_DIFFUSE] = { 1.f, 1.f, 1.f, 1.f },
        .colors[COLOR_EMISSIVE] = { 0.f, 0.f, 0.f, 0.f }
      }));
      memset(&vec_last(materials).textures, 0xff, MAX_MATERIAL_TEXTURES * sizeof(int));
    } else if (STARTS_WITH(s, "Kd")) {
      f32 r, g, b;
      int count = sscanf(s + 2, "%f %f %f\n%n", &r, &g, &b, &lineLength);
      lovrAssert(count == 3, "Bad OBJ: Expected 3 components for diffuse color");
      ModelMaterial* material = &vec_last(materials);
      material->colors[COLOR_DIFFUSE] = (Color) { r, g, b, 1.f };
    } else if (STARTS_WITH(s, "map_Kd")) {

      // Read file
      char filename[128];
      bool hasFilename = sscanf(s + 7, "%s\n%n", filename, &lineLength);
      lovrAssert(hasFilename, "Bad OBJ: Expected a texture filename");
      char path[1024];
      snprintf(path, 1023, "%s%s", base, filename);
      usize size = 0;
      void* data = lovrFilesystemRead(path, -1, &size);
      lovrAssert(data && size > 0, "Unable to read texture from %s", path);
      Blob* blob = lovrBlobCreate(data, size, NULL);

      // Load texture, assign to material
      TextureData* texture = lovrTextureDataCreateFromBlob(blob, true);
      lovrAssert(materials->length > 0, "Tried to set a material property without declaring a material first");
      ModelMaterial* material = &vec_last(materials);
      material->textures[TEXTURE_DIFFUSE] = textures->length;
      material->filters[TEXTURE_DIFFUSE].mode = FILTER_TRILINEAR;
      material->wraps[TEXTURE_DIFFUSE] = (TextureWrap) { .s = WRAP_REPEAT, .t = WRAP_REPEAT };
      vec_push(textures, texture);
      lovrRelease(Blob, blob);
    } else {
      char* newline = memchr(s, '\n', length);
      lineLength = newline - s + 1;
    }

    s += lineLength;
    length -= lineLength;
    while (length && (*s == ' ' || *s == '\t')) length--, s++;
  }

  free(data);
}

ModelData* lovrModelDataInitObj(ModelData* model, Blob* source) {
  char* data = (char*) source->data;
  usize length = source->size;

  if (!memchr(data, '\n', length)) {
    return NULL;
  }

  vec_group_t groups;
  vec_void_t textures;
  vec_material_t materials;
  map_int_t materialNames;
  vec_float_t vertexBlob;
  vec_int_t indexBlob;
  map_int_t vertexMap;
  vec_float_t positions;
  vec_float_t normals;
  vec_float_t uvs;

  vec_init(&groups);
  vec_init(&textures);
  vec_init(&materials);
  map_init(&materialNames);
  vec_init(&vertexBlob);
  vec_init(&indexBlob);
  map_init(&vertexMap);
  vec_init(&positions);
  vec_init(&normals);
  vec_init(&uvs);

  vec_push(&groups, ((objGroup) { .material = -1 }));

  char base[1024];
  lovrAssert(strlen(source->name) < sizeof(base), "OBJ filename is too long");
  strcpy(base, source->name);
  char* slash = strrchr(base, '/');
  char* root = slash ? (slash + 1) : base;
  *root = '\0';

  while (length > 0) {
    i32 lineLength = 0;

    if (STARTS_WITH(data, "v ")) {
      f32 x, y, z;
      i32 count = sscanf(data + 2, "%f %f %f\n%n", &x, &y, &z, &lineLength);
      lovrAssert(count == 3, "Bad OBJ: Expected 3 coordinates for vertex position");
      vec_pusharr(&positions, ((f32[3]) { x, y, z }), 3);
    } else if (STARTS_WITH(data, "vn ")) {
      f32 x, y, z;
      i32 count = sscanf(data + 3, "%f %f %f\n%n", &x, &y, &z, &lineLength);
      lovrAssert(count == 3, "Bad OBJ: Expected 3 coordinates for vertex normal");
      vec_pusharr(&normals, ((f32[3]) { x, y, z }), 3);
    } else if (STARTS_WITH(data, "vt ")) {
      f32 u, v;
      i32 count = sscanf(data + 3, "%f %f\n%n", &u, &v, &lineLength);
      lovrAssert(count == 2, "Bad OBJ: Expected 2 coordinates for texture coordinate");
      vec_pusharr(&uvs, ((f32[2]) { u, v }), 2);
    } else if (STARTS_WITH(data, "f ")) {
      char* s = data + 2;
      for (u32 i = 0; i < 3; i++) {
        char terminator = i == 2 ? '\n' : ' ';
        char* space = strchr(s, terminator);
        if (space) {
          *space = '\0'; // I'll be back
          i32* index = map_get(&vertexMap, s);
          if (index) {
            vec_push(&indexBlob, *index);
          } else {
            i32 v, vt, vn;
            i32 newIndex = vertexBlob.length / 8;
            vec_push(&indexBlob, newIndex);
            map_set(&vertexMap, s, newIndex);

            // Can be improved
            if (sscanf(s, "%d/%d/%d", &v, &vt, &vn) == 3) {
              vec_pusharr(&vertexBlob, positions.data + 3 * (v - 1), 3);
              vec_pusharr(&vertexBlob, normals.data + 3 * (vn - 1), 3);
              vec_pusharr(&vertexBlob, uvs.data + 2 * (vt - 1), 2);
            } else if (sscanf(s, "%d//%d", &v, &vn) == 2) {
              vec_pusharr(&vertexBlob, positions.data + 3 * (v - 1), 3);
              vec_pusharr(&vertexBlob, normals.data + 3 * (vn - 1), 3);
              vec_pusharr(&vertexBlob, ((float[2]) { 0 }), 2);
            } else if (sscanf(s, "%d/%d", &v, &vt) == 2) {
              vec_pusharr(&vertexBlob, positions.data + 3 * (v - 1), 3);
              vec_pusharr(&vertexBlob, ((float[3]) { 0 }), 3);
              vec_pusharr(&vertexBlob, uvs.data + 2 * (vt - 1), 2);
            } else if (sscanf(s, "%d", &v) == 1) {
              vec_pusharr(&vertexBlob, positions.data + 3 * (v - 1), 3);
              vec_pusharr(&vertexBlob, ((float[5]) { 0 }), 5);
            } else {
              lovrThrow("Bad OBJ: Unknown face format");
            }
          }
          *space = terminator;
          s = space + 1;
        }
      }
      vec_last(&groups).count += 3;
      lineLength = s - data;
    } else if (STARTS_WITH(data, "mtllib ")) {
      char filename[1024];
      bool hasName = sscanf(data + 7, "%1024s\n%n", filename, &lineLength);
      lovrAssert(hasName, "Bad OBJ: Expected filename after mtllib");
      char path[1024];
      snprintf(path, 1023, "%s%s", base, filename);
      parseMtl(path, &textures, &materials, &materialNames, base);
    } else if (STARTS_WITH(data, "usemtl ")) {
      char name[128];
      bool hasName = sscanf(data + 7, "%s\n%n", name, &lineLength);
      i32* material = map_get(&materialNames, name);
      lovrAssert(hasName, "Bad OBJ: Expected a material name");

      // If the last group didn't have any faces, just reuse it, otherwise make a new group
      objGroup* group = &vec_last(&groups);
      if (group->count > 0) {
        i32 start = group->start + group->count; // Don't put this in the compound literal (realloc)
        vec_push(&groups, ((objGroup) {
          .material = material ? *material : -1,
          .start = start,
          .count = 0
        }));
      } else {
        group->material = material ? *material : -1;
      }
    } else {
      char* newline = memchr(data, '\n', length);
      if (!newline) {
        break;
      }
      lineLength = newline - data + 1;
    }

    data += lineLength;
    length -= lineLength;
    while (length && (*data == ' ' || *data == '\t')) length--, data++;
  }

  if (vertexBlob.length == 0 || indexBlob.length == 0) {
    return NULL;
  }

  model->blobCount = 2;
  model->bufferCount = 2;
  model->attributeCount = 3 + groups.length;
  model->primitiveCount = groups.length;
  model->nodeCount = 1;
  model->textureCount = textures.length;
  model->materialCount = materials.length;
  lovrModelDataAllocate(model);

  model->blobs[0] = lovrBlobCreate(vertexBlob.data, vertexBlob.length * sizeof(f32), "obj vertex data");
  model->blobs[1] = lovrBlobCreate(indexBlob.data, indexBlob.length * sizeof(u32), "obj index data");

  model->buffers[0] = (ModelBuffer) {
    .data = model->blobs[0]->data,
    .size = model->blobs[0]->size,
    .stride = 8 * sizeof(f32)
  };

  model->buffers[1] = (ModelBuffer) {
    .data = model->blobs[1]->data,
    .size = model->blobs[1]->size,
    .stride = sizeof(u32)
  };

  memcpy(model->textures, textures.data, model->textureCount * sizeof(TextureData*));
  memcpy(model->materials, materials.data, model->materialCount * sizeof(ModelMaterial));

  model->attributes[0] = (ModelAttribute) {
    .buffer = 0,
    .offset = 0,
    .count = vertexBlob.length / 8,
    .type = F32,
    .components = 3
  };

  model->attributes[1] = (ModelAttribute) {
    .buffer = 0,
    .offset = 3 * sizeof(f32),
    .count = vertexBlob.length / 8,
    .type = F32,
    .components = 3
  };

  model->attributes[2] = (ModelAttribute) {
    .buffer = 0,
    .offset = 6 * sizeof(f32),
    .count = vertexBlob.length / 8,
    .type = F32,
    .components = 2
  };

  for (i32 i = 0; i < groups.length; i++) {
    objGroup* group = &groups.data[i];
    model->attributes[3 + i] = (ModelAttribute) {
      .buffer = 1,
      .offset = group->start * sizeof(i32),
      .count = group->count,
      .type = U32,
      .components = 1
    };
  }

  for (i32 i = 0; i < groups.length; i++) {
    objGroup* group = &groups.data[i];
    model->primitives[i] = (ModelPrimitive) {
      .mode = DRAW_TRIANGLES,
      .attributes = {
        [ATTR_POSITION] = &model->attributes[0],
        [ATTR_NORMAL] = &model->attributes[1],
        [ATTR_TEXCOORD] = &model->attributes[2]
      },
      .indices = &model->attributes[3 + i],
      .material = group->material
    };
  }

  model->nodes[0] = (ModelNode) {
    .transform = MAT4_IDENTITY,
    .primitiveIndex = 0,
    .primitiveCount = groups.length
  };

  vec_deinit(&groups);
  vec_deinit(&textures);
  vec_deinit(&materials);
  map_deinit(&materialNames);
  map_deinit(&vertexMap);
  vec_deinit(&positions);
  vec_deinit(&normals);
  vec_deinit(&uvs);
  return model;
}
