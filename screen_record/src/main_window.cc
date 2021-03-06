#include "screen_record/src/main_window.h"

#include <windows.h>
#include <shlobj.h>

#include <QtGui/QMouseEvent>
#include <QtWidgets/QMessageBox>

#include "base/check.h"
#include "base/strings/sys_string_conversions.h"
#include "logger/logger.h"
#include "res/version.h"
#include "screen_record/src/argument.h"
#include "screen_record/src/screen_recorder.h"
#include "screen_record/src/setting/setting_dialog.h"

const char kName[] = "ScreenRecord";

namespace {

const char kFilter[] = "MainWindow";

}  // namespace

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
      should_move_window_(false),
      record_time_(0) {
  ui_.setupUi(this);

  // 设置title
  std::wstring name = base::SysMultiByteToWide(PRODUCT_NAME, CP_ACP);
  std::wstring version = base::SysMultiByteToWide(VERSION_STR, CP_ACP);
  QString title = QString::asprintf("%ls%ls", name.c_str(), version.c_str());
  ui_.titleLabel->setText(title);

  initLocalPath();

  // 去除边框、标题栏
  setWindowFlags(Qt::Window |
                 Qt::FramelessWindowHint |
                 Qt::WindowSystemMenuHint |
                 Qt::WindowMinimizeButtonHint);

  ui_.recordTimeLabel->hide();
  ui_.btnStop->setEnabled(false);

  layout()->setSizeConstraint(QLayout::SetFixedSize);
  ui_.contentFrame->layout()->setSizeConstraint(QLayout::SetFixedSize);

  connectSignals();

  screen_recorder_ = std::make_unique<ScreenRecorder>(
      [this]() { emit recordCompleted(); },
      [this]() { emit recordCanceled(); },
      [this]() { emit recordFailed(); }
  );
}

MainWindow::~MainWindow() {
}

void MainWindow::onClose() {
  // 如果在录屏，先取消
  if (screen_recorder_->status() == ScreenRecorder::Status::RECORDING ||
      screen_recorder_->status() == ScreenRecorder::Status::PAUSE) {
    screen_recorder_->cancelRecord();
  }

  // 先隐藏窗口，然后等待录屏结束
  hide();
  while (screen_recorder_->status() != ScreenRecorder::Status::STOPPED) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  // 关闭
  close();
}

void MainWindow::onMinimize() {
  showMinimized();
}

void MainWindow::onClickSettingButton() {
  SettingDialog setting_dialog(this);
  setting_dialog.exec();
}

void MainWindow::onClickStartBtn() {
  switch (screen_recorder_->status()) {
    case ScreenRecorder::Status::RECORDING:
      pause();
      break;

    case ScreenRecorder::Status::PAUSE:
      restart();
      break;

    case ScreenRecorder::Status::STOPPED:
      start();
      break;

    case ScreenRecorder::Status::CANCELING:
    case ScreenRecorder::Status::STOPPING:
      break;

    default:
      DCHECK(false);
      break;
  }
}

void MainWindow::onClickStopBtn() {
  stop();
}

void MainWindow::onClickOpenBtn() {
  openLocalPath();
}

void MainWindow::onUpdateTime() {
  ++record_time_;

  uint32_t second = record_time_ % 60;
  uint32_t minute = (record_time_ / 60) % 60;
  uint32_t hour = record_time_ / 3600;
  QString time_str = QString::asprintf("%02u:%02u:%02u", hour, minute, second);
  ui_.recordTimeLabel->setText(time_str);
}

void MainWindow::onRecordCompleted() {
  ui_.btnStart->setEnabled(true);
}

void MainWindow::onRecordFailed() {
  LOG_ERROR(kFilter, "录屏失败");
}

