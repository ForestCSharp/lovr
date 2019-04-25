#include "types.h"
#include "util.h"
#include "lib/err.h"
#include <stdlib.h>

void lovrAnimatorDestroy(void*);
void lovrAudioStreamDestroy(void*);
void lovrBlobDestroy(void*);
void lovrBufferDestroy(void*);
void lovrCanvasDestroy(void*);
void lovrChannelDestroy(void*);
void lovrColliderDestroy(void*);
void lovrControllerDestroy(void*);
void lovrCurveDestroy(void*);
void lovrFileDestroy(void*);
void lovrFontDestroy(void*);
void lovrJointDestroy(void*);
void lovrMaterialDestroy(void*);
void lovrMeshDestroy(void*);
void lovrMicrophoneDestroy(void*);
void lovrModelDestroy(void*);
void lovrModelDataDestroy(void*);
void lovrPoolDestroy(void*);
void lovrRandomGeneratorDestroy(void*);
void lovrRasterizerDestroy(void*);
void lovrShaderDestroy(void*);
void lovrShaderBlockDestroy(void*);
void lovrShapeDestroy(void*);
void lovrSoundDataDestroy(void*);
void lovrSourceDestroy(void*);
void lovrTextureDestroy(void*);
void lovrTextureDataDestroy(void*);
void lovrThreadDestroy(void*);
void lovrWorldDestroy(void*);
#define INFO(T) [T_ ## T] = { #T, lovr ## T ## Destroy, T_NONE }
#define SUPERINFO(T, S) [T_ ## T] = { #T, lovr ## S ## Destroy, T_ ## S }
const TypeInfo lovrTypeInfo[T_MAX] = {
  INFO(Animator),
  INFO(AudioStream),
  SUPERINFO(BallJoint, Joint),
  INFO(Blob),
  SUPERINFO(BoxShape, Shape),
  INFO(Buffer),
  INFO(Canvas),
  SUPERINFO(CapsuleShape, Shape),
  INFO(Channel),
  INFO(Collider),
  INFO(Controller),
  INFO(Curve),
  SUPERINFO(CylinderShape, Shape),
  SUPERINFO(DistanceJoint, Joint),
  INFO(File),
  INFO(Font),
  SUPERINFO(HingeJoint, Joint),
  INFO(Joint),
  INFO(Material),
  INFO(Mesh),
  INFO(Microphone),
  INFO(Model),
  INFO(ModelData),
  INFO(Pool),
  INFO(RandomGenerator),
  INFO(Rasterizer),
  INFO(Shader),
  INFO(ShaderBlock),
  INFO(Shape),
  SUPERINFO(SliderJoint, Joint),
  INFO(SoundData),
  INFO(Source),
  SUPERINFO(SphereShape, Shape),
  INFO(Texture),
  INFO(TextureData),
  INFO(Thread),
  INFO(World)
};
#undef INFO
#undef SUPERINFO

Ref* _lovrAlloc(size_t size, Type type) {
  Ref* ref = calloc(1, size);
  lovrAssert(ref, "Out of memory");
  ref->type = type;
  ref->count = 1;
  return ref;
}
