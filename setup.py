#!python3

# git

# GH_URL_metal_cpp = "https://github.com/bkaradzic/metal-cpp.git"

import os

if not os.path.isdir("third_party/metal-cpp"):
    print("Download latest version of metal-cpp and extract into third_party(https://developer.apple.com/metal/cpp/)")
#     print("Downloading third party resources")
#     # we hate comprehensive progresses/codes, right?
#     os.system("cd third_party;git clone " + GH_URL_metal_cpp)

# cmake

CMAKE_OPTIONS = ""
os.system("cmake -S . -B out" + CMAKE_OPTIONS)

print("`cmake --build out` to build header and tests")