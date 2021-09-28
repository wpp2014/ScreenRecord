#ifndef SCREEN_RECORD_SRC_SCREEN_RECORDER_H_
#define SCREEN_RECORD_SRC_SCREEN_RECORDER_H_

#include <atomic>
#include <functional>
#include <thread>

#include <QtCore/QThread>

#include "screen_record/src/data_queue.h"

const uint32_t kMaxSize = 1024 * 1024 * 1024;

class VoiceCapturer;

class ScreenRecorder : public QThread {
 public:
  enum class Status {
    RECORDING = 0,
    PAUSE,
    CANCELING,
    STOPPING,
    STOPPED,
  };

  // on_recording_completed: 录屏成功的回调函数
  // on_recording_canceled: 取消录屏的回调函数
  // on_recording_failed: 录屏失败的回调函数
  // 有可能在别的线程里调用这些函数
  ScreenRecorder(const std::function<void()>& on_recording_completed,
                 const std::function<void()>& on_recording_canceled,
                 const std::function<void()>& on_recording_failed);
  ~ScreenRecorder();

  // 开始录屏
  // 这个函数是异步操作，会创建一个线程来执行录屏操作
  // 参数：
  //  dir: 视频保存路径
  //  fps: 帧率
  void startRecord(const QString& dir, int fps);
  // 停止录屏
  void stopRecord();
  // 取消录屏
  void cancelRecord();
  // 暂停录屏
  void pauseRecord();
  // 重新开始录屏
  void restartRecord();

 private:
  // 录屏线程
  void run() override;

  // 处理声音数据的回调函数
  void handleVoiceDataCallback(const uint8_t* data, int len);

  // 截屏线程
  void capturePictureThread(int fps);

  // 当前状态
  std::atomic<Status> status_;

  // 帧率
  int fps_;

  // 保存路径
  std::string output_dir_;

  DataQueue<kMaxSize> data_queue_;
  std::thread capture_picture_thread_;

  std::unique_ptr<VoiceCapturer> voice_capturer_;

  std::function<bool()> abort_func_;

  std::function<void()> on_recording_completed_;
  std::function<void()> on_recording_canceled_;
  std::function<void()> on_recording_failed_;

  ScreenRecorder() = delete;
  ScreenRecorder(const ScreenRecorder&) = delete;
  ScreenRecorder& operator=(const ScreenRecorder&) = delete;
};  // class ScreenRecorder

#endif  // SCREEN_RECORD_SRC_SCREEN_RECORDER_H_
