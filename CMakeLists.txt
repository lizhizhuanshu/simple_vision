cmake_minimum_required(VERSION 3.7.0)

project(slv)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")

option(VISION_SHARED "build shared library" ON)
option(VISION_STATIC "build static library" OFF)
option(BUILD_TESTING "build tests" OFF)


include_directories(./src ./lodepng)

aux_source_directory(./src DIR_SRCS)
list(APPEND DIR_SRCS ./lodepng/lodepng.cpp)

if(VISION_SHARED)
  add_library(${PROJECT_NAME} SHARED ${DIR_SRCS})
  set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
endif()

if(VISION_STATIC)
  add_library(${PROJECT_NAME}_static STATIC ${DIR_SRCS})
endif()

