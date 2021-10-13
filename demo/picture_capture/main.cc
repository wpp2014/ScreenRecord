#include <assert.h>
#include <stdio.h>

#include <chrono>
#include <string>
#include <thread>

#include <objidl.h>
#include <gdiplus.h>
#include <shlwapi.h>

#include "capturer/picture_capturer_d3d9.h"

using namespace Gdiplus;

const int kPathLen = 1024;
const int kCount = 20;

static HWND g_hwnd = NULL;
static int g_width = 0;
static int g_height = 0;

static CLSID g_bmp_clsid;

int64_t GetCurrentMilliseconds() {
  auto now = std::chrono::system_clock::now();
  auto duration = now.time_since_epoch();
  int64_t milliseconds =
      std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
  return milliseconds;
}

uint8_t* ARGBToRGB(const uint8_t* argb, int width, int height) {
  // argb[0] -> b
  // argb[1] -> g
  // argb[2] -> r
  // argb[3] -> a
  // ......
  // rgb[0]  -> b
  // rgb[1]  -> g
  // rgb[2]  -> r
  // ......
  uint8_t* rgb = new uint8_t[width * height * 3];
  const int len = width * height;
  for (int j = 0; j < len; ++j) {
    rgb[3 * j] = argb[j * 4];
    rgb[3 * j + 1] = argb[j * 4 + 1];
    rgb[3 * j + 2] = argb[j * 4 + 2];
  }

  return rgb;
}

void SaveBMP(PixelFormat format,
             uint8_t* data,
             int width,
             int height,
             int len,
             const wchar_t* path) {
  Gdiplus::Bitmap bitmap(width, height, len / height, format, data);
  bitmap.Save(path, &g_bmp_clsid, NULL);
}

std::wstring GenerateOutDir() {
  const int len = 1024;
  wchar_t path[len];
  memset(path, 0, sizeof(wchar_t) * len);

  GetModuleFileName(NULL, path, len);
  PathRemoveFileSpec(path);

  PathAppend(path, L"out");
  DWORD fileattr = ::GetFileAttributes(path);
  if (fileattr != INVALID_FILE_ATTRIBUTES) {
    if ((fileattr & FILE_ATTRIBUTE_DIRECTORY) == 0) {
      assert(false);
    }
  } else {
    if (!CreateDirectory(path, NULL)) {
      assert(false);
    }
  }

  PathAppend(path, L"picture_capture");
  fileattr = ::GetFileAttributes(path);
  if (fileattr != INVALID_FILE_ATTRIBUTES) {
    if ((fileattr & FILE_ATTRIBUTE_DIRECTORY) == 0) {
      assert(false);
    }
  } else {
    if (!CreateDirectory(path, NULL)) {
      assert(false);
    }
  }

  wcsncat(path, L"\\", wcslen(L"\\"));
  return std::wstring(path);
}

