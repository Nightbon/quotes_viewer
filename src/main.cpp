#include <QApplication>

#include "controller/controller.h"
#include "view/main_window.h"

int main(int argc, char* argv[]) {
  QApplication app(argc, argv);
  s21::Controller controller;
  MainWindow window(controller);
  window.show();

  return app.exec();
}
