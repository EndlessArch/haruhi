cmake_minimum_required(VERSION 3.26)
project(TEST_HARUHI)

set(CMAKE_CXX_STANDARD 20)

add_executable(testInclude include.cxx)
add_executable(testDevice device.cxx)
add_executable(testMetallib metallib.cxx)

set(MTL_COMPILE_TARGETS add_arrays.air default.metallib)
add_custom_target(
  testRenderer_compile_mtl ALL
  DEPENDS ${MTL_COMPILE_TARGETS}
)
# add_custom_command(
#   OUTPUT add_arrays.air
#   COMMAND xcrun -sdk macosx metal -c ${CMAKE_CURRENT_SOURCE_DIR}/add.metal -o add_arrays.air
# )
# add_custom_command(
#   OUTPUT add_arrays.metallib
#   COMMAND xcrun -sdk macosx metal -c add_arrays.air -o add_arrays.metallib
# )
add_custom_command(
  OUTPUT ${MTL_COMPILE_TARGETS}
  COMMAND xcrun -sdk macosx metal -c ${CMAKE_CURRENT_SOURCE_DIR}/add.metal -o add_arrays.air
  COMMAND xcrun -sdk macosx metallib add_arrays.air -o default.metallib
)

# add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../header ${CMAKE_CURRENT_BINARY_DIR}/header)

set(TestExecList
  testInclude
  testDevice
  testMetallib
)

foreach(testListIt ${TestExecList})
  target_link_libraries(${testListIt} haruhi)
endforeach()

# if compiled successfully, maybe it's enough?

# add_test(NAME MetalHeaderTest COMMAND testInclude)
# set_tests_properties(MetalHeaderTest PROPERTIES DEPENDS TEST_HARUHI)

# add_test(NAME MetalDeviceTest COMMAND testDevice)
# set_tests_properties(MetalDeviceTest PROPERTIES DEPENDS TEST_HARUHI)

# add_test(NAME MetallibTest COMMAND testMetallib)
# set_tests_properties(MetallibTest PROPERTIES DEPENDS TEST_HARUHI)