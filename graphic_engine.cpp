#include "graphic_engine.h"

#include <QPainter>
#include <QThreadPool>
#include <QWheelEvent>
#include <QWidget>
#include <algorithm>
#include <cmath>
#include <functional>
#include <random>

#include "colors.h"
#include "core.h"

namespace mandelbrot {

namespace {

void produceCorrectRectangles(std::function<int(QRect const&)> getValue,
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

GraphicEngine::GraphicEngine(std::size_t nCells, QWidget* parent = nullptr)
    : QWidget(parent), nCells(nCells), threads_(new QThreadPool(this)) {
  threads_->setMaxThreadCount(std::min(size_t(8), nCells * nCells));
  resetRectangles(size());
}

void GraphicEngine::paintEvent(QPaintEvent* event) {
  QPainter painter(this);
  std::uniform_int_distribution<> distrib(0, blurred_images_pool.size() - 1);
  for (auto const& rect : processed_areas_) {
    rect->draw(painter, blurred_images_pool[distrib(gen)]);
  }
}

void GraphicEngine::wheelEvent(QWheelEvent* event) {
  auto scale_change = event->angleDelta().y() * (10 + scale) / 1000;
  auto nscale = std::max(scale + scale_change, 10.);
  base += (event->position() / scale) - (event->position() / nscale);
  scale = nscale;
  resetRectangles(size());
  update();
}

void GraphicEngine::mousePressEvent(QMouseEvent* event) {
  if (event->button() == Qt::LeftButton) {
    lastDragPos = event->globalPosition();
  }
}

std::list<QRect> GraphicEngine::filterIntersectedRectangles(QRect main) {
  filterAreas([main](QRect rect) { return rect.intersects(main); });
  std::list<QRect> result;
  std::transform(processed_areas_.begin(), processed_areas_.end(),
                 std::back_inserter(result),
                 std::mem_fn(&ProcessingArea::position));
  return result;
}

QSize GraphicEngine::cellSize() const { return size() / nCells; }

void GraphicEngine::mouseMoveEvent(QMouseEvent* event) {
  if (event->buttons() & Qt::LeftButton) {
    auto delta = QPointF(event->globalPosition() - lastDragPos) / scale;
    lastDragPos = event->globalPosition();
    base -= delta;
    for (auto& rect : processed_areas_) {
      rect->translate((delta * scale).toPoint());
    }

    QRect screen(QPoint(), size());
    auto correct_rects = filterIntersectedRectangles(screen);
    int correct_rects_number = correct_rects.size();

    if (correct_rects_number == 0) {
      resetRectangles(size());
      update();
      return;
    }

    produceCorrectRectangles(
        std::mem_fn(&QRect::top), std::less{}, correct_rects,
        [this](QRect rect) { return rect.translated(0, -cellSize().height()); },
        screen);
    produceCorrectRectangles(
        std::mem_fn(&QRect::bottom), std::greater{}, correct_rects,
        [this](QRect rect) { return rect.translated(0, cellSize().height()); },
        screen);
    produceCorrectRectangles(
        std::mem_fn(&QRect::left), std::less{}, correct_rects,
        [this](QRect rect) { return rect.translated(-cellSize().width(), 0); },
        screen);
    produceCorrectRectangles(
        std::mem_fn(&QRect::right), std::greater{}, correct_rects,
        [this](QRect rect) { return rect.translated(cellSize().width(), 0); },
        screen);

    std::for_each(std::next(correct_rects.begin(), correct_rects_number),
                  correct_rects.end(), [this](QRect area) { process(area); });

    update();
  }
}

void GraphicEngine::resetRectangles(QSize size) {
  auto const cell = cellSize();
  std::size_t const pool_size = (nCells + 1) * 2;
  blurred_images_pool.clear();
  blurred_images_pool.reserve(pool_size);
  for (auto i = 0; i < pool_size; ++i) {
    blurred_images_pool.push_back(generateBlur(cell, gen));
  }

  filterAreas([](QRect) { return false; });
  for (auto i = 0; i < nCells; ++i) {
    for (auto j = 0; j < nCells; ++j) {
      QRect position(i * cell.width(), j * cell.height(), cell.width(),
                     cell.height());
      process(position);
    }
  }
}

void GraphicEngine::resizeEvent(QResizeEvent* event) {
  resetRectangles(event->size());
  update();
}

void GraphicEngine::process(QRect area) {
  auto* processing_area = new ProcessingArea(base, area, scale, threads_, this);
  connect(processing_area, &ProcessingArea::processFinished,
          [this]() { update(); });
  processing_area->start();
  processed_areas_.emplace_back(std::move(processing_area));
}

void GraphicEngine::filterAreas(std::function<bool(QRect)> f) {
  for (auto it = processed_areas_.begin(); it != processed_areas_.end();) {
    if (f((*it)->position())) {
      ++it;
    } else {
      (*it)->cancel();
      delete *it;
      it = processed_areas_.erase(it);
    }
  }
}

}  // namespace mandelbrot
