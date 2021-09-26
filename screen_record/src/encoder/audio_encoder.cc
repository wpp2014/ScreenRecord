#include "screen_record/src/encoder/audio_encoder.h"

#include "glog/logging.h"

AudioEncoder::AudioEncoder(const AudioConfig& audio_config)
    : initialized_(false),
      need_resample_(false),
      codec_(nullptr),
      codec_context_(nullptr),
      frame_(nullptr),
      resampler_(nullptr),
      config_(audio_config),
      src_buf_size_(0),
      src_data_(nullptr) {
}

AudioEncoder::~AudioEncoder() {
  if (src_data_) {
    av_freep(&src_data_[0]);
  }
  av_freep(&src_data_);
  src_data_ = nullptr;

  if (frame_) {
    av_frame_free(&frame_);
    frame_ = nullptr;
  }
  if (resampler_) {
    swr_free(&resampler_);
    resampler_ = nullptr;
  }
  if (codec_context_) {
    avcodec_free_context(&codec_context_);
    codec_context_ = nullptr;
    codec_ = nullptr;
  }
}

bool AudioEncoder::Initialize() {
  if (initialized_) {
    DCHECK(false);
    return true;
  }

  codec_ = avcodec_find_encoder(config_.codec_id);
  if (!codec_) {
    DCHECK(false);
    return false;
  }

  codec_context_ = avcodec_alloc_context3(codec_);
  if (!codec_context_) {
    DCHECK(false);
    return false;
  }

  codec_context_->bit_rate = 64000;
  codec_context_->sample_rate = config_.sample_rate;
  codec_context_->sample_fmt =
      codec_->sample_fmts ? codec_->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
  codec_context_->channel_layout = config_.channel_layout;
  codec_context_->channels =
      av_get_channel_layout_nb_channels(codec_context_->channel_layout);

  resampler_ = CreateResampler(
      codec_context_->sample_fmt, codec_context_->sample_rate,
      static_cast<int64_t>(codec_context_->channel_layout), config_.sample_fmt,
      config_.sample_rate, config_.channel_layout);
  if (need_resample_ && !resampler_) {
    return false;
  }

  initialized_ = true;
  return true;
}

bool AudioEncoder::Open(AVStream* audio_stream) {
  if (!initialized_ || !audio_stream) {
    DCHECK(false);
    return false;
  }

  int ret = avcodec_open2(codec_context_, codec_, NULL);
  if (ret < 0) {
    DCHECK(false);
    return false;
  }

  ret = avcodec_parameters_from_context(audio_stream->codecpar, codec_context_);
  if (ret < 0) {
    DCHECK(false);
    return false;
  }

  // 需要先调用avcodec_open2函数，
  // 因为如果不调用avcodec_open2，codec_context_->frame_size为0
  frame_ = CreateFrame(codec_context_->sample_fmt,
                       codec_context_->channels,
                       codec_context_->sample_rate);
  if (!frame_) {
    DCHECK(false);
    return false;
  }

  return true;
}

int AudioEncoder::PushEncodeFrame(uint8_t* data,
                                  int len,
                                  int width,
                                  int height,
                                  int stride,
                                  int64_t time_stamp,
                                  AVFrame** encoded_frame) {
  DCHECK(encoded_frame);

  const int nb_samples = len / 4;
  DCHECK(nb_samples == frame_->nb_samples);

  *encoded_frame = nullptr;
  if (data && nb_samples > 0) {
    DCHECK(src_buf_size_ >= len);

    int ret = av_frame_make_writable(frame_);
    if (ret < 0) {
      DCHECK(false);
      return ret;
    }

    memset(src_data_[0], 0, src_buf_size_);
    memcpy(src_data_[0], data, len);

    if (need_resample_) {
      ret = swr_convert(resampler_, frame_->data, nb_samples,
                        (const uint8_t**)src_data_, nb_samples);
      if (ret < 0) {
        DCHECK(false);
        return ret;
      }
    } else {
      frame_->data[0] = src_data_[0];
    }

    *encoded_frame = frame_;
  }

  return nb_samples;
}

AVCodecContext * AudioEncoder::GetCodecContext() const {
  return codec_context_;
}

int AudioEncoder::FrameSize() const {
  DCHECK(frame_);
  return frame_->nb_samples;
}

SwrContext* AudioEncoder::CreateResampler(AVSampleFormat dst_sample_fmt,
                                          int dst_sample_rate,
                                          int64_t dst_channel_layout,
                                          AVSampleFormat src_sample_fmt,
                                          int src_sample_rate,
                                          int64_t src_channel_layout) {
  if ((dst_sample_fmt != src_sample_fmt) ||
      (dst_sample_rate != src_sample_rate) ||
      (dst_channel_layout != src_channel_layout)) {
    need_resample_ = true;
  }

  if (!need_resample_) {
    return nullptr;
  }

  SwrContext* resampler = swr_alloc_set_opts(
      resampler_,
      dst_channel_layout, dst_sample_fmt, dst_sample_rate,
      src_channel_layout, src_sample_fmt, src_sample_rate,
      0, NULL);
  if (!resampler) {
    DCHECK(false);
    return nullptr;
  }

  int ret = swr_init(resampler);
  if (ret < 0) {
    swr_free(&resampler);
    return nullptr;
  }

  return resampler;
}

AVFrame* AudioEncoder::CreateFrame(
    AVSampleFormat sample_fmt, int channels, int sample_rate) {
  if (!initialized_) {
    DCHECK(false);
    return nullptr;
  }

  int nb_samples = 0;
  if (codec_->capabilities & AV_CODEC_CAP_VARIABLE_FRAME_SIZE) {
    nb_samples = 100000;
  } else if (codec_context_->frame_size > 0) {
    nb_samples = codec_context_->frame_size;
  } else {
    nb_samples = 1024;
  }

  AVFrame* frame = av_frame_alloc();
  if (!frame) {
    return nullptr;
  }

  frame->nb_samples = nb_samples;
  frame->format = sample_fmt;
  frame->channels = channels;
  frame->sample_rate = sample_rate;

  int ret = 0;
  if (nb_samples > 0) {
    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
      return nullptr;
    }
  }

  ret = av_samples_alloc_array_and_samples(
      &src_data_, &src_buf_size_, config_.channels, nb_samples,
      config_.sample_fmt, 0);
  if (ret < 0) {
    return nullptr;
  }

  return frame;
}
