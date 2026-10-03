cmake_minimum_required(VERSION 3.8)
project(drill_16_pluginlib_create)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(pluginlib REQUIRED)
find_package(geometry_msgs REQUIRED)

# プラグインは SHARED ライブラリにする。pluginlib が実行時に dlopen して読み込むので、
# 実行ファイルに静的にリンクされた形では、読み込む相手がいなくなってしまう。
add_library(${PROJECT_NAME}_filters SHARED src/filters.cpp)
target_include_directories(${PROJECT_NAME}_filters PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(${PROJECT_NAME}_filters PUBLIC
  rclcpp::rclcpp pluginlib::pluginlib ${geometry_msgs_TARGETS})

# plugins.xml を pluginlib に登録する部分は、受講者が書く cmake/export_plugins.cmake に切り出してある。
# （課題の CMakeLists.txt をそのまま置くと、colcon が templates/ や solutions/ を
#   別パッケージとして拾って重複エラーになるため、別ファイルにしている。）
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/export_plugins.cmake)

install(
  TARGETS ${PROJECT_NAME}_filters
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)

  # テストはプラグインのライブラリにリンクしない。
  # pluginlib の ClassLoader が、実行時に plugins.xml を頼りに読み込む。
  ament_add_gtest(test_exercise test/test_exercise.cpp)
  target_include_directories(test_exercise PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../tools)
  target_link_libraries(test_exercise rclcpp::rclcpp pluginlib::pluginlib ${geometry_msgs_TARGETS})
endif()

ament_package()
