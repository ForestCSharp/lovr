#include "data/modelData.h"
#include "data/blob.h"
#include "data/textureData.h"
#include "lib/err.h"

ModelData* lovrModelDataInit(ModelData* model, Blob* source) {
  if (lovrModelDataInitGltf(model, source)) {
    return model;
  } else if (lovrModelDataInitObj(model, source)) {
    return model;
  }

  lovrThrow("Unable to load model from '%s'", source->name);
  return NULL;
}

void lovrModelDataDestroy(void* ref) {
  ModelData* model = ref;
  for (u32 i = 0; i < model->blobCount; i++) {
    lovrRelease(Blob, model->blobs[i]);
  }
  for (u32 i = 0; i < model->textureCount; i++) {
    lovrRelease(TextureData, model->textures[i]);
  }
  free(model->data);
}

// Note: this code is a scary optimization
void lovrModelDataAllocate(ModelData* model) {
  usize totalSize = 0;
  usize sizes[13];
  totalSize += sizes[0] = model->blobCount * sizeof(Blob*);
  totalSize += sizes[1] = model->bufferCount * sizeof(ModelBuffer);
  totalSize += sizes[2] = model->textureCount * sizeof(TextureData*);
  totalSize += sizes[3] = model->materialCount * sizeof(ModelMaterial);
  totalSize += sizes[4] = model->attributeCount * sizeof(ModelAttribute);
  totalSize += sizes[5] = model->primitiveCount * sizeof(ModelPrimitive);
  totalSize += sizes[6] = model->animationCount * sizeof(ModelAnimation);
  totalSize += sizes[7] = model->skinCount * sizeof(ModelSkin);
  totalSize += sizes[8] = model->nodeCount * sizeof(ModelNode);
  totalSize += sizes[9] = model->channelCount * sizeof(ModelAnimationChannel);
  totalSize += sizes[10] = model->childCount * sizeof(u32);
  totalSize += sizes[11] = model->jointCount * sizeof(u32);
  totalSize += sizes[12] = model->charCount * sizeof(char);

  usize offset = 0;
  char* p = model->data = calloc(1, totalSize);
  lovrAssert(model->data, "Out of memory");
  model->blobs = (Blob**) (p + offset), offset += sizes[0];
  model->buffers = (ModelBuffer*) (p + offset), offset += sizes[1];
  model->textures = (TextureData**) (p + offset), offset += sizes[2];
  model->materials = (ModelMaterial*) (p + offset), offset += sizes[3];
  model->attributes = (ModelAttribute*) (p + offset), offset += sizes[4];
  model->primitives = (ModelPrimitive*) (p + offset), offset += sizes[5];
  model->animations = (ModelAnimation*) (p + offset), offset += sizes[6];
  model->skins = (ModelSkin*) (p + offset), offset += sizes[7];
  model->nodes = (ModelNode*) (p + offset), offset += sizes[8];
  model->channels = (ModelAnimationChannel*) (p + offset), offset += sizes[9];
  model->children = (u32*) (p + offset), offset += sizes[10];
  model->joints = (u32*) (p + offset), offset += sizes[11];
  model->chars = (char*) (p + offset), offset += sizes[12];
}
