#include <QtCore/QTextCodec>
#include <QtWidgets/QApplication>

#include "screen_record/src/main_window.h"

int main(int argc, char** argv) {
  QApplication app(argc, argv);

  // 设置编码格式为UTF-8
  QTextCodec* codec = QTextCodec::codecForName("UTF-8");
  QTextCodec::setCodecForLocale(codec);

  MainWindow window;
  window.adjustSize();
  window.show();

  return app.exec();
}
