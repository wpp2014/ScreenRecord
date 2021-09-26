#include <windows.h>

#include <QtCore/QTextCodec>
#include <QtWidgets/QApplication>

#include "glog/logging.h"
#include "screen_record/src/main_window.h"

int main(int argc, char** argv) {
  QApplication app(argc, argv);

  CoInitialize(NULL);

  google::InitGoogleLogging(argv[0]);
  FLAGS_alsologtostderr = true;

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
