#ifndef PICTURE_QUEUE_H_
#define PICTURE_QUEUE_H_

#include <stdint.h>

#include <condition_variable>
#include <mutex>
#include <queue>

#include "picture_capturer.h"

template<uint32_t MAX_SIZE>
class PictureQueue {
 public:
  PictureQueue() : total_size_(0){ }

  template<typename T>
  bool Push(PictureCapturer::Picture* data, T abort_func) {
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

  template<typename T = std::false_type>
  bool Pop(PictureCapturer::Picture** data, T abort_func = T()) {
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
      PictureCapturer::Picture* data = queue_.front();
      if (data) {
        total_size_ -= data->len;
        delete data;
      }

      queue_.pop();
    }

    assert(total_size_ == 0);
  }

  void Notify() {
    cond_.notify_all();
  }

 private:
  bool IsFull() const {
    return total_size_ >= MAX_SIZE;
  }

  uint32_t total_size_;
  std::queue<PictureCapturer::Picture*> queue_;

  std::mutex mutex_;
  std::condition_variable cond_;

  PictureQueue(const PictureQueue&) = delete;
  PictureQueue& operator=(const PictureQueue&) = delete;
};  // class PictureQueue

#endif  // PICTURE_QUEUE_H_
