cmake_minimum_required(VERSION 3.16)

project(QtDataBrowser VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(QTDATABROWSER_BUILD_EXAMPLES ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Widgets Svg)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Widgets Svg)

include(FetchContent)

set(FETCHCONTENT_BASE_DIR ${CMAKE_BINARY_DIR}/external)
set(FETCHCONTENT_QUIET FALSE)
set(FETCHCONTENT_TRY_FIND_PACKAGE_MODE NEVER)

FetchContent_Declare(qmatplotwidget
      GIT_REPOSITORY https://github.com/gapost/qmatplotwidget.git
      GIT_TAG dev
      GIT_SUBMODULES_RECURSE FALSE
      GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(qmatplotwidget)

# Library.
add_subdirectory(src)

# Examples.
if (QTDATABROWSER_BUILD_EXAMPLES)
      add_subdirectory(example)
endif()


