#ifndef SCREEN_CAPTURE_DXGI_CAPTURE_H_
#define SCREEN_CAPTURE_DXGI_CAPTURE_H_

#include <d3d11.h>
#include <dxgi.h>
#include <dxgi1_2.h>
#include <wrl/client.h>

#include "screen_capture/screen_capture.h"

class DXGICapture : public ScreenCapture {
 public:
  DXGICapture();
  ~DXGICapture() override;

  ScreenPicture* Capture() override;

 private:
  void Initialize();

  bool initialized_;

  Microsoft::WRL::ComPtr<ID3D11Device> d3d11_device_;
  Microsoft::WRL::ComPtr<ID3D11DeviceContext> d3d11_device_ctx_;

  Microsoft::WRL::ComPtr<IDXGIDevice> dxgi_device_;
  Microsoft::WRL::ComPtr<IDXGIAdapter> dxgi_adapter_;
  Microsoft::WRL::ComPtr<IDXGIOutput> dxgi_output_;
  Microsoft::WRL::ComPtr<IDXGIOutput1> dxgi_output1_;

  D3D_FEATURE_LEVEL d3d_feature_level_;
};

#endif  // SCREEN_CAPTURE_DXGI_CAPTURE_H_
