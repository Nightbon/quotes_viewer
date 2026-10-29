cmake_minimum_required(VERSION 3.16)
project(s21_quotes_viewer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_compile_options(-std=c++20 -Wall -Wextra -Wpedantic)

add_library(model STATIC
            model/csv_loader.cpp
            model/cubic_spline.cpp
            model/least_squares.cpp
            model/model.cpp)
target_include_directories(model PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(model PUBLIC cxx_std_20)

add_library(controller STATIC controller/controller.cpp)
target_link_libraries(controller PUBLIC model)

add_executable(quotes_viewer
               main.cpp
               view/main_window.cpp
               view/plot_widget.cpp)
target_link_libraries(quotes_viewer PRIVATE controller Qt6::Widgets)

option(BUILD_TESTS "модульные тесты модели (GTest)" OFF)
if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(model/tests)
endif()

# ================ Документация ========================
option(BUILD_DVI "Build docs" OFF)
if(BUILD_DVI)
    find_package(Doxygen)
    if(DOXYGEN_FOUND)
        set(DOXYGEN_PROJECT_NAME "quotes_viewer")
        set(DOXYGEN_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/docs)
        set(DOXYGEN_GENERATE_HTML YES)
        set(DOXYGEN_GENERATE_LATEX NO)
        set(DOXYGEN_EXTRACT_ALL YES)
        set(DOXYGEN_OUTPUT_LANGUAGE Russian)
        set(DOXYGEN_GENERATE_TREEVIEW YES)
        set(DOXYGEN_CALL_GRAPH YES)
        set(DOXYGEN_HAVE_DOT YES)
        set(DOXYGEN_JAVADOC_AUTOBRIEF YES)
        set(DOXYGEN_SOURCE_BROWSER YES)
        set(DOXYGEN_QUIET YES)
        set(DOXYGEN_EXCLUDE_PATTERNS "*test*")
        include(FindDoxygen)
        doxygen_add_docs(doxygen ${CMAKE_CURRENT_SOURCE_DIR} ALL WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} COMMENT "Generating API documentation with Doxygen" )
    endif()
endif()