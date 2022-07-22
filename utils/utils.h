#ifndef UTILS_UTILS_H_
#define UTILS_UTILS_H_

#include <windows.h>

#include <stdint.h>

#include <string>

namespace utils {

int GetCurrentPath(std::wstring* path);
int GetCurrentDir(std::wstring* path);

int ARGBToJpeg(const uint8_t* data,
               uint32_t width,
               uint32_t height,
               uint32_t size,
               const wchar_t* path);

}  // namespace utils

#endif  // UTILS_UTILS_H_
