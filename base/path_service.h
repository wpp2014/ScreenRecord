#ifndef BASE_PATH_SERVICE_H_
#define BASE_PATH_SERVICE_H_

#include "base/base_paths.h"

namespace base {

class FilePath;

class PathService {
 public:
  // Populates |path| with a special directory or file. Returns true on success,
  // in which case |path| is guaranteed to have a non-empty value. On failure,
  // |path| will not be changed.
  static bool Get(int key, FilePath* path);

  // To extend the set of supported keys, you can register a path provider,
  // which is just a function mirroring PathService::Get.  The ProviderFunc
  // returns false if it cannot provide a non-empty path for the given key.
  // Otherwise, true is returned.
  //
  // WARNING: This function could be called on any thread from which the
  // PathService is used, so a the ProviderFunc MUST BE THREADSAFE.
  //
  typedef bool (*ProviderFunc)(int, FilePath*);
};  // class PathService

}  // namespace

#endif  // BASE_PATH_SERVICE_H_
