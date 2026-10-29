#include "view/main_window.h"

#include <QDate>
#include <QDateEdit>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTime>
#include <QTimeEdit>
#include <QVBoxLayout>
#include <QWidget>

#include "view/plot_widget.h"

namespace {

constexpr int kMaxGraphs = 5;

const QColor kGraphColors[kMaxGraphs] = {
    QColor(0x1f, 0x77, 0xb4),  // синий
    QColor(0xff, 0x7f, 0x0e),  // оранжевый
    QColor(0x2c, 0xa0, 0x2c),  // зелёный
    QColor(0x94, 0x67, 0xbd),  // фиолетовый
    QColor(0x17, 0xbe, 0xcf),  // бирюзовый
};

}  // namespace

MainWindow::MainWindow(s21::Controller& controller, QWidget* parent)
    : QMainWindow(parent), controller_(controller) {
  setWindowTitle(QStringLiteral("Кубический сплайн и МНК"));
  resize(1360, 760);

  QWidget* central = new QWidget(this);
  QVBoxLayout* layout = new QVBoxLayout(central);
  layout->setContentsMargins(10, 10, 10, 10);
  layout->setSpacing(8);

  QPushButton* load_button =
      new QPushButton(QStringLiteral("Загрузить CSV…"), central);
  connect(load_button, &QPushButton::clicked, this, &MainWindow::OnLoadFile);
  file_info_ = new QLabel(QStringLiteral("Файл не загружен"), central);
  QHBoxLayout* top_bar = new QHBoxLayout();
  top_bar->addWidget(load_button);
  top_bar->addWidget(file_info_, 1);
  layout->addLayout(top_bar);

  tabs_ = new QTabWidget(central);
  tabs_->addTab(CreateSplineTab(),
                QStringLiteral("Интерполяция кубическим сплайном"));
  tabs_->addTab(CreateApproxTab(), QStringLiteral("Аппроксимация МНК"));
  layout->addWidget(tabs_, 1);
  setCentralWidget(central);

  UpdateButtons();
}

QWidget* MainWindow::CreateSplineTab() {
  QWidget* page = new QWidget(this);
  QVBoxLayout* layout = new QVBoxLayout(page);
  layout->setContentsMargins(8, 8, 8, 8);

  QHBoxLayout* controls = new QHBoxLayout();
  controls->addWidget(new QLabel(QStringLiteral("Количество точек:"), page));
  spline_points_ = new QSpinBox(page);
  spline_points_->setRange(2, 100000);
  spline_points_->setValue(2);
  controls->addWidget(spline_points_);
  draw_spline_btn_ = new QPushButton(QStringLiteral("Построить сплайн"), page);
  connect(draw_spline_btn_, &QPushButton::clicked, this,
          &MainWindow::OnDrawSpline);
  controls->addWidget(draw_spline_btn_);
  controls->addStretch(1);
  clear_spline_btn_ = new QPushButton(QStringLiteral("Очистить поле"), page);
  connect(clear_spline_btn_, &QPushButton::clicked, this,
          &MainWindow::OnClearSpline);
  controls->addWidget(clear_spline_btn_);
  layout->addLayout(controls);

  // Поле отрисовки.
  spline_plot_ = new PlotWidget(page);
  layout->addWidget(spline_plot_, 1);

  // Легенда: цвет, имя файла.
  spline_legend_ = new QListWidget(page);
  spline_legend_->setFixedHeight(100);
  layout->addWidget(spline_legend_);

  // Вычисление значения сплайна по дате и времени.
  QHBoxLayout* value_row = new QHBoxLayout();
  value_row->addWidget(
      new QLabel(QStringLiteral("Значение сплайна в дату:"), page));
  spline_date_ = new QDateEdit(page);
  spline_date_->setDisplayFormat(QStringLiteral("dd.MM.yyyy"));
  spline_date_->setCalendarPopup(true);
  value_row->addWidget(spline_date_);
  spline_time_ = new QTimeEdit(page);
  spline_time_->setDisplayFormat(QStringLiteral("HH:mm"));
  value_row->addWidget(spline_time_);
  spline_value_btn_ = new QPushButton(QStringLiteral("Вычислить"), page);
  connect(spline_value_btn_, &QPushButton::clicked, this,
          &MainWindow::OnSplineValue);
  value_row->addWidget(spline_value_btn_);
  spline_value_out_ = new QLabel(page);
  value_row->addWidget(spline_value_out_, 1);
  layout->addLayout(value_row);

  return page;
}

