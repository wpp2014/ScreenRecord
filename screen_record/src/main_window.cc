#include "screen_record/src/main_window.h"

#include <QtGui/QMouseEvent>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      should_move_window_(false) {
  ui_.setupUi(this);

  // 去除边框、标题栏
  setWindowFlags(Qt::Window |
                 Qt::FramelessWindowHint |
                 Qt::WindowSystemMenuHint |
                 Qt::WindowMinimizeButtonHint);

  ui_.recordTimeLabel->hide();

  layout()->setSizeConstraint(QLayout::SetFixedSize);
  ui_.contentFrame->layout()->setSizeConstraint(QLayout::SetFixedSize);

  connectSignals();
}

MainWindow::~MainWindow() {
}

void MainWindow::onClose() {
  close();
}

void MainWindow::onMinimize() {
  showMinimized();
}

void MainWindow::mousePressEvent(QMouseEvent* event) {
  if (ui_.titleFrame->rect().contains(event->pos())) {
    should_move_window_ = true;
    mouse_pressed_pt_ = event->globalPos();
    window_pt_before_move_ = frameGeometry().topLeft();
  }
}

void MainWindow::mouseReleaseEvent(QMouseEvent* event) {
  should_move_window_ = false;
}

void MainWindow::mouseMoveEvent(QMouseEvent* event) {
  if (!should_move_window_) {
    return;
  }

  QPoint delta = event->globalPos() - mouse_pressed_pt_;
  QPoint new_position = delta + window_pt_before_move_;
  move(new_position);
}

void MainWindow::connectSignals() {
  // 最小化/关闭
  connect(ui_.minButton, &QPushButton::clicked, this, &MainWindow::onMinimize);
  connect(ui_.closeButton, &QPushButton::clicked, this, &MainWindow::onClose);
}
