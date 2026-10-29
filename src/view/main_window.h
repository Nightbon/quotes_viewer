#pragma once

#include <QColor>
#include <QMainWindow>
#include <QString>
#include <QVector>

#include "controller/controller.h"

class QDateEdit;
class QLabel;
class QListWidget;
class QPushButton;
class QSpinBox;
class QTabWidget;
class QTimeEdit;
class PlotWidget;

class MainWindow final : public QMainWindow {
  Q_OBJECT

 public:
  explicit MainWindow(s21::Controller& controller, QWidget* parent = nullptr);

 private slots:
  void OnLoadFile();
  void OnDrawSpline();
  void OnDrawApprox();
  void OnClearSpline();
  void OnClearApprox();
  void OnSplineValue();
  void OnApproxValue();

 private:
  QWidget* CreateSplineTab();
  QWidget* CreateApproxTab();
  bool LoadFrom(const QString& path);
  void UpdateButtons();
  void AppendLegend(QListWidget* list, const QColor& color,
                    const QString& text);

  s21::Controller& controller_;

  QTabWidget* tabs_{nullptr};
  QLabel* file_info_{nullptr};
  QString current_file_name_;

  // интерполяция
  PlotWidget* spline_plot_{nullptr};
  QSpinBox* spline_points_{nullptr};
  QPushButton* draw_spline_btn_{nullptr};
  QPushButton* clear_spline_btn_{nullptr};
  QDateEdit* spline_date_{nullptr};
  QTimeEdit* spline_time_{nullptr};
  QLabel* spline_value_out_{nullptr};
  QListWidget* spline_legend_{nullptr};
  QPushButton* spline_value_btn_{nullptr};

  // аппроксимация
  PlotWidget* approx_plot_{nullptr};
  QSpinBox* approx_points_{nullptr};
  QSpinBox* approx_degree_{nullptr};
  QSpinBox* approx_days_{nullptr};
  QPushButton* draw_approx_btn_{nullptr};
  QPushButton* clear_approx_btn_{nullptr};
  QDateEdit* approx_date_{nullptr};
  QTimeEdit* approx_time_{nullptr};
  QLabel* approx_value_out_{nullptr};
  QListWidget* approx_legend_{nullptr};
  QPushButton* approx_value_btn_{nullptr};
  QVector<QPointF> source_markers_;
  int approx_days_prev_ = 0;
};
