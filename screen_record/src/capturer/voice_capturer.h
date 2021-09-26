#ifndef SCREEN_RECORD_LIB_VOICE_CAPTURER_H_
#define SCREEN_RECORD_LIB_VOICE_CAPTURER_H_

#include <stdint.h>

#include <functional>

#include <windows.h>
#include <mmreg.h>
#include <mmsystem.h>

class VoiceCapturer {
 public:
  // channels: 声道数
  // samples_per_second: 每秒采样数
  // bits_per_sample: 每个样本字节数
  // format_type: 声音格式
  // handle_voice_callback: 处理声音数据的回调函数，在另外的线程里被触发
  explicit VoiceCapturer(
      uint16_t channels,
      uint32_t samples_per_second,
      uint16_t bits_per_sample,
      uint16_t format_type,
      const std::function<void(const uint8_t* data, int len)>& handle_voice_callback);
  ~VoiceCapturer();

  int Initialize();

  // 调用waveInStart开始录音
  bool Start();

  // 停止录音
  bool Stop();

 private:
  static DWORD WINAPI HandleVoiceThread(void* param);

  void OnReceiveVoiceData(bool stop_capture,
                          HWAVEIN wave_handle,
                          WAVEHDR* wave_header);

  static const int kWaveHeaderCount = 4;

  // 需要根据初始化的结果决定是否执行录音功能
  bool initialized_;

  // 如果为true，析构时需要关闭设备
  bool device_is_opened_;

  uint16_t channels_;
  uint32_t samples_per_second_;
  uint16_t bits_per_sample_;
  uint16_t format_type_;

  DWORD handle_data_thread_id_;
  HANDLE handle_data_thread_handle_;

  WAVEFORMATEX input_format_;
  HWAVEIN micor_handle_;

  int8_t* wave_buffers_[kWaveHeaderCount];
  WAVEHDR wave_headers_[kWaveHeaderCount];

  std::function<void(const uint8_t* data, int len)> handle_voice_callback_;

  VoiceCapturer() = delete;
  VoiceCapturer(const VoiceCapturer&) = delete;
  VoiceCapturer& operator=(const VoiceCapturer&) = delete;
};  // class VoiceCapturer

#endif  // SCREEN_RECORD_LIB_VOICE_CAPTURER_H_
