cmake_minimum_required(VERSION 3.8)
project(drill_04_service_server)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(example_interfaces REQUIRED)

# 受講者が編集するクラスはライブラリにする。
# 実行ファイル（ros2 run 用）とテストの両方から使うため。
add_library(${PROJECT_NAME}_node src/add_two_ints_server.cpp)
target_include_directories(${PROJECT_NAME}_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(${PROJECT_NAME}_node PUBLIC rclcpp::rclcpp ${example_interfaces_TARGETS})

add_executable(server src/server_main.cpp)
target_link_libraries(server PUBLIC ${PROJECT_NAME}_node)
target_link_libraries(server PUBLIC rclcpp::rclcpp ${example_interfaces_TARGETS})

install(TARGETS server
  DESTINATION lib/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)

  ament_add_gtest(test_exercise test/test_exercise.cpp)
  target_include_directories(test_exercise PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../tools)
  target_link_libraries(test_exercise ${PROJECT_NAME}_node)
  target_link_libraries(test_exercise rclcpp::rclcpp ${example_interfaces_TARGETS})
endif()

ament_package()
