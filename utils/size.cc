#include "utils/size.h"

namespace utils {

void Size::operator+=(const Size& size) {
  SetSize(width_ + size.width(), height_ + size.height());
}

void Size::operator-=(const Size& size) {
  SetSize(width_ - size.width(), height_ - size.height());
}

SIZE Size::ToSIZE() const {
  SIZE s;
  s.cx = width();
  s.cy = height();
  return s;
}

}  // namespace utils;
