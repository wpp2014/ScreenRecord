#ifndef CAPTURER_VOICE_CAPTURER_H_
#define CAPTURER_VOICE_CAPTURER_H_

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

  // 调用waveInStart开始录音
  void Start();

  // 停止录音
  void Stop();

  // 暂停录音
  void Pause();

 private:
  static DWORD WINAPI HandleVoiceThread(void* param);

  void OnReceiveVoiceData(bool stop_capture,
                          bool pause_capture,
                          HWAVEIN wave_handle,
                          WAVEHDR* wave_header);

  static const int kWaveHeaderCount = 4;

  // 录音设备是否已打开
  bool device_is_opened_;

  // 是否正在录音
  bool is_recording_;

  // 调用Start函数的线程ID，确保在同一个线程调用Start和Stop
  DWORD start_thread_id_;

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

#endif  // CAPTURER_VOICE_CAPTURER_H_
