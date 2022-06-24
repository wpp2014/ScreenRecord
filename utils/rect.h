#ifndef UTILS_RECT_H_
#define UTILS_RECT_H_

#include "utils/point.h"
#include "utils/size.h"

typedef struct tagRECT RECT;

namespace utils {

class Rect {
 public:
  constexpr Rect() = default;
  constexpr Rect(int width, int height) : size_(width, height) {}
  constexpr Rect(int x, int y, int width, int height)
      : origin_(x, y), size_(width, height) {}
  constexpr explicit Rect(const Size& size) : size_(size) {}
  constexpr Rect(const Point& origin, const Size& size)
      : origin_(origin), size_(size) {}
  explicit Rect(const RECT& r);

  RECT ToRECT() const;

  constexpr const Point& origin() const { return origin_; }
  constexpr const Size& size() const { return size_; }

  constexpr int x() const { return origin_.x(); }
  constexpr int y() const { return origin_.y(); }
  constexpr int width() const { return size_.width(); }
  constexpr int height() const { return size_.height(); }

  constexpr int right() const { return x() + width(); }
  constexpr int bottom() const { return y() + height(); }

  constexpr Point top_right() const { return Point(right(), y()); }
  constexpr Point bottom_left() const { return Point(x(), bottom()); }
  constexpr Point bottom_right() const { return Point(right(), bottom()); }

  // Returns true if the area of the rectangle is zero.
  bool IsEmpty() const { return size_.IsEmpty(); }

 private:
  Point origin_;
  Size size_;
};

inline bool operator==(const Rect& lhs, const Rect& rhs) {
  return lhs.origin() == rhs.origin() && lhs.size() == rhs.size();
}

inline bool operator!=(const Rect& lhs, const Rect& rhs) {
  return !(lhs == rhs);
}

}  // namespace utils

#endif  // UTILS_RECT_H_
