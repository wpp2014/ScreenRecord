#ifndef SCREEN_RECORD_SRC_MAIN_WINDOW_H_
#define SCREEN_RECORD_SRC_MAIN_WINDOW_H_

#include "screen_record/uic/ui_main_window.h"

class QMouseEvent;

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(QWidget* parent = Q_NULLPTR);
  ~MainWindow();

 private slots:
  void onClose();
  void onMinimize();

 private:
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;

  void connectSignals();

  // 是否应该移动窗口
  bool should_move_window_;
  // 按下鼠标时的坐标
  QPoint mouse_pressed_pt_;
  // 移动前窗口左上角坐标
  QPoint window_pt_before_move_;

  Ui::MainWindowUI ui_;
};  // class MainWindow

#endif  // SCREEN_RECORD_SRC_MAIN_WINDOW_H_
