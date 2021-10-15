#ifndef SCREEN_RECORD_SRC_SETTING_SETTING_MANAGER_H_
#define SCREEN_RECORD_SRC_SETTING_SETTING_MANAGER_H_

#include <QtCore/QScopedPointer>
#include <QtCore/QString>

#include "base/files/file_path.h"
#include "encoder/ffmpeg.h"

class QSettings;

class SettingManager {
 public:
  struct VideoEncoderInfo {
    AVCodecID codec_id;
    const char* description;
  };

  static constexpr int kDefaultFps = 25;
  static constexpr AVCodecID kDefaultVideoCodecID = AV_CODEC_ID_H264;
  static constexpr char* kDefaultVideoEncoder = "H.264(x264)";
  static constexpr char* kDefaultFileFormat = "mp4";
  static constexpr char* kDefaultCaptureType = "GDI";

  static constexpr int kFpsList[] = { 16, 25, 30, 60 };
  static constexpr char* kCaptureTypeList[] = { "GDI", "DXGI", nullptr };
  static constexpr char* kFileFormatList[] = { "mp4", "mkv", nullptr };
  static constexpr VideoEncoderInfo kVideoEncoderList[] = {
    {AV_CODEC_ID_H264, "H.264(x264)"},
  };

  static SettingManager* GetInstance();

  int fps() const { return fps_; }
  QString FileFormat() const { return file_format_; }
  QString CaptureType() const { return capture_type_; }
  QString VideoEncoder() const { return video_encoder_; }

  AVCodecID VideoCodecID() const;

  void SetFps(int new_fps);
  void SetFileFormat(const QString& new_format);
  void SetCaptureType(const QString& new_type);
  void SetVideoEncoder(const QString& new_encoder);

 private:
  SettingManager();
  ~SettingManager();

  // 配置恢复初始值
  void Reset();

  // 解析设置
  void DecodeConfig();

  SettingManager(const SettingManager&) = delete;
  SettingManager& operator=(const SettingManager&) = delete;

  int32_t fps_;
  QString file_format_;
  QString capture_type_;
  QString video_encoder_;

  QString config_folder_;
  QString config_path_;

  QScopedPointer<QSettings> settings_;
};  // class SettingManager

#endif  // SCREEN_RECORD_SRC_SETTING_SETTING_MANAGER_H_
