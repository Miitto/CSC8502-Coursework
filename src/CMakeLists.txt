cmake_minimum_required(VERSION 3.15..4.0)

add_executable(${PROJECT_NAME})

include(warnings)
enable_warnings(${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME} PRIVATE engine::engine)

target_sources(${PROJECT_NAME}
  PRIVATE
    FILE_SET HEADERS
  PRIVATE
    main.cpp
 "logger/logger.cpp" "renderer.cpp"  "heightmap.cpp"  "postprocess.cpp")

 target_compile_definitions(${PROJECT_NAME}
   PRIVATE
   MESHDIR="${CMAKE_CURRENT_SOURCE_DIR}/../meshes/"
   SHADERDIR="${CMAKE_CURRENT_SOURCE_DIR}/../shaders/"
   TEXTUREDIR="${CMAKE_CURRENT_SOURCE_DIR}/../textures/"
 )
