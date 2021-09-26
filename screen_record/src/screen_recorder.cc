﻿#include "screen_record/src/screen_recorder.h"

#include <cmath>

#include <QtCore/QDateTime>

#include "glog/logging.h"
#include "screen_record/src/capturer/picture_capturer_d3d9.h"
#include "screen_record/src/capturer/picture_capturer_gdi.h"
#include "screen_record/src/capturer/voice_capturer.h"
#include "screen_record/src/encoder/av_config.h"
#include "screen_record/src/encoder/av_muxer.h"
#include "screen_record/src/util/time_helper.h"

namespace {

// 声道数
const uint16_t kChannels = 2;
// 采样率：每秒钟采集的样本的个数
const uint32_t kSamplesPerSec = 44100;
// 每个样本bit数
const uint16_t kBitsPerSample = 16;
// 声音格式
const uint16_t kFormatType = WAVE_FORMAT_PCM;

// 构造输出路径
std::string GenerateOutputPath(const std::string& output_dir) {
  std::string filename = QDateTime::currentDateTime()
                             .toString("yyyy-MM-dd hh.mm.ss.zzz")
                             .toStdString();
  filename.append(".mp4");

  std::string output_path;
  output_path.append(output_dir).append("/").append(filename);
  return output_path;
}

};  // namespace

ScreenRecorder::ScreenRecorder(
    const std::function<void()>& on_recording_completed,
    const std::function<void()>& on_recording_canceled,
    const std::function<void()>& on_recording_failed)
    : status_(Status::STOPPED),
      fps_(0),
      on_recording_completed_(on_recording_completed),
      on_recording_canceled_(on_recording_canceled),
      on_recording_failed_(on_recording_failed) {
  abort_func_ = [this]() {
    return status_ == Status::CANCELING ||
           status_ == Status::STOPPING ||
           status_ == Status::STOPPED;
  };
}

ScreenRecorder::~ScreenRecorder() {
  if (capture_picture_thread_.joinable()) {
    capture_picture_thread_.join();
  }
  if (isRunning()) {
    wait(30000);
  }
}

void ScreenRecorder::startRecord(const QString& dir, int fps) {
  fps_ = fps;
  Q_ASSERT(fps_ > 0);

  output_dir_ = dir.toStdString();

  // 将状态设置为正在录屏
  status_ = Status::RECORDING;

  data_queue_.Clear();

  // 开启截屏线程
  Q_ASSERT(!capture_picture_thread_.joinable());
  capture_picture_thread_ =
      std::thread(&ScreenRecorder::capturePictureThread, this, fps_);

  start();
}

void ScreenRecorder::stopRecord() {
  status_ = Status::STOPPING;
}

void ScreenRecorder::cancelRecord() {
  status_ = Status::CANCELING;
}

void ScreenRecorder::pauseRecord() {
  Q_ASSERT(status_ == Status::RECORDING);
  status_ = Status::PAUSE;
}

void ScreenRecorder::restartRecord() {
  Q_ASSERT(status_ == Status::PAUSE);
  status_ = Status::RECORDING;
}

