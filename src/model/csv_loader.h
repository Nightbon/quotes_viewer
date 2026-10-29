#pragma once

#include <filesystem>
#include <string>

#include "model/model_types.h"

namespace s21 {

class CsvLoader {
 public:
  Result<RawPointList> Load(const std::filesystem::path& path) const;

 private:
  static std::optional<RawPoint> ParseLine(const std::string& line);
};

}  // namespace s21
