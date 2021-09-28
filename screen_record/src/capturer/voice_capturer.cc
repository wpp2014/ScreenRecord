#include "screen_record/src/capturer/voice_capturer.h"

#include <thread>

#include "glog/logging.h"

#define IDM_STOP_CAPTURE            6001
#define IDM_PAUSE_CAPTURE           6002

// static
DWORD WINAPI VoiceCapturer::HandleVoiceThread(void* param) {
  VoiceCapturer* capturer = static_cast<VoiceCapturer*>(param);
  DCHECK(capturer);

  bool stop_capture = false;
  bool pause_capture = false;

  MSG msg = { 0 };
  while (GetMessage(&msg, NULL, 0, 0)) {
    switch (msg.message) {
      case MM_WIM_OPEN:
        break;

      case MM_WIM_CLOSE:
        return 0;

      case MM_WIM_DATA:
        capturer->OnReceiveVoiceData(stop_capture, pause_capture,
                                     reinterpret_cast<HWAVEIN>(msg.wParam),
                                     reinterpret_cast<WAVEHDR*>(msg.lParam));
        break;

      case IDM_STOP_CAPTURE:
        stop_capture = true;
        break;

      case IDM_PAUSE_CAPTURE:
        pause_capture = !pause_capture;
        break;

      default:
        break;
    }
  }

  return 0;
}

void VoiceCapturer::OnReceiveVoiceData(bool stop_capture,
                                       bool pause_capture,
                                       HWAVEIN wave_handle,
                                       WAVEHDR* wave_header) {
  if (!wave_handle || !wave_header) {
    DCHECK(false);
    return;
  }

  const uint8_t* data = (uint8_t*)wave_header->lpData;
  const int len = (int)wave_header->dwBytesRecorded;

  if (len > 0 && !pause_capture) {
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
    : device_is_opened_(false),
      is_recording_(false),
      start_thread_id_(0),
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

void VoiceCapturer::Start() {
  DCHECK(!is_recording_);

  DCHECK(start_thread_id_ == 0);
  start_thread_id_ = GetCurrentThreadId();

  if (waveInGetNumDevs() <= 0) {
    LOG(WARNING) << "未找到录音设备";
    return;
  }

  // 创建处理音频数据的线程
  DCHECK(handle_data_thread_handle_ == NULL);
  handle_data_thread_handle_ =
      CreateThread(NULL, 0, VoiceCapturer::HandleVoiceThread, this, 0,
                   &handle_data_thread_id_);
  if (!handle_data_thread_handle_) {
    LOG(ERROR) << "创建处理音频数据的线程失败";
    return;
  }

  bool res = false;
  MMRESULT result = MMSYSERR_NOERROR;

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

  result = waveInOpen(&micor_handle_, WAVE_MAPPER, &input_format_,
                      handle_data_thread_id_, NULL, CALLBACK_THREAD);
  if (result != MMSYSERR_NOERROR) {
    goto end;
  }

  // 录音设备已打开
  device_is_opened_ = true;

  for (int i = 0; i < kWaveHeaderCount; ++i) {
    wave_buffers_[i] = new int8_t[samples_per_second_];

    ZeroMemory(&wave_headers_[i], sizeof(WAVEHDR));
    wave_headers_[i].dwBufferLength = samples_per_second_;
    wave_headers_[i].lpData = reinterpret_cast<char*>(wave_buffers_[i]);
    result =
        waveInPrepareHeader(micor_handle_, &wave_headers_[i], sizeof(WAVEHDR));
    if (result != MMSYSERR_NOERROR) {
      LOG(ERROR) << "调用waveInPrepareHeader失败";
      goto end;
    }

    result = waveInAddBuffer(micor_handle_, &wave_headers_[i], sizeof(WAVEHDR));
    if (result != MMSYSERR_NOERROR) {
      LOG(ERROR) << "调用waveInAddBuffer失败";
      goto end;
    }
  }

  result = waveInStart(micor_handle_);
  if (res != MMSYSERR_NOERROR) {
    LOG(ERROR) << "开始录音失败";
    goto end;
  }

  is_recording_ = true;
  res = true;

end:
  if (res) {
    return;
  }

  TerminateThread(handle_data_thread_handle_, 0);
  handle_data_thread_handle_ = NULL;

  if (device_is_opened_) {
    waveInClose(micor_handle_);
    device_is_opened_ = false;
    micor_handle_ = NULL;
  }

  for (int i = 0; i < kWaveHeaderCount; ++i) {
    if (wave_buffers_[i]) {
      delete[] wave_buffers_[i];
      wave_buffers_[i] = nullptr;
    }
  }
}

void VoiceCapturer::Stop() {
  DCHECK(start_thread_id_ == GetCurrentThreadId());

  if (!is_recording_) {
    return;
  }

  // 发送消息通知录音结束
  PostThreadMessage(handle_data_thread_id_, IDM_STOP_CAPTURE, 0, 0);

  MMRESULT result = MMSYSERR_NOERROR;

  result = waveInStop(micor_handle_);
  if (result != MMSYSERR_NOERROR) {
    goto end;
  }

  result = waveInReset(micor_handle_);
  if (result != MMSYSERR_NOERROR) {
    goto end;
  }

  // 等待缓冲区结束
  std::this_thread::sleep_for(std::chrono::seconds(1));

  for (int i = 0; i < kWaveHeaderCount; ++i) {
    result = waveInUnprepareHeader(
        micor_handle_, &wave_headers_[i], sizeof(WAVEHDR));
    if (result != MMSYSERR_NOERROR) {
      goto end;
    }
  }

  result = waveInClose(micor_handle_);
  if (result != MMSYSERR_NOERROR) {
    goto end;
  }

end:
  // 等待线程结束
  WaitForSingleObject(handle_data_thread_handle_, INFINITE);
  handle_data_thread_handle_ = NULL;
  handle_data_thread_id_ = 0;

  for (int i = 0; i < kWaveHeaderCount; ++i) {
    if (wave_buffers_[i]) {
      delete[] wave_buffers_[i];
      wave_buffers_[i] = nullptr;
    }
  }

  micor_handle_ = NULL;
  device_is_opened_ = false;
  is_recording_ = false;

  start_thread_id_ = 0;
}

void VoiceCapturer::Pause() {
  if (!is_recording_) {
    return;
  }
  PostThreadMessage(handle_data_thread_id_, IDM_PAUSE_CAPTURE, 0, 0);
}
