set(CMAKE_CXX_STANDARD 20)

file(GLOB HARU_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cxx)

find_package(SPNG REQUIRED
  PATHS ${CMAKE_CURRENT_BINARY_DIR}/../third_party/libspng)

add_executable(haruhi ${HARU_SOURCES})
target_compile_options(haruhi PRIVATE -fno-common $<IF:$<CONFIG:Debug>,-g,>)

add_custom_target(
  genDbgSyms ALL
  COMMAND dsymutil ${CMAKE_CURRENT_BINARY_DIR}/haruhi
)
add_dependencies(genDbgSyms haruhi)

add_custom_target(genexdbg COMMAND ${CMAKE_COMMAND} -E echo "debug:$<IF:$<CONFIG:Debug>,Yes,No>")

# target_include_directories(haru PRIVATE spng::spng_static)

# hate cmake target file, nothing works well
# alternatively just simply use full path

find_package(ZLIB REQUIRED)
target_include_directories(haruhi PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../third_party/libspng/spng)
target_link_libraries(haruhi
  haruhi_mtl
  ZLIB::ZLIB
  ${CMAKE_CURRENT_BINARY_DIR}/../third_party/libspng/libspng_static.a
)

set(HARU_RESOURCES ${CMAKE_CURRENT_BINARY_DIR}/resource)

add_custom_target(
  haru_resource_patch ALL
  DEPENDS ${HARU_RESOURCES}
)
add_custom_command(
  OUTPUT ${HARU_RESOURCES}
  COMMAND cp -r ${CMAKE_CURRENT_SOURCE_DIR}/../resource ${HARU_RESOURCES}
)