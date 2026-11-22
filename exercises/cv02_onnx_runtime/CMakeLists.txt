cmake_minimum_required(VERSION 3.8)
project(drill_cv02_onnx_runtime)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
# Jazzy には ros-<distro>-onnxruntime-vendor がある。無いディストリビューションでは
# /usr/local に置いた公式の CPU ビルドを使う（Dockerfile がそちらへ落とす）。
find_package(onnxruntime_vendor QUIET)
if(onnxruntime_vendor_FOUND)
  set(DRILL_ONNX_LIBS ${onnxruntime_vendor_LIBRARIES})
else()
  find_path(ONNXRUNTIME_INCLUDE_DIR onnxruntime_cxx_api.h)
  find_library(ONNXRUNTIME_LIBRARY onnxruntime)
  if(NOT ONNXRUNTIME_INCLUDE_DIR OR NOT ONNXRUNTIME_LIBRARY)
    message(FATAL_ERROR "onnxruntime が見つかりません。ros-${ROS_DISTRO}-onnxruntime-vendor か、公式の CPU ビルドを入れてください。")
  endif()
  set(DRILL_ONNX_LIBS ${ONNXRUNTIME_LIBRARY})
endif()

add_library(${PROJECT_NAME}_lib src/color_session.cpp)
target_include_directories(${PROJECT_NAME}_lib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)
if(onnxruntime_vendor_FOUND)
  target_link_libraries(${PROJECT_NAME}_lib PUBLIC ${DRILL_ONNX_LIBS})
else()
  target_include_directories(${PROJECT_NAME}_lib PUBLIC ${ONNXRUNTIME_INCLUDE_DIR})
  target_link_libraries(${PROJECT_NAME}_lib PUBLIC ${DRILL_ONNX_LIBS})
endif()

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(test_exercise test/test_exercise.cpp)
  target_include_directories(test_exercise PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_include_directories(test_exercise PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../tools)
  target_compile_definitions(test_exercise PRIVATE
    DRILL_MODEL_PATH="${CMAKE_CURRENT_SOURCE_DIR}/model/color_marker.onnx")
  target_link_libraries(test_exercise ${PROJECT_NAME}_lib)
endif()

ament_package()
