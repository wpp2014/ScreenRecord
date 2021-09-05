#ifndef SCREEN_RECORD_SRC_CAPTURER_PICTURE_CAPTURER_H_
#define SCREEN_RECORD_SRC_CAPTURER_PICTURE_CAPTURER_H_

#include <stdint.h>

#include <memory>

#include <d3d9.h>
#include <wrl/client.h>

class PictureCapturer {
 public:
  // argb
  class Picture {
   public:
    Picture();
    Picture(const Picture& other);
    Picture(Picture&& other);
    ~Picture();

    Picture& operator=(const Picture& other);
    Picture& operator=(Picture&&);

    int width() const { return width_; }
    int height() const { return height_; }
    int stride() const { return stride_; }
    uint8_t* data() const { return data_; }

    void Reset();
    void Reset(const uint8_t* data, int width, int height, int stride);

   private:
    int width_;
    int height_;
    int stride_;
    uint8_t* data_;
  };  // class Picture

  PictureCapturer();
  ~PictureCapturer();

  std::unique_ptr<Picture> CaptureScreen();

 private:
  bool InitD3D9();

  bool d3d9_initialized_;

  int width_;
  int height_;

  Microsoft::WRL::ComPtr<IDirect3D9> d3d9_;
  Microsoft::WRL::ComPtr<IDirect3DDevice9> d3d9_device_;
  Microsoft::WRL::ComPtr<IDirect3DSurface9> render_target_;
  Microsoft::WRL::ComPtr<IDirect3DSurface9> dest_target_;

  PictureCapturer(const PictureCapturer&) = delete;
  PictureCapturer& operator=(const PictureCapturer&) = delete;
};  // class PictureCapturer

#endif  // SCREEN_RECORD_SRC_CAPTURER_PICTURE_CAPTURER_H_
