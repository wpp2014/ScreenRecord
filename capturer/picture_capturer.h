﻿#ifndef SCREEN_RECORD_SRC_CAPTURER_PICTURE_CAPTURER_H_
#define SCREEN_RECORD_SRC_CAPTURER_PICTURE_CAPTURER_H_

#include <windows.h>

#include "capturer/av_data.h"

class PictureCapturer {
 public:
  PictureCapturer() { }
  virtual ~PictureCapturer() { }

  virtual AVData* CaptureScreen() = 0;

 protected:
  void DrawMouseIcon(HDC hdc);
};  // class PictureCapturer

#endif  // SCREEN_RECORD_SRC_CAPTURER_PICTURE_CAPTURER_H_