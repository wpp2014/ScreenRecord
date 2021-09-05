#include "screen_record/src/capturer/picture_capturer.h"

#include <QtCore/QDebug>

using Microsoft::WRL::ComPtr;

// PictureCapturer::Picture ----------------------------------------------------

PictureCapturer::Picture::Picture()
    : width_(0), height_(0), stride_(0), data_(nullptr) {}

PictureCapturer::Picture::Picture(const Picture& other)
    : width_(other.width_), height_(other.height_), stride_(other.stride_) {
  if (other.data_) {
    int len = stride_ * height_;
    Q_ASSERT(len > 0);

    data_ = new uint8_t[len];
    memcpy(data_, other.data_, sizeof(uint8_t) * len);
  }
}

PictureCapturer::Picture::Picture(Picture&& other)
    : width_(other.width_),
      height_(other.height_),
      stride_(other.stride_),
      data_(other.data_) {
  other.width_ = other.height_ = other.stride_ = 0;
  other.data_ = nullptr;
}

PictureCapturer::Picture::~Picture() {
  width_ = height_ = stride_ = 0;
  if (data_) {
    delete[] data_;
    data_ = nullptr;
  }
}

void PictureCapturer::Picture::Reset() {
  width_ = height_ = stride_ = 0;
  if (data_) {
    delete[] data_;
    data_ = nullptr;
  }
}

void PictureCapturer::Picture::Reset(const uint8_t* data,
                                     int width,
                                     int height,
                                     int stride) {
  Q_ASSERT(width > 0 && height > 0 && stride > 0 && data);

  if (data_) {
    delete[] data_;
  }

  width_ = width;
  height_ = height;
  stride_ = stride;

  int len = stride * height;
  data_ = new uint8_t[len];
  memcpy(data_, data, sizeof(uint8_t) * len);
}

PictureCapturer::Picture& PictureCapturer::Picture::operator=(
    const Picture& other) {
  width_ = other.width_;
  height_ = other.height_;
  stride_ = other.stride_;

  if (other.data_) {
    Q_ASSERT(width_ > 0 && stride_ > 0 && height_ > 0);
    int len = stride_ * height_;

    data_ = new uint8_t[len];
    memcpy(data_, other.data_, sizeof(uint8_t) * len);
  }

  return *this;
}

PictureCapturer::Picture& PictureCapturer::Picture::operator=(Picture&& other) {
  width_ = other.width_;
  height_ = other.height_;
  stride_ = other.stride_;
  data_ = other.data_;

  other.width_ = other.height_ = other.stride_ = 0;
  other.data_ = nullptr;

  return *this;
}

// PictureCapturer -------------------------------------------------------------

PictureCapturer::PictureCapturer()
    : d3d9_initialized_(false), width_(0), height_(0) {
  InitD3D9();
}

PictureCapturer::~PictureCapturer() {
  d3d9_.Reset();
  d3d9_device_.Reset();
  render_target_.Reset();
  dest_target_.Reset();
}

std::unique_ptr<PictureCapturer::Picture> PictureCapturer::CaptureScreen() {
  if (!d3d9_initialized_) {
    return nullptr;
  }

  D3DLOCKED_RECT lr;
  ZeroMemory(&lr, sizeof(lr));
  HRESULT hr = d3d9_device_->GetFrontBufferData(0, dest_target_.Get());
  if (FAILED(hr)) {
    qDebug() << "GetFrontBufferData failed";
    return nullptr;
  }

  hr = dest_target_->LockRect(&lr, NULL, D3DLOCK_READONLY);
  if (FAILED(hr) || !lr.pBits) {
    nullptr;
  }

  std::unique_ptr<Picture> picture(new Picture());
  picture->Reset((const uint8_t*)lr.pBits, width_, height_, lr.Pitch);

  dest_target_->UnlockRect();
  return std::move(picture);
}

bool PictureCapturer::InitD3D9() {
  d3d9_ = Direct3DCreate9(D3D_SDK_VERSION);
  if (!d3d9_.Get()) {
    return false;
  }

  D3DDISPLAYMODE d3d_display_mode;
  HRESULT hr =
      d3d9_->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3d_display_mode);
  if (FAILED(hr)) {
    return false;
  }

  width_ = d3d_display_mode.Width;
  height_ = d3d_display_mode.Height;

  D3DPRESENT_PARAMETERS d3d9_param;
  ZeroMemory(&d3d9_param, sizeof(d3d9_param));
  d3d9_param.Windowed = TRUE;
  d3d9_param.SwapEffect = D3DSWAPEFFECT_COPY;

  hr = d3d9_->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                           GetDesktopWindow(),
                           D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3d9_param,
                           d3d9_device_.GetAddressOf());
  if (FAILED(hr)) {
    return false;
  }

  hr = d3d9_device_->GetRenderTarget(0, render_target_.GetAddressOf());
  if (FAILED(hr)) {
    return false;
  }

  hr = d3d9_device_->CreateOffscreenPlainSurface(
      width_, height_, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM,
      dest_target_.GetAddressOf(), NULL);
  if (FAILED(hr)) {
    return false;
  }

  d3d9_initialized_ = true;
  return true;
}
