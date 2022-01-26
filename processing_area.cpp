#include "processing_area.h"

#include <QtConcurrent/QtConcurrent>
#include <random>

#include "colors.h"
#include "core.h"

namespace mandelbrot {

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
  painter.drawImage(current_position_,
                    isFinished() ? processed_area_.result() : blur);
}

void ProcessingArea::translate(QPoint offset) { current_position_ += offset; }

QRect ProcessingArea::position() const {
  return QRect(current_position_, area_size_);
}

void ProcessingArea::cancel() {
  cancel_flag_.store(true);
  processed_area_.waitForFinished();
}

void ProcessingArea::start() {
  processed_area_ = QtConcurrent::run(threads_, [this]() {
    auto result =
        core::calculate(from_point_, position().size(), scale_, cancel_flag_);
    emit processFinished();
    return result;
  });
}

bool ProcessingArea::isFinished() const { return processed_area_.isFinished(); }

ProcessingArea::~ProcessingArea() {
  cancel();
  processed_area_.waitForFinished();
}

}  // namespace mandelbrot
