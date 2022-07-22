#include <stdio.h>

#include <chrono>
#include <memory>
#include <thread>

#include "screen_capture/dxgi_capture.h"
#include "screen_capture/screen_picture.h"
#include "utils/utils.h"

int main() {
  printf("等待5秒...\n");
  std::this_thread::sleep_for(std::chrono::seconds(5));

  printf("开始截屏\n");

  std::unique_ptr<DXGICapture> captgure(new DXGICapture);
  std::unique_ptr<ScreenPicture> picture(captgure->Capture());
  if (!picture) {
    printf("failed to capture screen\n");
    return 0;
  }

  int ret = 0;
  std::wstring path;

  ret = utils::GetCurrentDir(&path);
  if (ret != 0) {
    printf("获取当前路径失败: %d\n", ret);
    return 0;
  }

  path.append(L"\\").append(std::to_wstring(time(NULL))).append(L".jpg");
  ret = utils::ARGBToJpeg(picture->argb, picture->width, picture->height, picture->size, path.c_str());
  if (ret != 0) {
    printf("生成jpeg文件失败: %d\n", ret);
    return 0;
  }

  printf("success\n");
  return 0;
}
