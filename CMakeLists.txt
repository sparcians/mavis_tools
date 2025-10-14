PROJECT(mavis_tools CXX)

cmake_minimum_required (VERSION 3.15)

set (CMAKE_CXX_STANDARD 23)
set (CMAKE_CXX_STANDARD_REQUIRED ON)

find_package (Boost 1.74.0 REQUIRED COMPONENTS program_options json)
include_directories (SYSTEM ${Boost_INCLUDE_DIRS})
message (STATUS "Using BOOST ${Boost_VERSION_STRING}")

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

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

include(std_filesystem)

set(MAVIS_CXX_STANDARD 23)
add_subdirectory(mavis)

add_subdirectory(tools)

if(MAVIS_TOOLS_INSTALL_DIR)
    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/mavis/json DESTINATION ${MAVIS_TOOLS_INSTALL_DIR}/../share/mavis_tools/mavis)
endif()

if(NOT CLANGFORMAT_EXECUTABLE)
  set(CLANGFORMAT_EXECUTABLE clang-format)
endif()

add_custom_target(clangformat
  git ls-files "*.hpp" "*.cpp" > clang-format-files.out
  COMMAND
     ${CLANGFORMAT_EXECUTABLE} -i --files=clang-format-files.out
  COMMAND
     rm -f clang-format-files.out
  WORKING_DIRECTORY
     ${CMAKE_SOURCE_DIR}
  COMMENT
     "Formatting with ${CLANGFORMAT_EXECUTABLE} ..."
  )
