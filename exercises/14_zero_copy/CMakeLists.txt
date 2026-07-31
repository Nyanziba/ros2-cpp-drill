cmake_minimum_required(VERSION 3.8)
project(drill_14_zero_copy)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

# 受講者が編集するクラスはライブラリにする。
# 実行ファイル（ros2 run 用）とテストの両方から使うため。
add_library(${PROJECT_NAME}_node src/zero_copy_nodes.cpp)
target_include_directories(${PROJECT_NAME}_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
ament_target_dependencies(${PROJECT_NAME}_node rclcpp std_msgs)

add_executable(zero_copy_demo src/zero_copy_demo_main.cpp)
target_link_libraries(zero_copy_demo ${PROJECT_NAME}_node)
ament_target_dependencies(zero_copy_demo rclcpp std_msgs)

install(TARGETS zero_copy_demo
  DESTINATION lib/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)

  ament_add_gtest(test_exercise test/test_exercise.cpp)
  target_include_directories(test_exercise PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../tools)
  target_link_libraries(test_exercise ${PROJECT_NAME}_node)
  ament_target_dependencies(test_exercise rclcpp std_msgs)
endif()

ament_package()
