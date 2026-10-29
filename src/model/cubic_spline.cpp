#include "model/cubic_spline.h"

#include <algorithm>

namespace s21 {

bool CubicSpline::Build(const std::vector<GraphPoint>& points) {
  const std::size_t n = points.size();
  xs_.resize(n);
  std::ranges::transform(points, xs_.begin(), &GraphPoint::x);

  if (n < 2) {
    return false;
  }

  for (std::size_t i = 1; i < n; ++i) {
    if (!(points[i].x > points[i - 1].x)) {
      return false;
    }
  }

  const std::size_t m = n - 1;
  std::vector<double> h(m, 0.0);
  std::vector<double> slope(m, 0.0);
  for (std::size_t i = 0; i < m; ++i) {
    h[i] = xs_[i + 1] - xs_[i];
    slope[i] = (points[i + 1].y - points[i].y) / h[i];
  }

  std::vector<double> c(n, 0.0);
  if (n > 2) {
    std::vector<double> alpha(n, 0.0);
    std::vector<double> beta(n, 0.0);

    for (std::size_t i = 1; i + 1 < n; ++i) {
      const double denom = 2.0 * (h[i - 1] + h[i]) + h[i - 1] * alpha[i - 1];
      alpha[i] = -h[i] / denom;
      beta[i] =
          (3.0 * (slope[i] - slope[i - 1]) - h[i - 1] * beta[i - 1]) / denom;
    }

    // Обратный ход
    for (std::size_t i = n - 2; i >= 1; --i) {
      c[i] = alpha[i] * c[i + 1] + beta[i];
    }
  }

  segs_.resize(m);
  for (std::size_t i = 0; i < m; ++i) {
    segs_[i].x_left = xs_[i];
    segs_[i].a = points[i].y;
    segs_[i].b = slope[i] - h[i] * (2.0 * c[i] + c[i + 1]) / 3.0;
    segs_[i].c = c[i];
    segs_[i].d = (c[i + 1] - c[i]) / (3.0 * h[i]);
  }
  return true;
}

double CubicSpline::Evaluate(double x) const {
  if (segs_.empty()) {
    return 0.0;
  }

  x = std::clamp(x, xs_.front(), xs_.back());

  auto it = std::lower_bound(xs_.begin(), xs_.end(), x);
  size_t seg_idx = (it == xs_.begin()) ? 0 : std::distance(xs_.begin(), it) - 1;

  const auto& seg = segs_[seg_idx];
  double t = x - seg.x_left;
  return seg.a + (seg.b + (seg.c + seg.d * t) * t) * t;
}

}  // namespace s21
