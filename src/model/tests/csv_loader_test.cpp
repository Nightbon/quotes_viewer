#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <string>

#include "model/csv_loader.h"
#include "test_utils.h"

namespace {

using s21::CsvLoader;
using s21::Date;

class CsvLoaderTest : public ::testing::Test {
 protected:
  void TearDown() override { s21::test::CleanupTempDir(); }
};

TEST_F(CsvLoaderTest, LoadsValidFileWithoutHeader) {
  const auto path = s21::test::WriteTempFile(
      "ok_no_header.csv", "2024-01-02,102.5\n2024-01-01,100.0\n");
  const auto result = CsvLoader{}.Load(path);
  ASSERT_TRUE(result.IsOk()) << result.Error();
  ASSERT_EQ(result.Value().size(), 2u);
  EXPECT_EQ(result.Value().front().close, 100.0);
  EXPECT_EQ(result.Value().back().close, 102.5);
}

TEST_F(CsvLoaderTest, SkipsHeaderAndSortsByDate) {
  const std::string content =
      "date,close\n"
      "2024-01-03,101.0\n"
      "2024-01-01,100.0\n"
      "2024-01-05,107.0\n"
      "2024-01-02,102.0\n"
      "2024-01-04,105.0\n";
  const auto path = s21::test::WriteTempFile("ok_header.csv", content);
  const auto result = CsvLoader{}.Load(path);
  ASSERT_TRUE(result.IsOk()) << result.Error();
  ASSERT_EQ(result.Value().size(), 5u);
  EXPECT_EQ(result.Value().front().date,
            (Date{std::chrono::year{2024}, std::chrono::month{1},
                  std::chrono::day{1}}));
  EXPECT_EQ(result.Value().back().date,
            (Date{std::chrono::year{2024}, std::chrono::month{1},
                  std::chrono::day{5}}));
  EXPECT_EQ(result.Value().front().close, 100.0);
  EXPECT_EQ(result.Value().back().close, 107.0);
}

TEST_F(CsvLoaderTest, AcceptsCrlfLineEndings) {
  const auto path = s21::test::WriteTempFile(
      "crlf.csv", "2024-01-01,100.0\r\n2024-01-02,101.5\r\n");
  const auto result = CsvLoader{}.Load(path);
  ASSERT_TRUE(result.IsOk()) << result.Error();
  EXPECT_EQ(result.Value().size(), 2u);
}

TEST_F(CsvLoaderTest, AcceptsWhitespacePadding) {
  const auto path = s21::test::WriteTempFile(
      "pad.csv", "  2024-01-01,100.0 \n2024-01-02,\t101.0\n");
  const auto result = CsvLoader{}.Load(path);
  ASSERT_TRUE(result.IsOk()) << result.Error();
  ASSERT_EQ(result.Value().size(), 2u);
  EXPECT_EQ(result.Value().front().close, 100.0);
  EXPECT_EQ(result.Value().back().close, 101.0);
}

TEST_F(CsvLoaderTest, FailsOnMissingFile) {
  const auto result =
      CsvLoader{}.Load(std::filesystem::path("no_such_dir/no_such_file.csv"));
  ASSERT_FALSE(result.IsOk());
  EXPECT_NE(result.Error().find("не удалось открыть файл"), std::string::npos);
}

TEST_F(CsvLoaderTest, FailsOnEmptyFile) {
  const auto path = s21::test::WriteTempFile("empty.csv", "");
  const auto result = CsvLoader{}.Load(path);
  ASSERT_FALSE(result.IsOk());
  EXPECT_NE(result.Error().find("не менее двух записей"), std::string::npos);
}

TEST_F(CsvLoaderTest, FailsWhenFewerThanTwoRecords) {
  const auto path =
      s21::test::WriteTempFile("single.csv", "2024-01-01,100.0\n");
  const auto result = CsvLoader{}.Load(path);
  ASSERT_FALSE(result.IsOk());
  EXPECT_NE(result.Error().find("не менее двух записей"), std::string::npos);
}

TEST_F(CsvLoaderTest, FailsOnMalformedLineAndReportsItsNumber) {
  const auto path = s21::test::WriteTempFile(
      "bad_line.csv", "2024-01-01,100.0\n2024-01-02,101.0\n2024-01-03,abc\n");
  const auto result = CsvLoader{}.Load(path);
  ASSERT_FALSE(result.IsOk());
  EXPECT_NE(result.Error().find("некорректная строка 3"), std::string::npos);
  EXPECT_NE(result.Error().find("2024-01-03,abc"), std::string::npos);
}

TEST_F(CsvLoaderTest, FailsOnDuplicateDates) {
  const auto path = s21::test::WriteTempFile(
      "duplicates.csv", "2024-01-01,100.0\n2024-01-01,101.0\n");
  const auto result = CsvLoader{}.Load(path);
  ASSERT_FALSE(result.IsOk());
  EXPECT_NE(result.Error().find("дубликат даты 2024-01-01"), std::string::npos);
}

TEST_F(CsvLoaderTest, FailsOnImpossibleDate) {
  const auto path = s21::test::WriteTempFile(
      "bad_date.csv", "2024-01-01,100.0\n2024-02-30,101.0\n");
  const auto result = CsvLoader{}.Load(path);
  ASSERT_FALSE(result.IsOk());
  EXPECT_NE(result.Error().find("некорректная строка 2"), std::string::npos);
}

TEST_F(CsvLoaderTest, FailsOnMonthGreaterThanTwelve) {
  const auto path = s21::test::WriteTempFile(
      "bad_month.csv", "2024-01-01,100.0\n2024-13-01,101.0\n");
  const auto result = CsvLoader{}.Load(path);
  ASSERT_FALSE(result.IsOk());
  EXPECT_NE(result.Error().find("некорректная строка 2"), std::string::npos);
}

TEST_F(CsvLoaderTest, FailsOnTrailingGarbageAfterValue) {
  const auto path = s21::test::WriteTempFile(
      "garbage.csv", "2024-01-01,100.0\n2024-01-02,101.0 junk\n");
  const auto result = CsvLoader{}.Load(path);
  ASSERT_FALSE(result.IsOk());
  EXPECT_NE(result.Error().find("некорректная строка 2"), std::string::npos);
}

TEST_F(CsvLoaderTest, FailsOnSemicolonSeparator) {
  const auto path = s21::test::WriteTempFile(
      "semicolon.csv", "2024-01-01,100.0\n2024-01-02;101.0\n");
  const auto result = CsvLoader{}.Load(path);
  ASSERT_FALSE(result.IsOk());
  EXPECT_NE(result.Error().find("некорректная строка 2"), std::string::npos);
}

}  // namespace
