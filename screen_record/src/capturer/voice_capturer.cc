#include "screen_record/src/capturer/voice_capturer.h"

#include <QtCore/QDebug>

namespace {

// 声道数
const uint16_t kChannels = 2;
// 采样率：每秒钟采集的样本的个数
const uint32_t kSamplesPerSec = 44100;
// 每个样本bit数
const uint16_t kBitsPerSample = 16;

}

VoiceCapturer::VoiceCapturer(DWORD thread_id)
    : initialized_(false),
      device_is_opened_(false),
      handle_data_thread_id_(thread_id),
      micor_handle_(NULL) {
  Q_ASSERT(handle_data_thread_id_ != 0);

  for (int i = 0; i < kWaveHeaderCount; ++i) {
    wave_buffers_[i] = nullptr;
  }
}

VoiceCapturer::~VoiceCapturer() {
  if (device_is_opened_ && micor_handle_) {
    waveInClose(micor_handle_);
    device_is_opened_ = false;
    micor_handle_ = NULL;
  }
}

bool VoiceCapturer::Initialize() {
  return true;
}

bool VoiceCapturer::Start(int frame_size) {
  ZeroMemory(&input_format_, sizeof(WAVEFORMATEX));
  input_format_.wFormatTag = WAVE_FORMAT_PCM;
  input_format_.nChannels = kChannels;
  input_format_.nSamplesPerSec = kSamplesPerSec;
  input_format_.wBitsPerSample = kBitsPerSample;
  input_format_.nBlockAlign =
      input_format_.nChannels * (input_format_.wBitsPerSample / 8);
  input_format_.nAvgBytesPerSec =
      input_format_.nBlockAlign * input_format_.nSamplesPerSec;
  input_format_.cbSize = 0;

  MMRESULT result = waveInOpen(&micor_handle_, WAVE_MAPPER, &input_format_,
                               handle_data_thread_id_, NULL, CALLBACK_THREAD);
  if (result != MMSYSERR_NOERROR) {
    return false;
  }
  device_is_opened_ = true;

  const size_t buf_size = frame_size * 2 * 2 * 32;
  for (int i = 0; i < kWaveHeaderCount; ++i) {
    wave_buffers_[i] = new uint16_t[buf_size];

    ZeroMemory(&wave_headers_[i], sizeof(WAVEHDR));
    wave_headers_[i].dwBufferLength = buf_size;
    wave_headers_[i].lpData = reinterpret_cast<char*>(wave_buffers_[i]);

    result =
        waveInPrepareHeader(micor_handle_, &wave_headers_[i], sizeof(WAVEHDR));
    if (result != MMSYSERR_NOERROR) {
      return false;
    }

    result = waveInAddBuffer(micor_handle_, &wave_headers_[i], sizeof(WAVEHDR));
    if (result != MMSYSERR_NOERROR) {
      return false;
    }
  }

  result = waveInStart(micor_handle_);
  return result == MMSYSERR_NOERROR;
}

bool VoiceCapturer::Stop() {
  if (!micor_handle_) {
    return true;
  }

  MMRESULT result = waveInStop(micor_handle_);
  if (result != MMSYSERR_NOERROR) {
    return false;
  }

  result = waveInReset(micor_handle_);
  if (result != MMSYSERR_NOERROR) {
    return false;
  }

  for (int i = 0; i < kWaveHeaderCount; ++i) {
    result = waveInUnprepareHeader(micor_handle_, &wave_headers_[i],
                                   sizeof(WAVEHDR));
    Q_ASSERT(result == MMSYSERR_NOERROR);
    delete[] wave_buffers_[i];
  }

  result = waveInClose(micor_handle_);
  if (result != MMSYSERR_NOERROR) {
    return false;
  }

  micor_handle_ = NULL;
  initialized_ = false;

  return false;
}

int VoiceCapturer::channels() const {
  return static_cast<int>(input_format_.nChannels);
}

int VoiceCapturer::sample_rate() const {
  return static_cast<int>(input_format_.nSamplesPerSec);
}

uint64_t VoiceCapturer::channel_layout() const {
  return 3;  // AV_CH_LAYOUT_STEREO
}
