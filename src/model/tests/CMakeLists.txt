find_package(GTest REQUIRED)

add_executable(model_tests
               csv_loader_test.cpp
               cubic_spline_test.cpp
               least_squares_test.cpp
               model_test.cpp)
target_compile_features(model_tests PRIVATE cxx_std_20)
target_link_libraries(model_tests PRIVATE model GTest::gtest_main)
gtest_discover_tests(model_tests)