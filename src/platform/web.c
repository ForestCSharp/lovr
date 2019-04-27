#include "platform.h"
#include <emscripten.h>

#include "platform/glfw.h"
#include "platform/log.c"

void lovrPlatformSleep(double seconds) {
  emscripten_sleep((unsigned int) (seconds * 1000));
}

int lovrPlatformGetExecutablePath(char* dest, uint32_t size) {
  return 1;
}

sds lovrPlatformGetApplicationId() {
	return NULL;
}
