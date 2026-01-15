cmake_minimum_required(VERSION 3.16)

project(QtDataBrowser 
  VERSION 0.2.0 
  LANGUAGES CXX
  DESCRIPTION "A Qt widget for exploring multi-dimensional scientific data"
  HOMEPAGE_URL https://github.com/ir2-lab/QtDataBrowser
)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(QTDATABROWSER_BUILD_EXAMPLES "Build example programs" OFF)

# Default install prefix (if not set by user)
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
  if((CMAKE_SYSTEM_NAME STREQUAL "Windows") AND (NOT CMAKE_CROSSCOMPILING))
    set(CMAKE_INSTALL_PREFIX "${CMAKE_BINARY_DIR}/${PROJECT_NAME}" CACHE PATH "Default install path" FORCE)
  else()
    set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/.local" CACHE PATH "Default install path" FORCE)
  endif()
endif()

# Install folder support
include(GNUInstallDirs)

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

message(STATUS "----------------------------------------")
message(STATUS "CMake configuration summary for ${PROJECT_NAME}")
message(STATUS "  CMake version:        ${CMAKE_VERSION} (Generator: ${CMAKE_GENERATOR})")
message(STATUS "  Project version:      ${PROJECT_VERSION}")
message(STATUS "  C++ standard:         ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build type:           ${CMAKE_BUILD_TYPE}")
message(STATUS "  Install prefix:       ${CMAKE_INSTALL_PREFIX}")
message(STATUS "  C++ compiler:         ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Qt5 at:               ${Qt5_DIR}")
message(STATUS "  QMatPlotWidget at:    ${QMatPlotWidget_DIR}")
message(STATUS "  Install dirs:         lib=${CMAKE_INSTALL_LIBDIR} bin=${CMAKE_INSTALL_BINDIR} include=${CMAKE_INSTALL_INCLUDEDIR}")
message(STATUS "----------------------------------------")


