#ifndef SCREEN_RECORD_SRC_SETTING_SETTING_DIALOG_H_
#define SCREEN_RECORD_SRC_SETTING_SETTING_DIALOG_H_

#include <QtWidgets/QDialog>

#include "screen_record/uic/ui_setting_dialog.h"

class QMouseEvent;

class SettingDialog : public QDialog {
 public:
  SettingDialog(QWidget* parent = Q_NULLPTR);
  ~SettingDialog();

 private slots:
  void onClose();

  void onFpsChanged(const QString& fps_str);
  void onFileFormatChanged(const QString& new_format);
  void onCaptureTypeChanged(const QString& new_type);
  void onVideoEncoderChanged(const QString& new_encoder);

 private:
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;

  // 是否应该移动窗口
  bool should_move_window_;
  // 按下鼠标时的坐标
  QPoint mouse_pressed_pt_;
  // 移动前窗口左上角坐标
  QPoint window_pt_before_move_;

  Ui::SettingDialogUI ui_;
};  // class SettingDialog

#endif  // SCREEN_RECORD_SRC_SETTING_SETTING_DIALOG_H_
