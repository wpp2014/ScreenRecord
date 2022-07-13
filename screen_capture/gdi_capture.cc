﻿#include "screen_capture/gdi_capture.h"

#include <stdio.h>

#include "screen_capture/screen_picture.h"
#include "utils/virtual_screen.h"

GDICapture::GDICapture()
    : initialized_(false)
    , image_width_(0)
    , image_height_(0)
    , image_size_(0)
    , virtual_screen_rect_({0})
    , src_dc_(NULL)
    , mem_dc_(NULL)
    , bitmap_info_({0})
    , capture_bmp_(NULL) {
  Initialize();
}

GDICapture::~GDICapture() {
}

ScreenPicture* GDICapture::Capture() {
  if (!initialized_) {
    assert(0);
    return nullptr;
  }

  uint8_t* bitmap_data = nullptr;
  utils::ScopedBitmap bitmap(
      CreateDIBSection(src_dc_, &bitmap_info_, DIB_RGB_COLORS,
                       reinterpret_cast<void**>(&bitmap_data), NULL, 0));
  if (!bitmap) {
    return nullptr;
  }

  SelectObject(mem_dc_, bitmap);
  BOOL res = BitBlt(mem_dc_,
                    0, 0,
                    image_width_, image_height_,
                    src_dc_,
                    virtual_screen_rect_.left, virtual_screen_rect_.top,
                    SRCCOPY);
  if (!res) {
    return nullptr;
  }

  DrawMouse(mem_dc_);

  ScreenPicture* picture = new ScreenPicture;
  picture->width = image_width_;
  picture->height = image_height_;
  picture->size = image_size_;
  picture->argb = new uint8_t[image_size_];
  memcpy(picture->argb, bitmap_data, image_size_);

  return picture;
}

bool GDICapture::Initialize() {
  src_dc_ = CreateDC(L"DISPLAY", NULL, NULL, NULL);
  if (!src_dc_) {
    printf("Create DC by DISPLAY failed\n");
    return false;
  }

  mem_dc_ = CreateCompatibleDC(src_dc_);
  if (!mem_dc_) {
    printf("failed to create compatible DC\n");
    return false;
  }

  utils::VirtualScreen virtual_screen;
  virtual_screen_rect_ = virtual_screen.VirtualScreenRect();
  image_width_ = virtual_screen.VirtualScreenWidth();
  image_height_ = virtual_screen.VirtualScreenHeight();
  image_size_ = ((image_width_ * bit_count_ + 31) & ~31) / 8 * image_height_;

  bitmap_info_.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bitmap_info_.bmiHeader.biWidth = image_width_;
  bitmap_info_.bmiHeader.biHeight = -image_height_;
  bitmap_info_.bmiHeader.biPlanes = 1;
  bitmap_info_.bmiHeader.biBitCount = bit_count_;
  bitmap_info_.bmiHeader.biCompression = BI_RGB;

  initialized_ = true;
  return true;
}

void GDICapture::DrawMouse(HDC hdc) {
  POINT point;
  if (!GetCursorPos(&point)) {
    return;
  }

  CURSORINFO cursor_info;
  ZeroMemory(&cursor_info, sizeof(CURSORINFO));
  cursor_info.cbSize = sizeof(CURSORINFO);
  if (!GetCursorInfo(&cursor_info)) {
    return;
  }

  HCURSOR hcursor = cursor_info.hCursor;
  ICONINFO icon_info;
  if (GetIconInfo(hcursor, &icon_info)) {
    point.x -= icon_info.xHotspot;
    point.y -= icon_info.yHotspot;

    BITMAP cursor_bmp = {0};
    GetObject(icon_info.hbmColor, sizeof(BITMAP), &cursor_bmp);
    DrawIconEx(hdc, point.x, point.y, cursor_info.hCursor, cursor_bmp.bmWidth, cursor_bmp.bmHeight, 0, NULL, DI_NORMAL);

    if (icon_info.hbmMask) {
      DeleteObject(icon_info.hbmMask);
    }
    if (icon_info.hbmColor) {
      DeleteObject(icon_info.hbmColor);
    }
  } else {
    DrawIcon(hdc, point.x, point.y, hcursor);
  }
}
