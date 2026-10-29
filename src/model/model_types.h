#pragma once

#include <chrono>
#include <cmath>
#include <format>
#include <string>
#include <vector>

namespace s21 {

using Date = std::chrono::year_month_day;

struct GraphPoint {
  double x{};
  double y{};
};

struct RawPoint {
  Date date{};
  double close{};
};

using RawPointList = std::vector<RawPoint>;

template <typename T>
class Result {
 public:
  static Result Ok(T&& value) {
    return Result(true, std::string(), std::move(value));
  }
  static Result Ok(T& value) { return Result(true, std::string(), value); }

  static Result Fail(std::string message) {
    return Result(false, std::move(message), T());
  }

  bool IsOk() const noexcept { return ok_; }
  const std::string& Error() const noexcept { return error_; }
  const T& Value() const noexcept { return value_; }

 private:
  Result(bool ok, std::string error, T&& value)
      : ok_(ok), error_(std::move(error)), value_(std::move(value)) {}

  Result(bool ok, std::string error, T& value)
      : ok_(ok), error_(std::move(error)), value_(value) {}

  bool ok_;
  std::string error_;
  T value_;
};

}  // namespace s21
