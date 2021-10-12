#ifndef CAPTURER_PICTURE_CAPTURER_DXGI_H_
#define CAPTURER_PICTURE_CAPTURER_DXGI_H_

#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

#include "capturer/picture_capturer.h"

class PictureCapturerDXGI : public PictureCapturer {
 public:
  PictureCapturerDXGI();
  ~PictureCapturerDXGI();

  AVData* CaptureScreen() override;

 private:
  // 鼠标指针信息
  struct PointerInfo {
    // 光标形状数据
    uint8_t* shape_buffer;
    // 光标形状的信息
    DXGI_OUTDUPL_POINTER_SHAPE_INFO shape_info;
    // 坐标
    POINT position;
    // 是否展示
    bool visible;
    // 光标形状数据大小
    uint32_t buffer_size;
    // 光标上次更新的时间戳
    LARGE_INTEGER last_time_stamp;
  };

  bool InitDXGI();

  bool GetMouse(DXGI_OUTDUPL_FRAME_INFO* frame_info);
  void DrawMouse();

  // 获取鼠标图像数据
  bool ProcessMonoMask(bool is_mono,
                       int* shape_width,
                       int* shape_height,
                       int* shape_x,
                       int* shape_y,
                       uint8_t** init_buffer,
                       D3D11_BOX* box);

  bool dxgi_initialized_;

  Microsoft::WRL::ComPtr<ID3D11Device> d3d11_device_;
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3d11_device_context_;
  Microsoft::WRL::ComPtr<IDXGIOutputDuplication> desk_dupl_;
  Microsoft::WRL::ComPtr<ID3D11Texture2D> acquired_desktop_image_;
  Microsoft::WRL::ComPtr<ID3D11Texture2D> shared_image_;
  DXGI_OUTPUT_DESC output_desc_;

  RECT desktop_bound_;

  PointerInfo pointer_info_;

  PictureCapturerDXGI(const PictureCapturerDXGI&) = delete;
  PictureCapturerDXGI& operator=(const PictureCapturerDXGI&) = delete;
};  // class PictureCapturerDXGI

#endif  // CAPTURER_PICTURE_CAPTURER_DXGI_H_
