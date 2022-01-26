#include "graphic_engine.h"

#include <QPainter>
#include <QThreadPool>
#include <QWheelEvent>
#include <QWidget>
#include <algorithm>
#include <cmath>
#include <functional>
#include <iostream>
#include <random>
#include <ranges>

#include "colors.h"
#include "core.h"

namespace mandelbrot {

namespace {

void produceDrawableAreas(std::function<int(QRect const&)> getValue,
                          std::function<bool(int, int)> cmp,
                          std::list<QRect>& correctRectangles,
                          std::function<QRect(QRect)> nextRectangle,
                          QRect screen) {
  auto i = getValue(*std::min_element(correctRectangles.begin(),
                                      correctRectangles.end(),
                                      [&getValue, &cmp](QRect a, QRect b) {
                                        return cmp(getValue(a), getValue(b));
                                      }));

  std::vector<QRect> toAdd;
  for (auto rect : correctRectangles) {
    if (getValue(rect) != i) continue;
    for (auto r = nextRectangle(rect); r.intersects(screen);
         r = nextRectangle(r)) {
      toAdd.push_back(r);
    }
  }
  std::copy(toAdd.begin(), toAdd.end(), std::back_inserter(correctRectangles));
}

QImage generateBlur(QSize size, std::mt19937& gen) {
  QImage result(size, QImage::Format::Format_RGB16);
  for (int i = 0; i < size.width(); ++i) {
    for (int j = 0; j < size.height(); ++j) {
      result.setPixelColor(QPoint(i, j), (i / 5 + j / 5) % 2 == gen() % 2
                                             ? colors::kWhite
                                             : colors::kBlack);
    }
  }
  return result;
}

}  // namespace

GraphicEngine::GraphicEngine(std::size_t n_cells, QWidget* parent = nullptr)
    : QWidget(parent),
      n_cells_(n_cells),
      threads_(new QThreadPool(this)),
      out_of_screen_process_cells_(n_cells / 2) {
  threads_->setMaxThreadCount(
      std::min(size_t(8), (n_cells_ + 1) * (n_cells_ + 1)));
  resetAreas(size());
}

void GraphicEngine::paintEvent(QPaintEvent* event) {
  QPainter painter(this);
  std::uniform_int_distribution<> distrib(0, blurred_images_pool.size() - 1);
  for (auto const& rect : processed_areas_) {
    rect->draw(painter, blurred_images_pool[distrib(gen)]);
  }
}

void GraphicEngine::wheelEvent(QWheelEvent* event) {
  auto scale_change = event->angleDelta().y() * (10 + scale_) / 1000;
  auto nscale = std::max(scale_ + scale_change, 10.);
  base_ += (event->position() / scale_) - (event->position() / nscale);
  scale_ = nscale;
  resetAreas(size());
}

void GraphicEngine::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton) {
    last_drag_ = event->globalPosition();
  }
}

std::list<QRect> GraphicEngine::getAreaPositions(QRect main) {
  std::list<QRect> result;
  std::transform(processed_areas_.begin(), processed_areas_.end(),
                 std::back_inserter(result),
                 std::mem_fn(&ProcessingArea::position));
  return result;
}

QSize GraphicEngine::cellSize() const { return size() / n_cells_; }

void GraphicEngine::mouseMoveEvent(QMouseEvent* event) {
  if (event->buttons() & Qt::LeftButton) {
    auto delta = QPointF(event->globalPosition() - last_drag_) / scale_;
    last_drag_ = event->globalPosition();
    base_ -= delta;
    auto const translate_by = (delta * scale_).toPoint();
    auto translate = [&translate_by](ProcessingArea* area) {
      return area->translate(translate_by);
    };
    std::for_each(processed_areas_.begin(), processed_areas_.end(), translate);
    processOutOfScreenAreas();
    update();
  }
}

void GraphicEngine::resetAreas(QSize size) {
  auto const cell = cellSize();
  std::size_t const pool_size = n_cells_ + 1;
  blurred_images_pool.clear();
  blurred_images_pool.reserve(pool_size);
  for (auto i = 0; i < pool_size; ++i) {
    blurred_images_pool.push_back(generateBlur(cell, gen));
  }

  filterAreas([](ProcessingArea*) { return false; });
  for (auto i = 0; i < n_cells_; ++i) {
    for (auto j = 0; j < n_cells_; ++j) {
      QRect position(i * cell.width(), j * cell.height(), cell.width(),
                     cell.height());
      process(position);
    }
  }
  processOutOfScreenAreas();
  update();
}

void GraphicEngine::processOutOfScreenAreas() {
  QRect screen(QPoint(), size());
  QPoint increasing(cellSize().width() * out_of_screen_process_cells_,
                    cellSize().height() * out_of_screen_process_cells_);
  QRect cached_screen(screen.topLeft() - increasing,
                      screen.bottomRight() + increasing);

  filterAreas([cached_screen](ProcessingArea* area) {
    return area->position().intersects(cached_screen);
  });

  auto correct_rects = getAreaPositions(screen);
  int correct_rects_number = correct_rects.size();

  if (correct_rects_number == 0) {
    resetAreas(size());
    return;
  }

  produceDrawableAreas(
      std::mem_fn(&QRect::top), std::less{}, correct_rects,
      [this](QRect rect) { return rect.translated(0, -cellSize().height()); },
      cached_screen);
  produceDrawableAreas(
      std::mem_fn(&QRect::bottom), std::greater{}, correct_rects,
      [this](QRect rect) { return rect.translated(0, cellSize().height()); },
      cached_screen);
  produceDrawableAreas(
      std::mem_fn(&QRect::left), std::less{}, correct_rects,
      [this](QRect rect) { return rect.translated(-cellSize().width(), 0); },
      cached_screen);
  produceDrawableAreas(
      std::mem_fn(&QRect::right), std::greater{}, correct_rects,
      [this](QRect rect) { return rect.translated(cellSize().width(), 0); },
      cached_screen);

  auto process_new_rectangles = [&correct_rects, this,
                                 correct_rects_number](auto f) {
    std::for_each(std::next(correct_rects.begin(), correct_rects_number),
                  correct_rects.end(), [f, this](QRect area) {
                    if (f(area)) process(area);
                  });
  };

  process_new_rectangles(
      [this, &screen](QRect area) { return area.intersects(screen); });
  process_new_rectangles(
      [this, &screen](QRect area) { return !area.intersects(screen); });
}

void GraphicEngine::resizeEvent(QResizeEvent* event) {
  resetAreas(event->size());
  update();
}

void GraphicEngine::process(QRect area) {
  auto* processing_area =
      new ProcessingArea(base_, area, scale_, threads_, this);
  connect(processing_area, &ProcessingArea::processFinished,
          [this]() { update(); });
  processing_area->start();
  processed_areas_.emplace_back(std::move(processing_area));
}

void GraphicEngine::filterAreas(std::function<bool(ProcessingArea*)> f) {
  for (auto it = processed_areas_.begin(); it != processed_areas_.end();) {
    if (f(*it)) {
      ++it;
    } else {
      delete *it;
      it = processed_areas_.erase(it);
    }
  }
}

}  // namespace mandelbrot
