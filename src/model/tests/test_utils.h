#pragma once

#include <filesystem>
#include <fstream>
#include <string>

namespace s21::test {

inline std::filesystem::path TempDir() {
  const std::filesystem::path dir =
      std::filesystem::temp_directory_path() / "s21_quotes_viewer_tests";
  std::filesystem::create_directories(dir);
  return dir;
}

inline std::filesystem::path WriteTempFile(const std::string& name,
                                           const std::string& content) {
  const std::filesystem::path path = TempDir() / name;
  std::ofstream out(path, std::ios::binary);
  out << content;
  return path;
}

inline void CleanupTempDir() { std::filesystem::remove_all(TempDir()); }

}  // namespace s21::test
