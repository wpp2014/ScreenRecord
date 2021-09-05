#ifndef SCREEN_RECORD_SRC_UTIL_TIME_HELPER_H_
#define SCREEN_RECORD_SRC_UTIL_TIME_HELPER_H_

#include <stdint.h>

class TimeHelper {
 public:
  TimeHelper();
  ~TimeHelper();

  double now();

 private:
  int64_t start_time_;

  double resolution_;
};  // class TimeHelper

#endif  // SCREEN_RECORD_SRC_UTIL_TIME_HELPER_H_
