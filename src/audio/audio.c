#include "audio/audio.h"
#include "audio/source.h"
#include "data/audioStream.h"
#include "lib/err.h"
#include "lib/maf.h"
#include "lib/vec/vec.h"
#include "util.h"
#include <stdlib.h>
#include <AL/al.h>
#include <AL/alc.h>
#include <AL/alext.h>

static struct {
  bool initialized;
  ALCdevice* device;
  ALCcontext* context;
  vec_void_t sources;
  bool isSpatialized;
  f32 orientation[4];
  f32 position[3];
  f32 velocity[3];
} state;

ALenum lovrAudioConvertFormat(u32 bitDepth, u32 channelCount) {
  if (bitDepth == 8 && channelCount == 1) {
    return AL_FORMAT_MONO8;
  } else if (bitDepth == 8 && channelCount == 2) {
    return AL_FORMAT_STEREO8;
  } else if (bitDepth == 16 && channelCount == 1) {
    return AL_FORMAT_MONO16;
  } else if (bitDepth == 16 && channelCount == 2) {
    return AL_FORMAT_STEREO16;
  }

  return 0;
}

bool lovrAudioInit() {
  if (state.initialized) return false;

  ALCdevice* device = alcOpenDevice(NULL);
  lovrAssert(device, "Unable to open default audio device");

  ALCcontext* context = alcCreateContext(device, NULL);
  if (!context || !alcMakeContextCurrent(context) || alcGetError(device) != ALC_NO_ERROR) {
    lovrThrow("Unable to create OpenAL context");
  }

#if ALC_SOFT_HRTF
  static LPALCRESETDEVICESOFT alcResetDeviceSOFT;
  alcResetDeviceSOFT = (LPALCRESETDEVICESOFT) alcGetProcAddress(device, "alcResetDeviceSOFT");
  state.isSpatialized = alcIsExtensionPresent(device, "ALC_SOFT_HRTF");

  if (state.isSpatialized) {
    alcResetDeviceSOFT(device, (ALCint[]) { ALC_HRTF_SOFT, ALC_TRUE, 0 });
  }
#endif

  state.device = device;
  state.context = context;
  vec_init(&state.sources);
  return state.initialized = true;
}

void lovrAudioDestroy() {
  if (!state.initialized) return;
  alcMakeContextCurrent(NULL);
  alcDestroyContext(state.context);
  alcCloseDevice(state.device);
  for (i32 i = 0; i < state.sources.length; i++) {
    lovrRelease(Source, state.sources.data[i]);
  }
  vec_deinit(&state.sources);
  memset(&state, 0, sizeof(state));
}

void lovrAudioUpdate() {
  i32 i; Source* source;
  vec_foreach_rev(&state.sources, source, i) {
    if (lovrSourceGetType(source) == SOURCE_STATIC) {
      continue;
    }

    u32 id = lovrSourceGetId(source);
    bool isStopped = lovrSourceIsStopped(source);
    ALint processed;
    alGetSourcei(id, AL_BUFFERS_PROCESSED, &processed);

    if (processed) {
      ALuint buffers[SOURCE_BUFFERS];
      alSourceUnqueueBuffers(id, processed, buffers);
      lovrSourceStream(source, buffers, processed);
      if (isStopped) {
        alSourcePlay(id);
      }
    } else if (isStopped) {
      vec_splice(&state.sources, i, 1);
      lovrRelease(Source, source);
    }
  }
}

void lovrAudioAdd(Source* source) {
  if (!lovrAudioHas(source)) {
    lovrRetain(source);
    vec_push(&state.sources, source);
  }
}

void lovrAudioGetDopplerEffect(f32* factor, f32* speedOfSound) {
  alGetFloatv(AL_DOPPLER_FACTOR, factor);
  alGetFloatv(AL_SPEED_OF_SOUND, speedOfSound);
}

void lovrAudioGetMicrophoneNames(const char* names[MAX_MICROPHONES], u32* count) {
  const char* name = alcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
  *count = 0;
  while (*name && *count < MAX_MICROPHONES) {
    names[(*count)++] = name;
    name += strlen(name);
  }
}

void lovrAudioGetOrientation(quat orientation) {
  quat_init(orientation, state.orientation);
}

void lovrAudioGetPosition(vec3 position) {
  vec3_init(position, state.position);
}

void lovrAudioGetVelocity(vec3 velocity) {
  vec3_init(velocity, state.velocity);
}

f32 lovrAudioGetVolume() {
  f32 volume;
  alGetListenerf(AL_GAIN, &volume);
  return volume;
}

bool lovrAudioHas(Source* source) {
  i32 index;
  vec_find(&state.sources, source, index);
  return index >= 0;
}

bool lovrAudioIsSpatialized() {
  return state.isSpatialized;
}

void lovrAudioPause() {
  i32 i; Source* source;
  vec_foreach(&state.sources, source, i) {
    lovrSourcePause(source);
  }
}

void lovrAudioResume() {
  i32 i; Source* source;
  vec_foreach(&state.sources, source, i) {
    lovrSourceResume(source);
  }
}

void lovrAudioRewind() {
  i32 i; Source* source;
  vec_foreach(&state.sources, source, i) {
    lovrSourceRewind(source);
  }
}

void lovrAudioSetDopplerEffect(f32 factor, f32 speedOfSound) {
  alDopplerFactor(factor);
  alSpeedOfSound(speedOfSound);
}

void lovrAudioSetOrientation(quat orientation) {

  // Rotate the unit forward/up vectors by the quaternion derived from the specified angle/axis
  f32 f[3] = { 0.f, 0.f, -1.f };
  f32 u[3] = { 0.f, 1.f,  0.f };
  quat_init(state.orientation, orientation);
  quat_rotate(state.orientation, f);
  quat_rotate(state.orientation, u);

  // Pass the rotated orientation vectors to OpenAL
  ALfloat directionVectors[6] = { f[0], f[1], f[2], u[0], u[1], u[2] };
  alListenerfv(AL_ORIENTATION, directionVectors);
}

void lovrAudioSetPosition(vec3 position) {
  vec3_init(state.position, position);
  alListenerfv(AL_POSITION, position);
}

void lovrAudioSetVelocity(vec3 velocity) {
  vec3_init(state.velocity, velocity);
  alListenerfv(AL_VELOCITY, velocity);
}

void lovrAudioSetVolume(f32 volume) {
  alListenerf(AL_GAIN, volume);
}

void lovrAudioStop() {
  i32 i; Source* source;
  vec_foreach(&state.sources, source, i) {
    lovrSourceStop(source);
  }
}
