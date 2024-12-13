cmake_minimum_required(VERSION 3.20)
project(raylib_starter)

set(CMAKE_CXX_STANDARD 23)

include(FetchContent)

# Generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Dependencies
#set(RAYLIB_VERSION 5.5)
#
#FetchContent_Declare(
#        raylib
#        DOWNLOAD_EXTRACT_TIMESTAMP OFF
#        URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
#        FIND_PACKAGE_ARGS
#)
#
#FetchContent_MakeAvailable(raylib)

add_subdirectory( vendor/raylib-master)

add_executable(${PROJECT_NAME})
add_subdirectory(src)
target_link_libraries(${PROJECT_NAME} raylib)

# Checks if OSX and links appropriate frameworks (Only required on MacOS)
if (APPLE)
    target_link_libraries(${PROJECT_NAME} "-framework IOKit")
    target_link_libraries(${PROJECT_NAME} "-framework Cocoa")
    target_link_libraries(${PROJECT_NAME} "-framework OpenGL")
endif()

add_custom_command(
        TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
)