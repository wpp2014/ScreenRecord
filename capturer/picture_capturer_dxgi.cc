#include "capturer/picture_capturer_dxgi.h"

#include <assert.h>

#include "glog/logging.h"

using Microsoft::WRL::ComPtr;

std::string NumToHexStr(HRESULT hr) {
  char buffer[16];
  sprintf(buffer, "0x%x\0", hr);
  return buffer;
}

PictureCapturerDXGI::PictureCapturerDXGI()
    : dxgi_initialized_(false) {
  ZeroMemory(&output_desc_, sizeof(output_desc_));
  ZeroMemory(&desktop_bound_, sizeof(desktop_bound_));
  ZeroMemory(&pointer_info_, sizeof(pointer_info_));

  dxgi_initialized_ = InitDXGI();
  DCHECK(dxgi_initialized_);
}

PictureCapturerDXGI::~PictureCapturerDXGI() {
}

bool PictureCapturerDXGI::CaptureScreen(AVData** av_data) {
  DCHECK(av_data);

  ComPtr<IDXGIResource> dxgi_resource;
  DXGI_OUTDUPL_FRAME_INFO frame_info;
  HRESULT hr = desk_dupl_->AcquireNextFrame(
      10, &frame_info, dxgi_resource.GetAddressOf());
  if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
    return true;
  }
  if (FAILED(hr)) {
    LOG(ERROR) << "抓屏失败: " << NumToHexStr(hr);
    return false;
  }

  acquired_desktop_image_.Reset();
  hr = dxgi_resource->QueryInterface(acquired_desktop_image_.GetAddressOf());
  if (FAILED(hr)) {
    LOG(ERROR) << "Failed to desktop image: " << NumToHexStr(hr);
    desk_dupl_->ReleaseFrame();
    return false;
  }

  d3d11_device_context_->CopyResource(
      shared_image_.Get(), acquired_desktop_image_.Get());

  // 绘制鼠标
  if (GetMouse(&frame_info) && pointer_info_.visible) {
    DrawMouse();
  }

  ComPtr<IDXGISurface> dxgi_surface;
  hr = shared_image_->QueryInterface(dxgi_surface.GetAddressOf());
  if (FAILED(hr)) {
    LOG(ERROR) << "Failed to get DXGISurface: " << NumToHexStr(hr);
    desk_dupl_->ReleaseFrame();
    return false;
  }

  DXGI_MAPPED_RECT dxgi_mapped_rect;
  hr = dxgi_surface->Map(&dxgi_mapped_rect, DXGI_MAP_READ);
  if (FAILED(hr)) {
    LOG(ERROR) << "Failed to map DXGISurface: " << NumToHexStr(hr);
    desk_dupl_->ReleaseFrame();
    return false;
  }

  D3D11_TEXTURE2D_DESC full_desc;
  shared_image_->GetDesc(&full_desc);

  AVData* tmp = new AVData();
  tmp->type = AVData::VIDEO;
  tmp->width = full_desc.Width;
  tmp->height = full_desc.Height;
  tmp->len = full_desc.Width * full_desc.Height * 4;
  tmp->data = new uint8_t[tmp->len];
  memcpy(tmp->data, (uint8_t*)dxgi_mapped_rect.pBits, sizeof(uint8_t) * tmp->len);

  *av_data = tmp;

  hr = dxgi_surface->Unmap();
  if (FAILED(hr)) {
    LOG(ERROR) << "Failed to unmap IDXGISurface: " << NumToHexStr(hr);
    desk_dupl_->ReleaseFrame();
    delete *av_data;
    *av_data = nullptr;
    return false;
  }

  desk_dupl_->ReleaseFrame();
  return true;
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

  const uint32_t driver_type_count = ARRAYSIZE(driver_types);
  const uint32_t feature_level_count = ARRAYSIZE(d3d_feature_levels);

  D3D_FEATURE_LEVEL feature_level;
  for (uint32_t i = 0; i < driver_type_count; ++i) {
    hr = D3D11CreateDevice(nullptr,
                           driver_types[i],
                           nullptr,
                           0,
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
    LOG(ERROR) << "Failed to call D3D11CreateDevice: " << NumToHexStr(hr);
    return false;
  }

  ComPtr<IDXGIDevice> dxgi_device;
  hr = d3d11_device_->QueryInterface(dxgi_device.GetAddressOf());
  if (FAILED(hr)) {
    LOG(ERROR) << "Failed to get DXGIDevice: " << NumToHexStr(hr);
    return false;
  }

  ComPtr<IDXGIAdapter> dxgi_adapter;
  hr = dxgi_device->GetParent(
      __uuidof(IDXGIAdapter),
      reinterpret_cast<void**>(dxgi_adapter.GetAddressOf()));
  if (FAILED(hr)) {
    LOG(ERROR) << "Failed to get DXGIAdapter: " << NumToHexStr(hr);
    return false;
  }

  // 0: 暂时只截取主显示器
  ComPtr<IDXGIOutput> dxgi_output;
  hr = dxgi_adapter->EnumOutputs(0, dxgi_output.GetAddressOf());
  if (FAILED(hr)) {
    LOG(ERROR) << "Failed to call EnumOutputs: " << NumToHexStr(hr);
    return false;
  }

  hr = dxgi_output->GetDesc(&output_desc_);
  if (FAILED(hr)) {
    LOG(ERROR) << "Failed to get output desc: " << NumToHexStr(hr);
    return false;
  }

  // 截屏区域
  desktop_bound_ = output_desc_.DesktopCoordinates;

  ComPtr<IDXGIOutput1> dxgi_output1;
  hr = dxgi_output->QueryInterface(dxgi_output1.GetAddressOf());
  if (FAILED(hr)) {
    LOG(ERROR) << "Failed to get DXGIOutput1: " << NumToHexStr(hr);
    return false;
  }

  hr = dxgi_output1->DuplicateOutput(d3d11_device_.Get(),
                                     desk_dupl_.GetAddressOf());
  if (FAILED(hr)) {
    LOG(ERROR) << "Failed to call DuplicateOutput: " << NumToHexStr(hr);
    return false;
  }

  D3D11_TEXTURE2D_DESC desc;
  desc.Width = desktop_bound_.right - desktop_bound_.left;
  desc.Height = desktop_bound_.bottom - desktop_bound_.top;
  desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  desc.Usage = D3D11_USAGE_STAGING;
  desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  desc.BindFlags = 0;
  desc.MiscFlags = 0;
  desc.MipLevels = 1;
  desc.ArraySize = 1;
  desc.SampleDesc.Count = 1;
  desc.SampleDesc.Quality = 0;

  hr = d3d11_device_->CreateTexture2D(
      &desc, NULL, shared_image_.GetAddressOf());
  if (FAILED(hr)) {
    LOG(ERROR) << "Failed to create shared surface: " << NumToHexStr(hr);
    return false;
  }

  return true;
}

