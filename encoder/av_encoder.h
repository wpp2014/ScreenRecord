#ifndef SCREEN_RECORD_LIB_ENCODER_AV_ENCODER_H_
#define SCREEN_RECORD_LIB_ENCODER_AV_ENCODER_H_

#include "encoder/ffmpeg.h"

class AVEncoder {
 public:
  virtual ~AVEncoder() {}

  virtual bool Open(AVStream* stream) = 0;
  virtual int PushEncodeFrame(uint8_t* data,
                              int len,
                              int width,
                              int height,
                              int stride,
                              int64_t time_stampt,
                              AVFrame** dst_frame) = 0;
  // virtual bool PullEncodedPacket(AVPacket* packet, bool* is_valid) = 0;
  virtual AVCodecContext* GetCodecContext() const = 0;
};  // class AVEncoder

#endif  // SCREEN_RECORD_LIB_ENCODER_AV_ENCODER_H_
