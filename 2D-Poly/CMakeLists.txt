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