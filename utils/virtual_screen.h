#ifndef UTILS_VIRTUAL_SCREEN_H_
#define UTILS_VIRTUAL_SCREEN_H_

#include <windows.h>

#include <vector>

namespace utils {

class VirtualScreen {
 public:
  VirtualScreen();
  ~VirtualScreen();

  bool IsValid() const { return is_valid_; }

  int MonitorCount() const { return monitor_count_; }

  int VirtualScreenWidth() const { return virtual_screen_width_; }
  int VirtualScreenHeight() const { return virtual_screen_height_; }
  RECT VirtualScreenRect() const { return virtual_screen_rect_; }

 private:
  static BOOL CALLBACK MonitorProc(HMONITOR monitor,
                                   HDC hdc,
                                   LPRECT rect,
                                   LPARAM param);

  BOOL MonitorProc(HMONITOR monitor, HDC hdc, LPRECT rect);

  bool Initialize();

  bool GetMonitorInfo(int index);

  bool is_valid_;

  // 显示器数量
  int monitor_count_;

  int virtual_screen_width_;
  int virtual_screen_height_;
  RECT virtual_screen_rect_;

  // std::vector<MONITORINFOEX> monitors_;
};

}  // namespace utils

#endif  // UTILS_VIRTUAL_SCREEN_H_
