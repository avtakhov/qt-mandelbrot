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
  GraphicEngine(std::size_t n_cells, QWidget *parent);

  void paintEvent(QPaintEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;

 private:
  std::list<QRect> getAreaPositions(QRect main);
  QSize cellSize() const;
  void process(QRect area);

  void filterAreas(std::function<bool(ProcessingArea *)>);
  void resetAreas(QSize size);
  void translateAreas(QPoint shift);
  void processOutOfScreenAreas();

  std::size_t n_cells_;
  std::list<ProcessingArea *> processed_areas_;
  QPointF base_ = QPoint(-5, -5);
  double scale_ = 50;
  QPointF last_drag_;
  QThreadPool *threads_;
  mutable std::mt19937 gen;
  std::vector<QImage> blurred_images_pool;
  std::int32_t out_of_screen_process_cells_;
};

}  // namespace mandelbrot
