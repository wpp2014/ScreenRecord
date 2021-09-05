#ifndef SCREEN_RECORD_LIB_VOICE_CAPTURER_H_
#define SCREEN_RECORD_LIB_VOICE_CAPTURER_H_

#include <windows.h>
#include <mmreg.h>
#include <mmsystem.h>

#include <stdint.h>

class VoiceCapturer {
 public:
  // thread_id: 处理录音数据的线程ID
  explicit VoiceCapturer(DWORD thread_id);
  ~VoiceCapturer();

  bool Initialize();

  // 调用waveInStart开始录音
  bool Start(int frame_size);

  // 停止录音
  bool Stop();

  int channels() const;
  int sample_rate() const;
  uint64_t channel_layout() const;

 private:
  static const int kWaveHeaderCount = 16;

  // 需要根据初始化的结果决定是否执行录音功能
  bool initialized_;

  // 如果为true，析构时需要关闭设备
  bool device_is_opened_;

  DWORD handle_data_thread_id_;

  WAVEFORMATEX input_format_;
  HWAVEIN micor_handle_;

  uint16_t* wave_buffers_[kWaveHeaderCount];
  WAVEHDR wave_headers_[kWaveHeaderCount];

  VoiceCapturer() = delete;
  VoiceCapturer(const VoiceCapturer&) = delete;
  VoiceCapturer& operator=(const VoiceCapturer&) = delete;
};  // class VoiceCapturer

#endif  // SCREEN_RECORD_LIB_VOICE_CAPTURER_H_