bool PictureCapturerDXGI::GetMouse(DXGI_OUTDUPL_FRAME_INFO* frame_info) {
  if (frame_info->LastMouseUpdateTime.QuadPart == 0) {
    return true;
  }

  bool update_position = true;
  if (!frame_info->PointerPosition.Visible) {
    update_position = false;
  }
  if (frame_info->PointerPosition.Visible && pointer_info_.visible &&
      pointer_info_.last_time_stamp.QuadPart > frame_info->LastMouseUpdateTime.QuadPart) {
    update_position = false;
  }

  if (update_position) {
    pointer_info_.position.x = frame_info->PointerPosition.Position.x;
    pointer_info_.position.y = frame_info->PointerPosition.Position.y;
    pointer_info_.last_time_stamp = frame_info->LastMouseUpdateTime;
    pointer_info_.visible = (frame_info->PointerPosition.Visible != FALSE);
  }

  if (frame_info->PointerShapeBufferSize == 0) {
    return true;
  }

  // 重新分配内存
  if (frame_info->PointerShapeBufferSize > pointer_info_.buffer_size) {
    if (pointer_info_.shape_buffer) {
      delete[] pointer_info_.shape_buffer;
      pointer_info_.shape_buffer = nullptr;
    }

    pointer_info_.shape_buffer =
        new (std::nothrow) uint8_t[frame_info->PointerShapeBufferSize];
    if (pointer_info_.shape_buffer == nullptr) {
      LOG(ERROR) << "分配内存失败。";
      return false;
    }
    pointer_info_.buffer_size = frame_info->PointerShapeBufferSize;
  }

  // 获取鼠标
  uint32_t buffer_required_size = 0;
  HRESULT hr = desk_dupl_->GetFramePointerShape(
      frame_info->PointerShapeBufferSize,
      reinterpret_cast<void*>(pointer_info_.shape_buffer),
      &buffer_required_size,
      &pointer_info_.shape_info);
  if (FAILED(hr)) {
    LOG(ERROR) << "获取光标数据失败: " << NumToHexStr(hr);

    delete pointer_info_.shape_buffer;
    pointer_info_.shape_buffer = nullptr;
    pointer_info_.buffer_size = 0;

    return false;
  }

  return true;
}

