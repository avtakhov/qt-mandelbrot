#include "core.h"

#include <cmath>

#include "colors.h"

namespace mandelbrot::core {

namespace {

bool at(qreal x, qreal y) {
  qreal p = std::hypot(x - 0.25, y);
  qreal q = std::atan2(y, x - 0.25);
  return p <= (1 - std::cos(q)) / 2;
}

int fromReal(qreal from, double real, double scale) {
  return (real - from) * scale;
}

qreal toReal(qreal from, int value, double scale) {
  return from + value / scale;
}

const std::uint32_t kMaxIterations = 5 * 100;

std::uint32_t iterations(qreal x0, qreal y0) {
  if (at(x0, y0)) return kMaxIterations;
  qreal x = 0;
  qreal y = 0;
  std::uint32_t iter = 0;
  for (; x * x + y * y <= 2 * 2 && iter < kMaxIterations; ++iter) {
    auto xtemp = x * x - y * y + x0;
    y = 2 * x * y + y0;
    x = xtemp;
  }
  return iter;
}

}  // namespace

QImage calculate(QPointF from, QSize size, double scale,
                 std::atomic<bool> const& cancel_flag) {
  QImage result = QImage(size, QImage::Format::Format_RGB32);
  for (auto i = 0; i < size.width(); ++i) {
    for (auto j = 0; j < size.height(); ++j) {
      if (cancel_flag.load()) {
        return result;
      }
      auto const realX = toReal(from.x(), i, scale);
      auto const realY = toReal(from.y(), j, scale);
      auto const iterations_number = iterations(realX, realY);
      result.setPixelColor(i, j, GetColor(iterations_number));
    }
  }
  return result;
}

}  // namespace mandelbrot::core
