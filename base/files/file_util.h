// Copyright 2006-2008 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MINI_CHROMIUM_BASE_FILES_FILE_UTIL_H_
#define MINI_CHROMIUM_BASE_FILES_FILE_UTIL_H_

#include "base/files/file.h"
#include "base/files/file_path.h"
#include "build/build_config.h"

#if defined(OS_POSIX)
#include <sys/types.h>
#endif  // OS_POSIX

namespace base {

#if defined(OS_POSIX)
bool ReadFromFD(int fd, char* buffer, size_t bytes);
#endif  // OS_POSIX

// Returns an absolute version of a relative path. Returns an empty path on
// error. On POSIX, this function fails if the path does not exist. This
// function can result in I/O so it can be slow.
FilePath MakeAbsoluteFilePath(const FilePath& input);

// Get the temporary directory provided by the system.
//
// WARNING: In general, you should use CreateTemporaryFile variants below
// instead of this function. Those variants will ensure that the proper
// permissions are set so that other users on the system can't edit them while
// they're open (which can lead to security issues).
bool GetTempDir(FilePath* path);

// Get the home directory. This is more complicated than just getenv("HOME")
// as it knows to fall back on getpwent() etc.
//
// You should not generally call this directly. Instead use DIR_HOME with the
// path service which will use this function but cache the value.
// Path service may also override DIR_HOME.
FilePath GetHomeDir();

// Returns true if the given path exists on the local filesystem,
// false otherwise.
bool PathExists(const FilePath& path);

// Returns true if the given path exists and is a directory, false otherwise.
bool DirectoryExists(const FilePath& path);

bool CreateDirectory(const FilePath& full_path);

bool CreateDirectoryAndGetError(const FilePath& full_path, File::Error* error);

// Gets the current working directory for the process.
bool GetCurrentDirectory(FilePath* path);

}  // namespace base


#endif  // MINI_CHROMIUM_BASE_FILES_FILE_UTIL_H_
