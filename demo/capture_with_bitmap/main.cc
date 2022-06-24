#include <windows.h>
#include <shlwapi.h>

#include <assert.h>
#include <stdio.h>
#include <time.h>

#include <chrono>
#include <string>
#include <thread>

#include "screen_capture/gdi_capture.h"
#include "screen_capture/screen_picture.h"
#include "utils/scoped_handle.h"
#include "utils/virtual_screen.h"

#include "turbojpeg.h"

#pragma comment(lib, "shlwapi.lib")

using namespace std::chrono_literals;

void SaveDataToBmp(const BITMAPFILEHEADER& bfh,
                   const BITMAPINFOHEADER& bih,
                   const uint8_t* bitmap_data,
                   uint32_t bitmap_size,
                   const std::wstring& filepath) {
  assert(bitmap_data);

  HANDLE file = CreateFile(filepath.c_str(),
                           GENERIC_WRITE,
                           0,
                           NULL,
                           CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NO_SCRUB_DATA,
                           NULL);
  if (!file || file == INVALID_HANDLE_VALUE) {
    wprintf(L"failed to create %ls\n", filepath.c_str());
    return;
  }

  DWORD written = 0;
  BOOL res = TRUE;
  do {
    res = WriteFile(file, (void*)&bfh, sizeof(bfh), &written, NULL);
    if (!res) {
      wprintf(L"failed to write bitmap file header, filepath: %ls\n", filepath.c_str());
      break;
    }

    res = WriteFile(file, (void*)&bih, sizeof(bih), &written, NULL);
    if (!res) {
      wprintf(L"failed to write bitmap info header, filepath: %ls\n", filepath.c_str());
      break;
    }

    res = WriteFile(file, (void*)bitmap_data, bitmap_size, &written, NULL);
    if (!res) {
      wprintf(L"failed to write bitmap data, filepath: %ls\n", filepath.c_str());
      break;
    }
  } while (false);

  wprintf(L"success to write file: %ls\n", filepath.c_str());
  CloseHandle(file);
}

void SaveDataToJpeg(const uint8_t* data,
                    uint32_t size,
                    const std::wstring& filepath) {
  utils::ScopedHandle file(CreateFile(filepath.c_str(),
                           GENERIC_WRITE,
                           0,
                           NULL,
                           CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NO_SCRUB_DATA,
                           NULL));
  if (!file || file == INVALID_HANDLE_VALUE) {
    wprintf(L"failed to create %ls\n", filepath.c_str());
    return;
  }

  DWORD written = 0;
  BOOL res = WriteFile(file, (void*)data, size, &written, NULL);
  if (!res) {
    wprintf(L"failed to write bitmap data, filepath: %ls\n", filepath.c_str());
    return;
  }
}

int main() {
  wchar_t buffer[1024];
  GetModuleFileName(NULL, buffer, 1024);
  PathRemoveFileSpec(buffer);

  std::wstring dir(buffer);

  std::unique_ptr<ScreenCapture> capturer(new GDICapture());
  if (!capturer) {
    printf("new GDICapture failed\n");
    return 0;
  }

  std::unique_ptr<ScreenPicture> first_picture(capturer->Capture());
  if (!first_picture) {
    printf("failed to capture\n");
    return 0;
  }

  first_picture.reset();

  int jpeg_handle_res = 0;
  for (int i = 0; i < 10; ++i) {
    std::unique_ptr<ScreenPicture> picture(capturer->Capture());

    tjhandle handle = tjInitCompress();
    uint8_t* jpeg_buffer = nullptr;
    uint32_t jpeg_size = 0;
    int pixel_format = TJPF_BGR;
    int jpeg_subsamp = TJSAMP_444;
    int jpeg_qual = 90;
    int flags = 0;

    int pitch = picture->size / picture->height;
    jpeg_handle_res = tjCompress2(handle,
                                  picture->rgb,
                                  picture->width,
                                  pitch,
                                  picture->height,
                                  pixel_format,
                                  &jpeg_buffer,
                                  (unsigned long*)&jpeg_size,
                                  jpeg_subsamp,
                                  jpeg_qual,
                                  flags);
    if (jpeg_handle_res != 0) {
      printf("failed to call tjCompress2\n");
    } else {
      std::wstring filename = std::to_wstring(time(NULL)) + L".jpg";
      std::wstring filepath = dir + L"\\" + filename;
      SaveDataToJpeg(jpeg_buffer, jpeg_size, filepath);
    }

    tjFree(jpeg_buffer);
    tjDestroy(handle);

    std::this_thread::sleep_for(2000ms);
  }

#if 0
  HWND hwnd = NULL;
  HDC src_dc = NULL;
  HDC mem_dc = NULL;

  BITMAPINFO bitmap_info = {0};
  BITMAPINFOHEADER* bih = &(bitmap_info.bmiHeader);

  // src_dc = GetDC(NULL);
  src_dc = CreateDC(TEXT("DISPLAY"), NULL, NULL, NULL);
  if (!src_dc) {
    printf("AAAAAA\n");
    return 0;
  }
  mem_dc = CreateCompatibleDC(src_dc);
  if (!mem_dc) {
    printf("BBBBBB\n");
    return 0;
  }

  utils::VirtualScreen vs;
  RECT rect = vs.VirtualScreenRect();
  const int width = vs.VirtualScreenWidth();
  const int height = vs.VirtualScreenHeight();

  const int bit_count = 24;
  bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
  bitmap_info.bmiHeader.biWidth = width;
  bitmap_info.bmiHeader.biHeight = -height;
  bitmap_info.bmiHeader.biPlanes = 1;
  bitmap_info.bmiHeader.biBitCount = bit_count;
  bitmap_info.bmiHeader.biCompression = BI_RGB;

  const uint32_t offset = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);
  const uint32_t bitmap_size = ((width * bit_count + 31) & ~31) / 8 * height;

  BITMAPFILEHEADER bfh;
  ZeroMemory(&bfh, sizeof(bfh));
  bfh.bfType = 0x4d42;
  bfh.bfReserved1 = 0;
  bfh.bfReserved2 = 0;
  bfh.bfSize = offset + bitmap_size;
  bfh.bfOffBits = offset;

  for (int i = 0; i < 10; ++i) {
    uint8_t* bitmap_data = nullptr;
    utils::ScopedBitmap bitmap;

    do {
      bitmap = CreateDIBSection(src_dc, &bitmap_info, DIB_RGB_COLORS, reinterpret_cast<void**>(&bitmap_data), NULL, 0);
      if (!bitmap) {
        DWORD err_code = GetLastError();
        printf("%-02d: failed to call CreateDIBSection, err code: %lu\n", i, err_code);
        break;
      }

      SelectObject(mem_dc, bitmap);
      BOOL res = BitBlt(mem_dc,
                        0, 0,
                        width, height,
                        src_dc,
                        rect.left, rect.top,
                        SRCCOPY);
      if (!res) {
        printf("%d: failed to call BitBlt\n", i);
        break;
      }

      std::wstring filename = std::to_wstring(time(NULL)) + L".bmp";
      std::wstring filepath = dir + L"\\" + filename;
      SaveDataToBmp(bfh, *bih, bitmap_data, bitmap_size, filepath);
    } while (false);

    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  DeleteDC(mem_dc);
  ReleaseDC(hwnd, src_dc);
#endif

  return 0;
}
