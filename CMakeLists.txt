cmake_minimum_required(VERSION 3.10)
project(Chip-8)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(vendor)
add_subdirectory(src)

add_custom_target(copy_games ALL 
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${PROJECT_SOURCE_DIR}/games
    ${PROJECT_BINARY_DIR}/games
    COMMENT "Copying games directory to build folder"
)
add_dependencies(${PROJECT_NAME} copy_games)
