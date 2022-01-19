#pragma once

#include <QCache>
#include <QPixmap>
#include <QSize>
#include <QThreadPool>
#include <QWidget>
#include <random>

#include "processing_area.h"

namespace mandelbrot {

class GraphicEngine : public QWidget {
 public:
  GraphicEngine(std::size_t nCells, QWidget *parent);

  void paintEvent(QPaintEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;

 private:
  void pixmapUpdate(QSize size);
  void resetRectangles(QSize size);
  std::list<QRect> filterIntersectedRectangles(QRect main);
  QSize cellSize() const;
  void process(QRect area);
  void filterAreas(std::function<bool(QRect)>);

  std::size_t nCells;
  std::list<std::shared_ptr<ProcessingArea>> processed_areas_;
  QPointF base = QPoint(-5, -5);
  double scale = 50;
  QPointF lastDragPos;
  QThreadPool *threads_;
  mutable std::mt19937 gen;
  std::vector<QImage> blurred_images_pool;
};

}  // namespace mandelbrot
