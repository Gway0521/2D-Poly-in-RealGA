cmake_minimum_required(VERSION 3.10)
project(2DPoly)

set(CMAKE_CXX_STANDARD 14)

include_directories(../RealGA/include)

link_directories(../RealGA/build/libs)

# 如果這裡出錯，應該是環境變數沒設定好。
# 環境變數: C:\opencv\build\x64\vc16\lib
find_package(OpenCV REQUIRED)
include_directories(${OpenCV_INCLUDE_DIRS})

file(GLOB SRC_FILES "*.cpp")
file(GLOB HEADER_FILES "*.h")

foreach(file ${SRC_FILES})
  get_filename_component(FILENAME ${file} NAME_WE)
  add_executable(${FILENAME} ${file})
  target_link_libraries(${FILENAME} realga_static)
  target_link_libraries(${FILENAME} ${OpenCV_LIBS})
endforeach()