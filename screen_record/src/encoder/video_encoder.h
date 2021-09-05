// 视频编码

#ifndef SCREEN_RECORD_LIB_ENCODER_VIDEO_ENCODER_H_
#define SCREEN_RECORD_LIB_ENCODER_VIDEO_ENCODER_H_

#include "screen_record/src/encoder/av_config.h"
#include "screen_record/src/encoder/av_encoder.h"

class VideoEncoder : public AVEncoder {
 public:
  VideoEncoder(const VideoConfig& video_config);
  ~VideoEncoder() override;

  bool Initialize();

  // Override from AVEncoder
  bool Open(AVStream* video_stream) override;
  int PushEncodeFrame(uint8_t* data,
                      int len,
                      int width,
                      int height,
                      int stride,
                      int64_t time_stamp,
                      AVFrame** encoded_frame) override;
  AVCodecContext* GetCodecContext() const override;

  AVRational GetTimeBase() const;

 private:
  SwsContext* CreateSoftwareScaler(
      AVPixelFormat src_pixel_format, int src_width, int src_height,
      AVPixelFormat dst_pixel_format, int dst_width, int dst_height);

  bool initialized_;

  AVCodec* codec_;
  AVCodecContext* codec_context_;
  AVFrame* frame_;

  SwsContext* sws_context_;

  AVDictionary* dict_;

  AVPixelFormat input_pixel_format_;
  AVPixelFormat output_pixel_format_;

  VideoConfig video_config_;

  VideoEncoder() = delete;
  VideoEncoder(const VideoEncoder&) = delete;
  VideoEncoder& operator=(const VideoEncoder&) = delete;
};  // class VideoEncoder

#endif  // SCREEN_RECORD_LIB_ENCODER_VIDEO_ENCODER_H_