QWidget* MainWindow::CreateApproxTab() {
  QWidget* page = new QWidget(this);
  QVBoxLayout* layout = new QVBoxLayout(page);
  layout->setContentsMargins(8, 8, 8, 8);

  QHBoxLayout* params = new QHBoxLayout();
  params->addWidget(new QLabel(QStringLiteral("Количество точек:"), page));
  approx_points_ = new QSpinBox(page);
  approx_points_->setRange(2, 100000);
  approx_points_->setValue(2);
  params->addWidget(approx_points_);
  params->addWidget(new QLabel(QStringLiteral("Степень полинома:"), page));
  approx_degree_ = new QSpinBox(page);
  approx_degree_->setRange(0, 15);
  approx_degree_->setValue(3);
  params->addWidget(approx_degree_);
  params->addWidget(new QLabel(QStringLiteral("Дней вперёд (M):"), page));
  approx_days_ = new QSpinBox(page);
  approx_days_->setRange(1, 3650);
  approx_days_->setValue(30);
  params->addWidget(approx_days_);
  params->addStretch(1);
  layout->addLayout(params);

  QHBoxLayout* controls = new QHBoxLayout();
  draw_approx_btn_ =
      new QPushButton(QStringLiteral("Построить аппроксимацию"), page);
  connect(draw_approx_btn_, &QPushButton::clicked, this,
          &MainWindow::OnDrawApprox);
  controls->addWidget(draw_approx_btn_);
  controls->addStretch(1);
  clear_approx_btn_ = new QPushButton(QStringLiteral("Очистить поле"), page);
  connect(clear_approx_btn_, &QPushButton::clicked, this,
          &MainWindow::OnClearApprox);
  controls->addWidget(clear_approx_btn_);
  layout->addLayout(controls);

  approx_plot_ = new PlotWidget(page);
  layout->addWidget(approx_plot_, 1);

  approx_legend_ = new QListWidget(page);
  approx_legend_->setFixedHeight(100);
  layout->addWidget(approx_legend_);

  QHBoxLayout* value_row = new QHBoxLayout();
  value_row->addWidget(
      new QLabel(QStringLiteral("Значение полинома в дату:"), page));
  approx_date_ = new QDateEdit(page);
  approx_date_->setDisplayFormat(QStringLiteral("dd.MM.yyyy"));
  approx_date_->setCalendarPopup(true);
  value_row->addWidget(approx_date_);
  approx_time_ = new QTimeEdit(page);
  approx_time_->setDisplayFormat(QStringLiteral("HH:mm"));
  value_row->addWidget(approx_time_);
  approx_value_btn_ = new QPushButton(QStringLiteral("Вычислить"), page);
  connect(approx_value_btn_, &QPushButton::clicked, this,
          &MainWindow::OnApproxValue);
  value_row->addWidget(approx_value_btn_);
  approx_value_out_ = new QLabel(page);
  value_row->addWidget(approx_value_out_, 1);
  layout->addLayout(value_row);

  return page;
}

void MainWindow::OnLoadFile() {
  const QString path = QFileDialog::getOpenFileName(
      this, QStringLiteral("Загрузка файла котировок"), QString(),
      QStringLiteral("Файлы котировок (*.csv);;"));
  if (path.isEmpty()) {
    return;
  }
  LoadFrom(path);
}

