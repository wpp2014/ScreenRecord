#include "screen_capture/dxgi_capture.h"

#include <comdef.h>

#include <assert.h>

#include "DirectXTex.h"

#include "screen_capture/screen_picture.h"

using Microsoft::WRL::ComPtr;

const uint32_t kTimeout = 1000;

DXGICapture::DXGICapture()
    : initialized_(false)
    , d3d_feature_level_(D3D_FEATURE_LEVEL_11_1) {
  Initialize();
}

DXGICapture::~DXGICapture() {}

ScreenPicture* DXGICapture::Capture() {
  if (!initialized_) {
    printf("failed to initialize\n");
    return nullptr;
  }

  HRESULT hr = S_OK;
  _com_error err(hr);

  ComPtr<IDXGIOutputDuplication> output_duplication;
  hr = dxgi_output1_->DuplicateOutput(d3d11_device_.Get(), output_duplication.GetAddressOf());
  if (FAILED(hr)) {
    err = _com_error(hr);
    wprintf(L"failed to call DuplicateOutput: %ls\n", err.ErrorMessage());
    return nullptr;
  }

  DXGI_OUTDUPL_FRAME_INFO frame_info = { 0 };
  ComPtr<IDXGIResource> dxgi_resource;
  while (true) {
    err = _com_error(output_duplication->AcquireNextFrame(
        kTimeout, &frame_info, dxgi_resource.GetAddressOf()));
    if (SUCCEEDED(err.Error()) && frame_info.LastPresentTime.QuadPart) {
      break;
    }

    wprintf(L"continue call AcquireNextFrame: %ls\n", err.ErrorMessage());
    output_duplication->ReleaseFrame();
    dxgi_resource.Reset();
    ZeroMemory(&frame_info, sizeof(DXGI_OUTDUPL_FRAME_INFO));
  }

  ComPtr<ID3D11Texture2D> texture;
  err = _com_error(dxgi_resource->QueryInterface(
      __uuidof(ID3D11Texture2D), (void**)texture.GetAddressOf()));
  if (FAILED(err.Error())) {
    wprintf(L"failed to get ID3D11Texture2D: %ls\n", err.ErrorMessage());
    return nullptr;
  }

#if 0

  DirectX::ScratchImage scratch_image;
  hr = DirectX::CaptureTexture(d3d11_device_.Get(),
                               d3d11_device_ctx_.Get(),
                               texture.Get(),
                               scratch_image);
  if (FAILED(hr)) {
    err = _com_error(hr);
    wprintf(L"failed to scratch image: %ls\n", err.ErrorMessage());
    return nullptr;
  }

  const DirectX::Image* image = scratch_image.GetImage(0, 0, 0);
  assert(image && image->pixels);

  ScreenPicture* picture = new ScreenPicture;
  picture->width = image->width;
  picture->height = image->height;
  picture->size = image->height * image->rowPitch;
  picture->argb = new uint8_t[picture->size];
  memcpy(picture->argb, image->pixels, picture->size);

#else

  D3D11_TEXTURE2D_DESC desc;
  texture->GetDesc(&desc);
  desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE | D3D11_CPU_ACCESS_READ;
  desc.Usage = D3D11_USAGE_STAGING;
  desc.BindFlags = 0;
  desc.MiscFlags = 0; // D3D11_RESOURCE_MISC_GDI_COMPATIBLE ?

  ComPtr<ID3D11Texture2D> cpu_texture;
  hr = d3d11_device_->CreateTexture2D(&desc, nullptr, cpu_texture.GetAddressOf());
  if (FAILED(hr)) {
    printf("Failed to create cpu texture: 0x%x\n", hr);
    return nullptr;
  }
  d3d11_device_ctx_->CopyResource(cpu_texture.Get(), texture.Get());

  D3D11_MAPPED_SUBRESOURCE sr;
  hr = d3d11_device_ctx_->Map(cpu_texture.Get(), 0, D3D11_MAP_READ, 0, &sr);
  if (FAILED(hr)) {
    printf("Failed to map gpu texture to cpu texture: 0x%x\n", hr);
    return nullptr;
  }

  ScreenPicture* picture = new ScreenPicture;
  picture->width = desc.Width;
  picture->height = desc.Height;
  picture->size = desc.Width * desc.Height * 4;
  picture->argb = new uint8_t[picture->size];

  const uint32_t row_len = desc.Width * 4;
  for (uint32_t y = 0; y < desc.Height; ++y) {
    memcpy(picture->argb + y * row_len, (uint8_t*)sr.pData + sr.RowPitch * y, row_len);
  }

  d3d11_device_ctx_->Unmap(cpu_texture.Get(), 0);

#endif

  return picture;
}

void DXGICapture::Initialize() {
  HRESULT hr = S_OK;
  _com_error err(S_OK);

  D3D_DRIVER_TYPE driver_types[] = {
      D3D_DRIVER_TYPE_HARDWARE,
      D3D_DRIVER_TYPE_WARP,
      D3D_DRIVER_TYPE_REFERENCE,
  };
  const uint32_t driver_type_count = ARRAYSIZE(driver_types);

  for (uint32_t i = 0; i < driver_type_count; ++i) {
    hr = D3D11CreateDevice(nullptr,
                           driver_types[i],
                           nullptr,
                           D3D11_CREATE_DEVICE_DEBUG || D3D11_CREATE_DEVICE_SINGLETHREADED,
                           nullptr,
                           0,
                           D3D11_SDK_VERSION,
                           d3d11_device_.GetAddressOf(),
                           &d3d_feature_level_,
                           d3d11_device_ctx_.GetAddressOf());
    if (SUCCEEDED(hr)) {
      break;
    }
  }
  if (FAILED(hr)) {
    err = _com_error(hr);
    wprintf(L"failed to call D3D11CreateDevice: %ls\n", err.ErrorMessage());
    return;
  }

  hr = d3d11_device_->QueryInterface(__uuidof(IDXGIDevice), (void**)dxgi_device_.GetAddressOf());
  if (FAILED(hr)) {
    err = _com_error(hr);
    wprintf(L"failed to get IDXGIDevice: %ls\n", err.ErrorMessage());
    return;
  }

  hr = dxgi_device_->GetParent(__uuidof(IDXGIAdapter), (void**)dxgi_adapter_.GetAddressOf());
  if (FAILED(hr)) {
    err = _com_error(hr);
    wprintf(L"failed to get IDXGIAdapter: %ls\n", err.ErrorMessage());
    return;
  }

  for (int i = 0; ; ++i) {
    if (dxgi_adapter_->EnumOutputs(i, dxgi_output_.GetAddressOf()) == DXGI_ERROR_NOT_FOUND) {
      printf("No output detected.\n");
      return;
    }

    DXGI_OUTPUT_DESC desc;
    dxgi_output_->GetDesc(&desc);
    if (desc.AttachedToDesktop) {
      break;
    }
  }

  hr = dxgi_output_->QueryInterface(__uuidof(IDXGIOutput1), (void**)dxgi_output1_.GetAddressOf());
  if (FAILED(hr)) {
    err = _com_error(hr);
    wprintf(L"failed to get IDXGIOutput1: %ls\n", err.ErrorMessage());
    return;
  }

  initialized_ = true;
}
