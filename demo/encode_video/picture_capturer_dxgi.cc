#include "picture_capturer_dxgi.h"

#include "log.h"

using Microsoft::WRL::ComPtr;

PictureCapturerDXGI::PictureCapturerDXGI(int type)
    : initialized_(false),
      type_(type),
      width_(0),
      height_(0),
      output_desc_({}) {
  assert(type_ == 0 || type_ == 1);

  InitDXGI();
  assert(initialized_);
}

PictureCapturerDXGI::~PictureCapturerDXGI() {}

PictureCapturer::Picture* PictureCapturerDXGI::Capture() {
  ComPtr<IDXGIResource> dxgi_resource;
  DXGI_OUTDUPL_FRAME_INFO frame_info;
  HRESULT hr = desk_dupl_->AcquireNextFrame(
      500, &frame_info, dxgi_resource.GetAddressOf());
  if (FAILED(hr)) {
    if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
      LOG("屏幕没有变化");
    } else {
      LOG("DXGI截屏失败");
    }
    return nullptr;
  }

  ComPtr<ID3D11Texture2D> acquired_desktop_image;
  hr = dxgi_resource->QueryInterface(
      __uuidof(ID3D11Texture2D),
      reinterpret_cast<void**>(acquired_desktop_image.GetAddressOf()));
  if (FAILED(hr)) {
    LOG("Failed to desktop image");
    desk_dupl_->ReleaseFrame();
    return nullptr;
  }

  D3D11_TEXTURE2D_DESC frame_descriptor;
  acquired_desktop_image->GetDesc(&frame_descriptor);
  frame_descriptor.Usage = D3D11_USAGE_STAGING;
  frame_descriptor.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  frame_descriptor.BindFlags = 0;
  frame_descriptor.MiscFlags = 0;
  frame_descriptor.MipLevels = 1;
  frame_descriptor.ArraySize = 1;
  frame_descriptor.SampleDesc.Count = 1;

  ComPtr<ID3D11Texture2D> new_desktop_image;
  hr = d3d11_device_->CreateTexture2D(
      &frame_descriptor, NULL, new_desktop_image.GetAddressOf());
  if (FAILED(hr)) {
    LOG("Failed to call CreateTexture2D");
    desk_dupl_->ReleaseFrame();
    return nullptr;
  }

  d3d11_device_context_->CopyResource(new_desktop_image.Get(),
                                      acquired_desktop_image.Get());

  ComPtr<IDXGISurface> dxgi_surface;
  hr = new_desktop_image->QueryInterface(
      __uuidof(IDXGISurface),
      reinterpret_cast<void**>(dxgi_surface.GetAddressOf()));
  if (FAILED(hr)) {
    LOG("Failed to get DXGISurface");
    desk_dupl_->ReleaseFrame();
    return nullptr;
  }

  DXGI_MAPPED_RECT dxgi_mapped_rect;
  hr = dxgi_surface->Map(&dxgi_mapped_rect, DXGI_MAP_READ);
  if (FAILED(hr)) {
    LOG("Failed to map DXGISurface");
    desk_dupl_->ReleaseFrame();
    return nullptr;
  }

  const int len = frame_descriptor.Height * dxgi_mapped_rect.Pitch;

  Picture* picture = new Picture();
  picture->width = frame_descriptor.Width;
  picture->height = frame_descriptor.Height;
  if (type_ == 0) {
    picture->len = picture->width * picture->height * 3;
    picture->rgb = ARGBToRGB((const uint8_t*)dxgi_mapped_rect.pBits,
                             frame_descriptor.Width,
                             frame_descriptor.Height);
  } else {
    picture->len = len;
    picture->argb = new uint8_t[len];
    memcpy(picture->argb, dxgi_mapped_rect.pBits, len);
  }

  dxgi_surface->Unmap();
  desk_dupl_->ReleaseFrame();

  return picture;
}

bool PictureCapturerDXGI::InitDXGI() {
  HRESULT hr = S_OK;

  D3D_DRIVER_TYPE driver_types[] = {
      D3D_DRIVER_TYPE_HARDWARE,
      D3D_DRIVER_TYPE_WARP,
      D3D_DRIVER_TYPE_REFERENCE,
  };
  D3D_FEATURE_LEVEL d3d_feature_levels[] = {
      D3D_FEATURE_LEVEL_11_0,
      D3D_FEATURE_LEVEL_10_1,
      D3D_FEATURE_LEVEL_10_0,
      D3D_FEATURE_LEVEL_9_3,
      D3D_FEATURE_LEVEL_9_2,
      D3D_FEATURE_LEVEL_9_1,
  };

  D3D_FEATURE_LEVEL feature_level;
  for (uint32_t i = 0; i < ARRAYSIZE(driver_types); ++i) {
    hr = D3D11CreateDevice(nullptr,
                           driver_types[i],
                           nullptr,
                           D3D11_CREATE_DEVICE_DEBUG,
                           d3d_feature_levels,
                           ARRAYSIZE(d3d_feature_levels),
                           D3D11_SDK_VERSION,
                           d3d11_device_.GetAddressOf(),
                           &feature_level,
                           d3d11_device_context_.GetAddressOf());
    if (SUCCEEDED(hr)) {
      break;
    }
  }
  if (FAILED(hr)) {
    LOG("Failed to call D3D11CreateDevice");
    return false;
  }

  ComPtr<IDXGIDevice> dxgi_device;
  hr = d3d11_device_->QueryInterface(
      __uuidof(IDXGIDevice),
      reinterpret_cast<void**>(dxgi_device.GetAddressOf()));
  if (FAILED(hr)) {
    LOG("Failed to get DXGIDevice");
    return false;
  }

  ComPtr<IDXGIAdapter> dxgi_adapter;
  hr = dxgi_device->GetParent(
      __uuidof(IDXGIAdapter),
      reinterpret_cast<void**>(dxgi_adapter.GetAddressOf()));
  if (FAILED(hr)) {
    LOG("Failed to get DXGIAdapter");
    return false;
  }

  uint32_t output_count = 0;
  ComPtr<IDXGIOutput> dxgi_output;
  hr = dxgi_adapter->EnumOutputs(output_count, dxgi_output.GetAddressOf());
  if (FAILED(hr)) {
    LOG("Failed to call EnumOutputs");
    return false;
  }

  hr = dxgi_output->GetDesc(&output_desc_);
  if (FAILED(hr)) {
    LOG("Failed to get output desc");
    return false;
  }

  hr = dxgi_output->QueryInterface(
      __uuidof(IDXGIOutput1),
      reinterpret_cast<void**>(dxgi_output1_.GetAddressOf()));
  if (FAILED(hr)) {
    LOG("Failed to get DXGIOutput1");
    return false;
  }

  hr = dxgi_output1_->DuplicateOutput(d3d11_device_.Get(),
                                      desk_dupl_.GetAddressOf());
  if (FAILED(hr)) {
    LOG("Failed to call DuplicateOutput");
    return false;
  }

  initialized_ = true;
  return true;
}
