#include "screen_record/src/capturer/picture_capturer_gdi.h"

#include <QtCore/QDebug>

namespace {

RECT GetVirtualScreenRect() {
  const int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
  const int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);
  const int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
  const int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
  return { x, y, x + width, y + height };
}

}  // namespace

PictureCapturerGdi::PictureCapturerGdi()
    : src_dc_(NULL),
      memory_dc_(NULL),
      bitmap_info_({}),
      bitmap_frame_(NULL),
      old_selected_bitmap_(NULL),
      bitmap_data_(nullptr) {
  src_dc_ = GetDC(NULL);
  memory_dc_ = CreateCompatibleDC(src_dc_);

  virtual_screen_rect_ = GetVirtualScreenRect();
  width_ = virtual_screen_rect_.right - virtual_screen_rect_.left;
  height_ = virtual_screen_rect_.bottom - virtual_screen_rect_.top;

  bitmap_info_.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bitmap_info_.bmiHeader.biWidth = width_;
  bitmap_info_.bmiHeader.biHeight = -height_;
  bitmap_info_.bmiHeader.biPlanes = 1;
  bitmap_info_.bmiHeader.biBitCount = 32;
  bitmap_info_.bmiHeader.biCompression = BI_RGB;

  bitmap_frame_ =
      CreateDIBSection(src_dc_, &bitmap_info_, DIB_RGB_COLORS,
                       reinterpret_cast<void**>(&bitmap_data_), NULL, 0);
  if (!bitmap_frame_) {
    Q_ASSERT(false);
  }
}

PictureCapturerGdi::~PictureCapturerGdi() {
  DeleteDC(memory_dc_);
  ReleaseDC(NULL, src_dc_);
}

std::unique_ptr<PictureCapturer::Picture> PictureCapturerGdi::CaptureScreen() {
  old_selected_bitmap_ = SelectObject(memory_dc_, bitmap_frame_);

  BOOL res = BitBlt(memory_dc_,
                    virtual_screen_rect_.left, virtual_screen_rect_.top,
                    width_, height_, src_dc_,
                    virtual_screen_rect_.left, virtual_screen_rect_.top,
                    SRCCOPY | CAPTUREBLT);
  if (!res) {
    SelectObject(memory_dc_, old_selected_bitmap_);
    return nullptr;
  }

  std::unique_ptr<PictureCapturer::Picture> picture(
      new PictureCapturer::Picture());
  picture->Reset(bitmap_data_, width_, height_, width_ * 4);

  SelectObject(memory_dc_, old_selected_bitmap_);
  return std::move(picture);
} 
