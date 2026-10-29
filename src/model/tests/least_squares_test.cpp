#include <gtest/gtest.h>

#include <cstddef>
#include <vector>

#include "model/least_squares.h"

namespace {

using s21::GraphPoint;
using s21::LeastSquares;

double TestLinear(double x) { return 3.0 * x - 2.0; }

double TestQuadratic(double x) { return 0.5 * x * x - 1.5 * x + 3.0; }

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

TEST(LeastSquaresTest, EvaluateWithoutBuildReturnsZero) {
  LeastSquares approx;
  EXPECT_DOUBLE_EQ(approx.Evaluate(2.0), 0.0);
}

TEST(LeastSquaresTest, BuildFailsForNegativeDegree) {
  LeastSquares approx;
  const std::vector<GraphPoint> points = {
      GraphPoint{.x = 0.0, .y = 1.0},
      GraphPoint{.x = 1.0, .y = 2.0}};
  EXPECT_FALSE(approx.Build(points, -1));
}

TEST(LeastSquaresTest, BuildFailsWhenPointsFewerThanDegreeNeeds) {
  LeastSquares approx;
  const std::vector<GraphPoint> two_points = {
      GraphPoint{.x = 0.0, .y = 1.0},
      GraphPoint{.x = 1.0, .y = 2.0}};
  EXPECT_FALSE(approx.Build(two_points, 2));
  EXPECT_FALSE(approx.Build(two_points, 3));
}

TEST(LeastSquaresTest, ZeroDegreeFitsMean) {
  const std::vector<GraphPoint> points = {
      GraphPoint{.x = 0.0, .y = 1.0},
      GraphPoint{.x = 1.0, .y = 2.0},
      GraphPoint{.x = 2.0, .y = 6.0}};
  LeastSquares approx;
  ASSERT_TRUE(approx.Build(points, 0));
  EXPECT_NEAR(approx.Evaluate(0.0), 3.0, 1e-9);
  EXPECT_NEAR(approx.Evaluate(5.0), 3.0, 1e-9);
  EXPECT_NEAR(approx.Evaluate(-10.0), 3.0, 1e-9);
}

TEST(LeastSquaresTest, FirstDegreeReproducesExactLinearData) {
  const std::vector<double> xs = {0.0, 1.0, 2.0, 3.0, 4.0};
  LeastSquares approx;
  ASSERT_TRUE(approx.Build(SamplePoints(xs, TestLinear), 1));
  for (const double x : {0.0, 0.5, 2.0, 3.7, 4.0, -1.0, 10.0}) {
    EXPECT_NEAR(approx.Evaluate(x), TestLinear(x), 1e-8) << "x = " << x;
  }
}

TEST(LeastSquaresTest, SecondDegreeReproducesExactQuadraticData) {
  const std::vector<double> xs = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
  LeastSquares approx;
  ASSERT_TRUE(approx.Build(SamplePoints(xs, TestQuadratic), 2));
  for (const double x : {0.0, 0.5, 3.0, 5.5, 6.0}) {
    EXPECT_NEAR(approx.Evaluate(x), TestQuadratic(x), 1e-8) << "x = " << x;
  }
}

TEST(LeastSquaresTest, ThirdDegreeReproducesExactCubicData) {
  const std::vector<double> xs = {0.0, 1.0, 2.0, 3.0, 4.0, 5.0};
  LeastSquares approx;
  ASSERT_TRUE(approx.Build(SamplePoints(xs, TestCubic), 3));
  for (const double x : {0.0, 1.3, 2.5, 4.8, 5.0}) {
    EXPECT_NEAR(approx.Evaluate(x), TestCubic(x), 1e-7) << "x = " << x;
  }
}

TEST(LeastSquaresTest, ConstantDataGivesConstantPolynomial) {
  std::vector<GraphPoint> points;
  points.reserve(6);
  for (std::size_t i = 0; i < 6; ++i) {
    points.push_back(
        GraphPoint{.x = static_cast<double>(i), .y = 7.0});
  }
  LeastSquares approx;
  ASSERT_TRUE(approx.Build(points, 2));
  EXPECT_NEAR(approx.Evaluate(2.5), 7.0, 1e-9);
  EXPECT_NEAR(approx.Evaluate(9.0), 7.0, 1e-9);
}

}  // namespace
