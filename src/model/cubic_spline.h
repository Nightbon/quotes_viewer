#pragma once

#include "model/model_types.h"

namespace s21 {

class CubicSpline {
 public:
  bool Build(const std::vector<GraphPoint>& points);
  [[nodiscard]] double Evaluate(double x) const;

 private:
  struct Segment {
    double x_left;
    double a{};
    double b{};
    double c{};
    double d{};
  };

  std::vector<double> xs_;
  std::vector<Segment> segs_;
};

}  // namespace s21
