#include "filesystem/file.h"
#include "lib/err.h"
#include <physfs.h>

File* lovrFileInit(File* file ,const char* path) {
  file->path = path;
  return file;
}

void lovrFileDestroy(void* ref) {
  File* file = ref;
  if (file->handle) {
    PHYSFS_close(file->handle);
  }
}

bool lovrFileOpen(File* file, FileMode mode) {
  lovrAssert(!file->handle, "File is already open");
  file->mode = mode;

  switch (mode) {
    case OPEN_READ: file->handle = PHYSFS_openRead(file->path); break;
    case OPEN_WRITE: file->handle = PHYSFS_openWrite(file->path); break;
    case OPEN_APPEND: file->handle = PHYSFS_openAppend(file->path); break;
  }

  return file->handle != NULL;
}

void lovrFileClose(File* file) {
  lovrAssert(file->handle, "File must be open to close it");
  PHYSFS_close(file->handle);
  file->handle = NULL;
}

usize lovrFileRead(File* file, void* data, usize bytes) {
  lovrAssert(file->handle && file->mode == OPEN_READ, "File must be open for reading");
  return (usize) PHYSFS_readBytes(file->handle, data, bytes);
}

usize lovrFileWrite(File* file, const void* data, usize bytes) {
  lovrAssert(file->handle && (file->mode == OPEN_READ || file->mode == OPEN_WRITE), "File must be open for writing");
  return (usize) PHYSFS_writeBytes(file->handle, data, bytes);
}

usize lovrFileGetSize(File* file) {
  lovrAssert(file->handle, "File must be open to get its size");
  return (usize) PHYSFS_fileLength(file->handle);
}

bool lovrFileSeek(File* file, usize position) {
  lovrAssert(file->handle, "File must be open to seek");
  return PHYSFS_seek(file->handle, position);
}

usize lovrFileTell(File* file) {
  lovrAssert(file->handle, "File must be open to tell");
  return (usize) PHYSFS_tell(file->handle);
}
