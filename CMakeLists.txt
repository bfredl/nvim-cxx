cmake_minimum_required(VERSION 2.8)

project(nvim_cxx)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --std=c++11")

find_package(Boost 1.53.0 REQUIRED COMPONENTS system)
include_directories(${Boost_INCLUDE_DIRS})

find_path(MSGPACK_INCLUDE_DIR msgpack.hpp)
include_directories(${MSGPACK_INCLUDE_DIR})

add_custom_command (
  OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/api_gen.hpp
  DEPENDS ${CMAKE_SOURCE_DIR}/api_gen.py
  COMMAND python ${CMAKE_SOURCE_DIR}/api_gen.py ${CMAKE_CURRENT_BINARY_DIR}/api_gen.hpp
)
include_directories( ${CMAKE_CURRENT_BINARY_DIR} )

include_directories(${CMAKE_SOURCE_DIR}/include)
add_executable(test_cxx src/test.cpp ${CMAKE_CURRENT_BINARY_DIR}/api_gen.hpp)

target_link_libraries(test_cxx ${Boost_LIBRARIES})
# msgpack.hpp is "include only", no linking

set(CMAKE_EXPORT_COMPILE_COMMANDS 1)
