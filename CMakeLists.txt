cmake_minimum_required(VERSION 3.26)
project(HARUHI_ROOT_BUILDER)

enable_testing()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/header)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/test)

# error failed to find target file which is created after the build
# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/third_party/libspng)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/src)