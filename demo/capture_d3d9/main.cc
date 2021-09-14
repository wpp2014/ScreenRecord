#include <assert.h>

#include <chrono>
#include <iostream>

#include <d3d9.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

ComPtr<IDirect3D9> g_d3d9;
ComPtr<IDirect3DDevice9> g_d3d9_device;
ComPtr<IDirect3DSurface9> g_render_target;
ComPtr<IDirect3DSurface9> g_dest_target;

int main() {
  g_d3d9 = Direct3DCreate9(D3D_SDK_VERSION);
  assert(g_d3d9.Get());

  D3DDISPLAYMODE d3d_display_mode;
  HRESULT hr =
      g_d3d9->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3d_display_mode);
  assert(SUCCEEDED(hr));

  const int width = d3d_display_mode.Width;
  const int height = d3d_display_mode.Height;

  D3DPRESENT_PARAMETERS d3d9_param;
  ZeroMemory(&d3d9_param, sizeof(d3d9_param));
  d3d9_param.Windowed = TRUE;
  d3d9_param.SwapEffect = D3DSWAPEFFECT_COPY;

  hr = g_d3d9->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                            GetDesktopWindow(),
                            D3DCREATE_HARDWARE_VERTEXPROCESSING, &d3d9_param,
                            g_d3d9_device.GetAddressOf());
  assert(SUCCEEDED(hr));

  hr = g_d3d9_device->GetRenderTarget(0, g_render_target.GetAddressOf());
  assert(SUCCEEDED(hr));

  hr = g_d3d9_device->CreateOffscreenPlainSurface(
      width, height, D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM,
      g_dest_target.GetAddressOf(), NULL);
  assert(SUCCEEDED(hr));

  int count = 0;
  auto start = std::chrono::high_resolution_clock::now();

  while (true) {
    D3DLOCKED_RECT lr;
    ZeroMemory(&lr, sizeof(lr));
    hr = g_d3d9_device->GetFrontBufferData(0, g_dest_target.Get());
    assert(SUCCEEDED(hr));

    hr = g_dest_target->LockRect(&lr, NULL, D3DLOCK_READONLY);
    assert(SUCCEEDED(hr));

    g_dest_target->UnlockRect();

    ++count;

    auto end = std::chrono::high_resolution_clock::now();
    auto delta = std::chrono::duration<double>(end - start).count();
    if (delta > 1) {
      std::cout << "FPS: " << count / delta << std::endl;
      count = 0;
      start = end;
    }
  }

  return 0;
}
