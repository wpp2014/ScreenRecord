#ifndef PICTURE_CAPTURER_H_
#define PICTURE_CAPTURER_H_

#include <assert.h>
#include <stdint.h>

class PictureCapturer {
 public:
  struct Picture {
    uint8_t* rgb;
    uint8_t* argb;

    int width;
    int height;
    int len;

    double pts;

    Picture()
        : rgb(nullptr), argb(nullptr), width(0), height(0), len(0), pts(0.0) {}

    ~Picture() {
      assert(!(rgb && argb));

      if (rgb) {
        delete[] rgb;
        rgb = nullptr;
      }
      if (argb) {
        delete[] argb;
        argb = nullptr;
      }
    }
  };

  virtual ~PictureCapturer() { }

  virtual Picture* Capture() = 0;

  uint8_t* ARGBToRGB(const uint8_t* argb, int width, int height);
};

#endif  // PICTURE_CAPTURER_H_
