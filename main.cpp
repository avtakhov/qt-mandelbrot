#include <QApplication>

#include "graphic_engine.h"

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);
  mandelbrot::GraphicEngine w(10, nullptr);
  w.show();
  return a.exec();
}
