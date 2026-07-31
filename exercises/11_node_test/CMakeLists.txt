cmake_minimum_required(VERSION 3.8)
project(drill_11_node_test)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(std_msgs REQUIRED)

# limit_velocity() は rclcpp::Node を継承しない普通の関数として切り出してある。
# 「正しい実装」と「バグ入り実装」を別々のライブラリにしておき、
# 受講者が書く同じテスト（test/test_exercise.cpp）を両方にリンクし直して走らせる。
# これが gtest_mutation 方式の採点である。
add_library(velocity_limiter src/velocity_limiter.cpp)
target_include_directories(velocity_limiter PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

add_library(velocity_limiter_mutant src/velocity_limiter_mutant.cpp)
target_include_directories(velocity_limiter_mutant PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

# ros2 run で動かす実物のノード。こちらは正しい実装 (velocity_limiter) にリンクする。
add_library(${PROJECT_NAME}_node src/velocity_limiter_node.cpp)
target_include_directories(${PROJECT_NAME}_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
target_link_libraries(${PROJECT_NAME}_node velocity_limiter)
ament_target_dependencies(${PROJECT_NAME}_node rclcpp std_msgs)

add_executable(velocity_limiter_node src/velocity_limiter_node_main.cpp)
target_link_libraries(velocity_limiter_node ${PROJECT_NAME}_node)
ament_target_dependencies(velocity_limiter_node rclcpp std_msgs)

install(TARGETS velocity_limiter_node
  DESTINATION lib/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)

  # 受講者が書くテストは test/test_exercise.cpp の 1 つだけ。
  # それを 2 つの実装にリンクし直した 2 つの gtest ターゲットとして走らせる。
  #
  #   test_exercise … 正しい実装 (velocity_limiter) とリンク。落ちたら不合格。
  #   test_mutant   … バグ入り実装 (velocity_limiter_mutant) とリンク。
  #                   落ちなければ（＝バグを検出できていなければ）不合格。
  #
  # rclcpp には依存しない。limit_velocity() は普通の関数なので、
  # rclcpp::init() もノードの起動も spin も要らない。
  ament_add_gtest(test_exercise test/test_exercise.cpp)
  target_include_directories(test_exercise PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_link_libraries(test_exercise velocity_limiter)

  ament_add_gtest(test_mutant test/test_exercise.cpp)
  target_include_directories(test_mutant PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_link_libraries(test_mutant velocity_limiter_mutant)
endif()

ament_package()
