#include "screen_record/src/screen_recorder.h"

#include <cmath>

#include <QtCore/QDateTime>

#include "capturer/picture_capturer_d3d9.h"
#include "capturer/picture_capturer_dxgi.h"
#include "capturer/picture_capturer_gdi.h"
#include "capturer/voice_capturer.h"
#include "encoder/av_config.h"
#include "encoder/av_muxer.h"
#include "logger/logger.h"
#include "screen_record/src/argument.h"
#include "screen_record/src/util/time_helper.h"

namespace {

const char kFilter[] = "ScreenRecorder";

// 声道数
const uint16_t kChannels = 2;
// 采样率：每秒钟采集的样本的个数
const uint32_t kSamplesPerSec = 44100;
// 每个样本bit数
const uint16_t kBitsPerSample = 16;
// 声音格式
const uint16_t kFormatType = WAVE_FORMAT_PCM;

// 构造输出路径
std::string GenerateOutputPath(const std::string& output_dir,
                               const std::string& file_format) {
  std::string filename = QDateTime::currentDateTime()
                             .toString("yyyy-MM-dd-hh-mm-ss-zzz")
                             .toStdString();
  filename.append(".").append(file_format);

  std::string output_path;
  output_path.append(output_dir).append("/").append(filename);
  return output_path;
}

// 缩放比例
#if 0
double GetScale() {
  HWND hwnd = GetDesktopWindow();
  HMONITOR hmonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);

  MONITORINFOEX miex;
  miex.cbSize = sizeof(MONITORINFOEX);
  GetMonitorInfo(hmonitor, &miex);
  const double logical_width =
      static_cast<double>(miex.rcMonitor.right - miex.rcMonitor.left);

  DEVMODE dm;
  dm.dmSize = sizeof(DEVMODE);
  dm.dmDriverExtra = 0;
  EnumDisplaySettings(miex.szDevice, ENUM_CURRENT_SETTINGS, &dm);
  const double physical_width = (double)dm.dmPelsWidth;

  return physical_width / logical_width;
}
#endif

// 获取屏幕尺寸
void GetScreenSize(int* width, int* height) {
  DCHECK(width && height);

  HMONITOR monitor =
      MonitorFromWindow(GetDesktopWindow(), MONITOR_DEFAULTTOPRIMARY);
  DCHECK(monitor);

  MONITORINFO mi;
  ZeroMemory(&mi, sizeof(mi));
  mi.cbSize = sizeof(mi);

  BOOL res = GetMonitorInfo(monitor, &mi);
  DCHECK(res);

  *width = mi.rcMonitor.right - mi.rcMonitor.left;
  *height = mi.rcMonitor.bottom - mi.rcMonitor.top;
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

  voice_capturer_ = std::make_unique<VoiceCapturer>(
      kChannels, kSamplesPerSec, kBitsPerSample, kFormatType,
      [this](const uint8_t* data, int len) {
        handleVoiceDataCallback(data, len);
      });
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
  DCHECK(!capture_picture_thread_.joinable());
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

  voice_capturer_->Pause();
}

void ScreenRecorder::restartRecord() {
  Q_ASSERT(status_ == Status::PAUSE);
  status_ = Status::RECORDING;

  voice_capturer_->Pause();
}

void ScreenRecorder::run() {
  LOG_INFO(kFilter, "开始录屏");

  // 获取屏幕宽高
  int width = 0;
  int height = 0;
  GetScreenSize(&width, &height);

  AudioConfig audio_config;
  audio_config.channels = kChannels;
  audio_config.sample_rate = kSamplesPerSec;
  audio_config.sample_fmt = AV_SAMPLE_FMT_S16;
  audio_config.channel_layout = AV_CH_LAYOUT_STEREO;

  VideoConfig video_config;
  video_config.width = width;
  video_config.height = height;
  video_config.fps = fps_;
  video_config.input_pixel_format = AV_PIX_FMT_RGB32;
  video_config.codec_id = g_setting_manager->VideoCodecID();

  std::string file_format = g_setting_manager->FileFormat().toStdString();
  std::string filepath = GenerateOutputPath(output_dir_, file_format);
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

  while (true) {
    AVData* av_data = nullptr;
    if (!data_queue_.Pop(&av_data, abort_func_)) {
      break;
    }

    if (av_data->type == AVData::AUDIO) {
      av_muxer->EncodeAudioFrame(av_data->data, av_data->len);
    } else if (av_data->type == AVData::VIDEO) {
      int stride = av_data->len / av_data->height;
      int pts = std::llround(av_data->timestamp);
      av_muxer->EncodeVideoFrame(
          av_data->data, av_data->width, av_data->height, stride, pts);
    } else {
      Q_ASSERT(false);
    }

    delete av_data;
  }

  av_muxer.reset();

  if (status_ == Status::CANCELING) {
    on_recording_canceled_();
  } else {
    on_recording_completed_();
  }
  status_ = Status::STOPPED;

  capture_picture_thread_.join();

  LOG_INFO(kFilter, "结束录屏");
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
  // 开始录音
  voice_capturer_->Start();

  // 截屏的频率
  double interval = 1000.0 / fps;

  PictureCapturer* capturer = nullptr;
  QString capture_type = g_setting_manager->CaptureType();
  if (capture_type == QString("GDI")) {
    capturer = new PictureCapturerGdi();
  } else if (capture_type == QString("D3D9")) {
    capturer = new PictureCapturerD3D9();
  } else if (capture_type == QString("DXGI")) {
    capturer = new PictureCapturerDXGI();
  } else {
    CHECK(false) << "不支持的截屏方式";
  }

  const auto start_time = std::chrono::high_resolution_clock::now();
  auto start = start_time;
  auto end = start_time;

  auto t1 = std::chrono::high_resolution_clock::now();

  bool capture_result = true;

  uint32_t count = 0;
  uint64_t pts = 0;
  uint64_t pause_time = 0;
  while (true) {
    start = std::chrono::high_resolution_clock::now();
    pts = std::llround(
              std::chrono::duration<double, std::milli>(start - start_time)
                  .count()) -
          pause_time;

    if (abort_func_()) {
      break;
    }

    AVData* av_data = nullptr;
    if (!capturer->CaptureScreen(&av_data)) {
      capture_result = false;
      break;
    } else if (av_data) {
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
      std::this_thread::sleep_for(
          std::chrono::milliseconds(static_cast<int64_t>(sleep_time)));
    }

    // 暂停
    while (status_ == Status::PAUSE) {
      pause_time += 100;
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }

  char info[1024];
  memset(info, 0, 1024);

  if (capture_result) {
    auto t2 = std::chrono::high_resolution_clock::now();
    double diff =
        std::chrono::duration<double>(t2 - t1).count() - pause_time / 1000.0;

    // 结束录音
    voice_capturer_->Stop();

    sprintf(info, "截屏操作结束，耗时%.3f秒，截取%u帧，帧率: %.3f", diff, count,
            count / diff);
  } else {
    sprintf(info, "%s", "抓屏失败");

    // 结束录音
    voice_capturer_->Stop();

    on_recording_failed_();
  }

  delete capturer;

  // 队列发送通知，解决暂停录屏之后直接点击停止按钮，导致编码线程阻塞的问题
  data_queue_.Notify();

  LOG_INFO(kFilter, "%s", info);
}
