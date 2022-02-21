#include <windows.h>
#include <shlobj.h>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#include <string>

#include "plog/Appenders/ColorConsoleAppender.h"
#include "plog/Log.h"

#ifdef _UNICODE
#if defined _M_IX86
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
#endif

extern "C" IMAGE_DOS_HEADER __ImageBase;
#define CURRENT_MODULE() reinterpret_cast<HINSTANCE>(&__ImageBase)
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

#define IDC_BTN_START                      6001
#define IDC_BTN_STOP                       6002

static HWND g_main_window = NULL;
static HWND g_btn_start = NULL;
static HWND g_btn_stop = NULL;

static DWORD g_main_thread_id = 0;
static std::string* g_directory = nullptr;

constexpr int g_window_width = 300;
constexpr int g_window_height = 110;
constexpr int g_button_width = 280;
constexpr int g_button_height = 40;
constexpr wchar_t kWindowTitle[] = L"ScreenRecord";
constexpr wchar_t kClassName[] = L"ScreenRecord";

LRESULT CALLBACK WindowProc(
    HWND hwnd, uint32_t msg, WPARAM w_param, LPARAM l_param);
void CenterWindow(HWND hwnd);

std::string GetScreenRecordFolder();
bool DirectoryIsExists(const std::string& directory);

void OnMainWindowCreate(HWND hwnd);
void OnCommand(uint32_t id, HWND hwnd, bool& handled);

void OnClickStartBtn();
void OnClickStopBtn();

int main(int argc, char** argv) {
  g_main_thread_id = GetCurrentThreadId();

  static plog::ColorConsoleAppender<plog::TxtFormatter> console_appender;
  plog::init(plog::debug, &console_appender);

  g_directory = new std::string();
  *g_directory = GetScreenRecordFolder();
  if (g_directory->empty()) {
    delete g_directory;
    MessageBox(NULL, L"构造视频保存路径失败", L"错误", MB_ICONERROR);
    return 0;
  }
  if (!DirectoryIsExists(*g_directory) &&
      !::CreateDirectoryA(g_directory->c_str(), NULL)) {
    char buffer[1024];
    memset(buffer, 0, 1024);
    sprintf(buffer, "创建%s失败", g_directory->c_str());
    MessageBoxA(NULL, buffer, "错误", MB_ICONERROR);

    delete g_directory;
    return 0;
  }

  WNDCLASSEX wcex;
  ZeroMemory(&wcex, sizeof(wcex));
  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.lpfnWndProc = WindowProc;
  wcex.style = CS_HREDRAW | CS_VREDRAW;
  wcex.lpszClassName = kClassName;
  wcex.hInstance = HINST_THISCOMPONENT;
  wcex.hbrBackground = (HBRUSH)COLOR_WINDOW;
  wcex.hCursor = ::LoadCursor(NULL, IDC_ARROW);
  RegisterClassEx(&wcex);

  DWORD style = WS_OVERLAPPED | WS_CAPTION | WS_MINIMIZEBOX | WS_SYSMENU;
  DWORD style_ex = 0;
  g_main_window = CreateWindowEx(style_ex,
                                 kClassName,
                                 kWindowTitle,
                                 style,
                                 0, 0,
                                 g_window_width, g_window_height,
                                 NULL,
                                 NULL,
                                 CURRENT_MODULE(),
                                 NULL);
  if (!g_main_window) {
    MessageBox(NULL, L"创建窗口失败", L"错误", MB_ICONERROR);
    delete g_directory;
    return 0;
  }

  ShowWindow(g_main_window, SW_SHOW);
  UpdateWindow(g_main_window);

  CenterWindow(g_main_window);

  MSG msg = { };
  while (GetMessage(&msg, NULL, 0, 0)) {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  delete g_directory;
  return 0;
}

LRESULT CALLBACK WindowProc(
    HWND hwnd, uint32_t msg, WPARAM w_param, LPARAM l_param) {
  bool handled = false;
  switch (msg) {
    case WM_CREATE:
      OnMainWindowCreate(hwnd);
      return 0;

    case WM_COMMAND:
      OnCommand(LOWORD(w_param), (HWND)l_param, handled);
      if (handled) {
        return 0;
      } else {
        break;
      }

    case WM_DESTROY:
      PostQuitMessage(0);
      return 0;

    default:
      break;
  }
  return DefWindowProc(hwnd, msg, w_param, l_param);
}

void CenterWindow(HWND hwnd) {
  if (!IsWindow(hwnd))
    return;

  RECT client_rect, window_rect;
  GetClientRect(hwnd, &client_rect);
  GetWindowRect(hwnd, &window_rect);
  int client_width = client_rect.right - client_rect.left;
  int client_height = client_rect.bottom - client_rect.top;
  int window_width = window_rect.right - window_rect.left;
  int window_height = window_rect.bottom - window_rect.top;
  int width = window_width - client_width;
  int height = window_height - client_height;
  window_width = g_window_width + width;
  window_height = g_window_height + height;

  const int screen_width = GetSystemMetrics(SM_CXSCREEN);
  const int screen_height = GetSystemMetrics(SM_CYSCREEN);
  const int x = (screen_width - window_width) >> 1;
  const int y = (screen_height - window_height) >> 1;

  SetWindowPos(hwnd, HWND_TOP, x, y, window_width, window_height, SWP_NOZORDER);
}

std::string GetScreenRecordFolder() {
  char document_dir[MAX_PATH];
  memset(document_dir, 0, MAX_PATH);

  HRESULT hr = SHGetFolderPathA(
      NULL, CSIDL_MYDOCUMENTS, NULL, SHGFP_TYPE_CURRENT, document_dir);
  if (FAILED(hr)) {
    return std::string();
  }

  std::string result(document_dir);
  result.append("\\ScreenRecord");
  return result;
}

bool DirectoryIsExists(const std::string& directory) {
  DWORD fileattr = GetFileAttributesA(directory.c_str());
  if (fileattr != INVALID_FILE_ATTRIBUTES) {
    return (fileattr & FILE_ATTRIBUTE_DIRECTORY) != 0;
  }
  return false;
}

void OnMainWindowCreate(HWND hwnd) {
  int x = 10;
  int y = 10;

  g_btn_start = CreateWindow(L"BUTTON",
                             L"开始",
                             WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                             x, y, g_button_width, g_button_height,
                             hwnd,
                             (HMENU)IDC_BTN_START,
                             HINST_THISCOMPONENT,
                             NULL);

  y += (g_button_height + 10);
  g_btn_stop = CreateWindow(L"BUTTON",
                            L"结束",
                            WS_TABSTOP | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
                            x, y, g_button_width, g_button_height,
                            hwnd,
                            (HMENU)IDC_BTN_STOP,
                            HINST_THISCOMPONENT,
                            NULL);

  EnableWindow(g_btn_stop, FALSE);
}

void OnCommand(uint32_t id, HWND hwnd, bool& handled) {
  handled = true;
  switch (id) {
    case IDC_BTN_START:
      OnClickStartBtn();
      break;
    case IDC_BTN_STOP:
      OnClickStopBtn();
      break;
    default:
      handled = false;
      break;
  }
}

void OnClickStartBtn() {
  PLOG(plog::info) << "click start button";

  SetWindowText(g_btn_start, L"暂停");
  EnableWindow(g_btn_stop, TRUE);
}

void OnClickStopBtn() {
  PLOG(plog::info) << "click stop button";

  SetWindowText(g_btn_start, L"开始");
  EnableWindow(g_btn_stop, FALSE);
}
