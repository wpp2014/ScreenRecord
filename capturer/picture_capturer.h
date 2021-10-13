#ifndef SCREEN_RECORD_SRC_CAPTURER_PICTURE_CAPTURER_H_
#define SCREEN_RECORD_SRC_CAPTURER_PICTURE_CAPTURER_H_

#include <windows.h>

#include "capturer/av_data.h"

class PictureCapturer {
 public:
  PictureCapturer() { }
  virtual ~PictureCapturer() { }

  // DXGI截屏会存在屏幕没有发生变化而不截屏的情况
  // 这种情况是正常的，但是av_data会为nullptr
  virtual bool CaptureScreen(AVData** av_data) = 0;

 protected:
  void DrawMouseIcon(HDC hdc);
};  // class PictureCapturer

#endif  // SCREEN_RECORD_SRC_CAPTURER_PICTURE_CAPTURER_H_
