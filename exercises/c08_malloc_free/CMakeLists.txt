cmake_minimum_required(VERSION 3.8)
# C言語編なので C と CXX の両方を有効にする。
# 課題本体は .c（C コンパイラでビルド）、採点用のテストは .cpp（gtest）。
project(drill_c08_malloc_free C CXX)

# 規格の C を教えるので、GNU 拡張は切る。
# 既定のままだと -std=gnu99 になり、処理系依存の書き方を通してしまう。
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
  # AddressSanitizer を有効にしてメモリリークを検出する
  add_compile_options(-g -fsanitize=address)
  add_link_options(-fsanitize=address)
endif()

find_package(ament_cmake REQUIRED)

add_library(${PROJECT_NAME}_lib src/malloc_free.c)
target_include_directories(${PROJECT_NAME}_lib PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
)

if(BUILD_TESTING)
  find_package(ament_cmake_gtest REQUIRED)
  ament_add_gtest(test_exercise test/test_exercise.cpp)
  target_include_directories(test_exercise PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  target_link_libraries(test_exercise ${PROJECT_NAME}_lib)
endif()

ament_package()
