#include "screen_capture/screen_picture.h"

#include <assert.h>
#include <memory.h>

ScreenPicture::ScreenPicture(const ScreenPicture& other)
    : width(other.width)
    , height(other.height)
    , size(other.size) {
  assert(size > 0);

  rgb = new uint8_t[size];
  memcpy(rgb, other.rgb, size);
}

ScreenPicture::ScreenPicture(ScreenPicture&& other)
    : width(other.width)
    , height(other.height)
    , size(other.size)
    , rgb(other.rgb) {
  other.rgb = nullptr;
}

ScreenPicture::~ScreenPicture() {
  if (rgb) {
    delete[] rgb;
    rgb = nullptr;
  }
  width = 0;
  height = 0;
  size = 0;
}
