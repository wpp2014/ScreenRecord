#include <windows.h>
#include <shlobj.h>

#include "base/base_paths.h"
#include "base/files/file_path.h"
#include "base/win/current_module.h"

using base::FilePath;

namespace base {

bool PathProviderWin(int key, FilePath* result) {
  // We need to go compute the value. It would be nice to support paths with
  // names longer than MAX_PATH, but the system functions don't seem to be
  // designed for it either, with the exception of GetTempPath (but other
  // things will surely break if the temp path is too long, so we don't bother
  // handling it.
  wchar_t system_buffer[MAX_PATH];
  system_buffer[0] = 0;

  FilePath cur;
  switch (key) {
    case base::FILE_EXE:
      if (GetModuleFileName(NULL, system_buffer, MAX_PATH) == 0) {
        return false;
      }
      cur = FilePath(system_buffer);
      break;
    case base::FILE_MODULE: {
      // the resource containing module is assumed to be the one that
      // this code lives in, whether that's a dll or exe
      if (GetModuleFileName(CURRENT_MODULE(), system_buffer, MAX_PATH) == 0) {
        return false;
      }
      cur = FilePath(system_buffer);
      break;
    }
    case base::DIR_LOCAL_APP_DATA:
      if (FAILED(SHGetFolderPath(NULL, CSIDL_LOCAL_APPDATA, NULL,
                                 SHGFP_TYPE_CURRENT, system_buffer))) {
        return false;
      }
      cur = FilePath(system_buffer);
      break;
    default:
      return false;
  }

  *result = cur;
  return true;
}

}  // namespace base
