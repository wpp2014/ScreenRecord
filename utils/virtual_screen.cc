#include "utils/virtual_screen.h"

#include <stdio.h>

namespace utils {

VirtualScreen::VirtualScreen()
    : is_valid_(false)
    , monitor_count_(0)
    , virtual_screen_width_(0)
    , virtual_screen_height_(0)
    , virtual_screen_rect_({0, 0, 0, 0}) {
  Initialize();
  if (!is_valid_)
    return;

  virtual_screen_width_ = virtual_screen_rect_.right - virtual_screen_rect_.left;
  virtual_screen_height_ = virtual_screen_rect_.bottom - virtual_screen_rect_.top;

  printf("virtual screen rect: (%d, %d, %d, %d)\n",
         virtual_screen_rect_.left, virtual_screen_rect_.top,
         virtual_screen_rect_.right, virtual_screen_rect_.bottom);
}

VirtualScreen::~VirtualScreen() {
}

// static
BOOL CALLBACK VirtualScreen::MonitorProc(HMONITOR monitor,
                                         HDC hdc,
                                         LPRECT rect,
                                         LPARAM param) {
  VirtualScreen* self = (VirtualScreen*)param;
  if (!self)
    return FALSE;
  return self->MonitorProc(monitor, hdc, rect);
}

BOOL VirtualScreen::MonitorProc(HMONITOR monitor, HDC, LPRECT) {
  MONITORINFOEX info;
  info.cbSize = sizeof(info);
  if (!::GetMonitorInfo(monitor, (LPMONITORINFO)&info)) {
    is_valid_ = false;
    return FALSE;
  }

  ++monitor_count_;

  RECT rect = info.rcMonitor;
  wprintf(L"device name: %ls, rect: (%d, %d, %d, %d)\n",
          info.szDevice,
          rect.left, rect.top, rect.right, rect.bottom);

  virtual_screen_rect_.left = std::min(virtual_screen_rect_.left, rect.left);
  virtual_screen_rect_.top = std::min(virtual_screen_rect_.top, rect.top);
  virtual_screen_rect_.right = std::max(virtual_screen_rect_.right, rect.right);
  virtual_screen_rect_.bottom = std::max(virtual_screen_rect_.bottom, rect.bottom);

  return TRUE;
}

bool VirtualScreen::Initialize() {
  is_valid_ = false;

  int monitor_count = 0;
  while (true) {
    if (!GetMonitorInfo(monitor_count)) {
      break;
    }
    ++monitor_count;
  }
  if (monitor_count == 0) {
    return false;
  }

  monitor_count_ = monitor_count;
  is_valid_ = true;

  return true;
}

bool VirtualScreen::GetMonitorInfo(int index) {
  DISPLAY_DEVICE device;
  ZeroMemory(&device, sizeof(device));
  device.cb = sizeof(device);

  BOOL res = EnumDisplayDevices(NULL, index, &device, 0);
  if (!res) {
    return false;
  }

  DEVMODE device_mode;
  ZeroMemory(&device_mode, sizeof(device_mode));
  device_mode.dmSize = sizeof(device_mode);

  res = EnumDisplaySettingsEx(device.DeviceName, ENUM_CURRENT_SETTINGS, &device_mode, 0);
  if (!res) {
    return false;
  }

  long left = device_mode.dmPosition.x;
  long top = device_mode.dmPosition.y;
  long right = left + device_mode.dmPelsWidth;
  long bottom = top + device_mode.dmPelsHeight;
  wprintf(L"device name: %ls, rect: (%d, %d, %d, %d)\n",
          device.DeviceName, left, top, right, bottom);

  virtual_screen_rect_.left = std::min(virtual_screen_rect_.left, left);
  virtual_screen_rect_.top = std::min(virtual_screen_rect_.top, top);
  virtual_screen_rect_.right = std::max(virtual_screen_rect_.right, right);
  virtual_screen_rect_.bottom = std::max(virtual_screen_rect_.bottom, bottom);

  return true;
}

}  // namespace utils
