#ifndef UTILS_POINT_H_
#define UTILS_POINT_H_

#include <windows.h>

#include <string>

typedef struct tagPOINT POINT;

namespace utils {

class Point {
 public:
  constexpr Point() : x_(0), y_(0) {}
  constexpr Point(int x, int y) : x_(x), y_(y) {}

  explicit Point(DWORD point);
  explicit Point(const POINT& point);

  Point& operator=(const POINT& point);

  POINT ToPOINT() const;

  constexpr int x() const { return x_; }
  constexpr int y() const { return y_; }
  void set_x(int x) { x_ = x; }
  void set_y(int y) { y_ = y; }

  void SetPoint(int x, int y) {
    x_ = x;
    y_ = y;
  }

  bool IsOrigin() const { return x_ == 0 && y_ == 0; }

  std::string ToString() const;

 private:
  int x_;
  int y_;
};

constexpr bool operator==(const Point& lhs, const Point& rhs) {
  return lhs.x() == rhs.x() && lhs.y() == rhs.y();
}

inline bool operator!=(const Point& lhs, const Point& rhs) {
  return !(lhs == rhs);
}

}  // namespace utils

#endif  // UTILS_POINT_H_
