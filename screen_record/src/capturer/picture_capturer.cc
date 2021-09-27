#include "screen_record/src/capturer/picture_capturer.h"

// https://www.coder.work/article/1221121
void PictureCapturer::DrawMouseIcon(HDC hdc) {
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
    DrawIconEx(hdc, point.x, point.y, cursor_info.hCursor,
               cursor_bmp.bmWidth, cursor_bmp.bmHeight, 0, NULL, DI_NORMAL);

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
