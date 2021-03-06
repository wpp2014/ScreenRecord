#include "capturer/picture_capturer_gdi.h"

#include "base/check.h"

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
    : hwnd_(NULL),
      src_dc_(NULL),
      memory_dc_(NULL),
      bitmap_info_({}),
      bitmap_frame_(NULL),
      old_selected_bitmap_(NULL),
      bitmap_data_(nullptr) {
  hwnd_ = GetDesktopWindow();
  src_dc_ = GetDC(hwnd_);
  memory_dc_ = CreateCompatibleDC(src_dc_);

  // virtual_screen_rect_ = GetVirtualScreenRect();
  // width_ = virtual_screen_rect_.right - virtual_screen_rect_.left;
  // height_ = virtual_screen_rect_.bottom - virtual_screen_rect_.top;

  RECT rect;
  GetWindowRect(hwnd_, &rect);
  width_ = rect.right - rect.left;
  height_ = rect.bottom - rect.top;

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
    DCHECK(false);
  }
}

PictureCapturerGdi::~PictureCapturerGdi() {
  DeleteDC(memory_dc_);
  ReleaseDC(hwnd_, src_dc_);
}

bool PictureCapturerGdi::CaptureScreen(AVData** av_data) {
  DCHECK(av_data);

  old_selected_bitmap_ = SelectObject(memory_dc_, bitmap_frame_);

  BOOL res = BitBlt(memory_dc_,
                    0, 0,
                    width_, height_, src_dc_,
                    0, 0,
                    SRCCOPY);
  if (!res) {
    SelectObject(memory_dc_, old_selected_bitmap_);
    return false;
  }

  // 绘制鼠标
  DrawMouseIcon(memory_dc_);

  AVData* tmp = new AVData();
  tmp->type = AVData::VIDEO;
  tmp->len = width_ * height_ * 4;
  tmp->width = width_;
  tmp->height = height_;
  tmp->data = new uint8_t[tmp->len];
  memcpy(tmp->data, bitmap_data_, sizeof(uint8_t) * tmp->len);

  *av_data = tmp;

  SelectObject(memory_dc_, old_selected_bitmap_);
  return true;
}
