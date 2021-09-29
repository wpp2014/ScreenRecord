#include <stdio.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

#include "capturer/picture_capturer_d3d9.h"
#include "capturer/picture_capturer_gdi.h"
#include "encoder/av_muxer.h"
#include "encoder/video_encoder.h"
#include "glog/logging.h"

const int kTotalCount = 300;
const int kMaxSize = 1024 * 1024 * 1024;
const int kFrameCount = 2048;
const int kFps = 30;

static PictureCapturer* g_picture_capturer = nullptr;
static std::atomic_bool g_exit = false;

template <uint32_t MAX_SIZE>
class DataQueue {
 public:
  DataQueue() : total_size_(0) {}

  template <typename T>
  bool Push(AVData* data, T abort_func) {
    bool was_empty = false;
    {
      std::unique_lock<std::mutex> locker(mutex_);
      while (total_size_ >= MAX_SIZE) {
        if (abort_func()) {
          return false;
        }
        cond_.wait(locker);
      }

      was_empty = queue_.empty();

      queue_.push(data);
      total_size_ += data->len;
    }

    if (was_empty) {
      cond_.notify_all();
    }

    return true;
  }

  template <typename T = std::false_type>
  bool Pop(AVData** data, T abort_func = T()) {
    bool was_full;
    {
      std::unique_lock<std::mutex> locker(mutex_);
      while (queue_.empty()) {
        if (abort_func()) {
          return false;
        }
        cond_.wait(locker);
      }

      was_full = total_size_ >= MAX_SIZE;

      *data = queue_.front();
      queue_.pop();
      total_size_ -= (*data)->len;
    }

    if (was_full) {
      cond_.notify_all();
    }

    return true;
  }

  void Clear() {
    std::unique_lock<std::mutex> locker(mutex_);
    while (!queue_.empty()) {
      AVData* data = queue_.front();
      if (data) {
        total_size_ -= data->len;
        delete data;
      }

      queue_.pop();
    }

    DCHECK(total_size_ == 0);
  }

  void Notify() { cond_.notify_all(); }

 private:
  bool IsFull() const { return total_size_ >= MAX_SIZE; }

  uint32_t total_size_;
  std::queue<AVData*> queue_;

  std::mutex mutex_;
  std::condition_variable cond_;

  DataQueue(const DataQueue&) = delete;
  DataQueue& operator=(const DataQueue&) = delete;
};  // class DataQueue

static DataQueue<kMaxSize> g_data_queue;

void CaptureThread() {
  double interval = 1000.0 / kFps;

  const auto start_time = std::chrono::high_resolution_clock::now();
  auto current_time = start_time;

  uint64_t pts = 0;
  int count = 0;
  while (count++ < kFrameCount) {
    current_time = std::chrono::high_resolution_clock::now();
    pts = std::llround(
        std::chrono::duration<double, std::milli>(current_time - start_time)
            .count());

    AVData* av_data = g_picture_capturer->CaptureScreen();
    av_data->timestamp = pts;

    auto abort_func = []() { return false; };
    g_data_queue.Push(av_data, abort_func);
  }

  const auto end_time = std::chrono::high_resolution_clock::now();
  double total_time =
      std::chrono::duration<double>(end_time - start_time).count();
  double fps = count / total_time;
  printf("耗时 %.3f 秒，FPS: %.3f\n", total_time, fps);

  g_exit = true;
}

int main() {
  g_picture_capturer = new PictureCapturerGdi();

  AVData* picture = g_picture_capturer->CaptureScreen();
  VideoConfig video_config;
  video_config.width = picture->width;
  video_config.height = picture->height;
  video_config.fps = kFps;
  video_config.input_pixel_format = AV_PIX_FMT_RGB32;

  delete picture;

  std::unique_ptr<AVMuxer> av_muxer =
      std::make_unique<AVMuxer>(AudioConfig(), video_config, "1.mp4", false);
  if (!av_muxer->Initialize()) {
    return 0;
  }
  if (!av_muxer->Open()) {
    return 0;
  }

  auto abort_func = []() {
    if (g_exit)
      return true;
    return false;
  };

  std::thread capture_thread = std::thread(CaptureThread);

  while (true) {
    if (!g_data_queue.Pop(&picture, abort_func)) {
      break;
    }

    int stride = picture->len / picture->height;
    av_muxer->EncodeVideoFrame(picture->data, picture->width, picture->height, stride, picture->timestamp);
    delete picture;
  }

  av_muxer.reset();
  printf("编码结束");

  capture_thread.join();

  return 0;
}
