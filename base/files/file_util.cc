#include "base/files/file_util.h"

namespace base {

bool CreateDirectory(const FilePath& full_path) {
  return CreateDirectoryAndGetError(full_path, nullptr);
}

}  // namespace base
