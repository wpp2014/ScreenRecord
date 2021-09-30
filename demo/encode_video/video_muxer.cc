#include "video_muxer.h"

#include <assert.h>

#include <chrono>

#include <windows.h>

const int kPathLen = 1024;

struct FormatInfo {
  const char* format_name;
  const char* mime_type;
  const char* extension_name;
} kAllFormat[] = {
    {"mp4", "video/mp4", "mp4"},
    {"mkv", "video/mkv", "mkv"},
    {"avi", "video/avi", "avi"},
};

#if 0
std::string GeneratePath(const char* format) {
  char path[kPathLen];
  memset(path, 0, kPathLen);

  DWORD count = GetTempPathA(kPathLen, path);
  if (count > MAX_PATH || count == 0) {
    printf("获取临时目录失败\n");
    return std::string();
  }

  std::time_t t = std::time(nullptr);
  std::tm tm = *std::localtime(&t);

  char filename[64];
  memset(filename, 0, 64);
  sprintf(filename, "%04d-%02d-%02d-%02d-%02d-%02d.%s", tm.tm_year,
          tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec, format);

  strcat(path, filename);
  return path;
}
#endif

VideoMuxer::VideoMuxer()
    : opened_(false),
      format_ctx_(nullptr),
      output_fmt_(nullptr),
      codec_(nullptr),
      codec_ctx_(nullptr),
      stream_(nullptr),
      codec_options_(nullptr),
      frame_(nullptr),
      packet_(nullptr),
      scaler_(nullptr),
      config_({0}) {
}

VideoMuxer::~VideoMuxer() {
  Close();
}

bool VideoMuxer::Open(const VideoConfig& config, const std::string& out_path) {
  // avformat_write_header
  // av_interleaved_write_frame
  // avformat_write_trailer

  if (opened_) {
    Close();
  }

  bool res = false;
  int i = 0;
  int ret = 0;

  config_ = config;
  assert(config_.file_format &&
         config_.width > 0 &&
         config_.height > 0 &&
         config_.bit_rate > 0 &&
         config_.fps > 0);
  assert(config_.input_pix_fmt == AV_PIX_FMT_RGB32 ||
         config_.input_pix_fmt == AV_PIX_FMT_RGB24 ||
         config_.input_pix_fmt == AV_PIX_FMT_BGR24);

  out_path_ = out_path;

  if (config_.input_pix_fmt != AV_PIX_FMT_RGB32 &&
      config_.input_pix_fmt != AV_PIX_FMT_RGB24 &&
      config_.input_pix_fmt != AV_PIX_FMT_BGR24) {
    printf("只支持以下格式的图像数据:\n");
    printf("  AV_PIX_FMT_RGB32\n  AV_PIX_FMT_RGB24\n  AV_PIX_FMT_BGR24\n");
    goto end;
  }

  i = 0;
  while (i < sizeof(kAllFormat) / sizeof(FormatInfo)) {
    if (strcmp(config_.file_format, kAllFormat[i].format_name) == 0) {
      break;
    }
    ++i;
  }
  if (i == i < sizeof(kAllFormat) / sizeof(FormatInfo)) {
    printf("不支持的文件格式: %s\n", config_.file_format);
    goto end;
  }

  output_fmt_ = av_guess_format(config_.file_format, NULL, NULL);
  if (!output_fmt_) {
    printf("根据文件格式%s获取输出信息失败.\n", config_.file_format);
    goto end;
  }

  ret = avformat_alloc_output_context2(
      &format_ctx_, output_fmt_, NULL, out_path_.c_str());
  if (ret < 0) {
    printf("avformat_alloc_output_context2 failed: %d\n", ret);
    goto end;
  }

  codec_ = avcodec_find_encoder(AV_CODEC_ID_H264);
  if (!codec_) {
    printf("未发现H264编码器\n");
    goto end;
  }

  stream_ = avformat_new_stream(format_ctx_, codec_);
  if (!stream_) {
    printf("创建AVStream失败\n");
    goto end;
  }
  stream_->id = format_ctx_->nb_streams - 1;

  codec_ctx_ = avcodec_alloc_context3(codec_);
  if (!codec_ctx_) {
    printf("avcodec_alloc_context3 failed\n");
    goto end;
  }

  codec_ctx_->codec_type = AVMEDIA_TYPE_VIDEO;
  codec_ctx_->pix_fmt = AV_PIX_FMT_YUV420P;
  codec_ctx_->codec_id = codec_->id;
  codec_ctx_->width = config_.width;
  codec_ctx_->height = config_.height;
  codec_ctx_->framerate = {config_.fps, 1};
  codec_ctx_->time_base = {1, 1000};

  codec_ctx_->gop_size = config_.fps;
  codec_ctx_->max_b_frames = 1;

  // codec_ctx->bit_rate = 1000000;
  codec_ctx_->flags |= AV_CODEC_FLAG_QSCALE;

  if (output_fmt_->flags & AVFMT_GLOBALHEADER) {
    codec_ctx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  }

  av_opt_set(codec_options_, "crf", "18", 0);
  if (codec_ctx_->codec_id == AV_CODEC_ID_H264) {
    // av_opt_set(options, "tune", "stillimage", 0);
    av_opt_set(codec_options_, "preset", "ultrafast", 0);
  }

  ret = avcodec_open2(codec_ctx_, codec_, &codec_options_);
  if (ret != 0) {
    printf("open video codec context failed: %d\n", ret);
    goto end;
  }

  ret = avcodec_parameters_from_context(stream_->codecpar, codec_ctx_);
  if (ret < 0) {
    printf("call avcodec_parameters_from_context failed\n");
    goto end;
  }

  if (!CreateFrame() || !CreateScaler(config_.width, config_.height)) {
    goto end;
  }

  if (!(output_fmt_->flags & AVFMT_NOFILE)) {
    ret = avio_open(&format_ctx_->pb, out_path_.c_str(), AVIO_FLAG_WRITE);
    if (ret < 0) {
      printf("open % s failed.\n", out_path_.c_str());
      goto end;
    }
  }

  ret = avformat_write_header(format_ctx_, NULL);
  if (ret < 0) {
    printf("write header to %s failed.\n", out_path_.c_str());
    goto end;
  }

  av_dump_format(format_ctx_, 0, out_path_.c_str(), 1);

  res = true;
  opened_ = true;

end:
  if (res) {
    return true;
  }

  Close();
  return false;
}

