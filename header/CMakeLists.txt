project(haruhi)

get_filename_component(haruhi_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../third_party ABSOLUTE)

file(GLOB haruhi_INCLUDE_DIRS ${haruhi_HEADER_DIR}/*)
list(REMOVE_ITEM haruhi_INCLUDE_DIRS ${haruhi_HEADER_DIR}/.DS_STORE)

set(haruhi_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} ${haruhi_INCLUDE_DIRS})

add_library(haruhi_mtl INTERFACE)
target_include_directories(haruhi_mtl INTERFACE ${haruhi_INCLUDE_DIRS})

target_compile_options(haruhi_mtl INTERFACE -std=c++17)

# find_library(APPKIT_LIB AppKit)
# find_library(COREFOUNDATION_LIB CoreFoundation)
find_library(METAL_LIB Metal)
find_library(METALKIT_LIB MetalKit)
find_library(FOUNDATION_LIB Foundation)
find_library(QUARTZCORE_LIB QuartzCore)
target_link_libraries(haruhi_mtl INTERFACE
  # ${APPKIT_LIB}
  # ${COREFOUNDATION_LIB}
  ${METAL_LIB}
  ${METALKIT_LIB}
  ${FOUNDATION_LIB}
  ${QUARTZCORE_LIB}
)