#include "controller/controller.h"

#include <algorithm>
#include <chrono>

namespace s21 {
namespace {

ControllerDateTime ToControllerDateTime(Date date) noexcept {
  return ControllerDateTime{
      .year = static_cast<int>(date.year()),
      .month = static_cast<int>(static_cast<unsigned>(date.month())),
      .day = static_cast<int>(static_cast<unsigned>(date.day()))};
}

std::pair<Date, double> FromControllerDateTime(
    const ControllerDateTime& moment) noexcept {
  const Date date{std::chrono::year(moment.year),
                  std::chrono::month(static_cast<unsigned>(moment.month)),
                  std::chrono::day(static_cast<unsigned>(moment.day))};

  const double seconds_of_day =
      3600.0 * moment.hour + 60.0 * moment.minute + moment.second;
  return {date, seconds_of_day / 86400.0};
}

std::vector<ControllerPoint> ToControllerPoints(
    const std::vector<GraphPoint>& points) {
  std::vector<ControllerPoint> converted;
  converted.reserve(points.size());

  std::ranges::transform(points, std::back_inserter(converted),
                         [](const GraphPoint& point) {
                           return ControllerPoint{.x = point.x, .y = point.y};
                         });

  return converted;
}

}  // namespace

LoadInfo Controller::LoadFile(const std::string& path) {
  const Result<std::size_t> result = model_.LoadCsv(path);
  if (!result.IsOk()) {
    return LoadInfo{.ok = false, .error = result.Error()};
  }
  return LoadInfo{.ok = true,
                  .error = {},
                  .file_name = model_.SourceName(),
                  .points_count = static_cast<int>(model_.Count()),
                  .first_date = ToControllerDateTime(model_.FirstDate()),
                  .last_date = ToControllerDateTime(model_.LastDate())};
}

GraphResult Controller::SplineGraph(int points_count) {
  const Result<std::vector<GraphPoint>> result =
      model_.SplineGraph(points_count);
  if (!result.IsOk()) {
    return GraphResult{.ok = false, .error = result.Error()};
  }
  return GraphResult{.ok = true, .points = ToControllerPoints(result.Value())};
}

GraphResult Controller::ApproxGraph(int points_count, int degree,
                                    int days_ahead) {
  const Result<std::vector<GraphPoint>> result =
      model_.ApproxGraph(points_count, degree, days_ahead);
  if (!result.IsOk()) {
    return GraphResult{.ok = false, .error = result.Error()};
  }
  return GraphResult{.ok = true, .points = ToControllerPoints(result.Value())};
}

GraphResult Controller::SourcePoints() {
  const Result<std::vector<GraphPoint>> result = model_.SourcePoints();
  if (!result.IsOk()) {
    return GraphResult{.ok = false, .error = result.Error()};
  }
  return GraphResult{.ok = true, .points = ToControllerPoints(result.Value())};
}

ValueResult Controller::SplineValueAt(const ControllerDateTime& moment) {
  const auto [date, day_fraction] = FromControllerDateTime(moment);
  const Result<double> result = model_.SplineValueAt(date, day_fraction);
  if (!result.IsOk()) {
    return ValueResult{.ok = false, .error = result.Error()};
  }
  return ValueResult{.ok = true, .value = result.Value()};
}

ValueResult Controller::ApproxValueAt(const ControllerDateTime& moment,
                                      int degree) {
  const auto [date, day_fraction] = FromControllerDateTime(moment);
  const Result<double> result =
      model_.ApproxValueAt(date, day_fraction, degree);
  if (!result.IsOk()) {
    return ValueResult{.ok = false, .error = result.Error()};
  }
  return ValueResult{.ok = true, .value = result.Value()};
}

}  // namespace s21
