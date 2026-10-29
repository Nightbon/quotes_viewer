#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <filesystem>
#include <string>
#include <vector>

#include "model/model.h"
#include "test_utils.h"

namespace {

using s21::Date;
using s21::GraphPoint;
using s21::Model;

Date MakeDate(int year, unsigned month, unsigned day) {
  return Date{std::chrono::year{year}, std::chrono::month{month},
              std::chrono::day{day}};
}

double EpochDays(Date date) {
  return static_cast<double>(
      std::chrono::sys_days{date}.time_since_epoch().count());
}

const std::vector<double> kCloses = {100.0, 102.0, 101.0, 105.0, 107.0};

// Даты в файле идут не по порядку — модель обязана отсортировать их сама.
const std::string kQuotesCsv =
    "date,close\n"
    "2024-01-03,101.0\n"
    "2024-01-01,100.0\n"
    "2024-01-05,107.0\n"
    "2024-01-02,102.0\n"
    "2024-01-04,105.0\n";

class ModelTest : public ::testing::Test {
 protected:
  void SetUp() override {
    path_ = s21::test::WriteTempFile("quotes.csv", kQuotesCsv);
  }

  void TearDown() override { s21::test::CleanupTempDir(); }

  Model model_;
  std::filesystem::path path_;
};

TEST_F(ModelTest, LoadFailsForMissingFile) {
  const auto result = model_.LoadCsv(std::filesystem::path("missing.csv"));
  ASSERT_FALSE(result.IsOk());
  EXPECT_NE(result.Error().find("не удалось открыть файл"), std::string::npos);
  EXPECT_FALSE(model_.HasData());
}

TEST_F(ModelTest, LoadReturnsPointsCountAndFileName) {
  const auto result = model_.LoadCsv(path_);
  ASSERT_TRUE(result.IsOk()) << result.Error();
  EXPECT_EQ(result.Value(), 5u);
  EXPECT_TRUE(model_.HasData());
  EXPECT_EQ(model_.Count(), 5u);
  EXPECT_EQ(model_.SourceName(), "quotes.csv");
}

TEST_F(ModelTest, DatesAreSortedAfterLoad) {
  EXPECT_TRUE(model_.LoadCsv(path_).IsOk());
  EXPECT_EQ(model_.FirstDate(), MakeDate(2024, 1, 1));
  EXPECT_EQ(model_.LastDate(), MakeDate(2024, 1, 5));
}

TEST_F(ModelTest, EmptyModelHasDefaultDates) {
  EXPECT_EQ(model_.FirstDate(), Date{});
  EXPECT_EQ(model_.LastDate(), Date{});
}

TEST_F(ModelTest, EmptyModelRejectsAllQueries) {
  const auto source = model_.SourcePoints();
  ASSERT_FALSE(source.IsOk());
  EXPECT_NE(source.Error().find("данные не загружены"), std::string::npos);

  const auto spline_graph = model_.SplineGraph(10);
  ASSERT_FALSE(spline_graph.IsOk());
  EXPECT_NE(spline_graph.Error().find("данные не загружены"),
            std::string::npos);

  const auto approx_graph = model_.ApproxGraph(10, 2, 1);
  ASSERT_FALSE(approx_graph.IsOk());
  EXPECT_NE(approx_graph.Error().find("данные не загружены"),
            std::string::npos);

  const auto spline_value = model_.SplineValueAt(MakeDate(2024, 1, 2));
  ASSERT_FALSE(spline_value.IsOk());
  EXPECT_NE(spline_value.Error().find("данные не загружены"),
            std::string::npos);

  const auto approx_value = model_.ApproxValueAt(MakeDate(2024, 1, 2), 0.5, 1);
  ASSERT_FALSE(approx_value.IsOk());
  EXPECT_NE(approx_value.Error().find("данные не загружены"),
            std::string::npos);
}

TEST_F(ModelTest, SourcePointsMatchCsvData) {
  EXPECT_TRUE(model_.LoadCsv(path_).IsOk());
  const auto result = model_.SourcePoints();
  ASSERT_TRUE(result.IsOk()) << result.Error();
  ASSERT_EQ(result.Value().size(), 5u);
  for (std::size_t i = 0; i < result.Value().size(); ++i) {
    const GraphPoint& point = result.Value()[i];
    EXPECT_DOUBLE_EQ(point.x,
                     EpochDays(MakeDate(2024, 1, 1 + static_cast<unsigned>(i))));
    EXPECT_DOUBLE_EQ(point.y, kCloses[i]);
  }
}

TEST_F(ModelTest, ReloadingReplacesData) {
  const auto short_path =
      s21::test::WriteTempFile("short.csv", "2024-03-01,50.0\n2024-03-02,55.0\n");
  EXPECT_TRUE(model_.LoadCsv(path_).IsOk());
  const auto result = model_.LoadCsv(short_path);
  ASSERT_TRUE(result.IsOk()) << result.Error();
  EXPECT_EQ(model_.Count(), 2u);
  EXPECT_EQ(model_.SourceName(), "short.csv");
  EXPECT_EQ(model_.FirstDate(), MakeDate(2024, 3, 1));
  EXPECT_EQ(model_.LastDate(), MakeDate(2024, 3, 2));
}

TEST_F(ModelTest, SplineGraphRejectsTooFewPoints) {
  EXPECT_TRUE(model_.LoadCsv(path_).IsOk());
  const auto result = model_.SplineGraph(4);
  ASSERT_FALSE(result.IsOk());
  EXPECT_NE(result.Error().find("не меньше числа записей"), std::string::npos);
}

TEST_F(ModelTest, SplineGraphAtDataPointsReturnsQuotes) {
  EXPECT_TRUE(model_.LoadCsv(path_).IsOk());
  const auto result = model_.SplineGraph(5);
  ASSERT_TRUE(result.IsOk()) << result.Error();
  ASSERT_EQ(result.Value().size(), 5u);
  EXPECT_DOUBLE_EQ(result.Value().front().x, EpochDays(MakeDate(2024, 1, 1)));
  EXPECT_DOUBLE_EQ(result.Value().back().x, EpochDays(MakeDate(2024, 1, 5)));
  for (std::size_t i = 0; i < result.Value().size(); ++i) {
    EXPECT_NEAR(result.Value()[i].y, kCloses[i], 1e-9) << "i = " << i;
    EXPECT_TRUE(std::isfinite(result.Value()[i].y));
  }
}

TEST_F(ModelTest, SplineGraphProducesRequestedCount) {
  EXPECT_TRUE(model_.LoadCsv(path_).IsOk());
  const auto result = model_.SplineGraph(1000);
  ASSERT_TRUE(result.IsOk()) << result.Error();
  ASSERT_EQ(result.Value().size(), 1000u);
  EXPECT_DOUBLE_EQ(result.Value().front().x, EpochDays(MakeDate(2024, 1, 1)));
  EXPECT_DOUBLE_EQ(result.Value().back().x, EpochDays(MakeDate(2024, 1, 5)));
  for (std::size_t i = 1; i < result.Value().size(); ++i) {
    EXPECT_GE(result.Value()[i].x, result.Value()[i - 1].x);
    EXPECT_TRUE(std::isfinite(result.Value()[i].y));
  }
}

TEST_F(ModelTest, SplineValueAtKnotsReturnsQuotes) {
  EXPECT_TRUE(model_.LoadCsv(path_).IsOk());
  for (std::size_t i = 0; i < kCloses.size(); ++i) {
    const auto result =
        model_.SplineValueAt(MakeDate(2024, 1, 1 + static_cast<unsigned>(i)));
    ASSERT_TRUE(result.IsOk()) << result.Error();
    EXPECT_NEAR(result.Value(), kCloses[i], 1e-9) << "i = " << i;
  }
}

TEST_F(ModelTest, SplineValueAtSupportsDayFraction) {
  EXPECT_TRUE(model_.LoadCsv(path_).IsOk());
  const auto result = model_.SplineValueAt(MakeDate(2024, 1, 2), 0.5);
  ASSERT_TRUE(result.IsOk()) << result.Error();
  EXPECT_TRUE(std::isfinite(result.Value()));
}

TEST_F(ModelTest, SplineValueAtClampsOutsideRange) {
  EXPECT_TRUE(model_.LoadCsv(path_).IsOk());
  const auto before = model_.SplineValueAt(MakeDate(2023, 12, 25));
  ASSERT_TRUE(before.IsOk()) << before.Error();
  EXPECT_NEAR(before.Value(), 100.0, 1e-12);

  const auto after = model_.SplineValueAt(MakeDate(2024, 2, 1));
  ASSERT_TRUE(after.IsOk()) << after.Error();
  EXPECT_NEAR(after.Value(), 107.0, 1e-9);
}

TEST_F(ModelTest, SplineValueAtRejectsInvalidDate) {
  EXPECT_TRUE(model_.LoadCsv(path_).IsOk());
  const auto result = model_.SplineValueAt(MakeDate(2024, 2, 30));
  ASSERT_FALSE(result.IsOk());
  EXPECT_NE(result.Error().find("некорректная дата"), std::string::npos);
}

TEST_F(ModelTest, ApproxGraphValidatesArguments) {
  EXPECT_TRUE(model_.LoadCsv(path_).IsOk());

  const auto too_big_degree = model_.ApproxGraph(10, 5, 1);
  ASSERT_FALSE(too_big_degree.IsOk());
  EXPECT_NE(too_big_degree.Error().find("степень полинома"),
            std::string::npos);

  const auto negative_degree = model_.ApproxGraph(10, -1, 1);
  ASSERT_FALSE(negative_degree.IsOk());

  const auto zero_days_ahead = model_.ApproxGraph(10, 2, 0);
  ASSERT_FALSE(zero_days_ahead.IsOk());
  EXPECT_NE(zero_days_ahead.Error().find("дней M"), std::string::npos);

  const auto too_few_points = model_.ApproxGraph(4, 2, 1);
  ASSERT_FALSE(too_few_points.IsOk());
}

TEST_F(ModelTest, ApproxGraphBuildsPolynomialWithForecastRange) {
  EXPECT_TRUE(model_.LoadCsv(path_).IsOk());
  const auto result = model_.ApproxGraph(50, 2, 7);
  ASSERT_TRUE(result.IsOk()) << result.Error();
  ASSERT_EQ(result.Value().size(), 50u);
  EXPECT_DOUBLE_EQ(result.Value().front().x, EpochDays(MakeDate(2024, 1, 1)));
  EXPECT_NEAR(result.Value().back().x, EpochDays(MakeDate(2024, 1, 5)) + 7.0,
              1e-9);
  for (const GraphPoint& point : result.Value()) {
    EXPECT_TRUE(std::isfinite(point.y));
  }
}

TEST_F(ModelTest, ApproxGraphAllowsMaxDegree) {
  EXPECT_TRUE(model_.LoadCsv(path_).IsOk());
  const auto result = model_.ApproxGraph(20, 4, 1);
  ASSERT_TRUE(result.IsOk()) << result.Error();
  EXPECT_EQ(result.Value().size(), 20u);
}

TEST_F(ModelTest, ApproxValueAtZeroDegreeIsMean) {
  EXPECT_TRUE(model_.LoadCsv(path_).IsOk());
  const auto result = model_.ApproxValueAt(MakeDate(2024, 1, 3), 0.0, 0);
  ASSERT_TRUE(result.IsOk()) << result.Error();
  EXPECT_NEAR(result.Value(), 103.0, 1e-9);

  const auto with_fraction =
      model_.ApproxValueAt(MakeDate(2024, 1, 4), 0.5, 0);
  ASSERT_TRUE(with_fraction.IsOk()) << with_fraction.Error();
  EXPECT_NEAR(with_fraction.Value(), 103.0, 1e-9);
}

TEST_F(ModelTest, ApproxValueAtValidatesArguments) {
  EXPECT_TRUE(model_.LoadCsv(path_).IsOk());

  const auto bad_date = model_.ApproxValueAt(MakeDate(2024, 2, 30), 0.0, 1);
  ASSERT_FALSE(bad_date.IsOk());
  EXPECT_NE(bad_date.Error().find("некорректная дата"), std::string::npos);

  const auto bad_degree = model_.ApproxValueAt(MakeDate(2024, 1, 3), 0.0, 5);
  ASSERT_FALSE(bad_degree.IsOk());
  EXPECT_NE(bad_degree.Error().find("степень полинома"), std::string::npos);
}

TEST_F(ModelTest, ApproxValueAtAgreesWithApproxGraphStart) {
  EXPECT_TRUE(model_.LoadCsv(path_).IsOk());
  constexpr int kDegree = 2;
  const auto graph = model_.ApproxGraph(10, kDegree, 1);
  ASSERT_TRUE(graph.IsOk()) << graph.Error();
  const auto value = model_.ApproxValueAt(MakeDate(2024, 1, 1), 0.0, kDegree);
  ASSERT_TRUE(value.IsOk()) << value.Error();
  EXPECT_NEAR(value.Value(), graph.Value().front().y, 1e-9);
}

}  // namespace
