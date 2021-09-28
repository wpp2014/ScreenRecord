#ifndef SCREEN_RECORD_SRC_DATA_QUEUE_H_
#define SCREEN_RECORD_SRC_DATA_QUEUE_H_

#include <stdint.h>

#include <condition_variable>
#include <mutex>
#include <queue>

#include "glog/logging.h"

struct AVData;

template<uint32_t MAX_SIZE>
class DataQueue {
 public:
  DataQueue() : total_size_(0){ }

  template<typename T>
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

  template<typename T = std::false_type>
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

  void Notify() {
    cond_.notify_all();
  }

 private:
  bool IsFull() const {
    return total_size_ >= MAX_SIZE;
  }

  uint32_t total_size_;
  std::queue<AVData*> queue_;

  std::mutex mutex_;
  std::condition_variable cond_;

  DataQueue(const DataQueue&) = delete;
  DataQueue& operator=(const DataQueue&) = delete;
};  // class DataQueue

#endif  // SCREEN_RECORD_SRC_DATA_QUEUE_H_
