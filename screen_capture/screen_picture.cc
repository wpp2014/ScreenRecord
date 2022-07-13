#include "screen_capture/screen_picture.h"

#include <assert.h>
#include <memory.h>

ScreenPicture::ScreenPicture(const ScreenPicture& other)
    : width(other.width)
    , height(other.height)
    , size(other.size) {
  assert(size > 0);

  argb = new uint8_t[size];
  memcpy(argb, other.argb, size);
}

ScreenPicture::ScreenPicture(ScreenPicture&& other)
    : width(other.width)
    , height(other.height)
    , size(other.size)
    , argb(other.argb) {
  other.argb = nullptr;
}

ScreenPicture::~ScreenPicture() {
  if (argb) {
    delete[] argb;
    argb = nullptr;
  }
  width = 0;
  height = 0;
  size = 0;
}
