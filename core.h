#pragma once

#include <QImage>

namespace mandelbrot::core {

QImage calculate(QPointF from, QSize size, double scale,
                 std::atomic<bool> const& cancel_flag);

}  // namespace mandelbrot::core
