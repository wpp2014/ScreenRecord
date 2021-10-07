#ifndef PICTURE_CAPTURER_DXGI_H_
#define PICTURE_CAPTURER_DXGI_H_

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
  bool InitDXGI();

  bool initialized_;

  int type_;

  int width_;
  int height_;

  Microsoft::WRL::ComPtr<ID3D11Device> d3d11_device_;
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3d11_device_context_;
  Microsoft::WRL::ComPtr<IDXGIOutput1> dxgi_output1_;
  Microsoft::WRL::ComPtr<IDXGIOutputDuplication> desk_dupl_;
  DXGI_OUTPUT_DESC output_desc_;

  PictureCapturerDXGI(const PictureCapturerDXGI&) = delete;
  PictureCapturerDXGI& operator=(const PictureCapturerDXGI&) = delete;
};  // class PictureCapturerDXGI

#endif  // PICTURE_CAPTURER_DXGI_H_
