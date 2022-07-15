#include <windows.h>
#include <shlwapi.h>

#include <assert.h>
#include <stdio.h>
#include <time.h>

#include <chrono>
#include <string>
#include <thread>

#include "screen_capture/dxgi_capture.h"
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

void SaveToJpeg(std::unique_ptr<ScreenPicture> picture,
                const std::wstring& filepath) {
  tjhandle handle = tjInitCompress();
  if (!handle) {
    printf("failed to call tjInitCompress: %s\n", tjGetErrorStr());
    return;
  }

  uint8_t* jpeg_buffer = nullptr;
  uint32_t jpeg_size = 0;
  int pixel_format = TJPF_BGRA;
  int jpeg_subsamp = TJSAMP_444;
  int jpeg_qual = 90;
  int flags = 0;

  int pitch = picture->size / picture->height;
  int jpeg_handle_res = tjCompress2(handle,
                                    picture->argb,
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
    printf("failed to call tjCompress2: %s\n", tjGetErrorStr2(handle));
    if (jpeg_buffer)
      tjFree(jpeg_buffer);
    tjDestroy(handle);

    return;
  }

  utils::ScopedHandle file(CreateFile(filepath.c_str(),
                           GENERIC_WRITE,
                           0,
                           NULL,
                           CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NO_SCRUB_DATA,
                           NULL));
  if (!file || file == INVALID_HANDLE_VALUE) {
    wprintf(L"failed to create %ls\n", filepath.c_str());
    tjFree(jpeg_buffer);
    tjDestroy(handle);
    return;
  }

  DWORD written = 0;
  BOOL res = WriteFile(file, (void*)jpeg_buffer, jpeg_size, &written, NULL);
  if (!res) {
    wprintf(L"failed to write jpg file, filepath: %ls\n", filepath.c_str());
  }

  tjFree(jpeg_buffer);
  tjDestroy(handle);
}

int main() {
  wchar_t dir[1024];
  GetModuleFileName(NULL, dir, 1024);
  PathRemoveFileSpec(dir);

  std::unique_ptr<ScreenCapture> gdi_capturer(new GDICapture());
  std::unique_ptr<ScreenCapture> dxgi_capturer(new DXGICapture());

  uint64_t start = 0;
  uint64_t end = 0;
  wchar_t filepath[1024];

  for (int i = 0; i < 10; ++i) {
    memset(filepath, 0, sizeof(wchar_t) * 1024);
    swprintf(filepath, L"%ls\\gdi_%lld.jpg", dir, time(NULL));

    start = GetTickCount64();
    std::unique_ptr<ScreenPicture> picture(gdi_capturer->Capture());
    end = GetTickCount64();

    wprintf(L"gdi: %llu  %ls\n", end - start, filepath);

    SaveToJpeg(std::move(picture), filepath);

    std::this_thread::sleep_for(2000ms);
  }

  for (int i = 0; i < 10; ++i) {
    memset(filepath, 0, sizeof(wchar_t) * 1024);
    swprintf(filepath, L"%ls\\dxgi_%lld.jpg", dir, time(NULL));

    start = GetTickCount64();
    std::unique_ptr<ScreenPicture> picture(gdi_capturer->Capture());
    if (picture) {
      end = GetTickCount64();
      wprintf(L"dxgi: %llu  %ls\n", end - start, filepath);

      SaveToJpeg(std::move(picture), filepath);
    }

    std::this_thread::sleep_for(2000ms);
  }

  return 0;
}
