#ifndef SCREEN_RECORD_SRC_UTIL_STRING_H_
#define SCREEN_RECORD_SRC_UTIL_STRING_H_

#include <string>

std::wstring SysMultiByteToWide(const std::string& mb, uint32_t code_page);
std::string SysWideToMultiByte(const std::wstring& wide, uint32_t code_page);

#endif  // SCREEN_RECORD_SRC_UTIL_STRING_H_
