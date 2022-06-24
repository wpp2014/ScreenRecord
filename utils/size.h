#ifndef UTILS_SIZE_H_
#define UTILS_SIZE_H_

#include <windows.h>

#include <algorithm>

typedef struct tagSIZE SIZE;

namespace utils {

class Size {
 public:
  constexpr Size() : width_(0), height_(0) {}
  constexpr Size(int width, int height)
      : width_(std::max(0, width)), height_(std::max(0, height)) {}

  void operator+=(const Size& size);
  void operator-=(const Size& size);

  SIZE ToSIZE() const;

  constexpr int width() const { return width_; }
  constexpr int height() const { return height_; }

  void set_width(int width) { width_ = std::max(0, width); }
  void set_height(int height) { height_ = std::max(0, height); }

  void SetSize(int width, int height) {
    set_width(width);
    set_height(height);
  }

  bool IsEmpty() const { return !width() || !height(); }
  bool IsZero() const { return !width() && !height(); }

 private:
  int width_;
  int height_;
};

inline bool operator==(const Size& lhs, const Size& rhs) {
  return lhs.width() == rhs.width() && lhs.height() == rhs.height();
}

inline bool operator!=(const Size& lhs, const Size& rhs) {
  return !(lhs == rhs);
}

inline Size operator+(Size lhs, const Size& rhs) {
  lhs += rhs;
  return lhs;
}

inline Size operator-(Size lhs, const Size& rhs) {
  lhs -= rhs;
  return lhs;
}

}  // namespace utils

#endif  // UTILS_SIZE_H_
