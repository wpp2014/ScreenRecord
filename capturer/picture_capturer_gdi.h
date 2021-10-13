#ifndef CAPTURER_PICTURE_CAPTURER_GDI_H_
#define CAPTURER_PICTURE_CAPTURER_GDI_H_

#include <windows.h>

#include "capturer/picture_capturer.h"

class PictureCapturerGdi : public PictureCapturer {
 public:
  PictureCapturerGdi();
  ~PictureCapturerGdi() override;

  bool CaptureScreen(AVData** av_data) override;

 private:
  HWND hwnd_;
  HDC src_dc_;
  HDC memory_dc_;

  BITMAPINFO bitmap_info_;
  HBITMAP bitmap_frame_;
  HGDIOBJ old_selected_bitmap_;

  uint8_t* bitmap_data_;

  RECT virtual_screen_rect_;
  int width_;
  int height_;

  PictureCapturerGdi(const PictureCapturerGdi&) = delete;
  PictureCapturerGdi& operator=(const PictureCapturerGdi&) = delete;
};  // class PictureCapturerGdi

#endif  // CAPTURER_PICTURE_CAPTURER_GDI_H_