void PictureCapturerDXGI::DrawMouse() {
  int shape_width = 0;
  int shape_height = 0;
  int shape_x = 0;
  int shape_y = 0;
  uint8_t* init_buffer = nullptr;

  D3D11_BOX box;
  box.front = 0;
  box.back = 1;

  D3D11_SUBRESOURCE_DATA shape_init_data;
  RtlZeroMemory(&shape_init_data, sizeof(shape_init_data));
  shape_init_data.SysMemSlicePitch = 0;

  D3D11_TEXTURE2D_DESC shape_desc;
  RtlZeroMemory(&shape_desc, sizeof(shape_desc));

  ComPtr<ID3D11Texture2D> shape_texture;
  HRESULT hr = S_OK;

  bool res = true;
  switch (pointer_info_.shape_info.Type) {
    case DXGI_OUTDUPL_POINTER_SHAPE_TYPE_COLOR:
      DCHECK(pointer_info_.shape_buffer);

      shape_x = pointer_info_.position.x;
      shape_y = pointer_info_.position.y;
      shape_width = pointer_info_.shape_info.Width;
      shape_height = pointer_info_.shape_info.Height;

      shape_init_data.pSysMem = pointer_info_.shape_buffer;
      shape_init_data.SysMemPitch = pointer_info_.shape_info.Pitch;
      break;

    case DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MONOCHROME:
      res = ProcessMonoMask(
          true, &shape_width, &shape_height, &shape_x, &shape_y, &init_buffer, &box);
      shape_init_data.pSysMem = init_buffer;
      shape_init_data.SysMemPitch = shape_width * 4;
      break;

    case DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MASKED_COLOR:
      res = ProcessMonoMask(
          false, &shape_width, &shape_height, &shape_x, &shape_y, &init_buffer, &box);
      shape_init_data.pSysMem = init_buffer;
      shape_init_data.SysMemPitch = shape_width * 4;
      break;

    default:
      DCHECK(false);
      break;
  }

  if (!res) {
    goto end;
  }

  shape_desc.Width = shape_width;
  shape_desc.Height = shape_height;
  shape_desc.MipLevels = 1;
  shape_desc.ArraySize = 1;
  shape_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  shape_desc.SampleDesc.Count = 1;
  shape_desc.SampleDesc.Quality = 0;
  shape_desc.Usage = D3D11_USAGE_STAGING;
  shape_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  shape_desc.BindFlags = 0;
  shape_desc.MiscFlags = 0;

  hr = d3d11_device_->CreateTexture2D(
      &shape_desc, &shape_init_data, shape_texture.GetAddressOf());
  if (FAILED(hr)) {
    LOG(WARNING) << "Failed to create shape ID3D11Texture: " << NumToHexStr(hr);
    goto end;
  }

  box.left = 0;
  box.top = 0;
  box.right = shape_width;
  box.bottom = shape_height;
  d3d11_device_context_->CopySubresourceRegion(
      shared_image_.Get(), 0, shape_x, shape_y, 0, shape_texture.Get(), 0, &box);

end:
  if (init_buffer) {
    delete[] init_buffer;
    init_buffer = nullptr;
  }
  return;
}

