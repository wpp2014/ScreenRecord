#ifndef PICTURE_CAPTURE_GDI_H_
#define PICTURE_CAPTURE_GDI_H_

#include <windows.h>

#include "picture_capturer.h"

class PictureCapturerGdi : public PictureCapturer {
 public:
  // 0: rgb, 1: argb
  explicit PictureCapturerGdi(int type);
  ~PictureCapturerGdi() override;

  Picture* Capture() override;

 private:
  HWND hwnd_;
  HDC src_dc_;
  HDC mem_dc_;

  BITMAPINFO bitmap_info_;
  HBITMAP bitmap_frame_;
  HGDIOBJ old_selected_bitmap_;

  uint8_t* bitmap_data_;

  int width_;
  int height_;
  int len_;
  int bits_per_pixer_;

  PictureCapturerGdi(const PictureCapturerGdi&) = delete;
  PictureCapturerGdi& operator=(const PictureCapturerGdi&) = delete;
};

#endif  // PICTURE_CAPTURE_GDI_H_
