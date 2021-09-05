#ifndef SCREEN_RECORD_LIB_ENCODER_AV_CONFIG_H_
#define SCREEN_RECORD_LIB_ENCODER_AV_CONFIG_H_

#include "screen_record/src/ffmpeg.h"

struct VideoConfig {
  // 帧率
  int fps;

  int width;
  int height;

  AVPixelFormat input_pixel_format;
};  // struct VideoConfig

struct AudioConfig {
  // 编码ID
  AVCodecID codec_id;
  // 采样率
  int sample_rate;
  // 声道数
  int channels;
  // 声道布局
  uint64_t channel_layout;
  // 音频格式
  AVSampleFormat sample_fmt;

  AudioConfig()
      : codec_id(AV_CODEC_ID_NONE),
        sample_rate(0),
        channels(0),
        channel_layout(0),
        sample_fmt(AV_SAMPLE_FMT_NONE) {}
};

#endif  // SCREEN_RECORD_LIB_ENCODER_AV_CONFIG_H_