bool MainWindow::LoadFrom(const QString& path) {
  const s21::LoadInfo info = controller_.LoadFile(path.toStdString());
  const s21::GraphResult source = controller_.SourcePoints();
  if (!info.ok || !source.ok) {
    QMessageBox::warning(this, QStringLiteral("Ошибка загрузки"),
                         QString::fromStdString(info.error));
    return false;
  }

  spline_plot_->Clear();
  approx_plot_->Clear();
  spline_legend_->clear();
  approx_legend_->clear();
  spline_value_out_->clear();
  approx_value_out_->clear();
  current_file_name_ = QString::fromStdString(info.file_name);

  approx_days_prev_ = 0;
  source_markers_.clear();
  source_markers_.reserve(static_cast<qsizetype>(source.points.size()));
  for (const s21::ControllerPoint& point : source.points) {
    source_markers_.append(QPointF(point.x, point.y));
  }

  const QDate first(info.first_date.year, info.first_date.month,
                    info.first_date.day);
  const QDate last(info.last_date.year, info.last_date.month,
                   info.last_date.day);
  spline_points_->setMinimum(info.points_count);
  approx_points_->setMinimum(info.points_count);
  spline_points_->setValue(info.points_count);
  approx_points_->setValue(info.points_count);
  spline_date_->setDateRange(first, last);
  approx_date_->setDateRange(first, last.addDays(3650));
  spline_date_->setDate(first);
  approx_date_->setDate(first);

  file_info_->setText(QStringLiteral("Файл: %1 · записей: %2 · период: %3 — %4")
                          .arg(current_file_name_)
                          .arg(info.points_count)
                          .arg(first.toString(QStringLiteral("dd.MM.yyyy")))
                          .arg(last.toString(QStringLiteral("dd.MM.yyyy"))));
  UpdateButtons();
  return true;
}

void MainWindow::OnDrawSpline() {
  if (!controller_.HasData() || spline_plot_->GraphCount() >= kMaxGraphs) {
    return;
  }
  const s21::GraphResult result =
      controller_.SplineGraph(spline_points_->value());
  if (!result.ok) {
    QMessageBox::warning(this, QStringLiteral("Ошибка построения"),
                         QString::fromStdString(result.error));
    return;
  }
  QVector<QPointF> points;
  points.reserve(static_cast<qsizetype>(result.points.size()));
  for (const s21::ControllerPoint& point : result.points) {
    points.append(QPointF(point.x, point.y));
  }
  const QColor color = kGraphColors[spline_plot_->GraphCount()];
  const QString label =
      QStringLiteral("%1 · кубический сплайн").arg(current_file_name_);
  spline_plot_->AddGraph(PlotWidget::Graph{.points = points,
                                           .color = color,
                                           .label = label,
                                           .show_markers = false,
                                           .marker_points = {}});
  AppendLegend(spline_legend_, color, label);
  UpdateButtons();
}

void MainWindow::OnDrawApprox() {
  if (!controller_.HasData() || approx_plot_->GraphCount() >= kMaxGraphs) {
    return;
  }
  const int days = approx_days_->value();
  if (approx_days_prev_ != days) {
    approx_plot_->Clear();
    approx_legend_->clear();
    approx_days_prev_ = days;
  }
  const s21::GraphResult result = controller_.ApproxGraph(
      approx_points_->value(), approx_degree_->value(), days);
  if (!result.ok) {
    QMessageBox::warning(this, QStringLiteral("Ошибка построения"),
                         QString::fromStdString(result.error));
    return;
  }
  QVector<QPointF> points;
  points.reserve(static_cast<qsizetype>(result.points.size()));
  for (const s21::ControllerPoint& point : result.points) {
    points.append(QPointF(point.x, point.y));
  }
  const QColor color = kGraphColors[approx_plot_->GraphCount()];
  const QString label = QStringLiteral("%1 · МНК · степень %2 · M=%3")
                            .arg(current_file_name_)
                            .arg(approx_degree_->value())
                            .arg(days);
  approx_plot_->AddGraph(PlotWidget::Graph{.points = points,
                                           .color = color,
                                           .label = label,
                                           .show_markers = true,
                                           .marker_points = source_markers_});
  AppendLegend(approx_legend_, color, label);
  UpdateButtons();
}

