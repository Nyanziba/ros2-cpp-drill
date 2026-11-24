cmake_minimum_required(VERSION 3.8)
project(drill_cv03_yolo_dataset)

find_package(ament_cmake REQUIRED)

if(BUILD_TESTING)
  find_package(ament_cmake_pytest REQUIRED)
  ament_add_pytest_test(test_exercise test/test_exercise.py)
endif()

ament_package()
