#include "processing_area.h"

#include <QtConcurrent/QtConcurrent>
#include <random>

#include "colors.h"
#include "core.h"

namespace mandelbrot {

namespace {

struct CalculateFunction : QObject {
  std::shared_ptr<ProcessingArea> area;

  void operator()(QPointF pt, double scale,
                  std::atomic<bool> const& cancel_flag) {
    auto result =
        core::calculate(pt, area->position().size(), scale, cancel_flag);
    area->position().size();
  }
};

}  // namespace

ProcessingArea::ProcessingArea(QPointF base_p0, QRect area, double scale,
                               QThreadPool* threads, QObject* parent)
    : QObject(parent),
      current_position_(area.topLeft()),
      area_size_(area.size()),
      from_point_(base_p0 + QPointF(area.topLeft()) / scale),
      scale_(scale),
      threads_(threads),
      cancel_flag_(false) {}

void ProcessingArea::draw(QPainter& painter, QImage const& blur) const {
  painter.drawImage(current_position_, processed_area_.isFinished()
                                           ? processed_area_.result()
                                           : blur);
}

void ProcessingArea::translate(QPoint offset) { current_position_ += offset; }

QRect ProcessingArea::position() const {
  return QRect(current_position_, area_size_);
}

void ProcessingArea::cancel() { cancel_flag_.store(true); }

void ProcessingArea::start() {
  processed_area_ =
      QtConcurrent::run(threads_, [area = shared_from_this()]() -> QImage {
        auto result =
            core::calculate(area->from_point_, area->position().size(),
                            area->scale_, area->cancel_flag_);
        emit area->processFinished();
        return result;
      });
}

}  // namespace mandelbrot
