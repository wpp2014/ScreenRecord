#include "screen_record/src/capturer/voice_capturer.h"

#include <thread>

#include "glog/logging.h"

#define IDM_STOP_CAPTURE            6001

// static
DWORD WINAPI VoiceCapturer::HandleVoiceThread(void* param) {
  VoiceCapturer* capturer = static_cast<VoiceCapturer*>(param);
  DCHECK(capturer);

  bool stop_capture = false;
  MSG msg = { 0 };
  while (GetMessage(&msg, NULL, 0, 0)) {
    switch (msg.message) {
      case MM_WIM_OPEN:
        break;

      case MM_WIM_CLOSE:
        return 0;

      case MM_WIM_DATA:
        capturer->OnReceiveVoiceData(stop_capture,
                                     reinterpret_cast<HWAVEIN>(msg.wParam),
                                     reinterpret_cast<WAVEHDR*>(msg.lParam));
        break;

      case IDM_STOP_CAPTURE:
        stop_capture = true;
        break;

      default:
        break;
    }
  }

  return 0;
}

void VoiceCapturer::OnReceiveVoiceData(bool stop_capture,
                                       HWAVEIN wave_handle,
                                       WAVEHDR* wave_header) {
  if (!wave_handle || !wave_header) {
    DCHECK(false);
    return;
  }

  const uint8_t* data = (uint8_t*)wave_header->lpData;
  const int len = (int)wave_header->dwBytesRecorded;

  if (len > 0) {
    handle_voice_callback_(data, len);
  }

  if (!stop_capture) {
    MMRESULT result =
        waveInAddBuffer(wave_handle, wave_header, sizeof(WAVEHDR));
    DCHECK(result == MMSYSERR_NOERROR) << "waveInAddBuffer调用失败: " << result;
  }
}

VoiceCapturer::VoiceCapturer(
    uint16_t channels,
    uint32_t samples_per_second,
    uint16_t bits_per_sample,
    uint16_t format_type,
    const std::function<void(const uint8_t* data, int len)>& handle_voice_callback)
    : initialized_(false),
      device_is_opened_(false),
      channels_(channels),
      samples_per_second_(samples_per_second),
      bits_per_sample_(bits_per_sample),
      format_type_(format_type),
      handle_data_thread_id_(0),
      handle_data_thread_handle_(NULL),
      input_format_{},
      micor_handle_(NULL),
      handle_voice_callback_(handle_voice_callback) {
  // 目前只支持WAVE_FORMAT_PCM格式
  DCHECK(format_type_ == WAVE_FORMAT_PCM) << "只支持WAVE_FORMAT_PCM格式";

  for (int i = 0; i < kWaveHeaderCount; ++i) {
    wave_buffers_[i] = nullptr;
  }
}

VoiceCapturer::~VoiceCapturer() {
  if (device_is_opened_) {
    Stop();
  }

  for (int i = 0; i < kWaveHeaderCount; ++i) {
    if (wave_buffers_[i]) {
      delete[] wave_buffers_[i];
      wave_buffers_[i] = nullptr;
    }
  }
}

int VoiceCapturer::Initialize() {
  if (waveInGetNumDevs() <= 0) {
    LOG(WARNING) << "此电脑没有录音设备";
    return 1;
  }

  handle_data_thread_handle_ =
      CreateThread(NULL, 0, VoiceCapturer::HandleVoiceThread, this, 0,
                   &handle_data_thread_id_);
  if (!handle_data_thread_handle_) {
    LOG(ERROR) << "创建处理录入声音数据的线程失败";
    return 2;
  }

  ZeroMemory(&input_format_, sizeof(WAVEFORMATEX));
  input_format_.wFormatTag = format_type_;
  input_format_.nChannels = channels_;
  input_format_.nSamplesPerSec = samples_per_second_;
  input_format_.wBitsPerSample = bits_per_sample_;
  input_format_.nBlockAlign =
      input_format_.nChannels * (input_format_.wBitsPerSample / 8);
  input_format_.nAvgBytesPerSec =
      input_format_.nBlockAlign * input_format_.nSamplesPerSec;
  input_format_.cbSize = 0;

  MMRESULT result = waveInOpen(&micor_handle_, WAVE_MAPPER, &input_format_,
                               handle_data_thread_id_, NULL, CALLBACK_THREAD);
  if (result != MMSYSERR_NOERROR) {
    LOG(ERROR) << "调用waveInOpen函数失败";
    return 3;
  }
  device_is_opened_ = true;

  const int buf_size = samples_per_second_;
  for (int i = 0; i < kWaveHeaderCount; ++i) {
    wave_buffers_[i] = new int8_t[buf_size];

    ZeroMemory(&wave_headers_[i], sizeof(WAVEHDR));
    wave_headers_[i].dwBufferLength = buf_size;
    wave_headers_[i].lpData = reinterpret_cast<char*>(wave_buffers_[i]);
    result =
        waveInPrepareHeader(micor_handle_, &wave_headers_[i], sizeof(WAVEHDR));
    if (result != MMSYSERR_NOERROR) {
      LOG(ERROR) << "调用waveInPrepareHeader失败";
      return 4;
    }

    result = waveInAddBuffer(micor_handle_, &wave_headers_[i], sizeof(WAVEHDR));
    if (result != MMSYSERR_NOERROR) {
      LOG(ERROR) << "调用waveInAddBuffer失败";
      return 5;
    }
  }

  initialized_ = true;
  return 0;
}

bool VoiceCapturer::Start() {
  MMRESULT result = waveInStart(micor_handle_);
  return result == MMSYSERR_NOERROR;
}

bool VoiceCapturer::Stop() {
  if (!micor_handle_) {
    return true;
  }

  DCHECK(handle_data_thread_handle_);
  PostThreadMessage(handle_data_thread_id_, IDM_STOP_CAPTURE, 0, 0);

  MMRESULT result = waveInStop(micor_handle_);
  if (result != MMSYSERR_NOERROR) {
    return false;
  }

  result = waveInReset(micor_handle_);
  if (result != MMSYSERR_NOERROR) {
    return false;
  }

  // 等待缓冲区结束
  std::this_thread::sleep_for(std::chrono::seconds(1));

  for (int i = 0; i < kWaveHeaderCount; ++i) {
    WAVEHDR tmp = wave_headers_[i];
    result = waveInUnprepareHeader(micor_handle_, &wave_headers_[i],
                                   sizeof(WAVEHDR));
    DCHECK(result == MMSYSERR_NOERROR)
        << "waveInUnprepareHeader调用失败: " << result;
  }

  result = waveInClose(micor_handle_);
  if (result != MMSYSERR_NOERROR) {
    return false;
  }

  // 等待线程结束
  WaitForSingleObject(handle_data_thread_handle_, INFINITE);
  handle_data_thread_handle_ = NULL;
  handle_data_thread_id_ = 0;

  micor_handle_ = NULL;
  initialized_ = false;
  device_is_opened_ = false;

  return true;
}