void MainWindow::OnClearSpline() {
  spline_plot_->Clear();
  spline_legend_->clear();
  UpdateButtons();
}

void MainWindow::OnClearApprox() {
  approx_plot_->Clear();
  approx_legend_->clear();
  approx_days_prev_ = 0;
  UpdateButtons();
}

void MainWindow::OnSplineValue() {
  const QDate date = spline_date_->date();
  const QTime time = spline_time_->time();
  const s21::ControllerDateTime moment{.year = date.year(),
                                       .month = date.month(),
                                       .day = date.day(),
                                       .hour = time.hour(),
                                       .minute = time.minute(),
                                       .second = time.second()};
  const s21::ValueResult result = controller_.SplineValueAt(moment);
  if (!result.ok) {
    QMessageBox::warning(this, QStringLiteral("Ошибка вычисления"),
                         QString::fromStdString(result.error));
    return;
  }
  spline_value_out_->setText(
      QStringLiteral("%1 %2 → %3")
          .arg(date.toString(QStringLiteral("dd.MM.yyyy")),
               time.toString(QStringLiteral("HH:mm")),
               QString::number(result.value, 'f', 6)));
}

void MainWindow::OnApproxValue() {
  const QDate date = approx_date_->date();
  const QTime time = approx_time_->time();
  const s21::ControllerDateTime moment{.year = date.year(),
                                       .month = date.month(),
                                       .day = date.day(),
                                       .hour = time.hour(),
                                       .minute = time.minute(),
                                       .second = time.second()};
  const s21::ValueResult result =
      controller_.ApproxValueAt(moment, approx_degree_->value());
  if (!result.ok) {
    QMessageBox::warning(this, QStringLiteral("Ошибка вычисления"),
                         QString::fromStdString(result.error));
    return;
  }
  approx_value_out_->setText(
      QStringLiteral("степень %1: %2 %3 → %4")
          .arg(QString::number(approx_degree_->value()),
               date.toString(QStringLiteral("dd.MM.yyyy")),
               time.toString(QStringLiteral("HH:mm")),
               QString::number(result.value, 'f', 6)));
}

void MainWindow::UpdateButtons() {
  const bool has_data = controller_.HasData();
  draw_spline_btn_->setEnabled(has_data &&
                               spline_plot_->GraphCount() < kMaxGraphs);
  draw_approx_btn_->setEnabled(has_data &&
                               approx_plot_->GraphCount() < kMaxGraphs);
  clear_spline_btn_->setEnabled(spline_plot_->GraphCount() > 0);
  clear_approx_btn_->setEnabled(approx_plot_->GraphCount() > 0);
  spline_date_->setEnabled(has_data);
  spline_time_->setEnabled(has_data);
  spline_points_->setEnabled(has_data);
  spline_value_btn_->setEnabled(has_data);
  approx_date_->setEnabled(has_data);
  approx_time_->setEnabled(has_data);
  approx_value_btn_->setEnabled(has_data);
  approx_points_->setEnabled(has_data);
  approx_degree_->setEnabled(has_data);
  approx_days_->setEnabled(has_data);
}

void MainWindow::AppendLegend(QListWidget* list, const QColor& color,
                              const QString& text) {
  QPixmap swatch(14, 14);
  swatch.fill(color);
  QPainter painter(&swatch);
  painter.setPen(QColor(0x30, 0x30, 0x30));
  painter.drawRect(0, 0, 13, 13);
  painter.end();
  list->addItem(new QListWidgetItem(QIcon(swatch), text));
}