PROJECT(mavis_tools CXX)

cmake_minimum_required (VERSION 3.15)

set (CMAKE_CXX_STANDARD 17)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

find_package (Boost 1.74.0 REQUIRED COMPONENTS program_options)
include_directories (SYSTEM ${Boost_INCLUDE_DIRS})
message (STATUS "Using BOOST ${Boost_VERSION_STRING}")

add_compile_options (-g -Werror -fPIC
  -Wall -Wextra -Winline -Winit-self -Wno-unused-function
  -Wuninitialized -Wno-sequence-point -Wno-inline -Wno-unknown-pragmas
  -Wno-unused-parameter -Wno-missing-field-initializers -pipe
  -Winvalid-pch)

if(CMAKE_CXX_COMPILER_ID MATCHES GNU)
    add_compile_options(-Wno-template-id-cdtor)
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Woverloaded-virtual")

if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_compile_options(-Wpedantic -Wno-gnu-zero-variadic-macro-arguments)
endif ()

if (CMAKE_BUILD_TYPE MATCHES "^[Dd]ebug")
  add_compile_options (-O0)
endif()

add_subdirectory(mavis)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/mavis)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/mavis/elfio)

add_subdirectory(tools)
