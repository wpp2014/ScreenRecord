#ifndef LOGGER_LOGGER_H_
#define LOGGER_LOGGER_H_

#include <stdint.h>

#define LOG_INFO(filter, fmt, ...)  \
    logger::Log(filter, __FILE__, __FUNCTION__, __LINE__, "INFO", fmt, ##__VA_ARGS__)
#define LOG_WARN(filter, fmt, ...)  \
    logger::Log(filter, __FILE__, __FUNCTION__, __LINE__, "WARNING", fmt, ##__VA_ARGS__)
#define LOG_ERROR(filter, fmt, ...) \
    logger::Log(filter, __FILE__, __FUNCTION__, __LINE__, "ERROR", fmt, ##__VA_ARGS__)
#define LOG_DEBUG(filter, fmt, ...) \
    logger::Log(filter, __FILE__, __FUNCTION__, __LINE__, "DEBUG", fmt, ##__VA_ARGS__)

namespace logger {

using LoggingDestination = uint32_t;
enum : uint32_t {
  LOG_NONE = 0,
  LOG_TO_FILE = 1 << 0,
  LOG_TO_STDERR = 1 << 1,

  LOG_TO_ALL = LOG_TO_FILE | LOG_TO_STDERR,
  LOG_DEFAULT = LOG_TO_STDERR,
};

// On startup, should we delete or append to an existing log file (if any)?
// Defaults to APPEND_TO_OLD_LOG_FILE.
enum OldFileDeletionState { DELETE_OLD_LOG_FILE, APPEND_TO_OLD_LOG_FILE };

struct LoggingSettings {
  // Equivalent to logging destination enum, but allows for multiple
  // destinations.
  uint32_t logging_dest = LOG_DEFAULT;

  const wchar_t* log_file_path = nullptr;

  OldFileDeletionState delete_old = APPEND_TO_OLD_LOG_FILE;
};  // struct LoggingSettings

bool InitLogging(const LoggingSettings& settings);

class Logger {
public:
private:
  Logger(const Logger&) = delete;
  Logger& operator=(const Logger&) = delete;
};  // class Logger

void Log(const char* filter,
         const char* filename,
         const char* function,
         int line_number,
         const char* level,
         const char* format,
         ...);

}  // namespace logger

#endif  // LOGGER_LOGGER_H_
