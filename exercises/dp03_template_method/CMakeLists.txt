cmake_minimum_required(VERSION 3.8)
project(drill_dp03_template_method)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)

add_library(${PROJECT_NAME}_lib src/sensor_reader.cpp)
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
