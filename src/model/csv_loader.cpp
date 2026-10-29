#include "model/csv_loader.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <ranges>

namespace s21 {

namespace {
bool IsHeader(std::string_view line) {
  const auto first_non_ws = std::ranges::find_if(
      line, [](unsigned char c) { return !std::isspace(c); });
  return first_non_ws == line.end() ? true : !std::isdigit(*first_non_ws);
}

inline std::string DateToString(std::chrono::year_month_day date) {
  return std::format("{:04}-{:02}-{:02}", static_cast<int>(date.year()),
                     static_cast<unsigned>(date.month()),
                     static_cast<unsigned>(date.day()));
}
}  // namespace

Result<RawPointList> CsvLoader::Load(const std::filesystem::path& path) const {
  std::ifstream file(path);
  if (!file.is_open()) {
    return Result<RawPointList>::Fail("не удалось открыть файл: " +
                                      path.filename().string());
  }

  RawPointList raw_points;
  std::size_t line_number = 0;
  for (std::string line; std::getline(file, line);) {
    ++line_number;
    if (IsHeader(line)) {
      continue;
    }

    const std::optional<RawPoint> quote = ParseLine(line);
    if (!quote.has_value()) {
      return Result<RawPointList>::Fail("некорректная строка " +
                                        std::to_string(line_number) + ": \"" +
                                        line + "\"");
    }
    raw_points.push_back(*quote);
  }

  if (raw_points.size() < 2) {
    return Result<RawPointList>::Fail(
        "в файле должно быть не менее двух записей");
  }

  std::ranges::sort(raw_points, std::ranges::less(), &RawPoint::date);
  for (std::size_t i = 1; i < raw_points.size(); ++i) {
    if (raw_points[i].date == raw_points[i - 1].date) {
      return Result<RawPointList>::Fail("дубликат даты " +
                                        DateToString(raw_points[i].date));
    }
  }
  return Result<RawPointList>::Ok(raw_points);
}

std::optional<RawPoint> CsvLoader::ParseLine(const std::string& line) {
  int y = 0;
  unsigned m = 0, d = 0;
  double value = 0.0;
  int n = 0;

  std::sscanf(line.c_str(), "%d-%u-%u,%lf %n", &y, &m, &d, &value, &n);

  Date date{std::chrono::year{y}, std::chrono::month{m}, std::chrono::day{d}};

  if (n == 0 || static_cast<size_t>(n) < line.size() || !date.ok()) {
    return std::nullopt;
  }

  return RawPoint{.date = date, .close = value};
}

}  // namespace s21
