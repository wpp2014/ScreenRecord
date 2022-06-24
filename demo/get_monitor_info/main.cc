#include <windows.h>
#include <shellscalingapi.h>

#include <stdio.h>
#include <stdint.h>

int main() {
  EnumDisplayMonitors(
      NULL, NULL,
      [](HMONITOR hmonitor, HDC hdc, LPRECT rect, LPARAM param) {
        MONITORINFOEX miex;
        ZeroMemory(&miex, sizeof(miex));
        miex.cbSize = sizeof(miex);

        if (GetMonitorInfo(hmonitor, &miex) != 0) {
          wprintf(L"%ls\n", miex.szDevice);
          printf("monitor rect:  (%d, %d)-(%d, %d)\n",
                 miex.rcMonitor.left, miex.rcMonitor.top,
                 miex.rcMonitor.right, miex.rcMonitor.bottom);
          printf("work rect:     (%d, %d)-(%d, %d)\n",
                 miex.rcWork.left, miex.rcWork.top,
                 miex.rcWork.right, miex.rcWork.bottom);
        }

        uint32_t xdpi = 0;
        uint32_t ydpi = 0;
        LRESULT hr = GetDpiForMonitor(hmonitor, MDT_EFFECTIVE_DPI, &xdpi, &ydpi);
        if (hr == S_OK) {
          printf("DPI (effective): %lu, %lu\n", xdpi, ydpi);
        }
        hr = GetDpiForMonitor(hmonitor, MDT_ANGULAR_DPI, &xdpi, &ydpi);
        if (hr == S_OK) {
          printf("DPI (angular):  %lu, %lu\n", xdpi, ydpi);
        }
        hr = GetDpiForMonitor(hmonitor, MDT_RAW_DPI, &xdpi, &ydpi);
        if (hr == S_OK) {
          printf("DPI (raw):      %lu, %lu\n", xdpi, ydpi);
        }

        DEVMODE dm;
        ZeroMemory(&dm, sizeof(dm));
        dm.dmSize = sizeof(dm);

        if (EnumDisplaySettings(miex.szDevice, ENUM_CURRENT_SETTINGS, &dm) != 0) {
          printf("BPP:            %lu\n", dm.dmBitsPerPel);
          printf("resolution:     %d, %d\n", dm.dmPelsWidth, dm.dmPelsHeight);
          printf("frequency:      %d\n", dm.dmDisplayFrequency);
        }

        return TRUE;
      },
      NULL);
}
