cmake_minimum_required(VERSION 3.15..4.0)

add_executable(${PROJECT_NAME})

include(warnings)
enable_warnings(${PROJECT_NAME})

target_sources(${PROJECT_NAME}
  PRIVATE
    FILE_SET HEADERS
  PRIVATE
    main.cpp
)
