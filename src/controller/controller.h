#pragma once

#include "model/model.h"

namespace s21 {

struct ControllerDateTime {
  int year{};
  int month{};
  int day{};
  int hour{};
  int minute{};
  int second{};
};

struct ControllerPoint {
  double x{};
  double y{};
};

struct LoadInfo {
  bool ok{false};
  std::string error{};
  std::string file_name{};
  int points_count{};
  ControllerDateTime first_date{};
  ControllerDateTime last_date{};
};

struct GraphResult {
  bool ok{false};
  std::string error{};
  std::vector<ControllerPoint> points{};
};

struct ValueResult {
  bool ok{false};
  std::string error{};
  double value{};
};

class Controller {
 public:
  LoadInfo LoadFile(const std::string& path);
  GraphResult SplineGraph(int points_count);
  GraphResult ApproxGraph(int points_count, int degree, int days_ahead);
  ValueResult SplineValueAt(const ControllerDateTime& moment);
  ValueResult ApproxValueAt(const ControllerDateTime& moment, int degree);
  GraphResult SourcePoints();
  [[nodiscard]] bool HasData() const noexcept { return model_.HasData(); }

 private:
  Model model_;
};

}  // namespace s21
