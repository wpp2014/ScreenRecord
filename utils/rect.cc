#include "utils/rect.h"

namespace utils {

Rect::Rect(const RECT& r)
    : origin_(r.left, r.top), size_(r.right - r.left, r.bottom - r.top) {}

RECT Rect::ToRECT() const {
  RECT r;
  r.left = x();
  r.right = right();
  r.top = y();
  r.bottom = bottom();
  return r;
}

}  // namespace utils
