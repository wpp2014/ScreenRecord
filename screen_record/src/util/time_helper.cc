#include "screen_record/src/util/time_helper.h"

#include <windows.h>

TimeHelper::TimeHelper() {
  // 获得机器内部计时器的时钟频率
  int64_t freq = 0;
  QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&freq));

  // 内部机器一次计数所需的时间
  resolution_ = 1.0 / static_cast<double>(freq);

  // 获取当前的计数次数
  QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&start_time_));
}

TimeHelper::~TimeHelper() {
}

double TimeHelper::now() {
  int64_t count;
  QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&count));
  return resolution_ * count;
}
