#include "colors.h"

#include <random>

namespace mandelbrot {

thread_local std::mt19937 random_gen{std::random_device{}()};

namespace {

struct GreenNeon {
  static std::uint8_t Red(std::uint32_t iterations) { return iterations; }

  static std::uint8_t Green(std::uint32_t iterations) {
    return static_cast<std::uint8_t>(iterations) * iterations;
  }

  static std::uint8_t Blue(std::uint32_t iterations) { return iterations; }

  static std::string description() {
    return "Зеленая лампочка освещает бесконечную темноту";
  }
};

struct NoGreen {
  static std::uint8_t Red(std::uint32_t iterations) { return iterations; }

  static std::uint8_t Green(std::uint32_t iterations) { return 0; }

  static std::uint8_t Blue(std::uint32_t iterations) { return iterations; }

  static std::string description() { return "Не люблю зеленый"; }
};

struct Dawn {
  static std::uint8_t Red(std::uint32_t iterations) {
    return iterations ^ 0x182038123;
  }

  static std::uint8_t Green(std::uint32_t iterations) { return iterations; }

  static std::uint8_t Blue(std::uint32_t iterations) { return iterations; }

  static std::string description() { return "Тоже норм"; }
};

template <typename ColorGenerator>
QColor GetColorImpl(std::uint32_t iterations) {
  return QColor(ColorGenerator::Red(iterations),
                ColorGenerator::Green(iterations),
                ColorGenerator::Blue(iterations));
}

}  // namespace

QColor GetColor(std::uint32_t iterations) {
  return GetColorImpl<Dawn>(iterations);
}

}  // namespace mandelbrot
