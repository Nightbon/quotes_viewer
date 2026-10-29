#include "model/least_squares.h"

#include <algorithm>
#include <ranges>

namespace s21 {

namespace {

bool SolveGaussJordan(std::vector<std::vector<double>>& mat,
                      std::vector<double>& result) {
  const size_t n = mat.size();

  for (size_t col = 0; col < n; ++col) {
    size_t max_row = col;
    for (size_t k = col + 1; k < n; ++k) {
      if (std::abs(mat[k][col]) > std::abs(mat[max_row][col])) {
        max_row = k;
      }
    }

    if (std::abs(mat[max_row][col]) < 1e-12) {
      return false;
    }

    std::swap(mat[col], mat[max_row]);

    const double diagonal = mat[col][col];
    for (size_t j = col; j <= n; ++j) {
      mat[col][j] /= diagonal;
    }

    for (size_t row = 0; row < n; ++row) {
      if (row == col || mat[row][col] == 0.0) {
        continue;
      }
      double factor = mat[row][col];
      for (size_t j = col; j <= n; ++j) {
        mat[row][j] -= factor * mat[col][j];
      }
    }
  }

  for (size_t row = 0; row < n; ++row) {
    result[row] = mat[row][n];
  }

  return true;
}

}  // namespace

bool LeastSquares::Build(const std::vector<GraphPoint>& points, int degree) {
  const size_t n = points.size();
  if (degree < 0 || n < static_cast<size_t>(degree) + 1) {
    return false;
  }

  auto [min_it, max_it] =
      std::ranges::minmax_element(points, {}, &GraphPoint::x);
  x_min_ = min_it->x;
  x_span_ = max_it->x > x_min_ ? max_it->x - x_min_ : 1.0;

  const int k = degree + 1;
  std::vector<double> s(2 * degree + 1, 0.0);
  std::vector<double> t(k, 0.0);
  for (const GraphPoint& point : points) {
    const double u =
        (point.x - x_min_) / x_span_ * 2.0 - 1.0;  // Нормирование [-1; 1].
    double u_power = 1.0;
    for (int j = 0; j <= 2 * degree; ++j) {
      s[j] += u_power;
      if (j < k) {
        t[j] += point.y * u_power;
      }
      u_power *= u;
    }
  }

  // Расширенная матрица
  std::vector<std::vector<double>> matrix(k, std::vector<double>(k + 1, 0.0));
  for (int row = 0; row < k; ++row) {
    for (int col = 0; col < k; ++col) {
      matrix[row][col] = s[row + col];
    }
    matrix[row][k] = t[row];
  }

  coeffs_.resize(k);
  return SolveGaussJordan(matrix, coeffs_);
}

double LeastSquares::Evaluate(double x) const {
  if (coeffs_.empty()) return 0.0;

  const double u = (x - x_min_) / x_span_ * 2.0 - 1.0;
  double result = 0.0;

  for (const double coeff : std::views::reverse(coeffs_)) {
    result = result * u + coeff;
  }

  return result;
}

}  // namespace s21
