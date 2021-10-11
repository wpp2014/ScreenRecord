#ifndef PICTURE_CAPTURER_DXGI_H_
#define PICTURE_CAPTURER_DXGI_H_

#include <DirectXMath.h>
#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

#include "picture_capturer.h"

class PictureCapturerDXGI : public PictureCapturer {
 public:
  // 0: rgb, 1: argb
  explicit PictureCapturerDXGI(int type);
  ~PictureCapturerDXGI() override;

  Picture* Capture() override;

 private:
  struct MouseInfo {
    uint8_t* shape_buffer;
    DXGI_OUTDUPL_POINTER_SHAPE_INFO shape_info;
    POINT position;
    bool visible;
    uint32_t buffer_size;
    LARGE_INTEGER last_time_stamp;
  };

  bool InitDXGI();

  bool GetMouse(DXGI_OUTDUPL_FRAME_INFO* frame_info);
  void DrawMouse();

  bool ProcessMonoMask(bool is_mono,
                       _Out_ int* shape_width,
                       _Out_ int* shape_height,
                       _Out_ int* shape_x,
                       _Out_ int* shape_y,
                       _Outptr_result_bytebuffer_(*shape_width* * shape_height * 4) uint8_t** init_buffer,
                       _Out_ D3D11_BOX* box);

  bool initialized_;

  int type_;

  int width_;
  int height_;

  MouseInfo mouse_info_;

  RECT desktop_bound_;

  Microsoft::WRL::ComPtr<ID3D11Device> d3d11_device_;
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3d11_device_context_;
  Microsoft::WRL::ComPtr<IDXGIOutputDuplication> desk_dupl_;
  Microsoft::WRL::ComPtr<ID3D11Texture2D> acquired_desktop_image_;
  Microsoft::WRL::ComPtr<ID3D11Texture2D> shared_image_;
  DXGI_OUTPUT_DESC output_desc_;

  PictureCapturerDXGI(const PictureCapturerDXGI&) = delete;
  PictureCapturerDXGI& operator=(const PictureCapturerDXGI&) = delete;
};  // class PictureCapturerDXGI

#endif  // PICTURE_CAPTURER_DXGI_H_
