#include <windows.h>

#include <QtCore/QTextCodec>
#include <QtWidgets/QApplication>

#include "base/strings/sys_string_conversions.h"
#include "gflags/gflags.h"
#include "glog/logging.h"
#include "screen_record/src/argument.h"
#include "screen_record/src/main_window.h"
#include "screen_record/src/setting/setting_manager.h"

const char kLogDirName[] = "log";

void InitLogging() {
  QDir log_dir(QCoreApplication::applicationDirPath());
  if (!log_dir.exists(kLogDirName)) {
    log_dir.mkdir(kLogDirName);
  }

  QString log_dir_qt = log_dir.absolutePath() + "/" + kLogDirName;
  log_dir_qt = QDir::toNativeSeparators(log_dir_qt);

  std::wstring log_dst_dir = log_dir_qt.toStdWString();
  std::string log_dst_dir_a = base::SysWideToMultiByte(log_dst_dir, CP_ACP);

  FLAGS_alsologtostderr = true;
  FLAGS_log_dir = log_dst_dir_a;
}

int main(int argc, char** argv) {
  CoInitialize(NULL);

  // 使用gflags解析命令行参数
  // false表示则argv和argc会被保留，但是argv中的顺序可能会改变
  google::ParseCommandLineFlags(&argc, &argv, false);

  QApplication app(argc, argv);

  google::InitGoogleLogging(argv[0]);
  InitLogging();

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

  google::ShutdownGoogleLogging();

  CoUninitialize();
  return res;
}
