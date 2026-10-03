cmake_minimum_required(VERSION 3.8)
project(drill_18_pluginlib_yaml)

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
add_library(${PROJECT_NAME}_pipeline SHARED src/filter_pipeline.cpp)
target_include_directories(${PROJECT_NAME}_pipeline PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(${PROJECT_NAME}_pipeline PUBLIC
  rclcpp::rclcpp pluginlib::pluginlib ${geometry_msgs_TARGETS})

add_executable(filter_pipeline_node src/main.cpp)
target_link_libraries(filter_pipeline_node PRIVATE ${PROJECT_NAME}_pipeline)

install(
  TARGETS ${PROJECT_NAME}_filters ${PROJECT_NAME}_pipeline
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)
install(TARGETS filter_pipeline_node DESTINATION lib/${PROJECT_NAME})
install(DIRECTORY config DESTINATION share/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)

  ament_add_gtest(test_exercise test/test_exercise.cpp)
  target_include_directories(test_exercise PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../tools)
  target_link_libraries(test_exercise ${PROJECT_NAME}_pipeline)
  # テスト用の YAML（test/*.yaml）の場所。
  target_compile_definitions(test_exercise PRIVATE
    DRILL_TEST_DIRECTORY="${CMAKE_CURRENT_SOURCE_DIR}/test")
endif()

ament_package()
