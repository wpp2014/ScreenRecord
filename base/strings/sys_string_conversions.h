// Copyright 2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MINI_CHROMIUM_BASE_STRINGS_SYS_STRING_CONVERSIONS_H_
#define MINI_CHROMIUM_BASE_STRINGS_SYS_STRING_CONVERSIONS_H_

#include <stdint.h>

#include <string>

#include "build/build_config.h"

#if defined(OS_APPLE)

#include <CoreFoundation/CoreFoundation.h>

#if defined(__OBJC__)
#import <Foundation/Foundation.h>
#else
class NSString;
#endif

#endif  // defined(OS_APPLE)

namespace base {

#if defined(OS_APPLE)

std::string SysCFStringRefToUTF8(CFStringRef ref);
std::string SysNSStringToUTF8(NSString* nsstring);
CFStringRef SysUTF8ToCFStringRef(const std::string& utf8);
NSString* SysUTF8ToNSString(const std::string& utf8);

#endif  // defined(OS_APPLE)

#if defined(OS_WIN)

std::wstring SysMultiByteToWide(const std::string& mb, uint32_t code_page);
std::string SysWideToMultiByte(const std::wstring& wide, uint32_t code_page);

#endif  // defined(OS_WIN)

}  // namespace base

#endif  // MINI_CHROMIUM_BASE_STRINGS_SYS_STRING_CONVERSIONS_H_
