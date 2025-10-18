add_executable(databrowser 
    main.cpp
)

target_link_libraries(databrowser PRIVATE
    ${PROJECT_NAME}
)