void ScreenRecorder::run() {
  LOG(INFO) << "开始录屏";

  // 先抓取一张图片，获取宽和高
  std::unique_ptr<PictureCapturer> picture_capturer(new PictureCapturerD3D9());
  AVData* av_data = picture_capturer->CaptureScreen();
  if (!av_data) {
    on_recording_failed_();
    return;
  }

  AudioConfig audio_config;
  audio_config.channels = kChannels;
  audio_config.sample_rate = kSamplesPerSec;
  audio_config.sample_fmt = AV_SAMPLE_FMT_S16;
  audio_config.channel_layout = AV_CH_LAYOUT_STEREO;

  VideoConfig video_config;
  video_config.width = av_data->width;
  video_config.height = av_data->height;
  video_config.fps = fps_;
  video_config.input_pixel_format = AV_PIX_FMT_RGB32;

  picture_capturer.reset(nullptr);
  delete av_data;

  std::string filepath = GenerateOutputPath(output_dir_);
  std::unique_ptr<AVMuxer> av_muxer =
      std::make_unique<AVMuxer>(audio_config, video_config, filepath, true);
  if (!av_muxer->Initialize()) {
    on_recording_failed_();
    return;
  }
  if (!av_muxer->Open()) {
    on_recording_failed_();
    return;
  }

  // 启动录音
  std::unique_ptr<VoiceCapturer> voice_capturer =
      std::make_unique<VoiceCapturer>(kChannels, kSamplesPerSec, kBitsPerSample,
                                      kFormatType,
                                      [this](const uint8_t* data, int len) {
                                        handleVoiceDataCallback(data, len);
                                      });
  if (voice_capturer->Initialize() == 0) {
    if (!voice_capturer->Start()) {
      LOG(ERROR) << "录音失败";
    }
  }

  while (true) {
    av_data = nullptr;
    if (!data_queue_.Pop(&av_data, abort_func_)) {
      break;
    }

    if (av_data->type == AVData::AUDIO) {
      av_muxer->EncodeAudioFrame(av_data->data, av_data->len);
    } else if (av_data->type == AVData::VIDEO) {
      int stride = av_data->len / av_data->height;
      int pts = std::llround(av_data->timestamp);
      av_muxer->EncodeVideoFrame(av_data->data, av_data->width, av_data->height,
                                 stride, pts);
    } else {
      Q_ASSERT(false);
    }

    delete av_data;
  }

  voice_capturer->Stop();

  av_muxer.reset();

  if (status_ == Status::CANCELING) {
    on_recording_canceled_();
  } else {
    on_recording_completed_();
  }
  status_ = Status::STOPPED;

  LOG(INFO) << "结束录屏";
}

void ScreenRecorder::handleVoiceDataCallback(const uint8_t* data, int len) {
  Q_ASSERT(data && len > 0);

  AVData* av_data = new AVData();
  av_data->type = AVData::AUDIO;
  av_data->len = len;
  av_data->data = new uint8_t[len];
  memcpy(av_data->data, data, len);

  if (!data_queue_.Push(av_data, abort_func_)) {
    delete av_data;
  }
}

void ScreenRecorder::capturePictureThread(int fps) {
  double interval = 1000.0 / fps;

  std::unique_ptr<PictureCapturer> picture_capturer(new PictureCapturerD3D9());

  const auto start_time = std::chrono::high_resolution_clock::now();
  auto start = start_time;
  auto end = start_time;

  auto t1 = std::chrono::high_resolution_clock::now();

  uint32_t count = 0;
  uint64_t pts = 0;
  while (true) {
    start = std::chrono::high_resolution_clock::now();
    pts = std::llround(
        std::chrono::duration<double, std::milli>(start - start_time).count());

    if (abort_func_()) {
      break;
    }

    AVData* av_data = picture_capturer->CaptureScreen();
    if (!av_data) {
      LOG(ERROR) << "截屏失败";
    } else {
      av_data->timestamp = pts;
      if (!data_queue_.Push(av_data, abort_func_)) {
        delete av_data;
        break;
      }
    }

    ++count;

    end = std::chrono::high_resolution_clock::now();

    // 计算时间差(毫秒)
    double diff =
        std::chrono::duration<double, std::milli>(end - start).count();
    double sleep_time = interval - diff;
    if (sleep_time > 0) {
      pts += interval;
      std::this_thread::sleep_for(
          std::chrono::milliseconds(static_cast<int64_t>(sleep_time)));
    } else {
      pts += diff;
    }
  }

  auto t2 = std::chrono::high_resolution_clock::now();
  double diff = std::chrono::duration<double>(t2 - t1).count();

  char info[1024];
  memset(info, 0, 1024);
  sprintf(info, "截屏操作结束，耗时%.3f秒，截取%u帧", diff, count);
  LOG(INFO) << info;
}
