cmake_minimum_required(VERSION 3.8)
project(drill_15_composition)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_components REQUIRED)
find_package(std_msgs REQUIRED)

# 受講者が編集するクラスは SHARED ライブラリにする（add_executable ではない）。
# component_container はこの .so を実行時に dlopen してノードを生成するので、
# 静的リンクされた実行ファイルの中には存在しない形で成果物を作る必要がある。
add_library(${PROJECT_NAME}_node SHARED src/composable_talker.cpp)
target_include_directories(${PROJECT_NAME}_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
ament_target_dependencies(${PROJECT_NAME}_node rclcpp rclcpp_components std_msgs)

# PLUGIN の文字列はソース末尾の RCLCPP_COMPONENTS_REGISTER_NODE(ComposableTalker)
# と一致していなければならない。この呼び出しは 2 つのことをしてくれる。
#   1. ament リソースインデックス（share/ament_index/resource_index/rclcpp_components/）
#      にこのパッケージと PLUGIN 名を登録する。component_container や
#      `ros2 component types` はここを見てロード可能なコンポーネントを探す。
#   2. EXECUTABLE で指定した名前の実行ファイルを自動生成する。中身は
#      class_loader でこの .so を読み込んで単一ノードとして spin するだけの
#      薄いラッパーで、`ros2 run drill_15_composition composable_talker_node`
#      のように単体でも動かせる。
rclcpp_components_register_node(${PROJECT_NAME}_node
  PLUGIN "ComposableTalker"
  EXECUTABLE composable_talker_node
)

install(
  TARGETS ${PROJECT_NAME}_node
  ARCHIVE DESTINATION lib
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin
)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  find_package(class_loader REQUIRED)

  ament_add_gtest(test_exercise test/test_exercise.cpp)
  target_include_directories(test_exercise PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../tools)
  target_link_libraries(test_exercise ${PROJECT_NAME}_node)
  ament_target_dependencies(test_exercise rclcpp rclcpp_components std_msgs class_loader)

  # テスト「RCLCPP_COMPONENTS_REGISTER_NODE で登録されている」は、ビルドされた
  # 共有ライブラリそのものを class_loader で読み込んで確かめる。そのために
  # ライブラリの絶対パスをコンパイル時にテストへ埋め込む。
  target_compile_definitions(test_exercise PRIVATE
    DRILL_COMPONENT_LIBRARY="$<TARGET_FILE:${PROJECT_NAME}_node>")
endif()

ament_package()
