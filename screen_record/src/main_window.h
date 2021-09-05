#ifndef SCREEN_RECORD_SRC_MAIN_WINDOW_H_
#define SCREEN_RECORD_SRC_MAIN_WINDOW_H_

#include <memory>

#include <QtCore/QDir>
#include <QtCore/QScopedPointer>
#include <QtCore/QTimer>

#include "screen_record/uic/ui_main_window.h"

class QMouseEvent;
class QTimer;

class ScreenRecorder;

class MainWindow : public QMainWindow {
  Q_OBJECT

 public:
  enum class Status {
    RECORDDING = 0,
    PAUSE,
    STOPPED,
  };

  explicit MainWindow(QWidget* parent = Q_NULLPTR);
  ~MainWindow();

 signals:
  void recordCompleted();
  void recordFailed();
  void recordCanceled();

 private slots:
  void onClose();
  void onMinimize();

  void onClickStartBtn();
  void onClickStopBtn();
  void onClickOpenBtn();

  void onUpdateTime();

  void onRecordCompleted();
  void onRecordFailed();
  void onRecordCanceled();

 private:
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;

  void start();
  void pause();
  void restart();
  void stop();

  // 初始化本地存储路径
  void initLocalPath();

  // 打开本地存储路径
  void openLocalPath();

  void connectSignals();

  // 是否应该移动窗口
  bool should_move_window_;
  // 按下鼠标时的坐标
  QPoint mouse_pressed_pt_;
  // 移动前窗口左上角坐标
  QPoint window_pt_before_move_;

  // 当前状态
  Status status_;

  // 本地存储路径
  QDir local_path_;

  // 定时器，统计录屏时间
  uint32_t record_time_;
  QScopedPointer<QTimer> timer_;

  std::unique_ptr<ScreenRecorder> screen_recorder_;

  Ui::MainWindowUI ui_;
};  // class MainWindow

#endif  // SCREEN_RECORD_SRC_MAIN_WINDOW_H_
