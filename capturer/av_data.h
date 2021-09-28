#ifndef CAPTURER_AV_DATA_H_
#define CAPTURER_AV_DATA_H_

#include <stdint.h>

struct AVData {
  enum Type {
    UNKNOWN = 0,
    AUDIO,
    VIDEO,
  };

  Type type;

  uint8_t* data;
  int len;
  int width;
  int height;

  uint64_t timestamp;

  AVData()
      : type(UNKNOWN),
        data(nullptr),
        len(0),
        width(0),
        height(0),
        timestamp(0) {}

  ~AVData() {
    if (data) {
      delete data;
      data = nullptr;
    }
  }
};  // struct AVData

#endif  // CAPTURER_AV_DATA_H_
