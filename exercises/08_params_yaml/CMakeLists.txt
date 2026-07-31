cmake_minimum_required(VERSION 3.8)
project(drill_08_params_yaml)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)

# 採点対象ではない完成済みノード。--params-file で YAML を読ませて確認するための的。
add_executable(param_echo src/param_echo.cpp)
ament_target_dependencies(param_echo rclcpp)

install(TARGETS param_echo
  DESTINATION lib/${PROJECT_NAME})

# 受講者が編集する config/ と launch/ をインストールする。
# YAML や launch が未完成（TODO のまま）でもビルド自体は通る。
install(DIRECTORY launch config
  DESTINATION share/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_cmake_pytest REQUIRED)

  ament_add_pytest_test(test_exercise test/test_exercise.py)
endif()

ament_package()
