#include "model/model.h"

#include <ranges>
#include <string>
#include <vector>

namespace s21 {
namespace {

inline double ToDaysSinceEpoch(Date date, double day_fraction = 0.0) noexcept {
  const std::chrono::sys_days sys{date};
  return static_cast<double>(sys.time_since_epoch().count()) + day_fraction;
}

std::vector<GraphPoint> ToGraphPoints(const RawPointList& quotes) {
  std::vector<GraphPoint> points;
  points.reserve(quotes.size());
  for (const RawPoint& quote : quotes) {
    points.push_back(
        GraphPoint{.x = ToDaysSinceEpoch(quote.date), .y = quote.close});
  }
  return points;
}

template <std::invocable<double> Func>
std::vector<GraphPoint> ForPointsCount(double x0, double x1, size_t count,
                                       Func value_at) {
  std::vector<GraphPoint> points;
  points.reserve(count);
  const double step =
      count > 1 ? (x1 - x0) / static_cast<double>(count - 1) : 0.0;

  for (const size_t i : std::views::iota(0u, count)) {
    const double x = x0 + step * static_cast<double>(i);
    points.push_back(GraphPoint{.x = x, .y = value_at(x)});
  }
  return points;
}

}  // namespace

Result<std::size_t> Model::LoadCsv(const std::filesystem::path& path) {
  const Result<RawPointList> loaded = CsvLoader{}.Load(path);
  if (!loaded.IsOk()) {
    return Result<std::size_t>::Fail(loaded.Error());
  }
  raw_points_ = loaded.Value();
  source_name_ = path.filename().string();
  spline_.Build(ToGraphPoints(raw_points_));
  return Result<std::size_t>::Ok(raw_points_.size());
}

Date Model::FirstDate() const noexcept {
  return raw_points_.empty() ? Date{} : raw_points_.front().date;
}

Date Model::LastDate() const noexcept {
  return raw_points_.empty() ? Date{} : raw_points_.back().date;
}

Result<std::vector<GraphPoint>> Model::SplineGraph(size_t points_count) const {
  if (raw_points_.empty()) {
    return ResultT::Fail("данные не загружены");
  }

  if (points_count < raw_points_.size()) {
    return ResultT::Fail(
        "количество точек должно быть не меньше числа записей файла");
  }

  const double x0 = ToDaysSinceEpoch(raw_points_.front().date);
  const double x1 = ToDaysSinceEpoch(raw_points_.back().date);
  const auto value_at = [this](double x) { return spline_.Evaluate(x); };
  return ResultT::Ok(ForPointsCount(x0, x1, points_count, value_at));
}

Result<std::vector<GraphPoint>> Model::ApproxGraph(size_t points_count,
                                                   int degree,
                                                   int days_ahead) const {
  if (raw_points_.empty()) {
    return ResultT::Fail("данные не загружены");
  }
  if (points_count < raw_points_.size()) {
    return ResultT::Fail(
        "количество точек должно быть не меньше числа записей файла");
  }
  if (degree < 0 || static_cast<size_t>(degree) > raw_points_.size() - 1) {
    return ResultT::Fail(
        "степень полинома должна быть от 0 до числа точек данных минус один");
  }
  if (days_ahead < 1) {
    return ResultT::Fail("количество дней M должно быть не меньше единицы");
  }
  LeastSquares approx;
  if (!approx.Build(ToGraphPoints(raw_points_), degree)) {
    return ResultT::Fail("полином заданной степени построить не удалось");
  }
  const double x0 = ToDaysSinceEpoch(raw_points_.front().date);
  const double x1 = ToDaysSinceEpoch(raw_points_.back().date) +
                    static_cast<double>(days_ahead);
  const auto value_at = [&approx](double x) { return approx.Evaluate(x); };
  return ResultT::Ok(ForPointsCount(x0, x1, points_count, value_at));
}

Result<std::vector<GraphPoint>> Model::SourcePoints() const {
  if (raw_points_.empty()) {
    return ResultT::Fail("данные не загружены");
  }
  return ResultT::Ok(ToGraphPoints(raw_points_));
}

Result<double> Model::SplineValueAt(Date date, double day_fraction) const {
  if (raw_points_.empty()) {
    return Result<double>::Fail("данные не загружены");
  }
  if (!date.ok()) {
    return Result<double>::Fail("некорректная дата");
  }
  return Result<double>::Ok(
      spline_.Evaluate(ToDaysSinceEpoch(date, day_fraction)));
}

Result<double> Model::ApproxValueAt(Date date, double day_fraction,
                                    int degree) const {
  if (raw_points_.empty()) {
    return Result<double>::Fail("данные не загружены");
  }
  if (!date.ok()) {
    return Result<double>::Fail("некорректная дата");
  }
  if (degree < 0 || static_cast<size_t>(degree) > raw_points_.size() - 1) {
    return Result<double>::Fail(
        "степень полинома должна быть от 0 до числа точек данных минус один");
  }
  LeastSquares approx;
  if (!approx.Build(ToGraphPoints(raw_points_), degree)) {
    return Result<double>::Fail(
        "полином заданной степени построить не удалось");
  }
  return Result<double>::Ok(
      approx.Evaluate(ToDaysSinceEpoch(date, day_fraction)));
}

}  // namespace s21
