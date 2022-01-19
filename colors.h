#pragma once

#include <QColor>
#include <QSize>

namespace mandelbrot {

namespace colors {

inline QColor const kBlack(0, 0, 0);
inline QColor const kWhite(255, 255, 255);

}  // namespace colors

QColor GetColor(std::uint32_t iterations);

}  // namespace mandelbrot
