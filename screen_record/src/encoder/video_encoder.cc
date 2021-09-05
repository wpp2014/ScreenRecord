#include "screen_record/src/encoder/video_encoder.h"

#include <QtCore/QDebug>

namespace {

AVFrame* CreateVideoFrame(AVPixelFormat pix_fmt, int width, int height) {
  AVFrame *video_frame = av_frame_alloc();
  if (!video_frame) {
    Q_ASSERT(false);
    return nullptr;
  }

  video_frame->format = pix_fmt;
  video_frame->width = width;
  video_frame->height = height;

  int ret = av_frame_get_buffer(video_frame, 32);
  if (ret < 0) {
    av_frame_free(&video_frame);
    return nullptr;
  }

  return video_frame;
}

void FreeBuffer(uint8_t* buffer) {
  Q_ASSERT(buffer);
  delete[] buffer;
}

void FreeAVFrame(AVFrame* frame) {
  if (frame) {
    av_frame_free(&frame);
  }
}

}  // namespace AVFrame

VideoEncoder::VideoEncoder(const VideoConfig& video_config)
    : initialized_(false),
      codec_(nullptr),
      codec_context_(nullptr),
      frame_(nullptr),
      sws_context_(nullptr),
      dict_(nullptr),
      input_pixel_format_(video_config.input_pixel_format),
      output_pixel_format_(AV_PIX_FMT_YUV420P),
      video_config_(video_config) {
}

VideoEncoder::~VideoEncoder() {
  if (codec_context_) {
    avcodec_free_context(&codec_context_);
    codec_context_ = nullptr;
  }

  if (frame_) {
    av_frame_free(&frame_);
    frame_ = nullptr;
  }

  if (dict_) {
    av_dict_free(&dict_);
  }

  if (sws_context_) {
    sws_freeContext(sws_context_);
  }
}

bool VideoEncoder::Initialize() {
  Q_ASSERT(!initialized_);

  const AVCodecID codec_id = AV_CODEC_ID_H264;
  codec_ = avcodec_find_encoder(codec_id);
  if (!codec_) {
    // DCHECK(false) << "Unable to find video encoder: AV_CODEC_ID_MPEG4";
    Q_ASSERT(false);
    return false;
  }

  codec_context_ = avcodec_alloc_context3(codec_);
  if (!codec_context_) {
    // DCHECK(false) << "Unable to alloc video codec context.";
    Q_ASSERT(false);
    return false;
  }

  codec_context_->codec_id = codec_id;
  codec_context_->pix_fmt = AV_PIX_FMT_YUV420P;
  codec_context_->codec_type = AVMEDIA_TYPE_VIDEO;
  codec_context_->framerate = {video_config_.fps, 1};
  codec_context_->time_base = {1, video_config_.fps};
  codec_context_->gop_size = 30;
  codec_context_->max_b_frames = 1;
  codec_context_->width = video_config_.width;
  codec_context_->height = video_config_.height;
  codec_context_->sample_aspect_ratio.num = 1;
  codec_context_->sample_aspect_ratio.den = 1;
  // 设置了这个标志不需要设置bit_rate
  codec_context_->flags |= AV_CODEC_FLAG_QSCALE;

  // x264需要设置这个
  // 限制码率因子 0~51 0为无损模式，23为缺省值，51可能是最差的。该数字越小，图像质量越好。
  av_opt_set(codec_context_->priv_data, "crf", "18", 0);

  av_opt_set(codec_context_->priv_data, "brand", "mp42", 0);
  av_opt_set(codec_context_->priv_data, "movflags", "disable_chpl", 0);

  if (codec_context_->codec_id == AV_CODEC_ID_H264) {
    // tune的参数主要配合视频类型和视觉优化的参数，或特别的情况
    // https://www.jianshu.com/p/b46a33dd958d
    av_opt_set(codec_context_->priv_data, "tune", "stillimage", 0);
    // 调节编码速度和质量的平衡，有10个选项：
    //   ultrafast、superfast、veryfast、faster、fast、medium、slow、slower、veryslow、placebo
    // 从快到慢
    av_opt_set(codec_context_->priv_data, "preset", "ultrafast", 0);
  }

  frame_ = CreateVideoFrame(codec_context_->pix_fmt,
                            codec_context_->width,
                            codec_context_->height);
  if (!frame_) {
    return false;
  }

  sws_context_ = CreateSoftwareScaler(
      input_pixel_format_, codec_context_->width, codec_context_->height,
      output_pixel_format_, codec_context_->width, codec_context_->height);
  if (!sws_context_) {
    return false;
  }

  initialized_ = true;
  return initialized_;
}

bool VideoEncoder::Open(AVStream* video_stream) {
  Q_ASSERT(initialized_);
  Q_ASSERT(video_stream);

  int ret = avcodec_open2(codec_context_, codec_, &dict_);
  if (ret < 0) {
    // DCHECK(false) << "Unable to open video encoder.";
    Q_ASSERT(false);
    return false;
  }

  ret = avcodec_parameters_from_context(video_stream->codecpar, codec_context_);
  if (ret < 0) {
    // DCHECK(false) << "Failed to copy avcodec parameters.";
    Q_ASSERT(false);
    return false;
  }

  return true;
}

int VideoEncoder::PushEncodeFrame(uint8_t* data,
                                  int len,
                                  int width,
                                  int height,
                                  int stride,
                                  int64_t time_stamp,
                                  AVFrame** encoded_frame) {
  Q_ASSERT(encoded_frame);

  int ret = 0;
  *encoded_frame = nullptr;
  if (data) {
    ret = av_frame_make_writable(frame_);
    if (ret < 0) {
      // DCHECK(false) << "Unable to make temp video frame writable";
      Q_ASSERT(false);
      return -1;
    }

    const uint8_t* src_data = data;
    const int src_width = width;
    const int src_height = height;

    const int dst_width = codec_context_->width;
    const int dst_height = codec_context_->height;
    const AVPixelFormat dst_pixel_format = codec_context_->pix_fmt;

    /* \todo fix hard coded src ptr and stride. */
    uint8_t* src[3] = {const_cast<uint8_t*>(src_data), nullptr, nullptr};
    int src_stride[1] = {stride};

    ret = sws_scale(sws_context_, src, src_stride, 0, src_height, frame_->data,
                    frame_->linesize);
    if (ret < 0) {
      // DCHECK(false) << "Error while converting video picture.";
      Q_ASSERT(false);
      return ret;
    }

    // frame_->pts = time_stamp;
    *encoded_frame = frame_;
  }

  return 0;
}

AVCodecContext* VideoEncoder::GetCodecContext() const {
  return codec_context_;
}

AVRational VideoEncoder::GetTimeBase() const {
  Q_ASSERT(codec_context_);
  return codec_context_->time_base;
}

SwsContext* VideoEncoder::CreateSoftwareScaler(
    AVPixelFormat src_pixel_format, int src_width, int src_height,
    AVPixelFormat dst_pixel_format, int dst_width, int dst_height) {
  SwsContext* software_scaler_context =
      sws_getContext(src_width, src_height, src_pixel_format,
                     dst_width, dst_height, dst_pixel_format,
                     SWS_BICUBIC, nullptr, nullptr, nullptr);
  if (!software_scaler_context) {
    // Q_ASSERT(false) << "Could not initialize the conversion context.";
    Q_ASSERT(false);
    return nullptr;
  }
  return software_scaler_context;
}
