#include <chrono>
#include <iostream>
#include <thread>

#include "capturer/picture_capturer_d3d9.h"
#include "capturer/picture_capturer_dxgi.h"
#include "capturer/picture_capturer_gdi.h"

const double kSeconds = 10.0;

void CaptureGDI() {
  std::cout << "GDI×¥ÆÁ" << std::endl;

  std::unique_ptr<PictureCapturer> capturer(new PictureCapturerGdi());

  int count = 0;
  auto start_time = std::chrono::high_resolution_clock::now();
  auto start = start_time;
  auto end = start;
  double seconds = 0.0;
  double delta = 0.0;

  while (true) {
    AVData* av_data = nullptr;
    if (!capturer->CaptureScreen(&av_data)) {
      std::cout << "  ×¥ÆÁÊ§°Ü" << std::endl;
      break;
    }
    if (av_data) {
      delete av_data;
      av_data = nullptr;
    }

    ++count;
    end = std::chrono::high_resolution_clock::now();
    delta = std::chrono::duration<double>(end - start_time).count();
    if (delta > kSeconds) {
      break;
    }

    delta = std::chrono::duration<double>(end - start).count();
    if (delta > 1) {
      std::cout << "  FPS: " << count / delta << std::endl;
      count = 0;
      start = end;
    }
  }
}

void CaptureD3D9() {
  std::cout << "D3D9×¥ÆÁ" << std::endl;

  std::unique_ptr<PictureCapturer> capturer(new PictureCapturerD3D9());

  int count = 0;
  auto start_time = std::chrono::high_resolution_clock::now();
  auto start = start_time;
  auto end = start;
  double seconds = 0.0;
  double delta = 0.0;

  while (true) {
    AVData* av_data = nullptr;
    if (!capturer->CaptureScreen(&av_data)) {
      std::cout << "  ×¥ÆÁÊ§°Ü" << std::endl;
      break;
    }
    if (av_data) {
      delete av_data;
      av_data = nullptr;
    }

    ++count;
    end = std::chrono::high_resolution_clock::now();
    delta = std::chrono::duration<double>(end - start_time).count();
    if (delta > kSeconds) {
      break;
    }

    delta = std::chrono::duration<double>(end - start).count();
    if (delta > 1) {
      std::cout << "  FPS: " << count / delta << std::endl;
      count = 0;
      start = end;
    }
  }
}

void CaptureDXGI() {
  std::cout << "DXGI×¥ÆÁ" << std::endl;

  std::unique_ptr<PictureCapturer> capturer(new PictureCapturerDXGI());

  int count = 0;
  auto start_time = std::chrono::high_resolution_clock::now();
  auto start = start_time;
  auto end = start;
  double seconds = 0.0;
  double delta = 0.0;

  while (true) {
    AVData* av_data = nullptr;
    if (!capturer->CaptureScreen(&av_data)) {
      std::cout << "  ×¥ÆÁÊ§°Ü" << std::endl;
      break;
    }
    if (av_data) {
      delete av_data;
      av_data = nullptr;
    }

    ++count;

    end = std::chrono::high_resolution_clock::now();
    delta = std::chrono::duration<double>(end - start_time).count();
    if (delta > kSeconds) {
      break;
    }

    delta = std::chrono::duration<double>(end - start).count();
    if (delta > 1) {
      std::cout << "  FPS: " << count / delta << std::endl;
      count = 0;
      start = end;
    }
  }
}

int main() {
  CaptureGDI();
  CaptureD3D9();
  CaptureDXGI();

  return 0;
}