void VideoMuxer::Close() {
  if (opened_) {
    Encode(nullptr);
    av_write_trailer(format_ctx_);
  }
  opened_ = false;

  if (codec_ctx_) {
    avcodec_free_context(&codec_ctx_);
  }

#if 0
  // 调用avformat_close_input函数会执行下面的代码
  if (!(output_format->flags & AVFMT_NOFILE)) {
    ret = avio_close(format_ctx->pb);
    if (ret < 0) {
      fprintf(stderr, "close %s failed.", filepath);
    }
  }
#endif
  if (format_ctx_) {
    avformat_close_input(&format_ctx_);
  }

  if (scaler_) {
    sws_freeContext(scaler_);
  }
  if (packet_) {
    av_packet_free(&packet_);
  }
  if (frame_) {
    av_frame_free(&frame_);
  }
  if (codec_options_) {
    av_dict_free(&codec_options_);
  }

  format_ctx_ = nullptr;
  output_fmt_ = nullptr;

  codec_ = nullptr;
  codec_ctx_ = nullptr;
  stream_ = nullptr;
  codec_options_ = nullptr;

  frame_ = nullptr;
  packet_ = nullptr;
  scaler_ = nullptr;

  out_path_.clear();
}

bool VideoMuxer::PushData(uint8_t* data,
                          int width,
                          int height,
                          int len,
                          int64_t pts) {
  uint8_t* slice[3] = {data, nullptr, nullptr};
  int stride[1] = {len / height};

  int ret = av_frame_make_writable(frame_);
  if (ret != 0) {
    printf("av_frame_make_writable failed\n");
    return false;
  }

  ret = sws_scale(scaler_, slice, stride, 0, height, frame_->data,
                  frame_->linesize);
  if (ret < 0) {
    printf("sws_scale failed\n");
    return false;
  }

  frame_->pts = pts;
  Encode(frame_);

  return true;
}

bool VideoMuxer::CreateFrame() {
  assert(codec_ctx_);

  packet_ = av_packet_alloc();
  if (!packet_) {
    printf("av_packet_alloc failed\n");
    return false;
  }

  frame_ = av_frame_alloc();
  if (!frame_) {
    printf("av_frame_alloc failed\n");
    return false;
  }

  frame_->width = codec_ctx_->width;
  frame_->height = codec_ctx_->height;
  frame_->format = codec_ctx_->pix_fmt;
  if (av_frame_get_buffer(frame_, 0) != 0) {
    printf("av_frame_get_buffer failed\n");
    return false;
  }
  return true;
}

bool VideoMuxer::CreateScaler(int src_width, int src_height) {
  assert(codec_ctx_);

  scaler_ = sws_getCachedContext(
      scaler_, src_width, src_height, config_.input_pix_fmt, codec_ctx_->width,
      codec_ctx_->height, codec_ctx_->pix_fmt, SWS_BICUBIC, NULL, NULL, NULL);
  if (!scaler_) {
    printf("sws_getCachedContext failed\n");
    return false;
  }
  return true;
}

void VideoMuxer::Encode(AVFrame* frame) {
  int ret = avcodec_send_frame(codec_ctx_, frame);
  if (ret < 0) {
    printf("Error sending a frame for encoding.\n");
    exit(1);
  }

  while (ret >= 0) {
    ret = avcodec_receive_packet(codec_ctx_, packet_);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      return;
    } else if (ret < 0) {
      printf("Error during encoding.\n");
      exit(1);
    }

    av_packet_rescale_ts(packet_, codec_ctx_->time_base, stream_->time_base);
    packet_->stream_index = stream_->index;

    ret = av_interleaved_write_frame(format_ctx_, packet_);
    if (ret < 0) {
      printf("write frame failed: %d", ret);
      exit(1);
    }

    av_packet_unref(packet_);
  }
}
