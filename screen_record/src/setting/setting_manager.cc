#include "screen_record/src/setting/setting_manager.h"

#include <QtCore/QDir>
#include <QtCore/QSettings>

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/path_service.h"
#include "base/strings/sys_string_conversions.h"
#include "screen_record/src/constants.h"

namespace {

const char kFpsKey[] = "App/fps";
const char kFileFormatKey[] = "App/fileFormat";
const char kCaptureTypeKey[] = "App/captureType";
const char kVideoEncoderKey[] = "App/videoEncoder";

}  // namespace

// static
SettingManager* SettingManager::GetInstance() {
  static SettingManager instance;
  return &instance;
}

AVCodecID SettingManager::VideoCodecID() const {
  const int count = sizeof(kVideoEncoderList) / sizeof(VideoEncoderInfo);
  for (int i = 0; i < count; ++i) {
    if (video_encoder_ == QString(kVideoEncoderList[i].description)) {
      return kVideoEncoderList[i].codec_id;
    }
  }

  DCHECK(false);
  return kDefaultVideoCodecID;
}

void SettingManager::SetFps(int new_fps) {
  if (fps_ == new_fps) {
    return;
  }

  fps_ = new_fps;
  settings_->setValue(kFpsKey, QVariant::fromValue(fps_));
}

void SettingManager::SetFileFormat(const QString& new_format) {
  if (file_format_ == new_format) {
    return;
  }

  file_format_ = new_format;
  settings_->setValue(kFileFormatKey, QVariant::fromValue(file_format_));
}

void SettingManager::SetCaptureType(const QString& new_type) {
  if (capture_type_ == new_type) {
    return;
  }

  capture_type_ = new_type;
  settings_->setValue(kCaptureTypeKey, QVariant::fromValue(capture_type_));
}

void SettingManager::SetVideoEncoder(const QString& new_encoder) {
  if (video_encoder_ == new_encoder) {
    return;
  }

  video_encoder_ = new_encoder;
  settings_->setValue(kVideoEncoderKey, QVariant::fromValue(video_encoder_));
}

SettingManager::SettingManager() {
  DecodeConfig();
}

SettingManager::~SettingManager() {
}

void SettingManager::Reset() {
  fps_ = kDefaultFps;
  file_format_ = QString(kDefaultFileFormat);
  capture_type_ = QString(kDefaultCaptureType);
  video_encoder_ = QString(kDefaultVideoEncoder);

  DCHECK(settings_.get());
  settings_->setValue(kFpsKey, QVariant::fromValue(fps_));
  settings_->setValue(kFileFormatKey, QVariant::fromValue(file_format_));
  settings_->setValue(kCaptureTypeKey, QVariant::fromValue(capture_type_));
  settings_->setValue(kVideoEncoderKey, QVariant::fromValue(video_encoder_));
}

void SettingManager::DecodeConfig() {
  base::FilePath local_appdata;
  if (!base::PathService::Get(base::DIR_LOCAL_APP_DATA, &local_appdata)) {
    CHECK(false);
  }

  base::FilePath config_folder = local_appdata.Append(
      base::SysMultiByteToWide(constants::kConfigFolderName, CP_UTF8));
  if (!base::DirectoryExists(config_folder) &&
      !base::CreateDirectory(config_folder)) {
    CHECK(false);
  }

  config_folder_ = QDir::fromNativeSeparators(
      QString::fromStdWString(config_folder.value()));

  base::FilePath config_path = config_folder.Append(
      base::SysMultiByteToWide(constants::kConfigFileName, CP_UTF8));
  config_path_ =
      QDir::fromNativeSeparators(QString::fromStdWString(config_path.value()));

  if (!base::PathExists(config_path)) {
    settings_.reset(new QSettings(config_path_, QSettings::IniFormat));
    Reset();
    return;
  }

  settings_.reset(new QSettings(config_path_, QSettings::IniFormat));

  int fps = settings_->value(kFpsKey, QVariant::fromValue(0)).toInt();
  QString file_format = settings_->value(kFileFormatKey, QVariant::fromValue(QString())).toString();
  QString capture_type = settings_->value(kCaptureTypeKey, QVariant::fromValue(QString())).toString();
  QString video_encoder = settings_->value(kVideoEncoderKey, QVariant::fromValue(QString())).toString();

  int index = -1;

  int i = 0;
  for (; i < ARRAYSIZE(kFpsList); ++i) {
    if (fps == kFpsList[i]) {
      index = i;
      break;
    }
  }
  if (index == -1) {
    fps_ = kDefaultFps;
    settings_->setValue(kFpsKey, QVariant::fromValue(fps_));
  } else {
    fps_ = fps;
  }

  index = -1;
  i = 0;
  while (kFileFormatList[i]) {
    if (QString(kFileFormatList[i]) == file_format) {
      index = i;
      break;
    }
    ++i;
  }
  if (index == -1) {
    file_format_ = QString(kDefaultFileFormat);
    settings_->setValue(kFileFormatKey, QVariant::fromValue(file_format_));
  } else {
    file_format_ = file_format;
  }

  index = -1;
  i = 0;
  while (kCaptureTypeList[i]) {
    if (QString(kCaptureTypeList[i]) == capture_type) {
      index = i;
      break;
    }
    ++i;
  }
  if (index == -1) {
    capture_type_ = QString(kDefaultCaptureType);
    settings_->setValue(kCaptureTypeKey, QVariant::fromValue(capture_type_));
  } else {
    capture_type_ = capture_type;
  }

  index = -1;
  i = 0;
  for (; i < ARRAYSIZE(kVideoEncoderList); ++i) {
    if (video_encoder == QString(kVideoEncoderList[i].description)) {
      index = i;
      break;
    }
    ++i;
  }
  if (index == -1) {
    video_encoder_ = kDefaultVideoEncoder;
    settings_->setValue(kVideoEncoderKey, QVariant::fromValue(video_encoder_));
  } else {
    video_encoder_ = video_encoder;
  }
}
