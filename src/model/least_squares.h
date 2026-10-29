#pragma once

#include "model/model_types.h"

namespace s21 {

class LeastSquares {
 public:
  bool Build(const std::vector<GraphPoint>& points, int degree);
  [[nodiscard]] double Evaluate(double x) const;

 private:
  std::vector<double> coeffs_;
  double x_min_{0.0};
  double x_span_{1.0};
};

}  // namespace s21
