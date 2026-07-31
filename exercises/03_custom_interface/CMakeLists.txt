cmake_minimum_required(VERSION 3.8)
project(drill_03_custom_interface)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rosidl_default_generators REQUIRED)

# インターフェース定義（msg/srv）とそれを使う C++ を同じパッケージに入れる構成。
# 公式チュートリアル「Single package: define and use an interface」の手順。
# （公式の tutorial_interfaces は使う側と分離しているが、1パッケージで完結させる
#  こちらの構成も公式が明示している別解であり、この課題ではあえてこちらを使う。）
rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/Num.msg"
  "srv/AddThreeInts.srv"
)
ament_export_dependencies(rosidl_default_runtime)

# 同じパッケージ内で生成した型を C++ から使うときの Jazzy の作法。
# 別パッケージの型を使うだけなら find_package(<pkg> REQUIRED) と
# ament_target_dependencies(... <pkg>) だけで済むが、同一パッケージ内では
# rosidl_generate_interfaces() が作るターゲットが ament_target_dependencies から
# 見える形で「まだ存在しない」段階なので、生成された typesupport ターゲットを
# 名前で取得して直接 target_link_libraries する。
rosidl_get_typesupport_target(cpp_typesupport_target
  ${PROJECT_NAME} rosidl_typesupport_cpp)

# 受講者が編集するクラスはライブラリにする。
# 実行ファイル（ros2 run 用）とテストの両方から使うため。
add_library(${PROJECT_NAME}_node src/num_publisher.cpp)
target_include_directories(${PROJECT_NAME}_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
ament_target_dependencies(${PROJECT_NAME}_node rclcpp)
target_link_libraries(${PROJECT_NAME}_node "${cpp_typesupport_target}")

add_executable(num_node src/num_node_main.cpp)
target_link_libraries(num_node ${PROJECT_NAME}_node)
ament_target_dependencies(num_node rclcpp)

install(TARGETS num_node
  DESTINATION lib/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)

  ament_add_gtest(test_exercise test/test_exercise.cpp)
  target_include_directories(test_exercise PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../tools)
  target_link_libraries(test_exercise ${PROJECT_NAME}_node)
  ament_target_dependencies(test_exercise rclcpp)
endif()

ament_package()
