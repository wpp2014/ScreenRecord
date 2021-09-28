#include "encoder/av_muxer.h"

#include "glog/logging.h"

#include "encoder/audio_encoder.h"
#include "encoder/video_encoder.h"

#ifdef av_err2str
#undef av_err2str
#endif

namespace {

char* av_err2str(int errnum) {
  thread_local char str[AV_ERROR_MAX_STRING_SIZE]; 
  memset(str, 0, sizeof(str));
  return av_make_error_string(str, AV_ERROR_MAX_STRING_SIZE, errnum);
}

}  // namespace

AVMuxer::AVMuxer(const AudioConfig & audio_config,
                 const VideoConfig & video_config,
                 const std::string & output_path,
                 bool can_capture_voice)
    : initialized_(false),
      can_capture_voice_(can_capture_voice),
      audio_samples_(0),
      video_pts_(0),
      format_context_(nullptr),
      output_format_(nullptr),
      audio_config_(audio_config),
      audio_stream_(nullptr),
      video_config_(video_config),
      video_stream_(nullptr),
      output_path_(output_path),
      buffer_audio_frame_(nullptr),
      size_per_audio_frame_(0),
      last_index_(0) {
}

AVMuxer::~AVMuxer() {
  Flush();

  int ret = av_write_trailer(format_context_);
  DCHECK(ret == 0);

  audio_encoder_.reset(nullptr);
  video_encoder_.reset(nullptr);

  if (output_format_ && !(output_format_->flags & AVFMT_NOFILE) &&
      format_context_) {
    avio_closep(&format_context_->pb);
  }

  avformat_free_context(format_context_);

  format_context_ = nullptr;
  output_format_ = nullptr;

  if (buffer_audio_frame_) {
    delete[] buffer_audio_frame_;
    buffer_audio_frame_ = nullptr;
  }
}

bool AVMuxer::Initialize() {
  if (initialized_) {
    DCHECK(false);
    return true;
  }

  output_format_ = av_guess_format("mp4", NULL, NULL);
  if (!output_format_) {
    DCHECK(false);
    return false;
  }

  int ret = avformat_alloc_output_context2(
      &format_context_, output_format_, NULL, NULL);
  if (ret < 0) {
    DCHECK(false);
    return false;
  }

  video_encoder_.reset(new VideoEncoder(video_config_));
  if (!video_encoder_->Initialize()) {
    return false;
  }

  // 有可能没有录音设备
  audio_config_.codec_id = output_format_->audio_codec;
  audio_encoder_.reset(new AudioEncoder(audio_config_));
  can_capture_voice_ = audio_encoder_->Initialize();

  initialized_ = true;
  return true;
}

bool AVMuxer::Open() {
  if (!OpenVideo()) {
    return false;
  }

  can_capture_voice_ = OpenAudio();

  // 打开输出文件
  int ret = 0;
  if (!(output_format_->flags & AVFMT_NOFILE)) {
    ret = avio_open(
        &format_context_->pb, output_path_.c_str(), AVIO_FLAG_WRITE);
    if (ret < 0) {
      return false;
    }
  }

  ret = avformat_write_header(format_context_, NULL);
  if (ret < 0) {
    DCHECK(false);
    return false;
  }

  video_pts_ = 0;
  return true;
}

void AVMuxer::Flush() {
  if (can_capture_voice_) {
    EncodeAudioFrame(nullptr, 0);
  }
  EncodeVideoFrame(nullptr, 0, 0, 0, 0);
}

bool AVMuxer::EncodeAudioFrame(uint8_t* data, int len) {
  AVCodecContext* codec_ctx = audio_encoder_->GetCodecContext();

  if (!data || len <= 0) {
    return WriteFrame(format_context_, codec_ctx, audio_stream_, nullptr);
  }

  int index = 0;
  int copy_len = 0;
  int remaining_len = len;

  bool res = false;
  int ret = 0;
  AVFrame* encoded_frame = nullptr;
  while (remaining_len > 0) {
    DCHECK(last_index_ < size_per_audio_frame_);

    copy_len = size_per_audio_frame_ - last_index_;
    if (remaining_len < copy_len) {
      memcpy(buffer_audio_frame_ + last_index_, data + index, remaining_len);
      last_index_ += remaining_len;
      return true;
    }

    memcpy(buffer_audio_frame_ + last_index_, data + index, copy_len);
    ret = audio_encoder_->PushEncodeFrame(
        buffer_audio_frame_, size_per_audio_frame_, 0, 0, 0, 0, &encoded_frame);
    if (ret < 0) {
      return false;
    }

    encoded_frame->pts = av_rescale_q(
        audio_samples_, {1, codec_ctx->sample_rate}, codec_ctx->time_base);
    audio_samples_ += ret;

    DCHECK(encoded_frame);
    res = WriteFrame(format_context_, codec_ctx, audio_stream_, encoded_frame);
    if (!res) {
      return false;
    }

    index += copy_len;
    remaining_len -= copy_len;
    last_index_ = 0;
  }

#if 0
  while (remaining_len > 0) {
    if (last_index_ > 0) {
      Q_ASSERT(last_index_ < size_per_audio_frame_);

      copy_len = size_per_audio_frame_ - last_index_;
      if (remaining_len < copy_len) {
        copy_len = remaining_len;
      }
    } else {

    }

    if (remaining_len < size_per_audio_frame_) {
      memset(buffer_audio_frame_, 0, size_per_audio_frame_);
      memcpy(buffer_audio_frame_, data + index, remaining_len);
      last_index_ = remaining_len;
      return true;
    }

    ret = audio_encoder_->PushEncodeFrame(data + index, size_per_audio_frame_,
                                          0, 0, 0, 0, &encoded_frame);
    if (ret < 0) {
      return false;
    }

    encoded_frame->pts = av_rescale_q(
        audio_samples_, {1, codec_ctx->sample_rate}, codec_ctx->time_base);
    audio_samples_ += ret;

    Q_ASSERT(encoded_frame);
    res = WriteFrame(format_context_, codec_ctx, audio_stream_, encoded_frame);
    if (!res) {
      return false;
    }

    index += size_per_audio_frame_;
    remaining_len -= size_per_audio_frame_;
  }
#endif

#if 0
  const int buffer_size = audio_encoder_->src_buf_size();
  int index = 0;
  int copy_len = 0;
  int remaining_len = len;

  bool res = false;
  int ret = 0;
  AVFrame* encoded_frame = nullptr;
  while (remaining_len > 0) {
    Q_ASSERT(index < len);

    copy_len = remaining_len < buffer_size ? remaining_len : buffer_size;
    ret = audio_encoder_->PushEncodeFrame(data + index, copy_len, 0, 0, 0, 0,
                                          &encoded_frame);
    if (ret < 0) {
      return false;
    }

    encoded_frame->pts = av_rescale_q(
        audio_samples_, {1, codec_ctx->sample_rate}, codec_ctx->time_base);
    audio_samples_ += ret;

    Q_ASSERT(encoded_frame);
    res = WriteFrame(format_context_, codec_ctx, audio_stream_, encoded_frame);
    if (!res) {
      return false;
    }

    index += copy_len;
    remaining_len -= copy_len;
  }
#endif

  return true;
}

