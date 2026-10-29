#include "view/plot_widget.h"

#include <QDate>
#include <QPaintEvent>
#include <QPainter>
#include <QPolygonF>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <ranges>

namespace {

constexpr int kMaxGraphs = 5;
constexpr double kCurveWidth = 2.0;
constexpr double kMarkerRadius = 4.0;
const QColor kMarkerColor(0x20, 0x22, 0x26);

constexpr int kLeftMargin = 64;
constexpr int kTopMargin = 14;
constexpr int kRightMargin = 16;
constexpr int kBottomMargin = 36;

double NiceStep(double range, int target_ticks) {
  if (range <= 0.0 || target_ticks < 1) {
    return 1.0;
  }
  const double raw = range / target_ticks;
  const double magnitude = std::pow(10.0, std::floor(std::log10(raw)));
  const double normalized = raw / magnitude;
  double factor = 10.0;
  if (normalized < 1.5) {
    factor = 1.0;
  } else if (normalized < 3.5) {
    factor = 2.0;
  } else if (normalized < 7.5) {
    factor = 5.0;
  }
  return factor * magnitude;
}

}  // namespace

PlotWidget::PlotWidget(QWidget* parent) : QWidget(parent) {
  setMinimumSize(480, 320);
}

void PlotWidget::AddGraph(const Graph& graph) {
  if (graphs_.size() >= kMaxGraphs) {
    return;
  }
  graphs_.append(graph);
  update();
}

void PlotWidget::Clear() {
  if (graphs_.isEmpty()) {
    return;
  }
  graphs_.clear();
  update();
}

void PlotWidget::paintEvent(QPaintEvent* /*event*/) {
  if (graphs_.isEmpty()) return;

  const PlotParams params = prepareParams();
  if (params.area.isEmpty()) return;

  QPainter painter(this);
  painter.fillRect(rect(), QColor(0xfb, 0xfc, 0xfd));
  painter.setRenderHint(QPainter::Antialiasing, true);

  QFont tick_font = font();
  tick_font.setPointSize(8);
  painter.setFont(tick_font);

  drawGridX(painter, params);
  drawGridY(painter, params);
  drawAxes(painter, params);

  painter.setClipRect(params.area);
  drawCurves(painter, params);
  drawMarkers(painter, params);
  painter.setClipping(false);
}

PlotWidget::PlotParams PlotWidget::prepareParams() const {
  PlotParams p;
  QRectF data_bounds;

  for (const Graph& graph : graphs_) {
    if (!graph.points.isEmpty()) {
      data_bounds = data_bounds.united(QPolygonF(graph.points).boundingRect());
    }
    if (graph.show_markers && !graph.marker_points.isEmpty()) {
      data_bounds =
          data_bounds.united(QPolygonF(graph.marker_points).boundingRect());
    }
  }

  if (data_bounds.isNull()) return {};

  const double dx = std::max(data_bounds.width() * 0.05, 1e-9);
  const double dy = std::max(data_bounds.height() * 0.05, 1e-9);

  p.x_min = data_bounds.x() - dx;
  p.x_max = data_bounds.x() + data_bounds.width() + dx;
  p.y_min = data_bounds.y() - dy;
  p.y_max = data_bounds.y() + data_bounds.height() + dy;

  p.area = rect().marginsRemoved(
      QMargins(kLeftMargin, kTopMargin, kRightMargin, kBottomMargin));
  p.kx = p.area.width() / (p.x_max - p.x_min);
  p.ky = p.area.height() / (p.y_max - p.y_min);

  return p;
}

void PlotWidget::drawGridX(QPainter& painter, const PlotParams& params) const {
  const QPen grid_pen(QColor(0xdf, 0xe4, 0xe9), 1.0);
  const QColor caption_color(0x60, 0x68, 0x70);

  const double x_step = NiceStep(params.x_max - params.x_min, 8);
  const double x_start = std::ceil(params.x_min / x_step) * x_step;
  const bool long_period = (params.x_max - params.x_min) > 1000.0;

  for (int i = 0; i < 20; ++i) {
    const double value = x_start + i * x_step;
    if (value > params.x_max) break;

    const double px = params.toPixel(QPointF(value, params.y_min)).x();
    painter.setPen(grid_pen);
    painter.drawLine(QPointF(px, params.area.top()),
                     QPointF(px, params.area.bottom()));

    const QDate date =
        QDate(1970, 1, 1).addDays(static_cast<qint64>(std::llround(value)));
    const QString caption = long_period
                                ? date.toString(QStringLiteral("MM.yyyy"))
                                : date.toString(QStringLiteral("dd.MM.yy"));
    painter.setPen(caption_color);
    painter.drawText(QRectF(px - 34.0, params.area.bottom() + 4.0, 68.0, 16.0),
                     Qt::AlignHCenter | Qt::AlignVCenter, caption);
  }
}

void PlotWidget::drawGridY(QPainter& painter, const PlotParams& params) const {
  const QPen grid_pen(QColor(0xdf, 0xe4, 0xe9), 1.0);
  const QColor caption_color(0x60, 0x68, 0x70);

  const double y_step = NiceStep(params.y_max - params.y_min, 6);
  const double y_start = std::ceil(params.y_min / y_step) * y_step;

  for (int i = 0; i < 20; ++i) {
    const double value = y_start + i * y_step;
    if (value > params.y_max) break;

    const double py = params.toPixel(QPointF(params.x_min, value)).y();
    painter.setPen(grid_pen);
    painter.drawLine(QPointF(params.area.left(), py),
                     QPointF(params.area.right(), py));
    painter.setPen(caption_color);
    painter.drawText(
        QRectF(rect().left() + 4.0, py - 8.0, kLeftMargin - 10.0, 16.0),
        Qt::AlignRight | Qt::AlignVCenter, QString::number(value, 'g', 6));
  }
}

void PlotWidget::drawAxes(QPainter& painter, const PlotParams& params) const {
  const QPen axis_pen(QColor(0x50, 0x58, 0x60), 1.0);
  painter.setPen(axis_pen);
  painter.drawLine(QPointF(params.area.left(), params.area.top()),
                   QPointF(params.area.left(), params.area.bottom()));
  painter.drawLine(QPointF(params.area.left(), params.area.bottom()),
                   QPointF(params.area.right(), params.area.bottom()));
}

void PlotWidget::drawCurves(QPainter& painter, const PlotParams& params) const {
  for (const Graph& graph : graphs_) {
    QPolygonF polygon;
    polygon.reserve(graph.points.size());
    for (const QPointF& point : graph.points) {
      polygon.append(params.toPixel(point));
    }
    painter.setPen(QPen(graph.color, kCurveWidth, Qt::SolidLine, Qt::RoundCap,
                        Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);
    painter.drawPolyline(polygon);
  }
}

void PlotWidget::drawMarkers(QPainter& painter,
                             const PlotParams& params) const {
  for (const Graph& graph : graphs_) {
    if (!graph.show_markers) continue;

    painter.setPen(Qt::NoPen);
    painter.setBrush(kMarkerColor);
    for (const QPointF& point : graph.marker_points) {
      painter.drawEllipse(params.toPixel(point), kMarkerRadius, kMarkerRadius);
    }
  }
}
