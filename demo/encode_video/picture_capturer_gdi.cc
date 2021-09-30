#include "picture_capturer_gdi.h"

#include <assert.h>

PictureCapturerGdi::PictureCapturerGdi(int type)
    : hwnd_(NULL),
      src_dc_(NULL),
      mem_dc_(NULL),
      bitmap_info_({}),
      bitmap_frame_(NULL),
      old_selected_bitmap_(NULL),
      bitmap_data_(nullptr),
      width_(0),
      height_(0),
      len_(0),
      bits_per_pixer_(0) {
  if (type == 0) {
    bits_per_pixer_ = 24;
  } else if (type == 1) {
    bits_per_pixer_ = 32;
  } else {
    assert(false);
  }

  hwnd_ = GetDesktopWindow();

  // ÆÁÄ»³ß´ç
  {
    HMONITOR monitor = MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST);
    MONITORINFOEX miex;
    miex.cbSize = sizeof(MONITORINFOEX);
    GetMonitorInfo(monitor, &miex);

    DEVMODE dm;
    dm.dmSize = sizeof(DEVMODE);
    dm.dmDriverExtra = 0;
    EnumDisplaySettings(miex.szDevice, ENUM_CURRENT_SETTINGS, &dm);

    width_ = dm.dmPelsWidth;
    height_ = dm.dmPelsHeight;
  }

  len_ = width_ * height_ * bits_per_pixer_ >> 3;
  bitmap_data_ = new uint8_t[len_];

  bitmap_info_.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bitmap_info_.bmiHeader.biWidth = width_;
  bitmap_info_.bmiHeader.biHeight = -height_;
  bitmap_info_.bmiHeader.biPlanes = 1;
  bitmap_info_.bmiHeader.biBitCount = bits_per_pixer_;
  bitmap_info_.bmiHeader.biCompression = BI_RGB;

  src_dc_ = GetDC(hwnd_);
  mem_dc_ = CreateCompatibleDC(src_dc_);
  bitmap_frame_ = CreateCompatibleBitmap(src_dc_, width_, height_);
  SelectObject(mem_dc_, bitmap_frame_);
}

PictureCapturerGdi::~PictureCapturerGdi() {
  DeleteObject(bitmap_frame_);
  DeleteObject(mem_dc_);
  ReleaseDC(hwnd_, src_dc_);

  delete[] bitmap_data_;
}

PictureCapturer::Picture* PictureCapturerGdi::Capture() {
  BitBlt(mem_dc_, 0, 0, width_, height_, src_dc_, 0, 0, SRCCOPY);
  GetDIBits(mem_dc_,
            bitmap_frame_,
            0, height_,
            bitmap_data_, &bitmap_info_,
            DIB_RGB_COLORS);

  Picture* picture = new Picture();
  picture->width = width_;
  picture->height = height_;
  picture->len = len_;
  if (bits_per_pixer_ == 24) {
    picture->rgb = new uint8_t[len_];
    memcpy(picture->rgb, bitmap_data_, len_);
  } else {
    picture->argb = new uint8_t[len_];
    memcpy(picture->argb, bitmap_data_, len_);
  }
  return picture;
}
