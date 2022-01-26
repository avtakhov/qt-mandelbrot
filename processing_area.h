#pragma once

#include <QFuture>
#include <QImage>
#include <QPainter>
#include <QThreadPool>

namespace mandelbrot {

class ProcessingArea : public QObject {
  Q_OBJECT
 public:
  ProcessingArea(QPointF base_p0, QRect position, double scale,
                 QThreadPool* threads, QObject* parent);

  void draw(QPainter& painter, QImage const& blur) const;

  void translate(QPoint offset);

  QRect position() const;

  void cancel();

  void start();

  bool isFinished() const;

  ~ProcessingArea();

 signals:
  void processFinished();

 private:
  QPoint current_position_;
  QSize area_size_;
  QPointF from_point_;
  double scale_;
  QThreadPool* threads_;
  QFuture<QImage> processed_area_;
  std::atomic<bool> cancel_flag_;
};

}  // namespace mandelbrot
