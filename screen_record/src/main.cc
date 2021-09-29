#include <windows.h>

#include <QtCore/QTextCodec>
#include <QtWidgets/QApplication>

#include "glog/logging.h"
#include "screen_record/src/main_window.h"
#include "screen_record/src/util/string.h"

const char kLogDirName[] = "log";

void InitLogging() {
  QString exe_path_qt = QCoreApplication::applicationFilePath();
  exe_path_qt = QDir::toNativeSeparators(exe_path_qt);

  QDir log_dir(QCoreApplication::applicationDirPath());
  if (!log_dir.exists(kLogDirName)) {
    log_dir.mkdir(kLogDirName);
  }

  QString log_dir_qt = log_dir.absolutePath() + "/" + kLogDirName;
  log_dir_qt = QDir::toNativeSeparators(log_dir_qt);

  std::wstring exe_path_c = exe_path_qt.toStdWString();
  std::string exe_path = SysWideToMultiByte(exe_path_c, CP_ACP);

  google::InitGoogleLogging(exe_path.c_str());
  FLAGS_alsologtostderr = true;
  FLAGS_log_dir = SysWideToMultiByte(log_dir_qt.toStdWString(), CP_ACP);
}

int main(int argc, char** argv) {
  QApplication app(argc, argv);

  CoInitialize(NULL);

  InitLogging();

  // 设置编码格式为UTF-8
  QTextCodec* codec = QTextCodec::codecForName("UTF-8");
  QTextCodec::setCodecForLocale(codec);

  QFont font(QString("Microsoft YaHei"), 9);
  app.setFont(font);

  MainWindow window;
  window.adjustSize();
  window.show();

  int res = app.exec();

  google::ShutdownGoogleLogging();

  CoUninitialize();
  return res;
}