bool PictureCapturerDXGI::ProcessMonoMask(bool is_mono,
                                          int* shape_width,
                                          int* shape_height,
                                          int* shape_x,
                                          int* shape_y,
                                          uint8_t** init_buffer,
                                          D3D11_BOX* box) {
  DCHECK(shared_image_.Get());

  D3D11_TEXTURE2D_DESC full_desc;
  shared_image_->GetDesc(&full_desc);

  const int desktop_width = full_desc.Width;
  const int desktop_height = full_desc.Height;
  const int given_x = pointer_info_.position.x;
  const int given_y = pointer_info_.position.y;

  if (given_x < 0) {
    *shape_width = given_x + static_cast<int>(pointer_info_.shape_info.Width);
  } else if ((given_x + static_cast<int>(pointer_info_.shape_info.Width)) > desktop_width) {
    *shape_width = desktop_width - given_x;
  } else {
    *shape_width = static_cast<int>(pointer_info_.shape_info.Width);
  }

  if (is_mono) {
    pointer_info_.shape_info.Height /= 2;
  }

  if (given_y < 0) {
    *shape_height = given_y + static_cast<int>(pointer_info_.shape_info.Height);
  } else if ((given_y + static_cast<int>(pointer_info_.shape_info.Height)) > desktop_height) {
    *shape_height = desktop_height - given_y;
  } else {
    *shape_height = static_cast<int>(pointer_info_.shape_info.Height);
  }

  if (is_mono) {
    pointer_info_.shape_info.Height *= 2;
  }

  *shape_x = given_x < 0 ? 0 : given_x;
  *shape_y = given_y < 0 ? 0 : given_y;

  // Staging buffer/texture
  D3D11_TEXTURE2D_DESC copy_texture_desc;
  copy_texture_desc.Width = *shape_width;
  copy_texture_desc.Height = *shape_height;
  copy_texture_desc.MipLevels = 1;
  copy_texture_desc.ArraySize = 1;
  copy_texture_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  copy_texture_desc.SampleDesc.Count = 1;
  copy_texture_desc.SampleDesc.Quality = 0;
  copy_texture_desc.Usage = D3D11_USAGE_STAGING;
  copy_texture_desc.BindFlags = 0;
  copy_texture_desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  copy_texture_desc.MiscFlags = 0;

  ComPtr<ID3D11Texture2D> copy_texture;
  HRESULT hr = d3d11_device_->CreateTexture2D(&copy_texture_desc, nullptr,
                                              copy_texture.GetAddressOf());
  if (FAILED(hr)) {
    LOG(WARNING) << "Failed creating staging texture for pointer: " << NumToHexStr(hr);
    return false;
  }

  box->left = *shape_x;
  box->top = *shape_y;
  box->right = *shape_x + *shape_width;
  box->bottom = *shape_y + *shape_height;
  d3d11_device_context_->CopySubresourceRegion(
      copy_texture.Get(), 0, 0, 0, 0, shared_image_.Get(), 0, box);

  ComPtr<IDXGISurface> copy_surface;
  hr = copy_texture->QueryInterface(copy_surface.GetAddressOf());
  if (FAILED(hr)) {
    LOG(WARNING) << "Failed to QI staging texture into IDXGISurface for pointer: " << NumToHexStr(hr);
    return false;
  }

  DXGI_MAPPED_RECT mapped_surface;
  hr = copy_surface->Map(&mapped_surface, DXGI_MAP_READ);
  if (FAILED(hr)) {
    LOG(WARNING) << "Failed to map surface for pointer: " << NumToHexStr(hr);
    return false;
  }

  *init_buffer = new (std::nothrow) uint8_t[*shape_width * *shape_height * 4];
  if (!(*init_buffer)) {
    LOG(WARNING) << "Failed to allocate memory for new mouse shape buffer.";
    copy_surface->Unmap();
    return false;
  }

  uint32_t* init_buffer32 = reinterpret_cast<uint32_t*>(*init_buffer);
  uint32_t* desktop32 = reinterpret_cast<uint32_t*>(mapped_surface.pBits);
  uint32_t desktop_pitch_in_pixel = mapped_surface.Pitch / sizeof(uint32_t);

  uint32_t skip_x = (given_x < 0) ? (-1 * given_x) : 0;
  uint32_t skip_y = (given_y < 0) ? (-1 * given_y) : 0;

  if (is_mono) {
    for (int row = 0; row < *shape_height; ++row) {
      uint8_t mask = 0x80;
      mask = mask >> (skip_x % 8);
      for (int col = 0; col < *shape_width; ++col) {
        const int dst_idx = row * *shape_width + col;
        const int src_idx = row * desktop_pitch_in_pixel + col;
        const int and_idx = ((col + skip_x) / 8)
                          + ((row + skip_y) * (pointer_info_.shape_info.Pitch));
        const int xor_idx = ((col + skip_x) / 8)
                          + ((row + skip_y + (pointer_info_.shape_info.Height / 2)) * (pointer_info_.shape_info.Pitch));

        uint8_t and_mask = pointer_info_.shape_buffer[and_idx] & mask;
        uint8_t xor_mask = pointer_info_.shape_buffer[xor_idx] & mask;
        uint32_t and_mask32 = (and_mask) ? 0xFFFFFFFF : 0xFF000000;
        uint32_t xor_mask32 = (xor_mask) ? 0X00FFFFFF : 0x00000000;

        init_buffer32[dst_idx] = (desktop32[src_idx] & and_mask32) ^ xor_mask32;

        if (mask == 0x01) {
          mask = 0x80;
        } else {
          mask = mask >> 1;
        }
      }
    }
  } else {
    uint32_t* buffer32 = reinterpret_cast<uint32_t*>(pointer_info_.shape_buffer);

    for (int row = 0; row < *shape_height; ++row) {
      for (int col = 0; col < *shape_width; ++col) {
        const int dst_idx = row * *shape_width + col;

        uint32_t mask_val = 0xFF000000 & buffer32[(col + skip_x) + ((row + skip_y) * (pointer_info_.shape_info.Pitch / sizeof(uint32_t)))];
        if (mask_val) {
          init_buffer32[dst_idx] = (desktop32[row * desktop_pitch_in_pixel + col] ^ buffer32[(col + skip_x) + ((row + skip_y) * (pointer_info_.shape_info.Pitch / sizeof(uint32_t)))]) | 0xFF000000;
        } else {
          init_buffer32[dst_idx] = buffer32[(col + skip_x) + ((row + skip_y) * (pointer_info_.shape_info.Pitch / sizeof(uint32_t)))] | 0xFF000000;
        }
      }
    }
  }

  hr = copy_surface->Unmap();
  if (FAILED(hr)) {
    LOG(WARNING) << "Failed to unmap surface for pointer: " << NumToHexStr(hr);
    delete[](*init_buffer);
    *init_buffer = nullptr;
    return false;
  }

  return true;
}
