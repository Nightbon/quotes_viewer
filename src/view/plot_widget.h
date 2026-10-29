#pragma once

#include <QColor>
#include <QString>
#include <QVector>
#include <QWidget>

class QPaintEvent;
class QPointF;

class PlotWidget final : public QWidget {
  Q_OBJECT

 public:
  struct Graph {
    QVector<QPointF> points;
    QColor color;
    QString label;
    bool show_markers{false};
    QVector<QPointF> marker_points;
  };

  explicit PlotWidget(QWidget* parent = nullptr);

  void AddGraph(const Graph& graph);
  void Clear();
  [[nodiscard]] int GraphCount() const noexcept {
    return static_cast<int>(graphs_.size());
  }

 protected:
  void paintEvent(QPaintEvent* event) override;

 private:
  struct PlotParams {
    QRectF area;
    double x_min = 0.0;
    double x_max = 0.0;
    double y_min = 0.0;
    double y_max = 0.0;
    double kx = 0.0;
    double ky = 0.0;

    QPointF toPixel(const QPointF& p) const {
      return QPointF(area.left() + (p.x() - x_min) * kx,
                     area.bottom() - (p.y() - y_min) * ky);
    }
  };

  PlotParams prepareParams() const;
  void drawGridX(QPainter& painter, const PlotParams& params) const;
  void drawGridY(QPainter& painter, const PlotParams& params) const;
  void drawAxes(QPainter& painter, const PlotParams& params) const;
  void drawCurves(QPainter& painter, const PlotParams& params) const;
  void drawMarkers(QPainter& painter, const PlotParams& params) const;

  QVector<Graph> graphs_;
};
