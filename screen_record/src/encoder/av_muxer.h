// 音视频合成器

#ifndef SCREEN_RECORD_LIB_ENCODER_AV_MUXER_H_
#define SCREEN_RECORD_LIB_ENCODER_AV_MUXER_H_

#include <memory>
#include <string>

#include "screen_record/src/encoder/av_config.h"

class AudioEncoder;
class VideoEncoder;

class AVMuxer {
 public:
  AVMuxer(const AudioConfig& audio_config,
          const VideoConfig& video_config,
          const std::string& output_path,
          bool can_capture_voice);
  ~AVMuxer();

  bool Initialize();

  bool Open();
  void Flush();

  bool EncodeAudioFrame(uint8_t* data, int len);
  bool EncodeVideoFrame(uint8_t* data,
                        int width,
                        int height,
                        int stride,
                        uint64_t time_stamp);

  int AudioFrameSize() const;

 private:
  bool OpenAudio();
  bool OpenVideo();

  bool WriteFrame(AVFormatContext* format_ctx,
                  AVCodecContext* codec_ctx,
                  AVStream* stream,
                  AVFrame* encoded_frame);

  int WriteVideoFrame(const AVRational& time_base,
                      AVStream* stream,
                      AVPacket* pkt);

  bool initialized_;

  // 用来判断是否应该录音
  bool can_capture_voice_;

  // 录入的声音的样例数
  int audio_samples_;

  int64_t video_pts_;

  AVFormatContext* format_context_;
  AVOutputFormat* output_format_;

  AudioConfig audio_config_;
  std::unique_ptr<AudioEncoder> audio_encoder_;
  AVStream* audio_stream_;

  VideoConfig video_config_;
  std::unique_ptr<VideoEncoder> video_encoder_;
  AVStream* video_stream_;

  std::string output_path_;
  std::string output_dir_;

  // 存放每一帧的音频数据
  uint8_t* buffer_audio_frame_;
  // 每一个音频帧数据大小
  int size_per_audio_frame_;
  // 每次录音的数据大小不一定被size_per_audio_frame_整除
  int last_index_;

  AVMuxer() = delete;
  AVMuxer(const AVMuxer&) = delete;
  AVMuxer& operator=(const AVMuxer&) = delete;
};  // namespace AVMuxer

#endif  // SCREEN_RECORD_LIB_ENCODER_AV_MUXER_H_
