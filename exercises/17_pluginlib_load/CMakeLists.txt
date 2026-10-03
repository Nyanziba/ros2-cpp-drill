cmake_minimum_required(VERSION 3.8)
project(drill_17_pluginlib_load)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(pluginlib REQUIRED)
find_package(geometry_msgs REQUIRED)

# --- 3 つのプラグイン（完成済み。課題 16 で作ったものと同じ）---
add_library(${PROJECT_NAME}_filters SHARED src/filters.cpp)
target_include_directories(${PROJECT_NAME}_filters PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(${PROJECT_NAME}_filters PUBLIC
  rclcpp::rclcpp pluginlib::pluginlib ${geometry_msgs_TARGETS})
pluginlib_export_plugin_description_file(${PROJECT_NAME} plugins.xml)

# --- 受講者が書くノード ---
# プラグインのライブラリ（_filters）にはリンクしない。ClassLoader が実行時に読み込む。
add_library(${PROJECT_NAME}_host SHARED src/filter_host.cpp)
target_include_directories(${PROJECT_NAME}_host PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(${PROJECT_NAME}_host PUBLIC
  rclcpp::rclcpp pluginlib::pluginlib ${geometry_msgs_TARGETS})

add_executable(filter_host_node src/main.cpp)
target_link_libraries(filter_host_node PRIVATE ${PROJECT_NAME}_host)

install(
  TARGETS ${PROJECT_NAME}_filters ${PROJECT_NAME}_host
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
install(TARGETS filter_host_node DESTINATION lib/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)

  ament_add_gtest(test_exercise test/test_exercise.cpp)
  target_include_directories(test_exercise PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../tools)
  target_link_libraries(test_exercise ${PROJECT_NAME}_host)
endif()

ament_package()
