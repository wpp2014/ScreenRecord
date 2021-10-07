#ifndef LOG_H_
#define LOG_H_

#include <stdio.h>

#define LOG(format, ...)                                                     \
  do {                                                                       \
    fprintf(stderr, "[%lu %lu %s(%d)]: " format "\n", GetCurrentProcessId(), \
            GetCurrentThreadId(), __FILE__, __LINE__, ##__VA_ARGS__);        \
  } while (0)

#endif  // LOG_H_
