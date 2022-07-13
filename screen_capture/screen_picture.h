#ifndef SCREEN_CAPTURE_SCREEN_PICTURE_H_
#define SCREEN_CAPTURE_SCREEN_PICTURE_H_

#include <stdint.h>

struct ScreenPicture {
  int32_t width = 0;
  int32_t height = 0;
  int32_t size = 0;         // width * height * 4
  uint8_t* argb = nullptr;

  ScreenPicture() {}
  ScreenPicture(const ScreenPicture& other);
  ScreenPicture(ScreenPicture&& other);
  ~ScreenPicture();
};

#endif  // SCREEN_CAPTURE_SCREEN_PICTURE_H_
