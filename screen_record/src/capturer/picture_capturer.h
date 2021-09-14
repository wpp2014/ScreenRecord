#ifndef SCREEN_RECORD_SRC_CAPTURER_PICTURE_CAPTURER_H_
#define SCREEN_RECORD_SRC_CAPTURER_PICTURE_CAPTURER_H_

#include "screen_record/src/data_queue.h"

class PictureCapturer {
 public:
  PictureCapturer() { }
  virtual ~PictureCapturer() { }

  virtual AVData* CaptureScreen() = 0;
};  // class PictureCapturer

#endif  // SCREEN_RECORD_SRC_CAPTURER_PICTURE_CAPTURER_H_
