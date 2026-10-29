#pragma once

#include <filesystem>

#include "model/csv_loader.h"
#include "model/cubic_spline.h"
#include "model/least_squares.h"
#include "model/model_types.h"

namespace s21 {

class Model {
  using ResultT = Result<std::vector<GraphPoint>>;

 public:
  Result<std::size_t> LoadCsv(const std::filesystem::path& path);

  [[nodiscard]] bool HasData() const noexcept { return !raw_points_.empty(); }
  [[nodiscard]] std::size_t Count() const noexcept {
    return raw_points_.size();
  }
  [[nodiscard]] const std::string& SourceName() const noexcept {
    return source_name_;
  }
  [[nodiscard]] Date FirstDate() const noexcept;
  [[nodiscard]] Date LastDate() const noexcept;
  [[nodiscard]] ResultT SourcePoints() const;

  Result<std::vector<GraphPoint>> SplineGraph(size_t points_count) const;
  Result<std::vector<GraphPoint>> ApproxGraph(size_t points_count, int degree,
                                              int days_ahead) const;
  Result<double> SplineValueAt(Date date, double day_fraction = 0.0) const;
  Result<double> ApproxValueAt(Date date, double day_fraction,
                               int degree) const;

 private:
  RawPointList raw_points_;
  std::string source_name_;
  CubicSpline spline_;
};

}  // namespace s21
