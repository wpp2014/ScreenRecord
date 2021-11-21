#include "logger/logger.h"

#include <windows.h>

#include <assert.h>
#include <stdarg.h>

#include <string>

#include "base/strings/stringprintf.h"

namespace logger {

namespace {

int g_logging_destination = LOG_DEFAULT;

std::wstring* g_log_file_path = nullptr;
HANDLE g_log_file_handle = NULL;

void DeleteFilePath(const std::wstring& log_file) {
  DeleteFile(log_file.c_str());
}

std::wstring GetDefaultLogFile() {
  wchar_t module_name[MAX_PATH];
  GetModuleFileName(nullptr, module_name, MAX_PATH);

  std::wstring log_file = module_name;
  size_t last_backslash = log_file.rfind('\\', log_file.size());
  if (last_backslash != std::wstring::npos) {
    log_file.erase(last_backslash + 1);
  }
  log_file += L"debug.log";

  return log_file;
}

bool InitializeLogFileHandle() {
  if (g_log_file_handle) {
    return true;
  }

  if (!g_log_file_path) {
    g_log_file_path = new std::wstring(GetDefaultLogFile());
  }

  if ((g_logging_destination & LOG_TO_FILE) == 0) {
    return true;
  }

  g_log_file_handle = CreateFile(g_log_file_path->c_str(), FILE_APPEND_DATA,
                                 FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                 OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (g_log_file_handle == INVALID_HANDLE_VALUE ||
      g_log_file_handle == nullptr) {
    wchar_t system_buffer[MAX_PATH];
    system_buffer[0] = 0;
    DWORD len = ::GetCurrentDirectory(sizeof(system_buffer), system_buffer);
    if (len == 0 || len > sizeof(system_buffer)) {
      return false;
    }

    *g_log_file_path = system_buffer;
    if (g_log_file_path->back() != L'\\') {
      *g_log_file_path += L"\\";
    }
    *g_log_file_path += L"debug.log";

    g_log_file_handle = CreateFile(g_log_file_path->c_str(), FILE_APPEND_DATA,
                                   FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                                   OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (g_log_file_handle == INVALID_HANDLE_VALUE ||
        g_log_file_handle == nullptr) {
      g_log_file_handle = nullptr;
      return false;
    }
  }

  return true;
}

void CloseFile(HANDLE handle) {
  CloseHandle(handle);
}

void CloseLogFileUnlocked() {
  if (!g_log_file_handle) {
    return;
  }

  CloseFile(g_log_file_handle);
  g_log_file_handle = NULL;

  if (!g_log_file_path) {
    g_logging_destination &= ~LOG_TO_FILE;
  }
}

}  // namespace

bool InitLogging(const LoggingSettings& settings) {
  g_logging_destination = settings.logging_dest;
  if ((g_logging_destination & LOG_TO_FILE) == 0) {
    return true;
  }

  CloseLogFileUnlocked();

  assert(settings.log_file_path && "LOG_TO_FILE set but no log_file_path!");

  if (!g_log_file_path) {
    g_log_file_path = new std::wstring();
  }
  *g_log_file_path = settings.log_file_path;
  if (settings.delete_old & DELETE_OLD_LOG_FILE) {
    DeleteFilePath(*g_log_file_path);
  }

  return InitializeLogFileHandle();
}

void Log(const char* filter,
         const char* filename,
         const char* function,
         int line_number,
         const char* level,
         const char* format,
         ...) {
  assert(filter && filename && function && level && format);

  va_list args;
  va_start(args, format);

  std::string str;
  base::StringAppendV(&str, format, args);
  va_end(args);

  std::string file = filename;
  size_t pos = file.rfind('\\', file.size());
  if (pos != std::string::npos) {
    file = file.substr(pos + 1);
  }

  SYSTEMTIME local_time;
  GetLocalTime(&local_time);
  fprintf(stderr, "[%04d-%02d-%02d %02d:%02d:%02d.%03d][%lu:%lu][%s][%s(%d)][%s] %s\n",
          local_time.wYear, local_time.wMonth, local_time.wDay,
          local_time.wHour, local_time.wMinute, local_time.wSecond,
          local_time.wMilliseconds,
          GetCurrentProcessId(), GetCurrentThreadId(),
          level, file.c_str(), line_number, filter,
          str.c_str());
}

}  // namespace logger
