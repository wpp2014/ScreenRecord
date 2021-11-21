#include <windows.h>

#include <QtCore/QTextCodec>
#include <QtWidgets/QApplication>

#include "base/strings/sys_string_conversions.h"
#include "gflags/gflags.h"
#include "logger/logger.h"
#include "screen_record/src/argument.h"
#include "screen_record/src/main_window.h"
#include "screen_record/src/setting/setting_manager.h"

int main(int argc, char** argv) {
  CoInitialize(NULL);

  // 使用gflags解析命令行参数
  // false表示则argv和argc会被保留，但是argv中的顺序可能会改变
  google::ParseCommandLineFlags(&argc, &argv, false);

  QApplication app(argc, argv);

  // 设置编码格式为UTF-8
  QTextCodec* codec = QTextCodec::codecForName("UTF-8");
  QTextCodec::setCodecForLocale(codec);

  QFont font(QString("Microsoft YaHei"), 9);
  app.setFont(font);

  g_setting_manager = SettingManager::GetInstance();

  MainWindow window;
  window.adjustSize();
  window.show();

  int res = app.exec();

  CoUninitialize();
  return res;
}