void MainWindow::onRecordCanceled() {
  LOG_INFO(kFilter, "取消录屏");
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

void MainWindow::start() {
  DCHECK(screen_recorder_->status() == ScreenRecorder::Status::STOPPED);

  ui_.btnStop->setEnabled(true);
  ui_.btnStart->setText(QStringLiteral("暂停"));

  LOG_INFO(kFilter, "帧率: %d", g_setting_manager->fps());
  LOG_INFO(kFilter, "截屏方式: %s", g_setting_manager->CaptureType().toStdString().c_str());
  LOG_INFO(kFilter, "文件格式: %s", g_setting_manager->FileFormat().toStdString().c_str());
  LOG_INFO(kFilter, "视频编码格式: %s", g_setting_manager->VideoEncoder().toStdString().c_str());

  screen_recorder_->startRecord(
      local_path_.absolutePath(), g_setting_manager->fps());

  record_time_ = 0;

  ui_.recordTimeLabel->setText("00:00:00");
  ui_.recordTimeLabel->show();

  DCHECK(!timer_->isActive());
  timer_->start(std::chrono::milliseconds(1000));

  ui_.settingButton->setEnabled(false);
}

void MainWindow::pause() {
  DCHECK(screen_recorder_->status() == ScreenRecorder::Status::RECORDING);
  screen_recorder_->pauseRecord();

  timer_->stop();
  ui_.btnStart->setText(QStringLiteral("继续"));
}

void MainWindow::restart() {
  DCHECK(screen_recorder_->status() == ScreenRecorder::Status::PAUSE);
  screen_recorder_->restartRecord();

  timer_->start();
  ui_.btnStart->setText(QStringLiteral("暂停"));
}

void MainWindow::stop() { 
  DCHECK(screen_recorder_->status() == ScreenRecorder::Status::RECORDING ||
         screen_recorder_->status() == ScreenRecorder::Status::PAUSE);
  screen_recorder_->stopRecord();

  timer_->stop();
  ui_.recordTimeLabel->hide();

  ui_.btnStart->setText(QStringLiteral("开始"));
  ui_.btnStart->setEnabled(false);
  ui_.btnStop->setEnabled(false);

  ui_.settingButton->setEnabled(true);
}

void MainWindow::initLocalPath() {
  wchar_t system_buffer[MAX_PATH];
  memset(system_buffer, 0, sizeof(wchar_t) * MAX_PATH);

  QDir folder;
  HRESULT hr = SHGetFolderPath(NULL, CSIDL_MYVIDEO, NULL, SHGFP_TYPE_CURRENT,
                               system_buffer);
  if (FAILED(hr)) {
    folder = QDir(QCoreApplication::applicationDirPath());
  } else {
    folder = QDir(QString::fromWCharArray(system_buffer));
  }

  local_path_ = QDir(folder.absolutePath() + "/" + kName);
  if (!folder.exists(kName) && !folder.mkpath(kName)) {
    QString err_msg =
        QString(QStringLiteral("%1目录创建失败，请手动创建后在启动")
                    .arg(local_path_.absolutePath()));
    QMessageBox msg_box;
    msg_box.setWindowTitle("Error");
    msg_box.setText(err_msg);
    msg_box.exec();

    QTimer::singleShot(0, qApp, SLOT(quit()));
    return;
  }
}

void MainWindow::openLocalPath() {
  if (!local_path_.exists()) {
    QString msg = QString("%1 is not exist.").arg(local_path_.absolutePath());

    QMessageBox msg_box;
    msg_box.setWindowTitle("Error");
    msg_box.setText(msg);
    msg_box.exec();

    return;
  }

  wchar_t buffer[MAX_PATH];
  memset(buffer, 0, sizeof(wchar_t) * MAX_PATH);
  local_path_.absolutePath().toWCharArray(buffer);
  ShellExecute(NULL, L"open", buffer, NULL, NULL, SW_SHOWNORMAL);
}

void MainWindow::connectSignals() {
  // 最小化/关闭
  connect(ui_.minButton, &QPushButton::clicked, this, &MainWindow::onMinimize);
  connect(ui_.closeButton, &QPushButton::clicked, this, &MainWindow::onClose);

  // 打开设置对话框
  connect(ui_.settingButton, &QPushButton::clicked,
          this, &MainWindow::onClickSettingButton);

  // 开始、暂停、停止、打开保存目录
  connect(ui_.btnStart, &QPushButton::clicked,
          this, &MainWindow::onClickStartBtn);
  connect(ui_.btnStop, &QPushButton::clicked,
          this, &MainWindow::onClickStopBtn);
  connect(ui_.btnOpen, &QPushButton::clicked,
          this, &MainWindow::onClickOpenBtn);

  // 录屏结果
  connect(this, &MainWindow::recordCompleted,
          this, &MainWindow::onRecordCompleted);
  connect(this, &MainWindow::recordCanceled,
          this, &MainWindow::onRecordCanceled);
  connect(this, &MainWindow::recordFailed, this, &MainWindow::onRecordFailed);

  // 定时器
  timer_.reset(new QTimer(this));
  connect(timer_.get(), &QTimer::timeout, this, &MainWindow::onUpdateTime);
}
