#include "screen_record/src/setting/setting_dialog.h"

#include <QtGui/QMouseEvent>

#include "encoder/ffmpeg.h"
#include "glog/logging.h"
#include "screen_record/src/argument.h"
#include "screen_record/src/setting/setting_manager.h"

namespace {

}  // namespace

SettingDialog::SettingDialog(QWidget* parent)
    : should_move_window_(false) {
  ui_.setupUi(this);

  DCHECK(g_setting_manager);
  int fps = g_setting_manager->fps();
  QString file_format = g_setting_manager->FileFormat();
  QString capture_type = g_setting_manager->CaptureType();
  QString video_encoder = g_setting_manager->VideoEncoder();

  setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);

  int index = -1;
  int i = 0;

  int count = sizeof(SettingManager::kFpsList) / sizeof(int);
  for (i = 0; i < count; ++i) {
    ui_.fpsSelector->addItem(
        std::to_string(SettingManager::kFpsList[i]).c_str());
    if (fps == SettingManager::kFpsList[i]) {
      index = i;
    }
  }

  DCHECK(index != -1);
  ui_.fpsSelector->setCurrentIndex(index);

  index = -1;
  i = 0;
  while (SettingManager::kFileFormatList[i]) {
    ui_.fileFormatSelector->addItem(SettingManager::kFileFormatList[i]);
    if (file_format == QString(SettingManager::kFileFormatList[i])) {
      index = i;
    }
    ++i;
  }

  DCHECK(index != -1);
  ui_.fileFormatSelector->setCurrentIndex(index);

  index = -1;
  i = 0;
  while (SettingManager::kCaptureTypeList[i]) {
    ui_.capturerSelector->addItem(SettingManager::kCaptureTypeList[i]);
    if (capture_type == QString(SettingManager::kCaptureTypeList[i])) {
      index = i;
    }
    ++i;
  }

  DCHECK(index != -1);
  ui_.capturerSelector->setCurrentIndex(index);

  index = -1;
  count = sizeof(SettingManager::kVideoEncoderList) / sizeof(SettingManager::VideoEncoderInfo);
  for (i = 0; i < count; ++i) {
    ui_.videoCodecSelector->addItem(SettingManager::kVideoEncoderList[i].description);
    if (video_encoder == QString(SettingManager::kVideoEncoderList[i].description)) {
      index = i;
    }
  }

  DCHECK(index != -1);
  ui_.videoCodecSelector->setCurrentIndex(index);

  connect(ui_.btnClose, &QPushButton::clicked,
          this, &SettingDialog::onClose);
  connect(ui_.fpsSelector, &QComboBox::currentTextChanged,
          this, &SettingDialog::onFpsChanged);
  connect(ui_.fileFormatSelector, &QComboBox::currentTextChanged,
          this, &SettingDialog::onFileFormatChanged);
  connect(ui_.capturerSelector, &QComboBox::currentTextChanged,
          this, &SettingDialog::onCaptureTypeChanged);
  connect(ui_.videoCodecSelector, &QComboBox::currentTextChanged,
          this, &SettingDialog::onVideoEncoderChanged);
}

SettingDialog::~SettingDialog() {}

void SettingDialog::onFpsChanged(const QString& fps_str) {
  bool res = false;
  int new_fps = fps_str.toInt(&res);
  DCHECK(res);

  g_setting_manager->SetFps(new_fps);
}

void SettingDialog::onFileFormatChanged(const QString& new_format) {
  g_setting_manager->SetFileFormat(new_format);
}

void SettingDialog::onCaptureTypeChanged(const QString& new_type) {
  g_setting_manager->SetCaptureType(new_type);
}

void SettingDialog::onVideoEncoderChanged(const QString& new_encoder) {
  g_setting_manager->SetVideoEncoder(new_encoder);
}

void SettingDialog::mousePressEvent(QMouseEvent* event) {
  if (ui_.titleBar->rect().contains(event->pos())) {
    should_move_window_ = true;
    mouse_pressed_pt_ = event->globalPos();
    window_pt_before_move_ = frameGeometry().topLeft();
  }
}

void SettingDialog::mouseReleaseEvent(QMouseEvent* event) {
  should_move_window_ = false;
}

void SettingDialog::mouseMoveEvent(QMouseEvent* event) {
  if (!should_move_window_) {
    return;
  }

  QPoint delta = event->globalPos() - mouse_pressed_pt_;
  QPoint new_position = delta + window_pt_before_move_;
  move(new_position);
}

void SettingDialog::onClose() {
  close();
}
