cmake_minimum_required(VERSION 3.5)
project(2DPoly)

set(CMAKE_CXX_STANDARD 14)

include_directories(../RealGA/include)

link_directories(../RealGA/build/libs)

file(GLOB SRC_FILES "*.cpp")
foreach(file ${SRC_FILES})
  get_filename_component(FILENAME ${file} NAME_WE)
  add_executable(${FILENAME} ${file})
  target_link_libraries(${FILENAME} realga_static)
endforeach()