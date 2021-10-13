#include "capturer/picture_capturer_d3d9.h"

#include <memory>

#include "glog/logging.h"

PictureCapturerD3D9::PictureCapturerD3D9()
    : d3d9_initialized_(false), width_(0), height_(0) {
  InitD3D9();
  DCHECK(d3d9_initialized_);
}

PictureCapturerD3D9::~PictureCapturerD3D9() {
  render_target_.Reset();
  dest_target_.Reset();
  d3d9_device_.Reset();
  d3d9_.Reset();
}

// https://blog.csdn.net/qianbo042311/article/details/117535125
bool PictureCapturerD3D9::CaptureScreen(AVData** av_data) {
  DCHECK(av_data);

  if (!d3d9_initialized_) {
    return false;
  }

  D3DLOCKED_RECT lr;
  ZeroMemory(&lr, sizeof(lr));
  HRESULT hr = d3d9_device_->GetFrontBufferData(0, dest_target_.Get());
  if (FAILED(hr)) {
    LOG(ERROR) << "GetFrontBufferData failed";
    return false;
  }

  // 绘制鼠标
  HDC hdc = NULL;
  if (dest_target_->GetDC(&hdc) == D3D_OK) {
    DrawMouseIcon(hdc);
    dest_target_->ReleaseDC(hdc);
  }

  hr = dest_target_->LockRect(&lr, NULL, D3DLOCK_READONLY);
  if (FAILED(hr) || !lr.pBits) {
    return false;
  }

  AVData* tmp = new AVData();
  tmp->type = AVData::VIDEO;
  tmp->len = height_ * lr.Pitch;;
  tmp->width = width_;
  tmp->height = height_;
  tmp->data = new uint8_t[tmp->len];
  memcpy(tmp->data, lr.pBits, sizeof(uint8_t) * tmp->len);

  dest_target_->UnlockRect();
  return true;
}

bool PictureCapturerD3D9::InitD3D9() {
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

  // D3DDEVTYPE_REF: 软渲染，速度较慢，兼容性高
  // D3DDEVTYPE_HAL: 支持硬件加速，性能高，兼容性略低
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
