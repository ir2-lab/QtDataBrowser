add_library(${PROJECT_NAME} STATIC
    qdatabrowser.h 
    qdatabrowser.cpp
    abstractdatastore.h
    qdataview.h 
    qdataview.cpp
    qdatasliceselector.h 
    qdatasliceselector.cpp
    qtdatabrowser_resources.qrc
    dataslice.h dataslice.cpp
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
  PRIVATE
    QMatPlotWidget
    Qt${QT_VERSION_MAJOR}::Svg
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
