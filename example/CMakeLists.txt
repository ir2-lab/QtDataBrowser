add_executable(databrowser
    main.cpp
    widget.h
    widget.cpp
    random_data.h
    wave.h
    volume.h
    text.h
    text.cpp
)

target_link_libraries(databrowser PRIVATE
    ${PROJECT_NAME}
)

install(TARGETS databrowser
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

