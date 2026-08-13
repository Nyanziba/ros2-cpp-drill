cmake_minimum_required(VERSION 3.8)
project(drill_c09_function_pointer C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)

add_library(${PROJECT_NAME}_lib src/function_pointer.c)
target_include_directories(${PROJECT_NAME}_lib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(test_exercise test/test_exercise.cpp)
  target_include_directories(test_exercise PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_link_libraries(test_exercise ${PROJECT_NAME}_lib)
endif()

ament_package()
