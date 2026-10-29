#include <gtest/gtest.h>

#include <cmath>
#include <cstddef>
#include <vector>

#include "model/cubic_spline.h"

namespace {

using s21::CubicSpline;
using s21::GraphPoint;

double TestCubic(double x) { return x * x * x - 2.0 * x + 1.0; }

std::vector<GraphPoint> SamplePoints(const std::vector<double>& xs,
                                     double (*func)(double)) {
  std::vector<GraphPoint> points;
  points.reserve(xs.size());
  for (const double x : xs) {
    points.push_back(GraphPoint{.x = x, .y = func(x)});
  }
  return points;
}

TEST(CubicSplineTest, EvaluateWithoutBuildReturnsZero) {
  CubicSpline spline;
  EXPECT_DOUBLE_EQ(spline.Evaluate(1.0), 0.0);
}

TEST(CubicSplineTest, BuildFailsWithTooFewPoints) {
  CubicSpline spline;
  EXPECT_FALSE(spline.Build({}));
  EXPECT_FALSE(spline.Build({GraphPoint{.x = 0.0, .y = 1.0}}));
}

TEST(CubicSplineTest, BuildFailsWithNonIncreasingX) {
  CubicSpline spline;
  const std::vector<GraphPoint> equal_x = {
      GraphPoint{.x = 1.0, .y = 1.0},
      GraphPoint{.x = 1.0, .y = 2.0},
      GraphPoint{.x = 3.0, .y = 5.0}};
  EXPECT_FALSE(spline.Build(equal_x));

  const std::vector<GraphPoint> decreasing_x = {
      GraphPoint{.x = 3.0, .y = 1.0},
      GraphPoint{.x = 1.0, .y = 2.0}};
  EXPECT_FALSE(spline.Build(decreasing_x));
}

TEST(CubicSplineTest, TwoPointsInterpolateLinearly) {
  CubicSpline spline;
  const std::vector<GraphPoint> points = {
      GraphPoint{.x = 0.0, .y = 1.0},
      GraphPoint{.x = 10.0, .y = 101.0}};
  ASSERT_TRUE(spline.Build(points));
  EXPECT_NEAR(spline.Evaluate(5.0), 51.0, 1e-12);
  EXPECT_NEAR(spline.Evaluate(2.5), 26.0, 1e-12);
  EXPECT_NEAR(spline.Evaluate(7.25), 73.5, 1e-12);
}

TEST(CubicSplineTest, PassesExactlyThroughKnots) {
  const std::vector<double> xs = {0.0, 1.5, 2.0, 4.0, 7.5, 10.0};
  CubicSpline spline;
  ASSERT_TRUE(spline.Build(SamplePoints(xs, TestCubic)));
  for (const double x : xs) {
    EXPECT_NEAR(spline.Evaluate(x), TestCubic(x), 1e-9) << "x = " << x;
  }
}

TEST(CubicSplineTest, ReproducesLinearData) {
  const std::vector<double> xs = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0};
  CubicSpline spline;
  ASSERT_TRUE(spline.Build(
      SamplePoints(xs, [](double x) { return 3.0 * x - 2.0; })));
  for (const double x : {0.5, 1.7, 3.3, 4.9}) {
    EXPECT_NEAR(spline.Evaluate(x), 3.0 * x - 2.0, 1e-9) << "x = " << x;
  }
}

TEST(CubicSplineTest, ClampsOutsideDataRange) {
  const std::vector<double> xs = {0.0, 2.0, 5.0};
  CubicSpline spline;
  ASSERT_TRUE(spline.Build(SamplePoints(xs, TestCubic)));
  EXPECT_NEAR(spline.Evaluate(-100.0), TestCubic(0.0), 1e-12);
  EXPECT_NEAR(spline.Evaluate(100.0), TestCubic(5.0), 1e-9);
}

TEST(CubicSplineTest, IsSmoothAcrossInteriorKnots) {
  const std::vector<double> xs = {0.0, 1.5, 2.0, 4.0, 7.5, 10.0};
  CubicSpline spline;
  ASSERT_TRUE(spline.Build(SamplePoints(xs, TestCubic)));
  constexpr double kDelta = 1e-4;
  for (std::size_t i = 1; i + 1 < xs.size(); ++i) {
    const double knot = xs[i];
    const double left =
        (spline.Evaluate(knot) - spline.Evaluate(knot - kDelta)) / kDelta;
    const double right =
        (spline.Evaluate(knot + kDelta) - spline.Evaluate(knot)) / kDelta;
    EXPECT_NEAR(left, right, 1e-2) << "knot = " << knot;
  }
}

}  // namespace
