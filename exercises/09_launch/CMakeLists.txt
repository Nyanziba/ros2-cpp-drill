cmake_minimum_required(VERSION 3.8)
project(drill_09_launch)

find_package(ament_cmake REQUIRED)

# C++ のソースは無いので add_library / add_executable は不要。
# launch ファイルだけを share/ 以下にインストールして
# `ros2 launch drill_09_launch <ファイル名>` を使えるようにする。
install(DIRECTORY launch
  DESTINATION share/${PROJECT_NAME})

if(BUILD_TESTING)
  find_package(ament_cmake_pytest REQUIRED)

  ament_add_pytest_test(test_exercise test/test_exercise.py)
endif()

ament_package()
