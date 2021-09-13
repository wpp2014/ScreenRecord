#include <chrono>
#include <iostream>

#include <windows.h>

int main() {
  HWND hwnd = GetDesktopWindow();
  HDC src_hdc = GetWindowDC(hwnd);
  HDC mem_hdc = CreateCompatibleDC(src_hdc);

  RECT rect;
  GetWindowRect(hwnd, &rect);
  const int width = rect.right - rect.left;
  const int height = rect.bottom - rect.top;

  HBITMAP bitmap = CreateCompatibleBitmap(src_hdc, width, height);
  SelectObject(mem_hdc, bitmap);

  BITMAPINFO bitmap_info = {
      {sizeof(BITMAPINFOHEADER), width, height, 1, 24, 0, 0, 0, 0, 0, 0}, {}};

  int count = 0;
  auto start = std::chrono::high_resolution_clock::now();

  const int len = width * height * 3;
  uint8_t* rgb = new uint8_t[len];
  memset(rgb, 0, sizeof(uint8_t) * len);

  while (true) {
    BitBlt(mem_hdc, 0, 0, width, height, src_hdc, 0, 0, SRCCOPY);
    GetDIBits(mem_hdc, bitmap, 0, height, rgb, &bitmap_info, DIB_RGB_COLORS);
    memset(rgb, 0, sizeof(uint8_t) * len);
    ++count;

    auto end = std::chrono::high_resolution_clock::now();
    auto delta = std::chrono::duration<double>(end - start).count();
    if (delta > 1) {
      std::cout << "FPS: " << count / delta << std::endl;
      count = 0;
      start = end;
    }
  }

  DeleteObject(bitmap);
  DeleteObject(mem_hdc);
  ReleaseDC(hwnd, src_hdc);

  delete[] rgb;
  return 0;
}