void GdiCaptureRGB24(const std::wstring& out_dir) {
  HDC src_dc = GetWindowDC(g_hwnd);
  HDC mem_dc = CreateCompatibleDC(src_dc);

  const int bits_per_pixer = 24;
  const int len = g_width * g_height * bits_per_pixer >> 3;

  HBITMAP bitmap = CreateCompatibleBitmap(src_dc, g_width, g_height);
  SelectObject(mem_dc, bitmap);

  BITMAPINFO bitmap_info = {{sizeof(BITMAPINFOHEADER), g_width, -g_height, 1,
                             bits_per_pixer, 0, 0, 0, 0, 0, 0},
                            {}};

  wchar_t output[kPathLen];
  uint8_t* buffer = new uint8_t[len];

  int count = 0;
  while (count++ < kCount) {
    BitBlt(mem_dc, 0, 0, g_width, g_height, src_dc, 0, 0, SRCCOPY);
    GetDIBits(mem_dc, bitmap, 0, g_height, buffer, &bitmap_info,
              DIB_RGB_COLORS);

    memset(output, 0, sizeof(wchar_t) * kPathLen);
    wsprintf(output, L"%lsgdi_rgb24_%ls.bmp", out_dir.c_str(),
             std::to_wstring(GetCurrentMilliseconds()).c_str());
    SaveBMP(PixelFormat24bppRGB, buffer, g_width, g_height, len, output);

    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  DeleteObject(bitmap);
  DeleteObject(mem_dc);
  ReleaseDC(g_hwnd, src_dc);
}

void GdiCaptureRGB32(const std::wstring& out_dir) {
  HDC src_dc = GetWindowDC(g_hwnd);
  HDC mem_dc = CreateCompatibleDC(src_dc);

  const int bits_per_pixer = 32;
  const int len = g_width * g_height * bits_per_pixer >> 3;

  HBITMAP bitmap = CreateCompatibleBitmap(src_dc, g_width, g_height);
  SelectObject(mem_dc, bitmap);

  BITMAPINFO bitmap_info = {{sizeof(BITMAPINFOHEADER), g_width, -g_height, 1,
                             bits_per_pixer, 0, 0, 0, 0, 0, 0},
                            {}};

  wchar_t output[kPathLen];
  uint8_t* buffer = new uint8_t[len];

  int count = 0;
  while (count++ < kCount) {
    BitBlt(mem_dc, 0, 0, g_width, g_height, src_dc, 0, 0, SRCCOPY);
    GetDIBits(mem_dc, bitmap, 0, g_height, buffer, &bitmap_info, DIB_RGB_COLORS);

    memset(output, 0, sizeof(wchar_t) * kPathLen);
    wsprintf(output, L"%lsgdi_rgb32_%ls.bmp", out_dir.c_str(),
             std::to_wstring(GetCurrentMilliseconds()).c_str());
    SaveBMP(PixelFormat32bppARGB, buffer, g_width, g_height, len, output);

    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  DeleteObject(bitmap);
  DeleteObject(mem_dc);
  ReleaseDC(g_hwnd, src_dc);
}

void D3D9CaptureRGB24(const std::wstring& out_dir) {
  std::unique_ptr<PictureCapturer> capturer(new PictureCapturerD3D9());

  wchar_t output[kPathLen];
  int count = 0;

  while (count++ < kCount) {
    AVData* data = nullptr;
    if (!capturer->CaptureScreen(&data)) {
      printf("抓屏失败\n");
      break;
    }
    uint8_t* rgb = ARGBToRGB(data->data, data->width, data->height);

    wsprintf(output, L"%lsd3d9_rgb24_%ls.bmp", out_dir.c_str(),
             std::to_wstring(GetCurrentMilliseconds()).c_str());
    SaveBMP(PixelFormat24bppRGB, rgb, data->width, data->height,
            data->width * data->height * 3, output);

    delete data;
    delete rgb;
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

void D3D9CaptureRGB32(const std::wstring& out_dir) {
  std::unique_ptr<PictureCapturer> capturer(new PictureCapturerD3D9());

  wchar_t output[kPathLen];
  int count = 0;

  while (count++ < kCount) {
    AVData* data = nullptr;
    if (!capturer->CaptureScreen(&data)) {
      printf("抓屏失败\n");
      break;
    }

    wsprintf(output, L"%lsd3d9_rgb32_%ls.bmp", out_dir.c_str(),
             std::to_wstring(GetCurrentMilliseconds()).c_str());
    SaveBMP(PixelFormat32bppARGB, data->data, data->width, data->height,
            data->len, output);

    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
}

int main(int argc, char** argv) {
  GdiplusStartupInput gdiplus_startup_input;
  ULONG_PTR gdiplus_token;

  // 初始化GDI+
  GdiplusStartup(&gdiplus_token, &gdiplus_startup_input, NULL);

  // 屏幕尺寸
  g_hwnd = GetDesktopWindow();
  {
    HMONITOR monitor = MonitorFromWindow(g_hwnd, MONITOR_DEFAULTTONEAREST);

    MONITORINFOEX miex;
    miex.cbSize = sizeof(MONITORINFOEX);
    GetMonitorInfo(monitor, &miex);

    DEVMODE dm;
    dm.dmSize = sizeof(DEVMODE);
    dm.dmDriverExtra = 0;
    EnumDisplaySettings(miex.szDevice, ENUM_CURRENT_SETTINGS, &dm);

    g_width = dm.dmPelsWidth;
    g_height = dm.dmPelsHeight;
  }

  // jpg  {557cf401-1a04-11d3-9a73-0000f81ef32e}
  // bmp  {557cf400-1a04-11d3-9a73-0000f81ef32e}
  // png  {557cf406-1a04-11d3-9a73-0000f81ef32e}
  // gif  {557cf402-1a04-11d3-9a73-0000f81ef32e}
  // tif  {557cf405-1a04-11d3-9a73-0000f81ef32e}
  CLSIDFromString(L"{557cf400-1a04-11d3-9a73-0000f81ef32e}", &g_bmp_clsid);

  std::wstring out_dir = GenerateOutDir();
  std::thread capture_gdi_rgb24 = std::thread(GdiCaptureRGB24, out_dir);
  std::thread capture_gdi_rgb32 = std::thread(GdiCaptureRGB32, out_dir);
  std::thread capture_d3d9_rgb24 = std::thread(D3D9CaptureRGB24, out_dir);
  std::thread capture_d3d9_rgb32 = std::thread(D3D9CaptureRGB32, out_dir);

  capture_gdi_rgb24.join();
  capture_gdi_rgb32.join();
  capture_d3d9_rgb24.join();
  capture_d3d9_rgb32.join();

  GdiplusShutdown(gdiplus_token);
  return 0;
}
