#include "screen_record/src/screen_recorder.h"

#include <cmath>

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
      fps_(0),
      on_recording_completed_(on_recording_completed),
      on_recording_canceled_(on_recording_canceled),
      on_recording_failed_(on_recording_failed) {
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
  qDebug() << QStringLiteral("开始编码");

  // 先抓取一张图片，获取宽和高
  std::unique_ptr<PictureCapturer> picture_capturer(new PictureCapturerD3D9());
  AVData* av_data = picture_capturer->CaptureScreen();
  if (!av_data) {
    on_recording_failed_();
    return;
  }

  AudioConfig audio_config;

  VideoConfig video_config;
  video_config.width = av_data->width;
  video_config.height = av_data->height;
  video_config.fps = fps_;
  video_config.input_pixel_format = AV_PIX_FMT_RGB32;

  picture_capturer.reset(nullptr);
  delete av_data;

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

  auto abort_func = [this]() {
    return status_ == Status::CANCELING ||
           status_ == Status::STOPPING ||
           status_ == Status::STOPPED;
  };

  while (true) {
    av_data = nullptr;
    if (!data_queue_.Pop(&av_data, abort_func)) {
      break;
    }

    int stride = av_data->len / av_data->height;
    int pts = std::llround(av_data->timestamp);
    av_muxer->EncodeVideoFrame(
        av_data->data, av_data->width, av_data->height, stride, pts);

    delete av_data;
  }

  av_muxer.reset();

  if (status_ == Status::CANCELING) {
    on_recording_canceled_();
  } else {
    on_recording_completed_();
  }
  status_ = Status::STOPPED;

  qDebug() << QStringLiteral("编码结束");

}

void ScreenRecorder::capturePictureThread(int fps) {
  double interval = 1000.0 / fps;

  std::unique_ptr<PictureCapturer> picture_capturer(new PictureCapturerD3D9());

  auto abort_func = [this]() {
    return status_ == Status::CANCELING ||
           status_ == Status::STOPPING ||
           status_ == Status::STOPPED;
  };

  auto start = std::chrono::high_resolution_clock::now();
  auto end = std::chrono::high_resolution_clock::now();

  auto t1 = std::chrono::high_resolution_clock::now();

  uint32_t count = 0;
  double pts = 0.0;
  while (true) {
    start = std::chrono::high_resolution_clock::now();

    if (status_ == Status::CANCELING ||
        status_ == Status::STOPPING ||
        status_ == Status::STOPPED) {
      break;
    }

    AVData* av_data = picture_capturer->CaptureScreen();
    if (!av_data) {
      qDebug() << QStringLiteral("截图失败");
    } else {
      av_data->timestamp = pts;
      if (!data_queue_.Push(av_data, abort_func)) {
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

  QString info = QString::asprintf(
      QStringLiteral("截屏操作结束，耗时%f秒，截取%u帧").toStdString().c_str(),
      diff, count);
  qDebug() << info;
}
