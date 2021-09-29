#include <stdio.h>

#include <chrono>
#include <iostream>
#include <memory>

#include "capturer/picture_capturer_d3d9.h"
#include "capturer/picture_capturer_gdi.h"

const int kTotalCount = 300;

int main() {
  int total_count = 0;

  std::unique_ptr<PictureCapturer> d3d9_capturer(new PictureCapturerD3D9());
  std::unique_ptr<PictureCapturer> gdi_capturer(new PictureCapturerGdi());

  int count = 0;
  auto start = std::chrono::high_resolution_clock::now();
  auto end = std::chrono::high_resolution_clock::now();
  double delta = 0.0;

  printf("使用D3D9截屏\n");
  while (total_count++ < kTotalCount) {
    AVData* data = d3d9_capturer->CaptureScreen();
    delete data;

    ++count;

    end = std::chrono::high_resolution_clock::now();
    delta = std::chrono::duration<double>(end - start).count();
    if (delta > 1) {
      std::cout << "  D3D9 FPS: " << count / delta << std::endl;
      count = 0;
      start = end;
    }
  }
  printf("D3D9截屏结束\n");

  total_count = 0;
  count = 0;
  start = std::chrono::high_resolution_clock::now();

  printf("使用GDI截屏\n");
  while (total_count++ < kTotalCount) {
    AVData* data = gdi_capturer->CaptureScreen();
    delete data;

    ++count;

    end = std::chrono::high_resolution_clock::now();
    delta = std::chrono::duration<double>(end - start).count();
    if (delta > 1) {
      std::cout << "  GDI FPS: " << count / delta << std::endl;
      count = 0;
      start = end;
    }
  }
  printf("GDI截屏结束\n");

  return 0;
}
