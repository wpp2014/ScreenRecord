#include "utils/utils.h"

#include <shlwapi.h>

#include <assert.h>

#include "utils/scoped_handle.h"

#include "turbojpeg.h"

namespace utils {

int GetCurrentPath(std::wstring* path) {
  assert(path);

  wchar_t buffer[1024];
  memset(buffer, 0, sizeof(buffer));
  DWORD count = GetModuleFileName(NULL, buffer, 1024);
  if (!count || GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
    return -1;
  }

  *path = buffer;
  return 0;
}

int GetCurrentDir(std::wstring* path) {
  assert(path);

  wchar_t buffer[1024];
  memset(buffer, 0, sizeof(buffer));
  DWORD count = GetModuleFileName(NULL, buffer, 1024);
  if (!count || GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
    return -1;
  }

  if (!PathRemoveFileSpec(buffer)) {
    return -2;
  }

  *path = buffer;
  return 0;
}

int ARGBToJpeg(const uint8_t* data,
               uint32_t width,
               uint32_t height,
               uint32_t size,
               const wchar_t* path) {
  assert(data && path);
  assert(width > 0 && height > 0 && size > 0);

  int ret = 0;
  tjhandle handle = nullptr;

  uint8_t* jpeg_buffer = nullptr;
  uint32_t jpeg_size = 0;

  int pixel_format = TJPF_BGRA;
  int jpeg_subsamp = TJSAMP_444;
  int jpeg_qual = 90;
  int flags = 0;

  do {
    handle = tjInitCompress();
    if (!handle) {
      ret = -1;
      break;
    }

    int pitch = size / height;
    int jpeg_handle_res = tjCompress2(handle,
                                      data,
                                      width,
                                      pitch,
                                      height,
                                      pixel_format,
                                      &jpeg_buffer,
                                      (unsigned long*)&jpeg_size,
                                      jpeg_subsamp,
                                      jpeg_qual,
                                      flags);
    if (jpeg_handle_res != 0) {
      ret = -2;
      break;
    }

    ScopedHandle file(CreateFile(path,
                                 GENERIC_WRITE,
                                 0,
                                 NULL,
                                 CREATE_ALWAYS,
                                 FILE_ATTRIBUTE_NO_SCRUB_DATA,
                                 NULL));
    if (!file.IsValid()) {
      ret = -3;
      break;
    }

    DWORD written = 0;
    BOOL res = WriteFile(file, (void*)jpeg_buffer, jpeg_size, &written, NULL);
    if (!res || written != jpeg_size) {
      ret = -4;
      break;
    }
  } while (false);

  if (jpeg_buffer)
    tjFree(jpeg_buffer);
  if (handle)
    tjDestroy(handle);

  return ret;
}

}  // namespace utils
