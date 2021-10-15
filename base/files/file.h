#ifndef BASE_FILES_FILE_H_
#define BASE_FILES_FILE_H_

#include "build/build_config.h"

#if defined(OS_WIN)
#include <windows.h>
#endif

#if defined(OS_BSD) || defined(OS_APPLE) || defined(OS_NACL) || \
    defined(OS_FUCHSIA) || (defined(OS_ANDROID) && __ANDROID_API__ < 21)
struct stat;
namespace base {
typedef struct stat stat_wrapper_t;
}
#elif defined(OS_POSIX)
struct stat64;
namespace base {
typedef struct stat64 stat_wrapper_t;
}
#endif

namespace base {

class File {
 public:

  // This enum has been recorded in multiple histograms using PlatformFileError
  // enum. If the order of the fields needs to change, please ensure that those
  // histograms are obsolete or have been moved to a different enum.
  //
  // FILE_ERROR_ACCESS_DENIED is returned when a call fails because of a
  // filesystem restriction. FILE_ERROR_SECURITY is returned when a browser
  // policy doesn't allow the operation to be executed.
  enum Error {
    FILE_OK = 0,
    FILE_ERROR_FAILED = -1,
    FILE_ERROR_IN_USE = -2,
    FILE_ERROR_EXISTS = -3,
    FILE_ERROR_NOT_FOUND = -4,
    FILE_ERROR_ACCESS_DENIED = -5,
    FILE_ERROR_TOO_MANY_OPENED = -6,
    FILE_ERROR_NO_MEMORY = -7,
    FILE_ERROR_NO_SPACE = -8,
    FILE_ERROR_NOT_A_DIRECTORY = -9,
    FILE_ERROR_INVALID_OPERATION = -10,
    FILE_ERROR_SECURITY = -11,
    FILE_ERROR_ABORT = -12,
    FILE_ERROR_NOT_A_FILE = -13,
    FILE_ERROR_NOT_EMPTY = -14,
    FILE_ERROR_INVALID_URL = -15,
    FILE_ERROR_IO = -16,
    // Put new entries here and increment FILE_ERROR_MAX.
    FILE_ERROR_MAX = -17
  };

#if defined(OS_WIN)
  static Error OSErrorToFileError(DWORD last_error);
#elif defined(OS_POSIX) || defined(OS_FUCHSIA)
  static Error OSErrorToFileError(int saved_errno);
#endif

};  // class File

}  // namespace base

#endif  // BASE_FILES_FILE_H_
