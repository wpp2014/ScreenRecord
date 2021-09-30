#ifndef VIDEO_MUXER_H_
#define VIDEO_MUXER_H_

#include <string>

#include "video_config.h"

#ifdef __cplusplus
extern "C" {
#endif
#include "libavformat/avformat.h"
#include "libavutil/opt.h"
#include "libswscale/swscale.h"
#ifdef __cplusplus
}
#endif

class VideoMuxer {
 public:
  VideoMuxer();
  ~VideoMuxer();

  bool Open(const VideoConfig& config, const std::string& out_path);

  void Close();

  bool PushData(uint8_t* data,
                int width,
                int height,
                int len,
                int64_t pts);

 private:
  bool CreateFrame();
  bool CreateScaler(int src_width, int src_height);

  void Encode(AVFrame* frame);

  // 合成器是否打开
  bool opened_;

  AVFormatContext* format_ctx_;
  AVOutputFormat* output_fmt_;

  AVCodec* codec_;
  AVCodecContext* codec_ctx_;
  AVStream* stream_;
  AVDictionary* codec_options_;

  AVFrame* frame_;
  AVPacket* packet_;

  SwsContext* scaler_;

  // 最终生成的文件路径
  std::string out_path_;

  VideoConfig config_;

  VideoMuxer(const VideoMuxer&) = delete;
  VideoMuxer& operator=(const VideoMuxer&) = delete;
};

#endif  // VIDEO_MUXER_H_
