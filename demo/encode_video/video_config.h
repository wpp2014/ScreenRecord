#ifndef VIDEO_CONFIG_H_
#define VIDEO_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "libavutil/pixfmt.h"
#ifdef __cplusplus
}
#endif

struct VideoConfig {
  // 文件格式
  const char* file_format;

  int width;
  int height;

  int fps;
  int bit_rate;
  AVPixelFormat input_pix_fmt;
};

#endif  // VIDEO_CONFIG_H_
