#include "screen_record/src/screen_recorder.h"

#include <QtCore/QDateTime>
#include <QtCore/QDebug>

#include "screen_record/src/capturer/picture_capturer_d3d9.h"
#include "screen_record/src/capturer/picture_capturer_gdi.h"
#include "screen_record/src/encoder/av_config.h"
#include "screen_record/src/encoder/av_muxer.h"
#include "screen_record/src/util/time_helper.h"

namespace {

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
      on_recording_completed_(on_recording_completed),
      on_recording_canceled_(on_recording_canceled),
      on_recording_failed_(on_recording_failed) {
}

ScreenRecorder::~ScreenRecorder() {}

void ScreenRecorder::startRecord(const QString& dir, int fps) {
  fps_ = fps;
  output_dir_ = dir.toStdString();

  // 将状态设置为正在录屏
  status_ = Status::RECORDING;

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
  qDebug() << QStringLiteral("开始录制");

  // 先抓取一张图片，获取宽和高
  std::unique_ptr<PictureCapturer> picture_capturer(new PictureCapturerD3D9());
  std::unique_ptr<PictureCapturer::Picture> picture =
      picture_capturer->CaptureScreen();
  if (!picture) {
    on_recording_failed_();
    return;
  }

  AudioConfig audio_config;

  VideoConfig video_config;
  video_config.width = picture->width();
  video_config.height = picture->height();
  video_config.fps = fps_;
  video_config.input_pixel_format = AV_PIX_FMT_RGB32;

  picture.reset(nullptr);

  std::string filepath = GenerateOutputPath(output_dir_);
  std::unique_ptr<AVMuxer> av_muxer =
      std::make_unique<AVMuxer>(audio_config, video_config, filepath, false);
  if (!av_muxer->Initialize()) {
    on_recording_failed_();
    return;
  }
  if (!av_muxer->Open()) {
    on_recording_failed_();
    return;
  }

  // 截取屏幕的时间间隔
  double interval = 1.0 / fps_;

  double start_time = 0.0;
  double end_time = 0.0;

  int64_t pts = 0;

  TimeHelper time_helper;
  while (status_ == Status::RECORDING) {
    start_time = time_helper.now();

    std::unique_ptr<PictureCapturer::Picture> frame =
        picture_capturer->CaptureScreen();
    av_muxer->EncodeVideoFrame(frame->data(), frame->width(),
                               frame->height(), frame->stride(), pts);

    end_time = time_helper.now();

    double usage_time = end_time - start_time;
    double sleep_time = interval - usage_time;
    if (sleep_time > 0.0) {
      std::this_thread::sleep_for(
          std::chrono::milliseconds(static_cast<int>(sleep_time * 1000.0)));
      pts += static_cast<int>(interval * 1000.0);
    } else {
      pts += static_cast<int>(usage_time * 1000.0);
    }

    while (status_ == Status::PAUSE) {
      std::this_thread::sleep_for(std::chrono::microseconds(100));
    }
  }

  av_muxer.reset();

  if (status_ == Status::CANCELING) {
    on_recording_canceled_();
  } else {
    on_recording_completed_();
  }
  status_ = Status::STOPPED;
}
