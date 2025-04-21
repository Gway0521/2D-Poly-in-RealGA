
# 自動導入 vcpkg toolchain（避免用戶忘記）
if(NOT DEFINED CMAKE_TOOLCHAIN_FILE AND EXISTS "C:/vcpkg/scripts/buildsystems/vcpkg.cmake")
  set(CMAKE_TOOLCHAIN_FILE "C:/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
endif()

# 自動指定 OpenCV_DIR
if(NOT OpenCV_DIR)
  set(OpenCV_DIR "C:/vcpkg/installed/x64-windows/share/opencv4" CACHE PATH "")
endif()

cmake_minimum_required(VERSION 3.10)
project(2DPoly)

set(CMAKE_CXX_STANDARD 14)

# Headers folder
include_directories(../RealGA/include)
include_directories(include)

# Library
link_directories(../RealGA/build/libs)

# 如果這裡出錯，應該是環境變數沒設定好。
# 環境變數: C:\opencv\build\x64\vc16\lib
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})


# Source file
file(GLOB SRC_FILES "src/*.cpp")

add_library(triangulation STATIC ${SRC_FILES} ${HEADER_FILES})
target_link_libraries(triangulation realga_static ${OpenCV_LIBS})


#====================================== Build Tests ====================================
file(GLOB TESTS_FILES "tests/*.cpp")

foreach(file ${TESTS_FILES})
  get_filename_component(FILENAME ${file} NAME_WE)
  add_executable(${FILENAME} ${file})
  target_link_libraries(${FILENAME} triangulation realga_static ${OpenCV_LIBS})
endforeach()