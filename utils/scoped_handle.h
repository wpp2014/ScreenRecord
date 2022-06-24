#ifndef UTILS_SCOPED_HANDLE_H_
#define UTILS_SCOPED_HANDLE_H_

#include <windows.h>

#include <assert.h>

namespace utils {

class ScopedHandle {
 public:
  ScopedHandle() : handle_(NULL) {}

  explicit ScopedHandle(HANDLE handle) : handle_(NULL) {
    Set(handle);
  }

  ~ScopedHandle() {
    Close();
  }

  ScopedHandle& operator=(HANDLE handle) {
    Set(handle);
    return *this;
  }

  // Use this instead of comparing to INVALID_HANDLE_VALUE to pick up our NULL
  // usage for errors.
  bool IsValid() const {
    return handle_ != NULL;
  }

  void Set(HANDLE new_handle) {
    Close();

    if (new_handle != INVALID_HANDLE_VALUE) {
      handle_ = new_handle;
    }
  }

  HANDLE Get() const {
    return handle_;
  }

  operator HANDLE() { return handle_; }

  HANDLE Release() {
    HANDLE h = handle_;
    handle_ = NULL;
    return h;
  }

 private:
  void Close() {
    if (handle_) {
      if (!CloseHandle(handle_)) {
        assert(0);
      }
      handle_ = NULL;
    }
  }

  HANDLE handle_;

  ScopedHandle(const ScopedHandle&) = delete;
  ScopedHandle& operator=(const ScopedHandle&) = delete;
};

class ScopedHDC {
 public:
  ScopedHDC() : hdc_(NULL) {}

  explicit ScopedHDC(HDC hdc) : hdc_(hdc) {}

  ~ScopedHDC() {
    if (hdc_) {
      DeleteDC(hdc_);
      hdc_ = NULL;
    }
  }

  ScopedHDC& operator=(HDC hdc) {
    if (hdc_) {
      DeleteDC(hdc_);
    }
    hdc_ = hdc;

    return *this;
  }

  operator HDC() { return hdc_; }

  HDC Get() const {
    return hdc_;
  }

  HDC Release() {
    HDC hdc = hdc_;
    hdc_ = NULL;
    return hdc;
  }

 private:
  HDC hdc_;

  ScopedHDC(const ScopedHDC&) = delete;
  ScopedHDC& operator=(const ScopedHDC&) = delete;
};

class ScopedBitmap {
 public:
  ScopedBitmap() : hbitmap_(NULL) {}

  explicit ScopedBitmap(HBITMAP hbitmap) : hbitmap_(hbitmap) {}

  ~ScopedBitmap() {
    if (hbitmap_) {
      DeleteObject(hbitmap_);
      hbitmap_ = NULL;
    }
  }

  ScopedBitmap& operator=(HBITMAP hbitmap) {
    if (hbitmap_) {
      DeleteObject(hbitmap_);
    }
    hbitmap_ = hbitmap;

    return *this;
  }

  operator HBITMAP() { return hbitmap_; }

  HBITMAP Get() const {
    return hbitmap_;
  }

  HBITMAP Release() {
    HBITMAP h = hbitmap_;
    hbitmap_ = NULL;
    return h;
  }

 private:
  HBITMAP hbitmap_;

  ScopedBitmap(const ScopedBitmap&) = delete;
  ScopedBitmap& operator=(const ScopedBitmap&) = delete;
};

template<class T>
class ScopedHGlobal {
 public:
  explicit ScopedHGlobal(HGLOBAL glob) : glob_(glob) {
    data_ = static_cast<T*>(GlobalLock(glob_));
  }

  ~ScopedHGlobal() {
    GlobalUnlock(glob_);
  }

  T* Get() { return data_; }

  T* operator->() {
    assert(data_ != NULL);
    return data_;
  }

  size_t Size() const { return GlobalSize(glob_); }

 private:
  HGLOBAL glob_;

  T* data_;

  ScopedHGlobal(const ScopedHGlobal&) = delete;
  ScopedHGlobal& operator=(const ScopedHGlobal&) = delete;
};

}  // namespace utils

#endif  // UTILS_SCOPED_HANDLE_H_
