#!python3

def fix_file(target_file_path, word_before, word_after):
    cont = open(target_file_path, 'r').read()
    open(target_file_path, 'w').write(cont.replace(word_before, word_after))

import os

# git
os.system("git submodule update --init --recursive")
fix_file(
    './third_party/libspng/cmake/Config.cmake.in',
    'find_dependency',
    'find_package')

# # fix_file("./third_party/libspng/CMakeLists.txt",
# #          'SPNGTargets',
# #          '${CMAKE_CURRENT_BINARY_DIR}/Export/SPNGTargets.cmake')

os.system(
    "cmake -S ./third_party/libspng -B out/third_party/libspng "
    "-DBUILD_EXAMPLES=OFF "
    "-DSPNG_SHARED=OFF "
    "-DSPNG_STATIC=ON "
    )

os.system("cmake --build out/third_party/libspng")

import glob
target_file = glob.glob('out/third_party/libspng/CMakeFiles/Export/**/SPNGTargets.cmake')[0]
os.system('mv ' + target_file + ' ' + 'out/third_party/libspng/')

# if True:
#     spng_cmake_cfg_bin_pth = "out/third_party/libspng/SPNGConfig.cmake"
#     fixed = ''
#     with open(spng_cmake_cfg_bin_pth, 'r') as f:
#         contents = f.read()
#         # fix outdated cmake command
#         fixed = contents.replace("find_dependency", "find_package")
#         f.close()
#     # open(spng_cmake_cfg_pth,'w').close()
#     open(spng_cmake_cfg_bin_pth, 'w').write(fixed)


# GH_URL_metal_cpp = "https://github.com/bkaradzic/metal-cpp.git"
# print("Download latest version of metal-cpp and extract into third_party(https://developer.apple.com/metal/cpp/)")
# if not os.path.isdir("third_party/metal-cpp"):
#     print("Downloading third party resources")
#     # we hate comprehensive progresses/codes, right?
#     os.system("cd third_party;git clone " + GH_URL_metal_cpp)

# cmake

CMAKE_OPTIONS = ""
os.system("cmake -S . -B out" + CMAKE_OPTIONS)

print("`cmake --build out` to build header and tests")