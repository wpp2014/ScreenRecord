#ifndef PICTURE_CAPTURER_D3D9_H_
#define PICTURE_CAPTURER_D3D9_H_

#include <d3d9.h>
#include <wrl/client.h>

#include "picture_capturer.h"

class PictureCapturerD3D9 : public PictureCapturer {
 public:
  // 0: rgb, 1: argb
  explicit PictureCapturerD3D9(int type);
  ~PictureCapturerD3D9() override;

  Picture* Capture() override;

 private:
  bool InitD3D9();

  bool d3d9_initialized_;

  int type_;

  int width_;
  int height_;

  Microsoft::WRL::ComPtr<IDirect3D9> d3d9_;
  Microsoft::WRL::ComPtr<IDirect3DDevice9> d3d9_device_;
  Microsoft::WRL::ComPtr<IDirect3DSurface9> render_target_;
  Microsoft::WRL::ComPtr<IDirect3DSurface9> dest_target_;

  PictureCapturerD3D9(const PictureCapturerD3D9&) = delete;
  PictureCapturerD3D9& operator=(const PictureCapturerD3D9&) = delete;
};

#endif  // PICTURE_CAPTURER_D3D9_H_
