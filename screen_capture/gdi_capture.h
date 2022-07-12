#ifndef SCREEN_CAPTURE_GDI_CAPTURE_H_
#define SCREEN_CAPTURE_GDI_CAPTURE_H_

#include <windows.h>

#include "screen_capture/screen_capture.h"
#include "utils/scoped_handle.h"

class GDICapture : public ScreenCapture {
 public:
  GDICapture();
  ~GDICapture() override;

  bool CanCapture() const { return initialized_; }

  ScreenPicture* Capture() override;

 private:
  bool Initialize();

  void DrawMouse(HDC hdc);

  bool initialized_;

  const int bit_count_ = 24;

  int image_width_;
  int image_height_;
  int image_size_;
  RECT virtual_screen_rect_;

  utils::ScopedHDC src_dc_;
  utils::ScopedHDC mem_dc_;
  BITMAPINFO bitmap_info_;
  utils::ScopedBitmap capture_bmp_;
};

#endif  // SCREEN_CAPTURE_GDI_CAPTURE_H_
