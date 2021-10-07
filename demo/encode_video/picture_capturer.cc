#include "picture_capturer.h"

uint8_t* PictureCapturer::ARGBToRGB(const uint8_t* argb,
                                    int width,
                                    int height) {
  uint8_t* rgb = new uint8_t[width * height * 3];
  const int len = width * height;
  for (int j = 0; j < len; ++j) {
    rgb[3 * j] = argb[j * 4];
    rgb[3 * j + 1] = argb[j * 4 + 1];
    rgb[3 * j + 2] = argb[j * 4 + 2];
  }
  return rgb;
}
