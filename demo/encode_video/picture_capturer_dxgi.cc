#include "picture_capturer_dxgi.h"

#include <algorithm>

#include "log.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

const int kNumVertices = 6;
const int kBPP = 4;

PictureCapturerDXGI::PictureCapturerDXGI(int type)
    : initialized_(false),
      type_(type),
      width_(0),
      height_(0),
      output_desc_({}) {
  assert(type_ == 0 || type_ == 1);

  RtlZeroMemory(&desktop_bound_, sizeof(RECT));
  RtlZeroMemory(&mouse_info_, sizeof(MouseInfo));

  initialized_ = InitDXGI();
  assert(initialized_);
}

PictureCapturerDXGI::~PictureCapturerDXGI() {
  if (mouse_info_.shape_buffer) {
    delete[] mouse_info_.shape_buffer;
  }
  RtlZeroMemory(&mouse_info_, sizeof(MouseInfo));
}

PictureCapturer::Picture* PictureCapturerDXGI::Capture() {
  ComPtr<IDXGIResource> dxgi_resource;
  DXGI_OUTDUPL_FRAME_INFO frame_info;
  HRESULT hr = desk_dupl_->AcquireNextFrame(
      500, &frame_info, dxgi_resource.GetAddressOf());
  if (FAILED(hr)) {
    if (hr == DXGI_ERROR_WAIT_TIMEOUT) {
      // 屏幕没有变化的情况下会发生超时
      LOG("屏幕没有变化");
    } else {
      LOG("DXGI截屏失败: %lu", hr);
    }
    return nullptr;
  }

  acquired_desktop_image_.Reset();
  hr = dxgi_resource->QueryInterface(acquired_desktop_image_.GetAddressOf());
  if (FAILED(hr)) {
    LOG("Failed to desktop image");
    desk_dupl_->ReleaseFrame();
    return nullptr;
  }

#if 0
  bool should_draw_mouse = false;
  if (GetMouse(&frame_info) && mouse_info_.visible) {
    LOG("PointerPosition Visible=%d x=%d y=%d w=%d h=%d type=%d",
        mouse_info_.visible,
        mouse_info_.position.x,
        mouse_info_.position.y,
        mouse_info_.shape_info.Width,
        mouse_info_.shape_info.Height,
        mouse_info_.shape_info.Type);
    should_draw_mouse = true;
  }
#endif

  d3d11_device_context_->CopyResource(shared_image_.Get(),
                                      acquired_desktop_image_.Get());

  ComPtr<IDXGISurface> dxgi_surface;
  hr = shared_image_->QueryInterface(dxgi_surface.GetAddressOf());
  if (FAILED(hr)) {
    LOG("Failed to get DXGISurface");
    desk_dupl_->ReleaseFrame();
    return nullptr;
  }

  if (GetMouse(&frame_info) && mouse_info_.visible) {
    DrawMouse();
  }

  DXGI_MAPPED_RECT dxgi_mapped_rect;
  hr = dxgi_surface->Map(&dxgi_mapped_rect, DXGI_MAP_READ);
  if (FAILED(hr)) {
    LOG("Failed to map DXGISurface");
    desk_dupl_->ReleaseFrame();
    return nullptr;
  }

  D3D11_TEXTURE2D_DESC full_desc;
  shared_image_->GetDesc(&full_desc);

  Picture* picture = new Picture();
  picture->width = full_desc.Width;
  picture->height = full_desc.Height;
  if (type_ == 0) {
    picture->len = picture->width * picture->height * 3;
    picture->rgb = ARGBToRGB((const uint8_t*)dxgi_mapped_rect.pBits,
                             full_desc.Width,
                             full_desc.Height);
  } else {
    picture->len = full_desc.Height * dxgi_mapped_rect.Pitch;;
    picture->argb = new uint8_t[picture->len];
    memcpy(picture->argb, dxgi_mapped_rect.pBits, picture->len);
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

  desktop_bound_.left = min(output_desc_.DesktopCoordinates.left, INT_MAX);
  desktop_bound_.top = min(output_desc_.DesktopCoordinates.top, INT_MAX);
  desktop_bound_.right = max(output_desc_.DesktopCoordinates.right, INT_MIN);
  desktop_bound_.bottom = max(output_desc_.DesktopCoordinates.bottom, INT_MIN);

  ComPtr<IDXGIOutput1> dxgi_output1;
  hr = dxgi_output->QueryInterface(
      __uuidof(IDXGIOutput1),
      reinterpret_cast<void**>(dxgi_output1.GetAddressOf()));
  if (FAILED(hr)) {
    LOG("Failed to get DXGIOutput1");
    return false;
  }

  hr = dxgi_output1->DuplicateOutput(d3d11_device_.Get(),
                                     desk_dupl_.GetAddressOf());
  if (FAILED(hr)) {
    LOG("Failed to call DuplicateOutput");
    return false;
  }

  D3D11_TEXTURE2D_DESC desc;
  desc.Width = desktop_bound_.right - desktop_bound_.left;
  desc.Height = desktop_bound_.bottom - desktop_bound_.top;
  desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  desc.Usage = D3D11_USAGE_STAGING;
  desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
  desc.BindFlags = 0;;
  desc.MiscFlags = 0;
  desc.MipLevels = 1;
  desc.ArraySize = 1;
  desc.SampleDesc.Count = 1;
  desc.SampleDesc.Quality = 0;

  hr = d3d11_device_->CreateTexture2D(&desc, NULL, shared_image_.GetAddressOf());
  if (FAILED(hr)) {
    LOG("Failed to create shared surface");
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
  if (frame_info->PointerPosition.Visible &&
      mouse_info_.visible &&
      mouse_info_.last_time_stamp.QuadPart > frame_info->LastMouseUpdateTime.QuadPart) {
    update_position = false;
  }

  if (update_position) {
    mouse_info_.position.x = frame_info->PointerPosition.Position.x;
    mouse_info_.position.y = frame_info->PointerPosition.Position.y;
    mouse_info_.last_time_stamp = frame_info->LastMouseUpdateTime;
    mouse_info_.visible = (frame_info->PointerPosition.Visible != FALSE);
  }

  if (frame_info->PointerShapeBufferSize == 0) {
    return true;
  }

  // 重新分配内存
  if (frame_info->PointerShapeBufferSize > mouse_info_.buffer_size) {
    if (mouse_info_.shape_buffer) {
      delete[] mouse_info_.shape_buffer;
      mouse_info_.shape_buffer = nullptr;
    }

    mouse_info_.shape_buffer =
        new (std::nothrow) uint8_t[frame_info->PointerShapeBufferSize];
    if (mouse_info_.shape_buffer == nullptr) {
      LOG("分配内存失败");
      return false;
    }
    mouse_info_.buffer_size = frame_info->PointerShapeBufferSize;
  }

  // 获取鼠标
  uint32_t buffer_required_size = 0;
  HRESULT hr = desk_dupl_->GetFramePointerShape(
      frame_info->PointerShapeBufferSize,
      reinterpret_cast<void*>(mouse_info_.shape_buffer),
      &buffer_required_size,
      &mouse_info_.shape_info);
  if (FAILED(hr)) {
    LOG("获取鼠标失败: 0x%x", hr);

    delete mouse_info_.shape_buffer;
    mouse_info_.shape_buffer = nullptr;
    mouse_info_.buffer_size = 0;

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
  switch (mouse_info_.shape_info.Type) {
    case DXGI_OUTDUPL_POINTER_SHAPE_TYPE_COLOR:
      assert(mouse_info_.shape_buffer);

      shape_x = mouse_info_.position.x;
      shape_y = mouse_info_.position.y;
      shape_width = mouse_info_.shape_info.Width;
      shape_height = mouse_info_.shape_info.Height;

      shape_init_data.pSysMem = mouse_info_.shape_buffer;
      shape_init_data.SysMemPitch = mouse_info_.shape_info.Pitch;
      break;

    case DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MONOCHROME:
      res = ProcessMonoMask(true, &shape_width, &shape_height, &shape_x, &shape_y, &init_buffer, &box);
      shape_init_data.pSysMem = init_buffer;
      shape_init_data.SysMemPitch = shape_width * 4;
      break;

    case DXGI_OUTDUPL_POINTER_SHAPE_TYPE_MASKED_COLOR:
      res = ProcessMonoMask(false, &shape_width, &shape_height, &shape_x, &shape_y, &init_buffer, &box);
      shape_init_data.pSysMem = init_buffer;
      shape_init_data.SysMemPitch = shape_width * 4;
      break;

    default:
      assert(false);
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
    LOG("Failed to create shape ID3D11Texture: 0x%x", hr);
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
                                          _Out_ int* shape_width,
                                          _Out_ int* shape_height,
                                          _Out_ int* shape_x,
                                          _Out_ int* shape_y,
                                          _Outptr_result_bytebuffer_(*shape_width* * shape_height * 4) uint8_t** init_buffer,
                                          _Out_ D3D11_BOX* box) {
  D3D11_TEXTURE2D_DESC full_desc;
  shared_image_->GetDesc(&full_desc);

  const int desktop_width = full_desc.Width;
  const int desktop_height = full_desc.Height;
  const int given_x = mouse_info_.position.x;
  const int given_y = mouse_info_.position.y;

  if (given_x < 0) {
    *shape_width = given_x + static_cast<int>(mouse_info_.shape_info.Width);
  } else if ((given_x + static_cast<int>(mouse_info_.shape_info.Width)) > desktop_width) {
    *shape_width = desktop_width - given_x;
  } else {
    *shape_width = static_cast<int>(mouse_info_.shape_info.Width);
  }

  if (is_mono) {
    mouse_info_.shape_info.Height /= 2;
  }

  if (given_y < 0) {
    *shape_height = given_y + static_cast<int>(mouse_info_.shape_info.Height);
  } else if ((given_y + static_cast<int>(mouse_info_.shape_info.Height)) > desktop_height) {
    *shape_height = desktop_height - given_y;
  } else {
    *shape_height = static_cast<int>(mouse_info_.shape_info.Height);
  }

  if (is_mono) {
    mouse_info_.shape_info.Height *= 2;
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
  HRESULT hr = d3d11_device_->CreateTexture2D(
      &copy_texture_desc, nullptr, copy_texture.GetAddressOf());
  if (FAILED(hr)) {
    LOG("Failed creating staging texture for pointer: 0x%x", hr);
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
    LOG("Failed to QI staging texture into IDXGISurface for pointer: 0x%x", hr);
    return false;
  }

  DXGI_MAPPED_RECT mapped_surface;
  hr = copy_surface->Map(&mapped_surface, DXGI_MAP_READ);
  if (FAILED(hr)) {
    LOG("Failed to map surface for pointer: 0x%x", hr);
    return false;
  }

  *init_buffer = new (std::nothrow) uint8_t[*shape_width * *shape_height * 4];
  if (!(*init_buffer)) {
    LOG("Failed to allocate memory for new mouse shape buffer.");
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
        uint8_t and_mask =
            mouse_info_.shape_buffer[((col + skip_x) / 8) +
                                     ((row + skip_y) *
                                      (mouse_info_.shape_info.Pitch))] &
            mask;
        uint8_t xor_mask =
            mouse_info_.shape_buffer[((col + skip_x) / 8) +
                                     ((row + skip_y +
                                       (mouse_info_.shape_info.Height / 2)) *
                                      (mouse_info_.shape_info.Pitch))] &
            mask;
        uint32_t and_mask32 = (and_mask) ? 0xFFFFFFFF : 0xFF000000;
        uint32_t xor_mask32 = (xor_mask) ? 0X00FFFFFF : 0x00000000;

        init_buffer32[row * *shape_width + col] =
            (desktop32[row * desktop_pitch_in_pixel + col] & and_mask32) ^
            xor_mask32;

        if (mask == 0x01) {
          mask = 0x80;
        } else {
          mask = mask >> 1;
        }
      }
    }
  } else {
    uint32_t* buffer32 = reinterpret_cast<uint32_t*>(mouse_info_.shape_buffer);

    for (int row = 0; row < *shape_height; ++row) {
      for (int col = 0; col < *shape_width; ++col) {
        uint32_t mask_val =
            0xFF000000 &
            buffer32[(col + skip_x) +
                     ((row + skip_y) *
                      (mouse_info_.shape_info.Pitch / sizeof(uint32_t)))];
        if (mask_val) {
          init_buffer32[row * *shape_width + col] =
              (desktop32[row * desktop_pitch_in_pixel + col] ^
               buffer32[(col + skip_x) +
                        ((row + skip_y) *
                         (mouse_info_.shape_info.Pitch / sizeof(uint32_t)))]) |
              0xFF000000;
        } else {
          init_buffer32[row * *shape_width + col] =
              buffer32[(col + skip_x) +
                       ((row + skip_y) *
                        (mouse_info_.shape_info.Pitch / sizeof(uint32_t)))] |
              0xFF000000;
        }
      }
    }
  }

  hr = copy_surface->Unmap();
  if (FAILED(hr)) {
    LOG("Failed to unmap surface for pointer: 0x%x", hr);
    delete[](*init_buffer);
    *init_buffer = nullptr;
    return false;
  }

  return true;
}
