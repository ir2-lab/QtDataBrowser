cmake_minimum_required(VERSION 3.16)

project(QtDataBrowser VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(QTDATABROWSER_BUILD_EXAMPLES "Build example programs" OFF)

find_package(Qt5 REQUIRED COMPONENTS Core Widgets Svg)
message(STATUS "Found Qt5 at ${Qt5_DIR}")

find_package(QMatPlotWidget REQUIRED)
message(STATUS "Found QMatPlotWidget at ${QMatPlotWidget_DIR}")

# Library.
add_subdirectory(src)

# Examples.
if (QTDATABROWSER_BUILD_EXAMPLES)
      add_subdirectory(example)
endif()


