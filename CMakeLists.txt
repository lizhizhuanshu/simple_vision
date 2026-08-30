cmake_minimum_required(VERSION 3.16)

project(simple_vision
  VERSION 0.1.0
  DESCRIPTION "Pure C++ color/feature/image matching library for bitmap screenshots"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")

option(VISION_SHARED "build shared library" ON)
option(VISION_STATIC "build static library" OFF)
option(VISION_BUILD_TESTS "build tests" ON)

include_directories(./src ./lodepng)

aux_source_directory(./src DIR_SRCS)
list(APPEND DIR_SRCS ./lodepng/lodepng.cpp)

if(VISION_SHARED)
  add_library(${PROJECT_NAME} SHARED ${DIR_SRCS})
  set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "")
  set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR})
endif()

if(VISION_STATIC)
  add_library(${PROJECT_NAME}_static STATIC ${DIR_SRCS})
endif()

if(VISION_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()
