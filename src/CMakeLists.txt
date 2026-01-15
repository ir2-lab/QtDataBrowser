add_library(${PROJECT_NAME} STATIC
    qdatabrowser.h 
    qdatabrowser.cpp
    qdataview.h 
    qdataview.cpp
    qdatasliceselector.h 
    qdatasliceselector.cpp
    qtdatabrowser.qrc
    dataslice.h 
    dataslice.cpp
)

set(INSTALL_HEADERS
    qdatabrowser.h
    QDataBrowser
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Widgets
  PRIVATE
    QMatPlotWidget::QMatPlotWidget
    Qt${QT_VERSION_MAJOR}::Svg
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# -----------------------
# Installation and config
# -----------------------
include(CMakePackageConfigHelpers)

# Install the library target and export targets file
install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install public headers collected in INSTALL_HEADERS
install(FILES ${INSTALL_HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# location where CMake config files will be installed (relocatable)
set(CONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

# Install the exported targets so downstream projects can use target names
install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${CONFIG_INSTALL_DIR}
)

# Generate and install package config + version files
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY AnyNewerVersion
)

configure_package_config_file(
  "${CMAKE_CURRENT_SOURCE_DIR}/../cmake/${PROJECT_NAME}Config.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
  INSTALL_DESTINATION "${CONFIG_INSTALL_DIR}"
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
  DESTINATION "${CONFIG_INSTALL_DIR}"
)