bool AVMuxer::EncodeVideoFrame(
    uint8_t* data, int width, int height, int stride, uint64_t time_stamp) {
  AVFrame* encoded_frame = nullptr;
  int ret = video_encoder_->PushEncodeFrame(
      data, height * stride, width, height, stride,
      static_cast<uint64_t>(time_stamp), &encoded_frame);
  if (ret < 0) {
    return false;
  }
  if (encoded_frame) {
    encoded_frame->pts = time_stamp;
  }

  AVCodecContext* ctx = video_encoder_->GetCodecContext();
  bool res = WriteFrame(format_context_, ctx, video_stream_, encoded_frame);

  video_pts_ += av_rescale_q(1, ctx->time_base, video_stream_->time_base);

  return res;
}

int AVMuxer::AudioFrameSize() const {
  DCHECK(audio_encoder_ && audio_encoder_.get());
  return audio_encoder_->FrameSize();
}

bool AVMuxer::OpenAudio() {
  if (!can_capture_voice_) {
    return false;
  }

  DCHECK(initialized_);
  DCHECK(audio_encoder_ && audio_encoder_.get());

  AVCodecContext* audio_codec_ctx = audio_encoder_->GetCodecContext();
  if (!audio_codec_ctx) {
    return false;
  }

  audio_stream_ = avformat_new_stream(format_context_, audio_codec_ctx->codec);
  if (!audio_stream_) {
    DCHECK(false);
    return false;
  }

  audio_stream_->id = format_context_->nb_streams - 1;
  audio_stream_->time_base = {1, audio_codec_ctx->sample_rate};

  if (format_context_->oformat->flags & AVFMT_GLOBALHEADER) {
    audio_codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  }

  if (!audio_encoder_->Open(audio_stream_)) {
    return false;
  }

  size_per_audio_frame_ = audio_encoder_->src_buf_size();
  buffer_audio_frame_ = new uint8_t[size_per_audio_frame_];

  return true;
}

bool AVMuxer::OpenVideo() {
  DCHECK(initialized_);
  DCHECK(video_encoder_ && video_encoder_.get());

  AVCodecContext* video_codec_ctx = video_encoder_->GetCodecContext();
  if (!video_codec_ctx) {
    return false;
  }

  video_stream_ = avformat_new_stream(format_context_, video_codec_ctx->codec);
  if (!video_stream_) {
    DCHECK(false);
    return false;
  }
  video_stream_->time_base = {1, 90000};
  video_stream_->id = format_context_->nb_streams - 1;

  if (format_context_->oformat->flags & AVFMT_GLOBALHEADER) {
    video_codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  }

  if (!video_encoder_->Open(video_stream_)) {
    return false;
  }

  return true;
}

bool AVMuxer::WriteFrame(AVFormatContext* format_ctx,
                         AVCodecContext* codec_ctx,
                         AVStream* stream,
                         AVFrame* encoded_frame) {
  DCHECK(format_ctx && codec_ctx && stream);

  int ret = avcodec_send_frame(codec_ctx, encoded_frame);
  if (ret < 0) {
    return false;
  }

  while (ret >= 0) {
    AVPacket pkt = { 0 };
    ret = avcodec_receive_packet(codec_ctx, &pkt);
    if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
      break;
    } else if (ret < 0) {
      return false;
    }

    av_packet_rescale_ts(&pkt, codec_ctx->time_base, stream->time_base);
    pkt.stream_index = stream->index;

    ret = av_interleaved_write_frame(format_ctx, &pkt);
    av_packet_unref(&pkt);

    if (ret < 0) {
      return false;
    }
  }

  return true;
}

int AVMuxer::WriteVideoFrame(
    const AVRational& time_base, AVStream* stream, AVPacket* pkt) {
  /* rescale output packet timestamp values from codec to stream timebase */
  av_packet_rescale_ts(pkt, time_base, stream->time_base);
  pkt->stream_index = stream->index;

  /* Write the compressed frame to the media file. */
  const int ret = av_interleaved_write_frame(format_context_, pkt);
  DCHECK(ret == 0);
  return ret;
}
