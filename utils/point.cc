#include "utils/point.h"

namespace utils {

Point::Point(DWORD point) {
  POINTS points = MAKEPOINTS(point);
  x_ = points.x;
  y_ = points.y;
}

Point::Point(const POINT& point) : x_(point.x), y_(point.y) {
}

Point& Point::operator=(const POINT& point) {
  x_ = point.x;
  y_ = point.y;
  return *this;
}

POINT Point::ToPOINT() const {
  POINT p;
  p.x = x();
  p.y = y();
  return p;
}

std::string Point::ToString() const {
  std::string str;
  str.append(std::to_string(x()))
     .append(",")
     .append(std::to_string(y()));
  return str;
}

}  // namespace utils
