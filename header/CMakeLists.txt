project(haruhi)

get_filename_component(haruhi_HEADER_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../third_party ABSOLUTE)

file(GLOB haruhi_INCLUDE_DIRS ${haruhi_HEADER_DIR}/*)
list(REMOVE_ITEM haruhi_INCLUDE_DIRS ${haruhi_HEADER_DIR}/.DS_STORE)

set(haruhi_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR} ${haruhi_INCLUDE_DIRS})

add_library(haruhi INTERFACE)
target_include_directories(haruhi INTERFACE ${haruhi_INCLUDE_DIRS})
# target_compile_definitions(haruhi INTERFACE NS_PRIVATE_IMPLEMENTATION MTL_PRIVATE_IMPLEMENTATION CA_PRIVATE_IMPLEMENTATION)
target_compile_options(haruhi INTERFACE -std=c++17)

# target_compile_options(haruhi INTERFACE
#   -F/Application/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/System/Library/Frameworks
#   -F/System/Library/Frameworks
# )
# target_link_libraries(haruhi INTERFACE
#   "-framework Metal"
#   "-framework Foundation"
#   "-framework QuartzCore"
# )

find_library(APPKIT_LIB AppKit)
find_library(COREFOUNDATION_LIB CoreFoundation)
find_library(METAL_LIB Metal)
find_library(METALKIT_LIB MetalKit)
find_library(FOUNDATION_LIB Foundation)
find_library(QUARTZCORE_LIB QuartzCore)
target_link_libraries(haruhi INTERFACE
  ${APPKIT_LIB}
  ${COREFOUNDATION_LIB}
  ${METAL_LIB}
  ${METALKIT_LIB}
  ${FOUNDATION_LIB}
  ${QUARTZCORE_LIB}
)

# message(${haruhi_INCLUDE_DIRS})

###

# add_library(haruhi_mtl INTERFACE)

# target_include_directories(
#   haruhi_mtl INTERFACE
#   "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/metal-cpp/Foundation"
#   "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/metal-cpp/Metal"
#   "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/metal-cpp/QuartzCore"
#   "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/metal-cpp/SingleHeader"
# )
  
# add_library(haruhi INTERFACE)
# target_sources(
#   haruhi INTERFACE
#   mtl_metal.hpp
#   mtl_quartzcore.hpp
# )
# target_include_directories(haruhi INTERFACE haruhi_mtl)