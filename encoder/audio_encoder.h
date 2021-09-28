#ifndef ENCODER_AUDIO_ENCODER_H_
#define ENCODER_AUDIO_ENCODER_H_

#include "encoder/av_config.h"
#include "encoder/av_encoder.h"

class AudioEncoder : public AVEncoder {
 public:
  AudioEncoder(const AudioConfig& audio_config);
  ~AudioEncoder();

  bool Initialize();

  // Override from AVEncoder
  bool Open(AVStream* audio_stream) override;
  int PushEncodeFrame(uint8_t* data,
                      int len,
                      int width,
                      int height,
                      int stride,
                      int64_t time_stamp,
                      AVFrame** encoded_frame) override;
  AVCodecContext* GetCodecContext() const override;

  int FrameSize() const;
  int src_buf_size() const { return src_buf_size_; }

 private:
  SwrContext* CreateResampler(AVSampleFormat dst_sample_fmt,
                              int dst_sample_rate,
                              int64_t dst_channel_layout,
                              AVSampleFormat src_sample_fmt,
                              int src_sample_rate,
                              int64_t src_channel_layout);

  AVFrame* CreateFrame(AVSampleFormat sample_fmt,
                       int channels,
                       int sample_rate);

  bool initialized_;

  // 存在不需要重采样的情况
  bool need_resample_;

  AVCodec* codec_;
  AVCodecContext* codec_context_;
  AVFrame* frame_;
  SwrContext* resampler_;

  AudioConfig config_;

  int src_buf_size_;
  uint8_t** src_data_;

  AudioEncoder() = delete;
  AudioEncoder(const AudioEncoder&) = delete;
  AudioEncoder& operator=(const AudioEncoder&) = delete;
};  // class AudioEncoder

#endif  // ENCODER_AUDIO_ENCODER_H_
