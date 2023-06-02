set(CMAKE_CXX_STANDARD 20)

file(GLOB HARU_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/*.cxx)

add_executable(haru ${HARU_SOURCES})
target_link_libraries(haru haruhi)