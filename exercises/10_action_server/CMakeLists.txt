cmake_minimum_required(VERSION 3.8)
project(drill_10_action_server)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_action REQUIRED)
find_package(action_tutorials_interfaces REQUIRED)

# 受講者が編集するクラスはライブラリにする。
# 実行ファイル（ros2 run 用）とテストの両方から使うため。
add_library(${PROJECT_NAME}_node src/fibonacci_action_server.cpp)
target_include_directories(${PROJECT_NAME}_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
ament_target_dependencies(${PROJECT_NAME}_node rclcpp rclcpp_action action_tutorials_interfaces)

add_executable(fibonacci_action_server src/fibonacci_action_server_main.cpp)
target_link_libraries(fibonacci_action_server ${PROJECT_NAME}_node)
ament_target_dependencies(fibonacci_action_server rclcpp rclcpp_action action_tutorials_interfaces)

install(TARGETS fibonacci_action_server
  DESTINATION lib/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)

  ament_add_gtest(test_exercise test/test_exercise.cpp)
  target_include_directories(test_exercise PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../tools)
  target_link_libraries(test_exercise ${PROJECT_NAME}_node)
  ament_target_dependencies(test_exercise rclcpp rclcpp_action action_tutorials_interfaces)
endif()

ament_package()
