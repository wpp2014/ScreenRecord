#ifndef SCREEN_CAPTURE_SCREEN_CAPTURE_H_
#define SCREEN_CAPTURE_SCREEN_CAPTURE_H_

struct ScreenPicture;

class ScreenCapture {
 public:
  ScreenCapture() {}
  virtual ~ScreenCapture() {}

  virtual ScreenPicture* Capture() = 0;
};

#endif  // SCREEN_CAPTURE_SCREEN_CAPTURE_H_
